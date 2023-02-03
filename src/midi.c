//
//  midi.c
//  SerialToMIDI
//
//  Created by Michael Obed on 09/11/2021.
//

/* Includes. */
#include "midi.h"
#include <stdio.h>
#include <string.h>

int MidiInit(const char* devName, PortMidiStream** pIn, PortMidiStream** pOut)
{
    int devCount = -1;
    int devInId = -1;
    int devOutId = -1;
    PmError err;
    int i = 0;
    
    printf("Attempting to configure MIDI port \"%s\"...", devName);
    
    /* Enumerate MIDI ports. */
    err = Pm_Initialize();
    if(err)
    {
        printf("failed (%s)!", Pm_GetErrorText(err));
        return -1;
    }
    devCount = Pm_CountDevices();
    printf("(found %u devices)...", devCount);
    
    /* For every MIDI device, compare the name to the one we care about.
     * Aim to find two devices (one input and one output) that both have this name. */
    for(i = 0; i < devCount; i++)
    {
        PmDeviceInfo* pTemp = NULL;
        pTemp = (PmDeviceInfo*)Pm_GetDeviceInfo(i);
        if(strcmp(pTemp->name, devName) == 0)
        {
            if(pTemp->input > 0)
                devInId = i;
            else if(pTemp->output > 0)
                devOutId = i;
        }
        
        /* If we've found both an input and an output, stop. */
        if((devInId != -1) && (devOutId != -1))
            break;
    }
    
    /* If either the input or output device wasn't found, signal an error. */
    if((devInId == -1) || (devOutId == -1))
    {
        printf("failed! The desired MIDI device was not found.");
        return -1;
    }
    
    /* Open our devices for use. */
    if(((err = Pm_OpenOutput(pOut, devOutId, NULL, 16, NULL, NULL, 0)) < 0) ||
       ((err = Pm_OpenInput(pIn, devInId, NULL, 16, NULL, NULL)) < 0))
    {
        printf("failed (%s)!", Pm_GetErrorText(err));
        return -1;
    }
    
    printf("success!\n");
    return 0;
}
