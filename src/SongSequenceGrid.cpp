#include "SongSequenceGrid.hpp"

SongSequenceGrid::SongSequenceGrid(MidiPerformance *a_perf,
                                   QWidget *parent):
    QWidget(parent),
    m_mainperf(a_perf),
    m_roll_length_ticks(0),
    m_drop_sequence(0),
    m_moving(false),
    m_growing(false),
    m_adding(false),
    m_adding_pressed(false)
{
    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::Fixed);

    for( int i=0; i<c_total_seqs; ++i )
        m_sequence_active[i]=false;

    m_roll_length_ticks = m_mainperf->get_max_trigger();
    m_roll_length_ticks = m_roll_length_ticks -
            ( m_roll_length_ticks % ( c_ppqn * 16 ));
    m_roll_length_ticks +=  c_ppqn * 16;

    //start refresh timer to queue regular redraws
    mTimer = new QTimer(this);
    mTimer->setInterval(1);
    QObject::connect(mTimer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(update()));
    mTimer->start();
}

void SongSequenceGrid::paintEvent(QPaintEvent *)
{
    mPainter = new QPainter(this);
    mBrush = new QBrush(Qt::NoBrush);
    mPen = new QPen(Qt::black);
    mPen->setStyle(Qt::SolidLine);
    mFont.setPointSize(6);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    int beats = m_measure_length / m_beat_length;

    /* draw vert lines */
    for (int i = 0; i < width();)
    {
        /* solid line on every beat */
        if ( i % beats == 0 )
        {
            mPen->setStyle(Qt::SolidLine);
            mPen->setColor(Qt::black);
        }
        else
        {
            mPen->setColor(Qt::lightGray);
            mPen->setStyle(Qt::DotLine);
        }

        mPainter->setPen(*mPen);
        mPainter->drawLine(i * m_beat_length / c_perf_scale_x,
                           1,
                           i * m_beat_length / c_perf_scale_x,
                           height() - 1);

        // jump 2 if 16th notes
        if ( m_beat_length < c_ppqn/2 )
        {
            i += (c_ppqn / m_beat_length);
        }
        else
        {
            ++i;
        }

    }

    //draw horizontal lines
    for (int i = 0; i < height(); i += c_names_y)
    {
        mPen->setColor(Qt::black);
        mPen->setStyle(Qt::DotLine);
        mPainter->setPen(*mPen);
        mPainter->drawLine(0,
                           i,
                           width(),
                           i);
    }

    //draw background

    long first_measure = 0;

    int y_s = 0;
    int y_f = height() / c_names_y;

    //draw sequence block
    long tick_on;
    long tick_off;
    long offset;
    bool selected;

//    long tick_offset = c_ppqn * 16;
    long tick_offset = 0;
    long x_offset = tick_offset / c_perf_scale_x;

    for ( int y = y_s; y <= y_f; y++ )
    {
        int a_sequence = y;

        if ( a_sequence < c_total_seqs )
        {
            if ( m_mainperf->is_active( a_sequence )){

                m_sequence_active[a_sequence] = true;

                MidiSequence *seq =  m_mainperf->get_sequence( a_sequence );

                seq->reset_draw_trigger_marker();

//                for ( int i = first_measure;
//                      i < first_measure +
//                      (width() * c_perf_scale_x /
//                       (m_measure_length)) + 1;

//                      i++ )
//                {
//                    int x_pos = (i * m_measure_length) / c_perf_scale_x;
//                }

                long seq_length = seq->getLength();
                int length_w = seq_length / c_perf_scale_x;

                while ( seq->get_next_trigger( &tick_on, &tick_off, &selected, &offset  )){

                    if ( tick_off > 0 ){

                        long x_on  = tick_on  / c_perf_scale_x;
                        long x_off = tick_off / c_perf_scale_x;
                        int  w     = x_off - x_on + 1;

                        int x = x_on;
                        int y = c_names_y * a_sequence + 1;  // + 2
                        int h = c_names_y - 2; // - 4

                        // adjust to screen corrids
                        x = x - x_offset;

                        if ( selected )
                            mPen->setColor(Qt::red);
                        else
                            mPen->setColor(Qt::black);

                        //main seq icon box
                        mPen->setStyle(Qt::SolidLine);
                        mBrush->setColor(Qt::white);
                        mBrush->setStyle(Qt::SolidPattern);
                        mPainter->setBrush(*mBrush);
                        mPainter->setPen(*mPen);
                        mPainter->drawRect(x,
                                           y,
                                           w,
                                           h);

                        //little seq grab handle - left hand side
                        mBrush->setStyle(Qt::NoBrush);
                        mPainter->setBrush(*mBrush);
                        mPen->setColor(Qt::black);
                        mPainter->setPen(*mPen);
                        mPainter->drawRect(x,
                                           y,
                                           c_perfroll_size_box_w,
                                           c_perfroll_size_box_w);

                        //seq grab handle - right side
                        mPainter->drawRect(x+w-c_perfroll_size_box_w,
                                           y+h-c_perfroll_size_box_w,
                                           c_perfroll_size_box_w,
                                           c_perfroll_size_box_w);

                        mPen->setColor(Qt::black);
                        mPainter->setPen(*mPen);

                        long length_marker_first_tick = ( tick_on - (tick_on % seq_length) + (offset % seq_length) - seq_length);

                        long tick_marker = length_marker_first_tick;

                        while ( tick_marker < tick_off ){

                            long tick_marker_x = (tick_marker / c_perf_scale_x) - x_offset;

                            if ( tick_marker > tick_on ){

                                //lines to break up the seq at each tick
                                mPen->setColor(Qt::lightGray);
                                mPainter->setPen(*mPen);
                                mPainter->drawRect(tick_marker_x,
                                                   y+4,
                                                   1,
                                                   h-8);
                            }

                            int lowest_note = seq->get_lowest_note_event( );
                            int highest_note = seq->get_highest_note_event( );

                            int height = highest_note - lowest_note;
                            height += 2;

                            int length = seq->getLength( );

                            long tick_s;
                            long tick_f;
                            int note;

                            bool selected;

                            int velocity;
                            draw_type dt;

                            seq->reset_draw_marker();

                            mPen->setColor(Qt::black);
                            mPainter->setPen(*mPen);

                            while ( (dt = seq->get_next_note_event( &tick_s, &tick_f, &note,
                                                                    &selected, &velocity )) != DRAW_FIN ){

                                int note_y = ((c_names_y-6) -
                                              ((c_names_y-6)  * (note - lowest_note)) / height) + 1;

                                int tick_s_x = ((tick_s * length_w)  / length) + tick_marker_x;
                                int tick_f_x = ((tick_f * length_w)  / length) + tick_marker_x;

                                if ( dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF )
                                    tick_f_x = tick_s_x + 1;
                                if ( tick_f_x <= tick_s_x )
                                    tick_f_x = tick_s_x + 1;

                                if ( tick_s_x < x ){
                                    tick_s_x = x;
                                }

                                if ( tick_f_x > x + w ){
                                    tick_f_x = x + w;
                                }

                                if ( tick_f_x >= x && tick_s_x <= x+w )
                                    mPainter->drawLine(tick_s_x,
                                                       y + note_y,
                                                       tick_f_x,
                                                       y + note_y);
                            }

                            tick_marker += seq_length;
                        }
                    }
                }
            }
        }
    }

    //draw border
    mPen->setStyle(Qt::SolidLine);
    mPen->setColor(Qt::black);
    mPainter->setPen(*mPen);
    mPainter->drawRect(0,
                       0,
                       width(),
                       height() - 1);

    //draw playhead
    long tick = m_mainperf->get_tick();

    int progress_x = tick / c_perf_scale_x ;

    mPen->setColor(Qt::red);
    mPen->setStyle(Qt::SolidLine);
    mPainter->setPen(*mPen);
    mPainter->drawLine(progress_x, 1,
                       progress_x, height());
}

int SongSequenceGrid::getSnap() const
{
    return m_snap;
}

void SongSequenceGrid::setSnap(int snap)
{
    m_snap = snap;
}

QSize SongSequenceGrid::sizeHint() const
{
    return QSize(m_roll_length_ticks, c_names_y * c_max_sequence + 2);
}

void SongSequenceGrid::mousePressEvent(QMouseEvent *event)
{
    if ( m_mainperf->is_active( m_drop_sequence ))
    {
        m_mainperf->get_sequence( m_drop_sequence )->unselect_triggers( );
    }

    m_drop_x = event->x();
    m_drop_y = event->y();

    convert_xy( m_drop_x, m_drop_y, &m_drop_tick, &m_drop_sequence );

    /* left mouse button */
    if ( event->button() == Qt::LeftButton){

        long tick = m_drop_tick;

        /* add a new seq instance if we didnt select anything,
         * and are holding the right mouse btn */
        if (m_adding){

            m_adding_pressed = true;

            if (m_mainperf->is_active(m_drop_sequence)){

                long seq_length = m_mainperf->get_sequence( m_drop_sequence )->getLength();

                bool trigger_state = m_mainperf->get_sequence(m_drop_sequence)->get_trigger_state(tick);

                if (trigger_state)
                {
                    m_mainperf->push_trigger_undo();
                    m_mainperf->get_sequence(m_drop_sequence)->del_trigger( tick );
                }
                else
                {
                    // snap to length of sequence
                    tick = tick - (tick % seq_length);

                    m_mainperf->push_trigger_undo();
                    m_mainperf->get_sequence( m_drop_sequence )->add_trigger( tick, seq_length );

                }
            }
        }
        /* we aren't holding the right mouse btn */
        else {

            if ( m_mainperf->is_active( m_drop_sequence )){

                m_mainperf->push_trigger_undo();
                m_mainperf->get_sequence( m_drop_sequence )->select_trigger( tick );

                long start_tick = m_mainperf->get_sequence( m_drop_sequence )->get_selected_trigger_start_tick();
                long end_tick = m_mainperf->get_sequence( m_drop_sequence )->get_selected_trigger_end_tick();

                if ( tick >= start_tick &&
                     tick <= start_tick + (c_perfroll_size_box_click_w * c_perf_scale_x) &&
                     (m_drop_y % c_names_y) <= c_perfroll_size_box_click_w + 1 )
                {
                    m_growing = true;
                    m_grow_direction = true;
                    m_drop_tick_trigger_offset = m_drop_tick -
                            m_mainperf->get_sequence( m_drop_sequence )->
                            get_selected_trigger_start_tick( );
                }
                else
                    if ( tick >= end_tick - (c_perfroll_size_box_click_w * c_perf_scale_x) &&
                         tick <= end_tick &&
                         (m_drop_y % c_names_y) >= c_names_y - c_perfroll_size_box_click_w - 1 )
                    {
                        m_growing = true;
                        m_grow_direction = false;
                        m_drop_tick_trigger_offset =
                                m_drop_tick -
                                m_mainperf->get_sequence( m_drop_sequence )->get_selected_trigger_end_tick( );
                    }
                    else
                    {
                        m_moving = true;
                        m_drop_tick_trigger_offset = m_drop_tick -
                                m_mainperf->get_sequence( m_drop_sequence )->
                                get_selected_trigger_start_tick( );

                    }

            }
        }
    }

    /* right mouse button */
    if (event->button() == Qt::RightButton)
    {
        set_adding(true);
    }

    /* middle mouse button, split seq under cursor */
    if ( event->button() == Qt::MiddleButton )
    {
        if ( m_mainperf->is_active( m_drop_sequence ))
        {
            bool state = m_mainperf->get_sequence( m_drop_sequence )->get_trigger_state( m_drop_tick );

            if ( state )
            {
                half_split_trigger(m_drop_sequence, m_drop_tick);
            }
        }
    }

}

void SongSequenceGrid::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if ( m_adding )
        {
            m_adding_pressed = false;
        }
    }

    if (event->button() == Qt::RightButton)
    {
        m_adding_pressed = false;
        set_adding(false);
    }

    m_moving = false;
    m_growing = false;
    m_adding_pressed = false;
}

void SongSequenceGrid::mouseMoveEvent(QMouseEvent *event)
{
    long tick;
    int x = event->x();

    if (  m_adding && m_adding_pressed ){

        convert_x( x, &tick );

        if ( m_mainperf->is_active( m_drop_sequence )){

            long seq_length = m_mainperf->get_sequence( m_drop_sequence )->getLength();
            tick = tick - (tick % seq_length);

            long length = seq_length;

            m_mainperf->get_sequence( m_drop_sequence )
                    ->grow_trigger( m_drop_tick, tick, length);
        }
    }
    else if ( m_moving || m_growing ){

        if ( m_mainperf->is_active( m_drop_sequence)){

            convert_x( x, &tick );
            tick -= m_drop_tick_trigger_offset;

            tick = tick - tick % m_snap;

            if ( m_moving )
            {
                m_mainperf->get_sequence( m_drop_sequence )
                        ->move_selected_triggers_to( tick, true );
            }
            if ( m_growing )
            {
                if ( m_grow_direction )
                    m_mainperf->get_sequence( m_drop_sequence )
                            ->move_selected_triggers_to( tick, false, 0 );
                else
                    m_mainperf->get_sequence( m_drop_sequence )
                            ->move_selected_triggers_to( tick-1, false, 1 );
            }
        }
    }
}

void SongSequenceGrid::keyPressEvent(QKeyEvent *event)
{

}

void SongSequenceGrid::keyReleaseEvent(QKeyEvent *event)
{

}

/* performs a 'snap' on x */
void SongSequenceGrid::snap_x( int *a_x )
{
    // snap = number pulses to snap to
    // m_scale = number of pulses per pixel
    //	so snap / m_scale  = number pixels to snap to

    int mod = (m_snap / c_perf_scale_x );

    if ( mod <= 0 )
        mod = 1;

    *a_x = *a_x - (*a_x % mod );
}


void SongSequenceGrid::convert_x( int a_x, long *a_tick )
{

//    long tick_offset = c_ppqn * 16;
    long tick_offset = 0;
    *a_tick = a_x * c_perf_scale_x;
    *a_tick += tick_offset;
}


void SongSequenceGrid::convert_xy( int a_x, int a_y, long *a_tick, int *a_seq)
{

//    long tick_offset =  c_ppqn * 16;
    long tick_offset =  0;

    *a_tick = a_x * c_perf_scale_x;
    *a_seq = a_y / c_names_y;

    *a_tick += tick_offset;

    if ( *a_seq >= c_total_seqs )
        *a_seq = c_total_seqs - 1;

    if ( *a_seq < 0 )
        *a_seq = 0;
}

void SongSequenceGrid::half_split_trigger( int a_sequence, long a_tick )
{
    m_mainperf->push_trigger_undo();
    m_mainperf->get_sequence( a_sequence )->half_split_trigger( a_tick );
}

/* simply sets the snap member */
void SongSequenceGrid::set_guides( int a_snap, int a_measure, int a_beat )
{
    m_snap = a_snap;
    m_measure_length = a_measure;
    m_beat_length = a_beat;
}

void SongSequenceGrid::set_adding(bool a_adding)
{
    if ( a_adding )
    {
        setCursor(Qt::PointingHandCursor);

        m_adding = true;

    }
    else
    {
        setCursor(Qt::ArrowCursor);

        m_adding = false;
    }
}
