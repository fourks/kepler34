#ifndef EDITKEYS_HPP
#define EDITKEYS_HPP

#include "MidiSequence.hpp"

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QPen>

class EditKeys : public QWidget
{
    Q_OBJECT
public:
    explicit EditKeys(MidiSequence *a_seq,
                      QWidget *parent);

protected:
    //override painting event to draw on the frame
    void paintEvent         (QPaintEvent *);

    //override mouse events for interaction
    void mousePressEvent    (QMouseEvent * event);
    void mouseReleaseEvent  (QMouseEvent * event);
    void mouseMoveEvent     (QMouseEvent * event);

signals:

public slots:

private:
    MidiSequence *m_seq;

    QTimer      *m_timer;
    QPen        *m_pen;
    QPainter    *m_painter;
    QRect        m_rect;
    QPoint       m_point;
    QFont        m_font;
    QLine        m_line;

};

#endif // EDITKEYS_HPP