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
/*
void CircularWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QPainterPath path;
    
    // Calculate proper dimensions
    int side = qMin(width(), height());
    QRectF squareRect(0, 0, side, side);
    squareRect.moveCenter(rect().center());
    
    // Using margin to ensure the circle doesn't touch the edges
    qreal margin = 0.75;
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
*/
void CircularWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int side = qMin(width(), height());
    QRectF r(0, 0, side, side);
    r.moveCenter(rect().center());

    QPointF c = r.center();
    double margin = 1.1; // margin to prevent clipping
   
    qreal radius = side / 2.0 - margin; // small margin to prevent clipping

    QPainterPath path;
    path.addEllipse(c, radius, radius);

    // =========================
    // 1. BASE BODY (volume)
    // =========================
    QRadialGradient baseGrad(c, radius);
    baseGrad.setColorAt(0.0, m_color.lighter(140));   // inner light
    baseGrad.setColorAt(0.5, m_color);                // mid tone
    baseGrad.setColorAt(0.85, m_color.darker(130));   // falloff
    baseGrad.setColorAt(1.0, m_color.darker(180));    // edge shadow

    p.fillPath(path, baseGrad);

    // =========================
    // 2. INNER SHADOW (depth)
    // =========================
    QRadialGradient innerShadow(c, radius * 0.9);
    innerShadow.setColorAt(0.7, QColor(0,0,0,0));
    innerShadow.setColorAt(1.0, QColor(0,0,0,80));

    p.fillPath(path, innerShadow);

    // =========================
    // 3. RIM LIGHT (plastic edge)
    // =========================
    QPen rimPen(m_color.lighter(100), 0.5);
    rimPen.setCosmetic(true);
    p.setPen(rimPen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, radius - 1, radius - 1);

    // darker outer edge for contrast
    QPen outerPen(m_color.darker(200), 2.0);
    outerPen.setCosmetic(true);
    p.setPen(outerPen);
    p.drawEllipse(c, radius, radius);

    // =========================
    // 4. SPECULAR HIGHLIGHT (gloss)
    // =========================
    QPointF hCenter(c.x() - radius * 0.35, c.y() - radius * 0.35);

    QRadialGradient highlight(hCenter, radius * 0.4);
    highlight.setColorAt(0.0, QColor(255,255,255,180));
    highlight.setColorAt(0.3, QColor(255,255,255,80));
    highlight.setColorAt(1.0, QColor(255,255,255,0));

    QPainterPath hPath;
    hPath.addEllipse(hCenter, radius * 0.4, radius * 0.4);
    p.fillPath(hPath, highlight);

    // =========================
    // 5. SUBTLE BOTTOM REFLECTION
    // =========================
    QPointF bCenter(c.x() + radius * 0.2, c.y() + radius * 0.3);

    QRadialGradient bounce(bCenter, radius * 0.5);
    bounce.setColorAt(0.0, QColor(255,255,255,40));
    bounce.setColorAt(1.0, QColor(255,255,255,0));

    QPainterPath bPath;
    bPath.addEllipse(bCenter, radius * 0.5, radius * 0.5);
    p.fillPath(bPath, bounce);
}


void CircularWidget::setColor(const QColor &newColor)
{
    m_color = newColor;
}
