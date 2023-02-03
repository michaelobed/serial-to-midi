//
//  midi.h
//  SerialToMIDI
//
//  Created by Michael Obed on 09/11/2021.
//

#ifndef midi_h
#define midi_h

/* Includes. */
#include <portmidi.h>

/* Macros. */
typedef enum
{
    /* Channel voice messages. */
    MIDI_STATUS_NOTE_OFF = 0x80,
    MIDI_STATUS_NOTE_ON = 0x90,
    MIDI_STATUS_AFTERTOUCH = 0xa0,
    MIDI_STATUS_CONTROL_CHANGE = 0xb0,
    MIDI_STATUS_PROGRAM_CHANGE = 0xc0,
    MIDI_STATUS_CHANNEL_PRESSURE = 0xd0,
    MIDI_STATUS_PITCH_BEND = 0xe0,
    
    /* System realtime messages. */
    MIDI_STATUS_CLOCK = 0xf8,
    MIDI_STATUS_ACTIVE_SENSING = 0xfe
} midiStatus_e;

/* Global functions. */
int MidiInit(const char* devName, PortMidiStream** pIn, PortMidiStream** pOut);

#endif /* midi_h */
