#include "circularwidget.h"

#include <QPaintEvent>
#include <QPainterPath>
#include <QPainter>
#include <QResizeEvent>

CircularWidget::CircularWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);

}

CircularWidget::~CircularWidget(){

}

void CircularWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // For smooth edges

    QPainterPath path;
  
    double radius = std::min(width(), height()) / 2;

    QPointF center =rect().center();
    path.addEllipse(center, radius, radius);


    QRadialGradient g(center, radius);
    g.setColorAt(0.0, m_color.lighter(130));
    g.setColorAt(0.8, m_color);
    g.setColorAt(1.0, m_color.darker(150));

    painter.fillPath(path, g);
    //QWidget::paintEvent(event);

}

void CircularWidget::resizeEvent(QResizeEvent *event)
{

    int diameter = std::min(event->size().width(), event->size().height());
    int xOff = (event->size().width() - diameter) / 2;
    int yOff = (event->size().height() - diameter) / 2;
    setMask(QRegion(xOff, yOff, diameter, diameter, QRegion::Ellipse));
    QWidget::resizeEvent(event);
}

void CircularWidget::setColor(const QColor &newColor)
{
    m_color = newColor;
}
