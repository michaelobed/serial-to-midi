//
//  main.c
//  SerialToMIDI
//
//  Created by Michael Obed on 09/11/2021.
//

/* Includes. */
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "midi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* Function prototypes. */
int cfgSerial(int* pFd, const char* port, long baud);
void cleanup(void);

int main(int argc, const char * argv[])
{
    const char* exitMsg = " Exiting...\n";
    int i = 0;
    PortMidiStream* midiInStream = NULL;
    PortMidiStream* midiOutStream = NULL;
    char serialBytes[3] = {0x00, 0x00, 0x00};
    int serialFd = -1;
    
    /* Exit if too few arguments were supplied. */
    if(argc < 2)
    {
        printf("Too few arguments supplied!%s", exitMsg);
        return(EXIT_FAILURE);
    }

    /* If two arguments were supplied (the program name string (ignored) and the MIDI device), choose the first device starting with "usbserial".
     * Set it up for 76800 baud, and use the IAC Driver Bus 1 MIDI device as default. */
    else if(argc == 2)
    {
        char devPath[1032] = "/dev/";
        DIR* pDirDevs = opendir(devPath);
        struct dirent* pDirEnt = NULL;
        struct dirent* pTemp = readdir(pDirDevs);
        int devsFound = 0;
        do
        {
            if(strstr(pTemp->d_name, "cu.usbserial") != NULL)
            {
                devsFound++;
                pDirEnt = pTemp;
                strcat(devPath, pTemp->d_name);
                break;
            }
            else pTemp = readdir(pDirDevs);
        } while(pTemp != NULL);

        if(pDirEnt == NULL)
        {
            printf("usbserial device not found!%s", exitMsg);
            return(EXIT_FAILURE);
        }
        
        if((cfgSerial(&serialFd, devPath, 76800) < 0) ||
           (MidiInit(argv[1], &midiInStream, &midiOutStream) < 0))
        {
            printf("%s", exitMsg);
            return EXIT_FAILURE;
        }
    }
    
    /* Check for exactly four arguments:
     * 1. The program name (ignored).
     * 2. The serial port name.
     * 3. The serial port's baud rate.
     * 4. The MIDI port name. */
    else if((cfgSerial(&serialFd, argv[1], atol(argv[2])) < 0) ||
       (MidiInit(argv[3], &midiInStream, &midiOutStream) < 0))
    {
        printf("%s", exitMsg);
        return EXIT_FAILURE;
    }
    
    atexit(&cleanup);
    
    while(TRUE)
    {
        int bytesToRead = 0;
        
        /* Wait for a byte of serial input and regurgitate it immediately over MIDI. */
        /* TODO: Make this buffered! */
        read(serialFd, &serialBytes[0], 1);
        
        switch(serialBytes[0] & 0xf0)
        {
            case MIDI_STATUS_NOTE_OFF:
            case MIDI_STATUS_NOTE_ON:
            case MIDI_STATUS_CONTROL_CHANGE:
            case MIDI_STATUS_PITCH_BEND:
                bytesToRead = 2;
                break;
                
            case MIDI_STATUS_PROGRAM_CHANGE:
            case MIDI_STATUS_CHANNEL_PRESSURE:
                bytesToRead = 1;
                break;
                
            case MIDI_STATUS_ACTIVE_SENSING:
            case MIDI_STATUS_CLOCK:
                bytesToRead = 0;
                break;
        }
        
        read(serialFd, &serialBytes[1], bytesToRead);
        Pm_WriteShort(midiOutStream, 0, Pm_Message(serialBytes[0], serialBytes[1], serialBytes[2]));
        for(i = 0; i < 3; i++)
            serialBytes[i] = 0x00;
    }
}

int cfgSerial(int* pFd, const char* port, long baud)
{
    int flags = 0;
    struct termios options;
    
    /* Attempt to open the serial port. */
    printf("Attempting to open serial port \"%s\" at %lu baud...", port, baud);
    *pFd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(*pFd < 0)
    {
        printf("failed! Could not open connection.");
        return -1;
    }
    
    /* Make file descriptor access exclusive to this process. */
    if(ioctl(*pFd, TIOCEXCL) < 0)
    {
        close(*pFd);
        printf("failed! Could not get exclusive access to serial port.");
        return -1;
    }
    
    /* Now that that's done, set to blocking mode. */
    flags = fcntl(*pFd, F_GETFL);
    flags &= ~O_NONBLOCK;
    if(fcntl(*pFd, F_SETFL, flags) < 0)
    {
        close(*pFd);
        printf("failed! Could not change blocking attribute of serial port.");
        return -1;
    }
    
    /* Flush. */
    if(tcflush(*pFd, TCIOFLUSH) != 0)
    {
        close(*pFd);
        printf("failed! Could not flush serial port.");
        return -1;
    }
    
    /* Retrieve attributes for modification, then modify. */
    if(tcgetattr(*pFd, &options) < 0)
    {
        close(*pFd);
        printf("failed! Could not retrieve serial port configuration options.");
        return -1;
    }
    
    /* These options may get in the way of us sending and receiving raw binary bytes (according to https://www.pololu.com/docs/0J73/15.5 ), so switch them off. */
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    
    /* Configure other options. */
    options.c_cflag |= (CS8 | CREAD);
    options.c_cflag &= ~(CSTOPB | PARENB | CCTS_OFLOW | CRTS_IFLOW);
    
    /* Configure timeouts (one byte of data OR 100ms of inactivity). */
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 1;
    
    /* Configure baud rate - just pass it straight in for now. Validating is complicated... */
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);
    
    /* Commit attribute changes. */
    if(tcsetattr(*pFd, TCSANOW, &options) < 0)
    {
        close(*pFd);
        printf("failed! Could not write serial port configuration options.");
        return -1;
    }
    
    printf("success!\n");
    return 0;
}

void cleanup(void)
{
    printf("Cleaning up...");
    Pm_Terminate();
}
