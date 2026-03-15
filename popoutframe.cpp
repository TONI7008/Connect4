#include "popoutframe.h"
#include <QPainter>
#include <QDebug>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QTimer>

PopoutFrame::PopoutFrame(QWidget* parent) : TFrame(parent), animationGroup(new QSequentialAnimationGroup(this)) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setAttribute(Qt::WA_Hover);

    setWindowFlag(Qt::FramelessWindowHint);

    setMouseTracking(true);
    setCornerStyle(TFrame::CornerStyle::Default);
    disableBackground();


}

PopoutFrame::~PopoutFrame() {
    if (shrinkAnim) shrinkAnim->deleteLater();
    if (expandAnim) expandAnim->deleteLater();
    animationGroup->deleteLater();
}

void PopoutFrame::popout() {
    popout(m_popoutDuration, m_popoutScale);
}

QRect PopoutFrame::updateExpandedGeometry() {
    QRect expandedGeometry = calculateExpandedGeometry(originalGeometry);

    // Apply max width constraint if set
    if (m_popoutMaxWidth > 0 && expandedGeometry.width() > m_popoutMaxWidth) {
        int newWidth = m_popoutMaxWidth;
        int newX = originalGeometry.x() + (originalGeometry.width() - newWidth) / 2;
        expandedGeometry.setX(newX);
        expandedGeometry.setWidth(newWidth);
    }

    // Apply max height constraint if set
    if (m_popoutMaxHeight > 0 && expandedGeometry.height() > m_popoutMaxHeight) {
        int newHeight = m_popoutMaxHeight;
        int newY = originalGeometry.y() + (originalGeometry.height() - newHeight) / 2;
        expandedGeometry.setY(newY);
        expandedGeometry.setHeight(newHeight);
    }

    return expandedGeometry;
}

void PopoutFrame::setPopoutDuration(int duration) {
    if (duration > 0) {
        m_popoutDuration = duration;
    }
}

void PopoutFrame::setPopoutScale(qreal scale) {
    if (scale > 1.0) {
        m_popoutScale = scale;
    }
}

void PopoutFrame::setEasingCurve(QEasingCurve::Type curve) {
    currentCurve = curve;
}

void PopoutFrame::setPopoutColor(const QColor& color) {
    borderColor = color;
    QString style=styleSheet();
    style.replace("rgb(71,158,245)",color.name());
    setStyleSheet(style);
    repaint();
}

void PopoutFrame::setPopoutMaxWidth(int maxWidth) {
    m_popoutMaxWidth = maxWidth;
}

void PopoutFrame::setPopoutMaxHeight(int maxHeight) {
    m_popoutMaxHeight = maxHeight;
}

void PopoutFrame::setPopoutDirection(PopoutDirections direction) {
    m_direction = direction;
}

void PopoutFrame::moveEvent(QMoveEvent* event) {
    if (geometry().size() == originalGeometry.size()) {
        originalGeometry.moveTo(event->pos());
    }
    QFrame::moveEvent(event);
}

void PopoutFrame::resizeEvent(QResizeEvent *event) {
    TFrame::resizeEvent(event);
}

void PopoutFrame::paintEvent(QPaintEvent *event) {
    TFrame::paintEvent(event);
}

void PopoutFrame::popout(int duration, qreal scale) {
    raise();
    m_popoutScale = scale;
    originalGeometry = geometry();
    QRect expanded = calculateExpandedGeometry(originalGeometry);

    delete expandAnim;
    delete shrinkAnim;

    expandAnim = new QPropertyAnimation(this, "geometry");
    expandAnim->setDuration(duration);
    expandAnim->setStartValue(originalGeometry);
    expandAnim->setEndValue(expanded);
    expandAnim->setEasingCurve(currentCurve);

    shrinkAnim = new QPropertyAnimation(this, "geometry");
    shrinkAnim->setDuration(duration / 2);
    shrinkAnim->setStartValue(expanded);
    shrinkAnim->setEndValue(originalGeometry);
    shrinkAnim->setEasingCurve(currentCurve);

    animationGroup->clear();
    animationGroup->addAnimation(expandAnim);
    animationGroup->addAnimation(shrinkAnim);
    animationGroup->start();

    repaint();
}

QRect PopoutFrame::calculateExpandedGeometry(const QRect& baseGeometry) {
    int dw = baseGeometry.width() * (m_popoutScale - 1.0) * m_widthExpansionFactor;
    int dh = baseGeometry.height() * (m_popoutScale - 1.0);

    QSize expandedSize = baseGeometry.size();
    expandedSize += QSize(dw, dh);

    // Apply max size constraints
    if (m_popoutMaxWidth > 0 && expandedSize.width() > m_popoutMaxWidth) {
        expandedSize.setWidth(m_popoutMaxWidth);
    }
    if (m_popoutMaxHeight > 0 && expandedSize.height() > m_popoutMaxHeight) {
        expandedSize.setHeight(m_popoutMaxHeight);
    }

    QPoint expandedPos = calculateExpandedPosition(baseGeometry, expandedSize);
    return QRect(expandedPos, expandedSize);
}

QPoint PopoutFrame::calculateExpandedPosition(const QRect& baseGeometry, const QSize& expandedSize) {
    int dx = expandedSize.width() - baseGeometry.width();
    int dy = expandedSize.height() - baseGeometry.height();

    // Handle combined directions
    if (m_direction == (Right | Bottom)) {
        return QPoint(baseGeometry.x(),
                      baseGeometry.y());
    }
    else if (m_direction == (Right | Top)) {
        return QPoint(baseGeometry.x(),
                      baseGeometry.y() - dy);
    }
    else if (m_direction == (Left | Bottom)) {
        return QPoint(baseGeometry.x() - dx,
                      baseGeometry.y());
    }
    else if (m_direction == (Left | Top)) {
        return QPoint(baseGeometry.x() - dx,
                      baseGeometry.y() - dy);
    }
    // Handle single directions
    else if (m_direction & Top) {
        return QPoint(baseGeometry.x() - dx / 2,
                      baseGeometry.y() - dy);
    }
    else if (m_direction & Bottom) {
        return QPoint(baseGeometry.x() - dx / 2,
                      baseGeometry.y());
    }
    else if (m_direction & Left) {
        return QPoint(baseGeometry.x() - dx,
                      baseGeometry.y() - dy / 2);
    }
    else if (m_direction & Right) {
        return QPoint(baseGeometry.x(),
                      baseGeometry.y() - dy / 2);
    }
    // Default to Center
    return QPoint(baseGeometry.center().x() - expandedSize.width() / 2,
                  baseGeometry.center().y() - expandedSize.height() / 2);
}
qreal PopoutFrame::widthExpansionFactor() const
{
    return m_widthExpansionFactor;
}

void PopoutFrame::setWidthExpansionFactor(qreal newWidthExpansionFactor)
{
    m_widthExpansionFactor = newWidthExpansionFactor;
}


void PopoutFrame::setMaximumSize(const QSize& size) {
    setPopoutMaxWidth(size.width());
    setPopoutMaxHeight(size.height());
    QFrame::setMaximumSize(size);
}

void PopoutFrame::enterEvent(QEnterEvent* event) {
    if(isHovered) return;
    isHovered = true;
    emit hoverStarted();

    if (animationGroup->state() == QAbstractAnimation::Running){
        originalGeometry = geometry();

        startShrinkAnimation();
        repaint();
        return;
    }

    originalGeometry = geometry();

    startExpandAnimation();
    repaint();
    QFrame::enterEvent(event);
}

void PopoutFrame::leaveEvent(QEvent* event) {
    if (!isHovered) return;
    isHovered = false;
    emit hoverEnded();

    if (expandAnim && expandAnim->state() == QAbstractAnimation::Running){
        expandAnim->stop();
        startShrinkAnimation();
        return;
    }

    startShrinkAnimation();
    repaint();
    QFrame::leaveEvent(event);
}



void PopoutFrame::startExpandAnimation() {
    expandAnim = new QPropertyAnimation(this, "geometry");
    expandAnim->setDuration(m_popoutDuration/2);
    expandAnim->setStartValue(geometry());
    expandAnim->setEndValue(calculateExpandedGeometry(geometry()));
    expandAnim->setEasingCurve(currentCurve);
    expandAnim->start();
}

void PopoutFrame::startShrinkAnimation() {
    shrinkAnim = new QPropertyAnimation(this, "geometry");
    shrinkAnim->setDuration(m_popoutDuration / 2);
    shrinkAnim->setStartValue(geometry());
    shrinkAnim->setEndValue(originalGeometry);
    shrinkAnim->setEasingCurve(currentCurve);
    connect(shrinkAnim, &QPropertyAnimation::finished, this, [this]() {
        update();
    });

    shrinkAnim->start();
}
