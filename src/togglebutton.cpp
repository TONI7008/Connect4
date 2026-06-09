#include "togglebutton.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
//#include "thememanager.h"

ToggleButton::ToggleButton(QWidget *parent)
    : QPushButton(parent) {
    setCheckable(true);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(40, 20); // Set minimum size to prevent becoming too small

    initAnimations();
    updateAppearance();

    connect(this, &QPushButton::toggled,this,&ToggleButton::startAnimations);
    //connect(ThemeManager::instance(),&ThemeManager::dominantColorChanged,this,&ToggleButton::setActiveColor);

}
void ToggleButton::setChecked(bool checked) {
    if (isChecked() == checked) return;
    QPushButton::setChecked(checked);

    updateAppearance();
    startAnimations();
}


void ToggleButton::mousePressEvent(QMouseEvent *event) {
    QPushButton::mousePressEvent(event);
}

void ToggleButton::enterEvent(QEnterEvent *event) {
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void ToggleButton::leaveEvent(QEvent *event) {
    m_hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

ToggleButton::~ToggleButton() {

    delete m_positionAnimation;
    delete m_colorAnimation;
}

QSize ToggleButton::sizeHint() const {
    // Default size with good proportions (2:1 ratio)
    return QSize(60, 30);
}

int ToggleButton::thumbRadius() const {
    return height() * m_thumbRadiusRatio;
}

int ToggleButton::padding() const {
    return height() * m_paddingRatio;
}

int ToggleButton::cornerRadius() const {
    return height() * m_cornerRadiusRatio;
}

void ToggleButton::initAnimations() {
    m_positionAnimation = new QPropertyAnimation(this, "offset", this);
    m_positionAnimation->setEasingCurve(m_easingCurve);
    m_positionAnimation->setDuration(m_animationDuration);

    m_colorAnimation = new QPropertyAnimation(this, "animationProgress", this);
    m_colorAnimation->setEasingCurve(m_easingCurve);
    m_colorAnimation->setDuration(m_animationDuration);

    connect(m_positionAnimation, &QPropertyAnimation::finished,
            this, &ToggleButton::animationFinished);
}

// togglebutton.cpp
int ToggleButton::calculateEndPosition() const {
    int borderWidth = qMax(1, static_cast<int>(height() * m_borderRatio));
    if(m_borderEnabled) borderWidth=0;
    return isChecked()? width() - thumbRadius() * 2 - padding() - borderWidth/2 : padding() + borderWidth/2;
}

// togglebutton.cpp
void ToggleButton::paintEvent(QPaintEvent *event) {
    //QPushButton::paintEvent(event);
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);


    // Calculate border width based on height
    const int borderWidth = qMax(1, static_cast<int>(height() * m_borderRatio));
    const QRectF borderRect = QRectF(rect()).adjusted(borderWidth/2.0, borderWidth/2.0,
                                                      -borderWidth/2.0, -borderWidth/2.0);

    // Draw border
    if (isEnabled() && m_borderEnabled) {
        QPen borderPen(m_thumbColor, borderWidth);
        borderPen.setCosmetic(true);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(borderRect, cornerRadius(), cornerRadius());
    }

    // Draw background
    QPainterPath backgroundPath;
    backgroundPath.addRoundedRect(borderRect, cornerRadius(), cornerRadius());
    painter.fillPath(backgroundPath, currentBackgroundColor());

    // Draw thumb
    const int thumbDiameter = 2 * thumbRadius();
    const int yPos = (height() - thumbDiameter) / 2;

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_thumbColor);

    // Add shadow effect when enabled
    if (isEnabled()) {
        QRadialGradient shadowGrad(m_offset + thumbRadius(),
                                   yPos + thumbRadius(),
                                   thumbRadius() * 1.2);
        shadowGrad.setColorAt(0, QColor(0, 0, 0, 130));
        shadowGrad.setColorAt(1, Qt::transparent);
        painter.setBrush(shadowGrad);
        painter.drawEllipse(QPointF(m_offset + thumbRadius(),
                                    yPos + thumbRadius()),
                            thumbRadius() * 1.1, thumbRadius() * 1.1);
    }

    // Draw thumb
    painter.setBrush(m_thumbColor);
    painter.drawEllipse(m_offset, yPos, thumbDiameter, thumbDiameter);
}

void ToggleButton::setOffset(int offset) {
    if (m_offset != offset) {
        m_offset = offset;
        repaint();
    }
}

void ToggleButton::startAnimations() {
    const int endPos = calculateEndPosition();

    m_positionAnimation->stop();
    m_positionAnimation->setStartValue(m_offset);
    m_positionAnimation->setEndValue(endPos);

    m_colorAnimation->stop();
    m_colorAnimation->setStartValue(m_animationProgress);
    m_colorAnimation->setEndValue(isChecked() ? 1.0 : 0.0);

    m_positionAnimation->start();
    m_colorAnimation->start();
}

void ToggleButton::resizeEvent(QResizeEvent *event) {
    QPushButton::resizeEvent(event);
    updateAppearance();
}

void ToggleButton::updateAppearance() {
    setOffset(calculateEndPosition());
    setAnimationProgress(isChecked()? 1.0 : 0.0);

}

void ToggleButton::setAnimationProgress(qreal progress) {
    if (!qFuzzyCompare(m_animationProgress, progress)) {
        m_animationProgress = progress;
        update();
    }
}

QColor ToggleButton::currentBackgroundColor() const {
    if (!isEnabled()) {
        return m_disabledColor;
    }

    if (m_hovered && !isChecked()) {
        return m_hoverColor;
    }

    return QColor(
        m_inactiveColor.red() + (m_activeColor.red() - m_inactiveColor.red()) * m_animationProgress,
        m_inactiveColor.green() + (m_activeColor.green() - m_inactiveColor.green()) * m_animationProgress,
        m_inactiveColor.blue() + (m_activeColor.blue() - m_inactiveColor.blue()) * m_animationProgress
        );
}



bool ToggleButton::borderEnabled() const
{
    return m_borderEnabled;
}

void ToggleButton::setBorderEnabled(bool newBorderEnabled)
{
    m_borderEnabled = newBorderEnabled;
    repaint();
}

qreal ToggleButton::borderRatio() const
{
    return m_borderRatio;
}

void ToggleButton::setBorderRatio(qreal newBorderRatio)
{
    m_borderRatio = qBound(0.0, newBorderRatio, 0.1);
    repaint();
}

void ToggleButton::setActiveColor(const QColor &color) {
    if (m_activeColor != color) {
        m_activeColor = color;
        repaint();
    }
}

void ToggleButton::setInactiveColor(const QColor &color) {
    if (m_inactiveColor != color) {
        m_inactiveColor = color;
        repaint();
    }
}

void ToggleButton::setThumbColor(const QColor &color) {
    if (m_thumbColor != color) {
        m_thumbColor = color;
        repaint();
    }
}

void ToggleButton::setDisabledColor(const QColor &color) {
    if (m_disabledColor != color) {
        m_disabledColor = color;
        update();
    }
}

void ToggleButton::setAnimationDuration(int duration) {
    m_animationDuration = qMax(50, duration);
    m_positionAnimation->setDuration(m_animationDuration);
    m_colorAnimation->setDuration(m_animationDuration);
}

void ToggleButton::setEasing(QEasingCurve::Type easing) {
    m_easingCurve = easing;
    m_positionAnimation->setEasingCurve(m_easingCurve);
    m_colorAnimation->setEasingCurve(m_easingCurve);
}

void ToggleButton::setCornerRadiusRatio(qreal ratio) {
    m_cornerRadiusRatio = qBound(0.1, ratio, 0.5);
    repaint();
}
