#include "circularwidget.h"

#include <QPaintEvent>
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
    painter.setRenderHint(QPainter::Antialiasing, true); // For smooth edges


    // Set pen (outline) and brush (fill) properties
    painter.setPen(Qt::NoPen);

    QPointF center(width() / 2.0, height() / 2.0);
    double diameter = std::min(width(), height());

    QRadialGradient g(center, diameter / 2.0);
    g.setColorAt(0.0, m_color.lighter(130));
    g.setColorAt(0.8, m_color);
    g.setColorAt(1.0, m_color.darker(150));

    painter.setBrush(g);

    // Calculate the diameter for a perfect circle within the widget's bounds
    // Calculate the top-left corner of the square are  a where the circle will be drawn

    double x = (width() - diameter) / 2.0;
    double y = (height() - diameter) / 2.0;

    // Draw the ellipse (circle)
    painter.drawEllipse(x, y, diameter, diameter);
    QWidget::paintEvent(event);

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
