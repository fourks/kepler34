#include <iostream>
#include "MidiFile.hpp"

MidiFile::MidiFile(const QString &a_name) :
    m_pos(0),
    m_name(a_name)
{
}

MidiFile::~MidiFile ()
{
}

unsigned long
MidiFile::read_long ()
{
    unsigned long ret = 0;

    ret += (read_byte() << 24);
    ret += (read_byte() << 16);
    ret += (read_byte() << 8);
    ret += (read_byte());

    return ret;
}

unsigned short
MidiFile::read_short ()
{
    unsigned short ret = 0;

    ret += (read_byte() << 8);
    ret += (read_byte());

    //printf( "read_short 0x%4x\n", ret );
    return ret;
}

unsigned char
MidiFile::read_byte ()
{
    return m_d[m_pos++];
}

unsigned long
MidiFile::read_var ()
{
    unsigned long ret = 0;
    unsigned char c;

    /* while bit #7 is set */
    while (((c = read_byte()) & 0x80) != 0x00)
    {
        /* shift ret 7 bits */
        ret <<= 7;
        /* add bits 0-6 */
        ret += (c & 0x7F);
    }

    /* bit was clear */
    ret <<= 7;
    ret += (c & 0x7F);

    return ret;
}


bool MidiFile::parse (MidiPerformance * a_perf, int a_screen_set)
{
    /* open binary file */
    ifstream file(m_name.toUtf8().constData(), ios::in | ios::binary | ios::ate);

    if (!file.is_open ()) {
        fprintf(stderr, "Error opening MIDI file\n");
        return false;
    }

    int file_size = file.tellg ();

    /* run to start */
    file.seekg (0, ios::beg);

    /* alloc data */
    try
    {
        m_d.resize(file_size);
    }
    catch(std::bad_alloc& ex)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }
    file.read ((char *) &m_d[0], file_size);
    file.close ();

    /* set position to 0 */
    m_pos = 0;

    /* chunk info */
    unsigned long ID;
    unsigned long TrackLength;

    /* time */
    unsigned long Delta;
    unsigned long RunningTime;
    unsigned long CurrentTime;

    unsigned short Format;			/* 0,1,2 */
    unsigned short NumTracks;
    unsigned short ppqn;
    unsigned short perf;

    /* track name from file */
    char TrackName[256];

    /* used in small loops */
    int i;

    /* sequence pointer */
    MidiSequence * seq;
    MidiEvent e;

    /* read in header */
    ID = read_long ();
    TrackLength = read_long ();
    Format = read_short ();
    NumTracks = read_short ();
    ppqn = read_short ();

    //printf( "[%8lX] len[%ld] fmt[%d] num[%d] ppqn[%d]\n",
    //      ID, TrackLength, Format, NumTracks, ppqn );

    /* magic number 'MThd' */
    if (ID != 0x4D546864) {
        fprintf(stderr, "Invalid MIDI header detected: %8lX\n", ID);
        return false;
    }

    /* we are only supporting format 1 for now */
    if (Format != 1) {
        fprintf(stderr, "Unsupported MIDI format detected: %d\n", Format);
        return false;
    }

    /* We should be good to load now   */
    /* for each Track in the midi file */
    for (int curTrack = 0; curTrack < NumTracks; curTrack++)
    {
        /* done for each track */
        bool done = false;
        perf = 0;

        /* events */
        unsigned char status = 0, type, data[2], laststatus;
        long len;
        unsigned long proprietary = 0;

        /* Get ID + Length */
        ID = read_long ();
        TrackLength = read_long ();
        //printf( "[%8lX] len[%8lX]\n", ID,  TrackLength );


        /* magic number 'MTrk' */
        if (ID == 0x4D54726B)
        {
            /* we know we have a good track, so we can create
               a new sequence to dump it to */
            seq = new MidiSequence ();
            if (seq == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                return false;
            }
            seq->set_master_midi_bus (&a_perf->m_master_bus);

            /* reset time */
            RunningTime = 0;

            /* this gets each event in the Trk */
            while (!done)
            {
                /* get time delta */
                Delta = read_var ();

                /* get status */
                laststatus = status;
                status = m_d[m_pos];

                /* is it a status bit ? */
                if ((status & 0x80) == 0x00)
                {
                    /* no, its a running status */
                    status = laststatus;
                }
                else
                {
                    /* its a status, increment */
                    m_pos++;
                }

                /* set the members in event */
                e.set_status (status);

                RunningTime += Delta;
                /* current time is ppqn according to the file,
                   we have to adjust it to our own ppqn.
                   PPQN / ppqn gives us the ratio */
                CurrentTime = (RunningTime * c_ppqn) / ppqn;

                //printf( "D[%6ld] [%6ld] %02X\n", Delta, CurrentTime, status);
                e.set_timestamp (CurrentTime);

                /* switch on the channelless status */
                switch (status & 0xF0)
                {
                /* case for those with 2 data bytes */
                case EVENT_NOTE_OFF:
                case EVENT_NOTE_ON:
                case EVENT_AFTERTOUCH:
                case EVENT_CONTROL_CHANGE:
                case EVENT_PITCH_WHEEL:

                    data[0] = read_byte();
                    data[1] = read_byte();

                    // some files have vel=0 as note off
                    if ((status & 0xF0) == EVENT_NOTE_ON && data[1] == 0)
                    {
                        e.set_status (EVENT_NOTE_OFF);
                    }

                    //printf( "%02X %02X\n", data[0], data[1] );

                    /* set data and add */
                    e.set_data (data[0], data[1]);
                    seq->add_event (&e);

                    /* set midi channel */
                    seq->set_midi_channel (status & 0x0F);
                    break;

                    /* one data item */
                case EVENT_PROGRAM_CHANGE:
                case EVENT_CHANNEL_PRESSURE:

                    data[0] = read_byte();
                    //printf( "%02X\n", data[0] );

                    /* set data and add */
                    e.set_data (data[0]);
                    seq->add_event (&e);

                    /* set midi channel */
                    seq->set_midi_channel (status & 0x0F);
                    break;

                    /* meta midi events ---  this should be FF !!!!!  */
                case 0xF0:

                    if (status == 0xFF)
                    {
                        /* get meta type */
                        type = read_byte();
                        len = read_var ();

                        //printf( "%02X %08X ", type, (int) len );

                        switch (type)
                        {
                        /* proprietary */
                        case 0x7f:

                            /* FF 7F len data  */
                            if (len > 4)
                            {
                                proprietary = read_long ();
                                len -= 4;
                            }

                            if (proprietary == c_midibus)
                            {
                                seq->set_midi_bus (read_byte());
                                len--;
                            }

                            else if (proprietary == c_midich)
                            {
                                seq->set_midi_channel (read_byte());
                                len--;
                            }

                            else if (proprietary == c_timesig)
                            {
                                seq->setBeatsPerMeasure (read_byte());
                                seq->setBeatWidth (read_byte());
                                len -= 2;
                            }

                            else if (proprietary == c_triggers)
                            {
                                int num_triggers = len / 4;

                                for (int i = 0; i < num_triggers; i += 2)
                                {
                                    unsigned long on = read_long ();
                                    unsigned long length = (read_long () - on);
                                    len -= 8;
                                    seq->add_trigger(on, length, 0, false);
                                }
                            }

                            else if (proprietary == c_triggers_new)
                            {
                                int num_triggers = len / 12;

                                //printf( "num_triggers[%d]\n", num_triggers );
                                for (int i = 0; i < num_triggers; i++)
                                {
                                    unsigned long on = read_long ();
                                    unsigned long off = read_long ();
                                    unsigned long length = off - on + 1;
                                    unsigned long offset = read_long ();

                                    //printf( "< start[%d] end[%d] offset[%d]\n",
                                    //        on, off, offset );

                                    len -= 12;
                                    seq->add_trigger (on, length, offset, false);
                                }
                            }

                            /* eat the rest */
                            m_pos += len;
                            break;

                            /* Trk Done */
                        case 0x2f:

                            // If delta is 0, then another event happened at the same time
                            // as the track end.  the sequence class will discard the last
                            // note.  This is a fix for that.   Native Seq24 file will always
                            // have a Delta >= 1
                            if ( Delta == 0 ){
                                CurrentTime += 1;
                            }

                            seq->set_length (CurrentTime, false);
                            seq->zero_markers ();
                            done = true;
                            break;

                            /* Track name */
                        case 0x03:
                            for (i = 0; i < len; i++)
                            {
                                TrackName[i] = read_byte();
                            }

                            TrackName[i] = '\0';

                            //printf("[%s]\n", TrackName );
                            seq->set_name (TrackName);
                            break;

                            /* sequence number */
                        case 0x00:
                            if (len == 0x00)
                                perf = 0;
                            else
                                perf = read_short ();

                            //printf ( "perf %d\n", perf );
                            break;

                        default:
                            for (i = 0; i < len; i++)
                            {
                                read_byte();
                            }
                            break;
                        }
                    }
                    else if(status == 0xF0)
                    {
                        /* sysex */
                        len = read_var ();

                        /* skip it */
                        m_pos += len;

                        fprintf(stderr, "Warning, no support for SYSEX messages, discarding.\n");
                    }
                    else
                    {
                        fprintf(stderr, "Unexpected system event : 0x%.2X", status);
                        return false;
                    }

                    break;

                default:
                    fprintf(stderr, "Unsupported MIDI event: %hhu\n", status);
                    return false;
                    break;
                }

            }			/* while ( !done loading Trk chunk */

            /* the sequence has been filled, add it  */
            //printf ( "add_sequence( %d )\n", perf + (a_screen_set * c_seqs_in_set));
            a_perf->add_sequence (seq, perf + (a_screen_set * cSeqsInBank));
        }

        /* dont know what kind of chunk */
        else
        {
            /* its not a MTrk, we dont know how to deal with it,
               so we just eat it */
            fprintf(stderr, "Unsupported MIDI header detected: %8lX\n", ID);
            m_pos += TrackLength;
            done = true;
        }

    }				/* for(eachtrack) */

    //printf ( "file_size[%d] m_pos[%d]\n", file_size, m_pos );

    if ((file_size - m_pos) > (int) sizeof (unsigned long))
    {
        ID = read_long ();
        if (ID == c_midictrl)
        {
            unsigned long seqs = read_long ();

            for (unsigned int i = 0; i < seqs; i++)
            {

                a_perf->get_midi_control_toggle (i)->m_active = read_byte();
                a_perf->get_midi_control_toggle (i)->m_inverse_active =
                        read_byte();
                a_perf->get_midi_control_toggle (i)->m_status = read_byte();
                a_perf->get_midi_control_toggle (i)->m_data = read_byte();
                a_perf->get_midi_control_toggle (i)->m_min_value = read_byte();
                a_perf->get_midi_control_toggle (i)->m_max_value = read_byte();

                a_perf->get_midi_control_on (i)->m_active = read_byte();
                a_perf->get_midi_control_on (i)->m_inverse_active =
                        read_byte();
                a_perf->get_midi_control_on (i)->m_status = read_byte();
                a_perf->get_midi_control_on (i)->m_data = read_byte();
                a_perf->get_midi_control_on (i)->m_min_value = read_byte();
                a_perf->get_midi_control_on (i)->m_max_value = read_byte();

                a_perf->get_midi_control_off (i)->m_active = read_byte();
                a_perf->get_midi_control_off (i)->m_inverse_active =
                        read_byte();
                a_perf->get_midi_control_off (i)->m_status = read_byte();
                a_perf->get_midi_control_off (i)->m_data = read_byte();
                a_perf->get_midi_control_off (i)->m_min_value = read_byte();
                a_perf->get_midi_control_off (i)->m_max_value = read_byte();
            }
        }

        /* Get ID + Length */
        ID = read_long ();
        if (ID == c_midiclocks)
        {
            TrackLength = read_long ();
            /* TrackLength is nyumber of buses */
            for (unsigned int x = 0; x < TrackLength; x++)
            {
                int bus_on = read_byte();
                a_perf->get_master_midi_bus ()->set_clock (x, (clock_e) bus_on);
            }
        }
    }

    if ((file_size - m_pos) > (int) sizeof (unsigned long))
    {
        /* Get ID + Length */
        ID = read_long ();
        if (ID == c_notes)
        {
            unsigned int screen_sets = read_short ();

            for (unsigned int x = 0; x < screen_sets; x++)
            {
                /* get the length of the string */
                unsigned int len = read_short ();
                string notess;

                for (unsigned int i = 0; i < len; i++)
                    notess += read_byte();

                a_perf->setBankName (x, &notess);
            }
        }
    }

    if ((file_size - m_pos) > (int) sizeof (unsigned int))
    {
        /* Get ID + Length */
        ID = read_long ();
        if (ID == c_bpmtag)
        {
            long bpm = read_long ();
            a_perf->set_bpm (bpm);
        }
    }

    // read in the mute group info.
    if ((file_size - m_pos) > (int) sizeof (unsigned long))
    {
        ID = read_long ();
        if (ID == c_mutegroups)
        {
            long length = read_long ();
            if (c_gmute_tracks != length)
            {
                printf( "corrupt data in mutegroup section\n" );
            }
            for (int i = 0; i < cSeqsInBank; i++)
            {
                a_perf->select_group_mute(read_long ());
                for (int k = 0; k < cSeqsInBank; ++k) {
                    a_perf->set_group_mute_state(k, read_long ());
                }
            }
        }
    }

    //read in sequence colour settings
    if ((file_size - m_pos) > (int) sizeof (unsigned long))
    {
        ID = read_long ();
        if (ID == c_seq_colours)
        {
            for (int curTrack = 0;
                 curTrack < c_max_sequence;
                 curTrack++)
            {
                if (a_perf->is_active (curTrack))
                {
                    a_perf->setSequenceColour(curTrack, (thumb_colours_e) read_long());
                }
            }
        }
    }

    //read in sequence editing modes
    if ((file_size - m_pos) > (int) sizeof (unsigned long))
    {
        ID = read_long ();
        if (ID == c_seq_edit_mode)
        {
            for (int curTrack = 0;
                 curTrack < c_max_sequence;
                 curTrack++)
            {
                if (a_perf->is_active (curTrack))
                {
                    a_perf->setEditMode(curTrack, (edit_mode_e) read_long());
                }
            }
        }
    }


    // *** ADD NEW TAGS AT END **************/

    return true;
}


void
MidiFile::write_long (unsigned long a_x)
{
    write_byte ((a_x & 0xFF000000) >> 24);
    write_byte ((a_x & 0x00FF0000) >> 16);
    write_byte ((a_x & 0x0000FF00) >> 8);
    write_byte ((a_x & 0x000000FF));
}


void
MidiFile::write_short (unsigned short a_x)
{
    write_byte ((a_x & 0xFF00) >> 8);
    write_byte ((a_x & 0x00FF));
}

void
MidiFile::write_byte (unsigned char a_x)
{
    m_l.push_back (a_x);
}

bool MidiFile::write (MidiPerformance * a_perf)
{
    int numtracks = 0;

    /* get number of tracks */
    for (int i = 0; i < c_max_sequence; i++)
    {
        if (a_perf->is_active (i))
            numtracks++;
    }

    //printf ("numtracks[%d]\n", numtracks );

    /* write header */
    /* 'MThd' and length of 6 */
    write_long (0x4D546864);
    write_long (0x00000006);

    /* format 1, number of tracks, ppqn */
    write_short (0x0001);
    write_short (numtracks);
    write_short (c_ppqn);

    /* We should be good to load now   */
    /* for each Track in the midi file */
    for (int curTrack = 0; curTrack < c_max_sequence; curTrack++)
    {
        if (a_perf->is_active (curTrack))
        {
            /* sequence pointer */
            MidiSequence * seq = a_perf->get_sequence (curTrack);

            //printf ("track[%d]\n", curTrack );
            std::list<char> l;
            seq->fill_list (&l, curTrack);

            /* magic number 'MTrk' */
            write_long (0x4D54726B);
            write_long (l.size ());

            //printf("MTrk len[%d]\n", l.size());

            while (l.size () > 0)
            {
                write_byte (l.back ());
                l.pop_back ();
            }
        }
    }


    /* midi control */
    write_long (c_midictrl);
    write_long (0);


    /* bus mute/unmute data */
    write_long (c_midiclocks);
    write_long (0);


    /* notepad data */
    write_long (c_notes);
    write_short (c_max_num_banks);

    for (int i = 0; i < c_max_num_banks; i++)
    {
        string * note = a_perf->getBankName (i);
        write_short (note->length ());

        for (unsigned int j = 0; j < note->length (); j++)
            write_byte ((*note)[j]);
    }


    /* bpm */
    write_long (c_bpmtag);
    write_long (a_perf->get_bpm ());

    /* write out the mute groups */
    write_long (c_mutegroups);
    write_long (c_gmute_tracks);
    for (int j=0; j < cSeqsInBank; ++j)
    {
        a_perf->select_group_mute(j);
        write_long(j);
        for (int i=0; i < cSeqsInBank; ++i)
        {
            write_long( a_perf->get_group_mute_state(i) );
        }
    }

    //write out sequence colouring
    write_long (c_seq_colours);
    for (int curTrack = 0; curTrack < c_max_sequence; curTrack++)
    {
        if (a_perf->is_active (curTrack))
        {
            write_long(a_perf->getSequenceColour(curTrack));
        }
    }

    //write out sequence edit modes
    write_long (c_seq_edit_mode);
    for (int curTrack = 0; curTrack < c_max_sequence; curTrack++)
    {
        if (a_perf->is_active (curTrack))
        {
            write_long(a_perf->getEditMode(curTrack));
        }
    }


    /* open binary file */
    ofstream file (m_name.toUtf8().constData(), ios::out | ios::binary | ios::trunc);

    if (!file.is_open ())
        return false;

    /* enable bufferization */
    char file_buffer[1024];
    file.rdbuf()->pubsetbuf(file_buffer, sizeof file_buffer);

    for (list < unsigned char >::iterator i = m_l.begin ();
         i != m_l.end (); i++)
    {
        char c = *i;
        file.write(&c, 1);
    }

    m_l.clear ();

    return true;
}

