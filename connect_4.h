#ifndef CONNECT_4_H
#define CONNECT_4_H

#include "twidget.h"
#include <QVector>
#include <QPair>

class Connect_4 : public TWidget
{
    Q_OBJECT

public:
    Connect_4(QWidget *parent = nullptr);
    ~Connect_4();

    double holeDiameter() const { return m_currentDiameter; }


signals:
    void mouseMoved(QPoint);
    void diameterChanged(short);
    void Clicked(QPointF pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;


private:    

    const static short Rows = 6;
    const static short Cols = 7;
    const  short Margin = 8;
    const  double minHoleDiameter = 40.0;

    double m_currentDiameter = minHoleDiameter;

    QPointF board[Rows][Cols];

};
#endif // CONNECT_4_H
