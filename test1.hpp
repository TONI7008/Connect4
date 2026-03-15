#ifndef TEST1_HPP
#define TEST1_HPP

#include <QWidget>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPushButton>
#include <QMoveEvent>
#include "twidget.h"

class GameBoard : public TWidget {
    Q_OBJECT
public:
    GameBoard(QWidget *parent = nullptr) : TWidget(parent)
    {
        setMinimumSize(400,600);
        setStyleSheet("background-color:#2A180B; border:5px solid #3B2314;");

        stationary = new QWidget(this);
        stationary->setGeometry(150,250,100,100);
        stationary->setStyleSheet("background-color:transparent; border:2px solid white;");
        stationary->raise();

        piece = new QWidget(this);
        //piece->setGeometry(175,250,20,20);
        //piece->target = stationary;

        piece->move(100,20);   // IMPORTANT: initial position
        piece->lower();

        QPropertyAnimation* anim = new QPropertyAnimation(piece,"pos",this);

        anim->setDuration(5000);
        anim->setStartValue(QPoint(175,20));
        anim->setEndValue(QPoint(175,570));
        anim->setEasingCurve(QEasingCurve::InOutSine);

        //anim->setLoopCount(-1);  // infinite movement

        anim->start(QPropertyAnimation::DeleteWhenStopped);
        setEnableBackground(true);
        setBackgroundImage("/home/hacker/Documents/C++/Connect4/Images/Melamine-wood-005.png");
    }
protected:
    void paintEvent(QPaintEvent *event) override {
       TWidget::paintEvent(event);
        
       QPainter painter(this);
            
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        
        //painter.setBrush(Qt::transparent);
        painter.setPen(Qt::NoPen);

        const int cols = 7;
        const int rows = 6;
        const int margin = 5;

        // Calculate available space for the holes (subtracting the margins)
        // Formula: (Total Dimension - (Total Margins)) / Number of Items
        double holeW = (double)(width() - (margin * (cols + 1))) / cols;
        double holeH = (double)(height() - (margin * (rows + 1))) / rows;

        // Use the smaller dimension to keep holes circular, 
        // ensuring they are at least 40px
        double diameter = std::max(40.0, std::min(holeW, holeH));

        // Center the grid if the diameter is fixed at 40 (optional logic)
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                // Calculate X and Y to distribute them evenly
                double x = margin + c * (holeW + margin) + (holeW - diameter) / 2.0;
                double y = margin + r * (holeH + margin) + (holeH - diameter) / 2.0;
                QRectF holeRect(x, y, diameter, diameter);
                QPointF center = holeRect.center();
                

                QRadialGradient g(center, diameter / 2.0);
                g.setColorAt(1.0, QColor(0, 0, 0, 0));     // transparent at edge
                g.setColorAt(0.0, QColor(0, 0, 0, 120));   // darker center
                g.setColorAt(0.7, QColor(0, 0, 0, 40));    // fade

                painter.setBrush(g);
                painter.drawEllipse(holeRect);
            }
        }
        
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }
private:
    QWidget* piece=nullptr;
    QWidget* stationary=nullptr;

};

#endif
