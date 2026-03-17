#include "circularwidget.h"

#include <QPaintEvent>
#include <QPainterPath>
#include <QPainter>
#include <QResizeEvent>

CircularWidget::CircularWidget(QWidget* parent) : QWidget(parent) {
    //setAttribute(Qt::WA_TranslucentBackground);


}

CircularWidget::~CircularWidget(){

}

void CircularWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QPainterPath path;
    
    // Calculate proper dimensions
    int side = qMin(width(), height());
    QRectF squareRect(0, 0, side, side);
    squareRect.moveCenter(rect().center());
    
    // Using margin to ensure the circle doesn't touch the edges
    qreal margin = 2.0;
    qreal radius = (side / 2.0) - margin;
    QPointF center = squareRect.center();
    
    path.addEllipse(center, radius, radius);
    
    // Create radial gradient that starts from center
    QRadialGradient g(center, radius);
    g.setColorAt(0.0, m_color.lighter(130));
    g.setColorAt(0.7, m_color);
    g.setColorAt(0.9, m_color.darker(120));
    g.setColorAt(1.0, m_color.darker(150));
    
    // Fill the circle
    painter.fillPath(path, g);
    
    // Optional: Add a subtle border to define the edge
    QPen pen(m_color.darker(180), 1.5);
    pen.setCosmetic(true); // Keep border width consistent regardless of scaling
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    
    // Optional: Add a subtle highlight on the top-left for 3D effect
    QPainterPath highlightPath;
    QPointF highlightCenter(center.x() - radius * 0.3, center.y() - radius * 0.3);
    highlightPath.addEllipse(highlightCenter, radius * 0.2, radius * 0.2);
    
    QRadialGradient highlightGrad(highlightCenter, radius * 0.2);
    highlightGrad.setColorAt(0.0, QColor(255, 255, 255, 80));
    highlightGrad.setColorAt(1.0, QColor(255, 255, 255, 0));
    
    painter.fillPath(highlightPath, highlightGrad);
}


void CircularWidget::setColor(const QColor &newColor)
{
    m_color = newColor;
}
