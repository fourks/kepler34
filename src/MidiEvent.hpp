#pragma once

#include <stdio.h>
#include <vector>

#include "Globals.hpp"

const unsigned char  EVENT_STATUS_BIT       = 0x80;
const unsigned char  EVENT_NOTE_OFF         = 0x80;
const unsigned char  EVENT_NOTE_ON          = 0x90;
const unsigned char  EVENT_AFTERTOUCH       = 0xA0;
const unsigned char  EVENT_CONTROL_CHANGE   = 0xB0;
const unsigned char  EVENT_PROGRAM_CHANGE   = 0xC0;
const unsigned char  EVENT_CHANNEL_PRESSURE = 0xD0;
const unsigned char  EVENT_PITCH_WHEEL      = 0xE0;
const unsigned char  EVENT_CLEAR_CHAN_MASK  = 0xF0;
const unsigned char  EVENT_MIDI_SONG_POS    = 0xF2;
const unsigned char  EVENT_MIDI_CLOCK       = 0xF8;
const unsigned char  EVENT_MIDI_START       = 0xFA;
const unsigned char  EVENT_MIDI_CONTINUE    = 0xFB;
const unsigned char  EVENT_MIDI_STOP        = 0xFC;

const unsigned char  EVENT_SYSEX            = 0xF0;
const unsigned char  EVENT_SYSEX_END        = 0xF7;

class MidiEvent
{

 private:

    /* timestamp in ticks */
    unsigned long m_timestamp;

    /* status byte without channel     */
    /* channel will be appended on bus */
    /* high nibble = type of event*/
    /* low nibble = channel */
    /* bit 7 is present in all status bytes */
    unsigned char m_status;

    /* data for event */
    unsigned char m_data[2];

    /* data for sysex */
    vector<unsigned char> m_sysex;

    /* used to link note ons and offs together */
    MidiEvent *m_linked;
    bool m_has_link;

    /* is this event selected in editing */
    bool m_selected;

    /* is this event marked in processing */
    bool m_marked;

    /* is this event being painted */
    bool m_painted;

    /* used in sorting */
    int get_rank( ) const;

 public:

    MidiEvent();

    void set_timestamp( const unsigned long time );
    long get_timestamp();
    void mod_timestamp( unsigned long a_mod );

    void set_status( const char status  );
    unsigned char get_status( );
    void set_data( const char D1 );
    void set_data( const char D1, const char D2 );
    void get_data( unsigned char *D0, unsigned char *D1 );
	void increment_data1();
	void decrement_data1();
	void increment_data2();
	void decrement_data2();

    void start_sysex();
    bool append_sysex( unsigned char *a_data, long size );
    unsigned char *get_sysex();

    void set_note( char a_note );

    void set_size( long a_size );
    long get_size();

    void link( MidiEvent *MidiEvent );
    MidiEvent *get_linked( );
    bool is_linked( );
    void clear_link( );

    void paint( );
    void unpaint( );
    bool is_painted( );

    void mark( );
    void unmark( );
    bool is_marked( );

    void select( );
    void unselect( );
    bool is_selected( );

    /* set status to midi clock */
    void make_clock( );

    /* gets the note assuming its note on/off */
    unsigned char get_note();
    unsigned char get_note_velocity();
    void set_note_velocity( int a_vel );

    /* returns true if status is set */
    bool is_note_on();
    bool is_note_off();

    void print();

    /* overloads */

    bool operator> ( const MidiEvent &rhsevent );
    bool operator< ( const MidiEvent &rhsevent );

    bool operator<=( const unsigned long &rhslong );
    bool operator> ( const unsigned long &rhslong );

    friend class MidiSequence;
};

