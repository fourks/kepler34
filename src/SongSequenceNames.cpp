#include "SongSequenceNames.hpp"

///
/// \brief The SongSequenceNames class
///
/// Sequence labels for the side of the song editor

SongSequenceNames::SongSequenceNames(MidiPerformance *a_perf,
                                     QWidget *parent) :
    QWidget(parent),
    mPerf(a_perf)
{
    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::MinimumExpanding);

    for( int i=0; i<c_total_seqs; ++i )
        m_sequence_active[i]=false;
}

void SongSequenceNames::paintEvent(QPaintEvent *)
{
    mPainter = new QPainter(this);
    mPen = new QPen(Qt::black);
    mBrush = new QBrush(Qt::lightGray);
    mPen->setStyle(Qt::SolidLine);
    mBrush->setStyle((Qt::SolidPattern));
    mFont.setPointSize(6);
    mFont.setLetterSpacing(QFont::AbsoluteSpacing,
                           1);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    int y_s = 0;
    int y_f = height() / c_names_y;

    //draw border
    mPainter->drawRect(0,
                       0,
                       width(),
                       height() - 1);

    for ( int y = y_s; y <= y_f; y++ )
    {
        int seqId = y;

        if (seqId < c_total_seqs)
        {
            int i = seqId;
            //if first seq in bank
            if ( seqId % cSeqsInBank == 0 )
            {
                //black boxes to mark each bank
                mPen->setColor(Qt::black);
                mBrush->setColor(Qt::black);
                mBrush->setStyle(Qt::SolidPattern);
                mPainter->setPen(*mPen);
                mPainter->setBrush(*mBrush);
                mPainter->drawRect(1,
                                   (c_names_y * i) + 1,
                                   15,
                                   c_names_y - 1);

                char ss[3];
                int bankId = seqId / cSeqsInBank;
                snprintf(ss, sizeof(ss), "%2d", bankId );

                //draw bank number here
                mPen->setColor(Qt::white);
                mPainter->setPen(*mPen);
                mPainter->drawText(4,
                                   c_names_y * i + 15,
                                   ss);

                //offset and draw bank name sideways
                mPen->setColor(Qt::black);
                mPainter->setPen(*mPen);
                mPainter->save();
                QString bankName(mPerf->getBankName(bankId)->c_str());
                mPainter->translate(12,
                                    (c_names_y * i) +
                                    (c_names_y * cSeqsInBank * 0.5)
                                    + bankName.length() * 4);
                mPainter->rotate(270);
                mFont.setPointSize(9);
                mFont.setBold(true);
                mFont.setLetterSpacing(QFont::AbsoluteSpacing,
                                       2);
                mPainter->setFont(mFont);
                mPainter->drawText(0,
                                   0,
                                   bankName);
                mPainter->restore();
            }

            mPen->setStyle(Qt::SolidLine);
            mPen->setColor(Qt::black);
            if ( mPerf->is_active(seqId))
            {
                //get seq's assigned colour and beautify
                QColor colourSpec = QColor(colourMap.value(mPerf->getSequenceColour(seqId)));
                QColor backColour = QColor(colourSpec);
                if (backColour.value() != 255) //dont do this if we're white
                    backColour.setHsv(colourSpec.hue(),
                                      colourSpec.saturation() * 0.65,
                                      colourSpec.value() * 1.3);
                mBrush->setColor(backColour);
            }
            else
                mBrush->setColor(Qt::lightGray);

            // fill seq label background
            mPainter->setPen(*mPen);
            mPainter->setBrush(*mBrush);
            mPainter->drawRect(6 * 2 + 4,
                               (c_names_y * i),
                               c_names_x - 15,
                               c_names_y);

            if ( mPerf->is_active(seqId)){

                m_sequence_active[seqId]=true;

                //draw seq info on label
                char name[50];
                snprintf(name, sizeof(name), "%-14.14s                        %2d",
                         mPerf->get_sequence(seqId)->get_name(),
                         mPerf->get_sequence(seqId)->get_midi_channel() + 1);

                //seq name
                mPen->setColor(Qt::black);
                mPainter->setPen(*mPen);
                mPainter->drawText(18,
                                   c_names_y * i + 10,
                                   name);

                char str[20];
                snprintf(str, sizeof(str),
                         "%d-%d %ld/%ld",
                         mPerf->get_sequence(seqId)->get_midi_bus(),
                         mPerf->get_sequence(seqId)->get_midi_channel()+1,
                         mPerf->get_sequence(seqId)->getBeatsPerMeasure(),
                         mPerf->get_sequence(seqId)->getBeatWidth());

                //seq info
                mPainter->drawText(18,
                                   c_names_y * i + 20,
                                   str);

                bool muted = mPerf->get_sequence(seqId)->get_song_mute();

                mPen->setColor(Qt::black);
                mPainter->setPen(*mPen);
                mPainter->drawRect(6*2 + 6 * 20 + 2,
                                   (c_names_y * i),
                                   10,
                                   c_names_y  );

                //seq mute state
                if ( muted )
                {
                    mPainter->drawText(4 + 6*2 + 6 * 20,
                                       c_names_y * i + 14,
                                       "M");
                }
                else
                {
                    mPainter->drawText(4 + 6*2 + 6 * 20,
                                       c_names_y * i + 14,
                                       "M");
                }
            }
        }
    }
    delete mPen,mPainter,mBrush;
}


QSize SongSequenceNames::sizeHint() const
{
    return QSize(c_names_x, c_names_y * c_max_sequence + 1);
}

void SongSequenceNames::mousePressEvent(QMouseEvent *event)
{

}

void SongSequenceNames::mouseReleaseEvent(QMouseEvent *event)
{

}

void SongSequenceNames::mouseMoveEvent(QMouseEvent *event)
{

}

SongSequenceNames::~SongSequenceNames()
{
    delete mPen,mPainter,mBrush;
}
