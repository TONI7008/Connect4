#include "tstackedwidget.h"
#include <QPainter>
#include <QSequentialAnimationGroup>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QTimer>


TStackedWidget::TStackedWidget(QWidget* parent)
    : QStackedWidget(parent)
{
    setAnimationQuality(QPainter::Antialiasing);
}
TStackedWidget::~TStackedWidget()
{

}

void TStackedWidget::setCurrentIndex(int index, bool immediate)
{
    if (index < 0 || index >= count() || index == currentIndex()) {
        return;
    }
    if(m_isAnimating) return;

    executeAnimation(index, immediate);
}

void TStackedWidget::setCurrentWidget(QWidget *w, bool immediate)
{
    int index = indexOf(w);
    if (index != -1) {
        setCurrentIndex(index, immediate);
    }
}

void TStackedWidget::addWidget(QWidget* widget)
{
    QStackedWidget::addWidget(widget);
}

void TStackedWidget::setFadeOpacityRange(qreal start, qreal end)
{
    m_fadeStartOpacity = qBound(0.0, start, 1.0);
    m_fadeEndOpacity = qBound(0.0, end, 1.0);
}

void TStackedWidget::setAnimationQuality(QPainter::RenderHint hint)
{
    m_renderHint = hint;
}

void TStackedWidget::executeAnimation(int index, bool immediate)
{
    if (immediate || !isVisible()) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }

    m_isAnimating = true;
    emit animationStarted();

    switch (m_type) {
    case SlideFade:
        slideFadeAnimation(index);
        break;
    case HorizontalSlide:
        horizontalSlideAnimation(index);
        break;
    case VerticalSlide:
        verticalSlideAnimation(index);
        break;
    case AccordionVSlide:
        accordionAnimation(index);
        break;
    case CrossFade:
        crossFadeAnimation(index);
        break;
    case RotateAnim:
        rotateAnimation(index);
        break;
    case ZoomFade:
        zoomFadeAnimation(index);
        break;
    }
}

void TStackedWidget::slideFadeAnimation(int index)
{
    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget = widget(index);

    if (!currentWidget || !nextWidget) {
        m_isAnimating = false;
        return;
    }

    // Prepare effects
    QGraphicsOpacityEffect* fadeEffect = new QGraphicsOpacityEffect(currentWidget);
    currentWidget->setGraphicsEffect(fadeEffect);
    nextWidget->setGraphicsEffect(nullptr);

    // Show the next widget
    nextWidget->show();
    nextWidget->raise();

    // Set up geometries
    QRect currentEndGeometry = currentWidget->geometry();
    QRect nextStartGeometry = currentWidget->geometry();

    if (index > currentIndex()) {
        nextStartGeometry.moveTop(currentWidget->geometry().bottom());
    } else {
        nextStartGeometry.moveBottom(currentWidget->geometry().top());
    }

    //nextWidget->setGeometry(nextStartGeometry);

    // Create animation group
    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // Next widget slide animation
    QPropertyAnimation* slideAnim = new QPropertyAnimation(nextWidget, "geometry", group);
    slideAnim->setDuration(m_duration);
    slideAnim->setStartValue(nextStartGeometry);
    slideAnim->setEndValue(currentEndGeometry);
    slideAnim->setEasingCurve(m_curve);

    // Current widget fade animation
    if (m_blurEnabled) {
        QPropertyAnimation* fadeAnim = new QPropertyAnimation(fadeEffect, "opacity", group);
        fadeAnim->setDuration(m_duration);
        fadeAnim->setStartValue(m_fadeStartOpacity);
        fadeAnim->setEndValue(m_fadeEndOpacity);
    }

    // Connect finished signal
    connect(group, &QParallelAnimationGroup::finished, this, [this, index, currentWidget]() {
        QStackedWidget::setCurrentIndex(index);
        currentWidget->setGraphicsEffect(nullptr);
        m_isAnimating = false;
        emit animationFinished();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}


void TStackedWidget::horizontalSlideAnimation(int index)
{
    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget = widget(index);

    if (!currentWidget || !nextWidget) {
        m_isAnimating = false;
        return;
    }

    // Show the next widget
    nextWidget->show();
    nextWidget->raise();

    // Set up geometries
    QRect currentStartRect = currentWidget->geometry();
    QRect nextStartRect = currentWidget->geometry();
    QRect currentEndRect = currentWidget->geometry();
    QRect nextEndRect = currentWidget->geometry();

    if (index > currentIndex()) {
        nextStartRect.moveLeft(width());
        currentEndRect.moveLeft(-width());
    } else {
        nextStartRect.moveLeft(-width());
        currentEndRect.moveLeft(width());
    }

    //nextWidget->setGeometry(nextStartRect);

    // Create animation group
    QParallelAnimationGroup* group=new QParallelAnimationGroup(this);

    // Current widget animation
    QPropertyAnimation* currentAnim = new QPropertyAnimation(currentWidget, "geometry", group);
    currentAnim->setDuration(m_duration);
    currentAnim->setStartValue(currentStartRect);
    currentAnim->setEndValue(currentEndRect);
    currentAnim->setEasingCurve(m_curve);

    // Next widget animation
    QPropertyAnimation* nextAnim = new QPropertyAnimation(nextWidget, "geometry", group);
    nextAnim->setDuration(m_duration);
    nextAnim->setStartValue(nextStartRect);
    nextAnim->setEndValue(nextEndRect);
    nextAnim->setEasingCurve(m_curve);

    connect(group, &QParallelAnimationGroup::finished, this, [this,group, index]() {
        QStackedWidget::setCurrentIndex(index);
        m_isAnimating = false;
        emit animationFinished();
        group->clear();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void TStackedWidget::verticalSlideAnimation(int index)
{
    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget = widget(index);

    if (!currentWidget || !nextWidget) {
        m_isAnimating = false;
        return;
    }

    // Show the next widget
    nextWidget->show();
    nextWidget->raise();

    // Set up geometries
    QRect currentStartRect = currentWidget->geometry();
    QRect nextStartRect = currentWidget->geometry();
    QRect currentEndRect = currentWidget->geometry();
    QRect nextEndRect = currentWidget->geometry();

    if (index > currentIndex()) {
        nextStartRect.moveTop(height());
        currentEndRect.moveTop(-height());
    } else {
        nextStartRect.moveTop(-height());
        currentEndRect.moveTop(height());
    }

    nextWidget->setGeometry(nextStartRect);

    // Create animation group
    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // Current widget animation
    QPropertyAnimation* currentAnim = new QPropertyAnimation(currentWidget, "geometry", group);
    currentAnim->setDuration(m_duration);
    currentAnim->setStartValue(currentStartRect);
    currentAnim->setEndValue(currentEndRect);
    currentAnim->setEasingCurve(m_curve);

    // Next widget animation
    QPropertyAnimation* nextAnim = new QPropertyAnimation(nextWidget, "geometry", group);
    nextAnim->setDuration(m_duration);
    nextAnim->setStartValue(nextStartRect);
    nextAnim->setEndValue(nextEndRect);
    nextAnim->setEasingCurve(m_curve);

    // Connect finished signal
    connect(group, &QParallelAnimationGroup::finished, this, [this,group, index]() {
        QStackedWidget::setCurrentIndex(index);
        m_isAnimating = false;
        emit animationFinished();
        group->clear();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void TStackedWidget::accordionAnimation(int index)
{
    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget = widget(index);

    if (!currentWidget || !nextWidget) {
        m_isAnimating = false;
        return;
    }

    nextWidget->show();
    nextWidget->raise();

    QRect currentStart = currentWidget->geometry();
    QRect nextStart = currentWidget->geometry();
    QRect currentEnd = currentWidget->geometry();
    QRect nextEnd = currentWidget->geometry();

    bool movingDown = index > currentIndex();

    if (movingDown) {
        currentStart.setHeight(height());
        currentEnd.setHeight(0);
        nextStart.setHeight(0);
        nextEnd.setHeight(height());
    } else {
        currentStart.setHeight(height());
        currentEnd.setHeight(0);
        nextStart.setHeight(0);
        nextEnd.setHeight(height());
    }

    nextWidget->setGeometry(nextStart);

    QSequentialAnimationGroup* sequence = new QSequentialAnimationGroup(this);

    // First phase - current widget collapses
    QParallelAnimationGroup* collapseGroup = new QParallelAnimationGroup;
    QPropertyAnimation* currentCollapse = new QPropertyAnimation(currentWidget, "geometry", collapseGroup);
    currentCollapse->setDuration(m_duration/2);
    currentCollapse->setStartValue(currentStart);
    currentCollapse->setEndValue(currentEnd);
    currentCollapse->setEasingCurve(QEasingCurve::InQuad);
    collapseGroup->addAnimation(currentCollapse);

    // Second phase - next widget expands
    QParallelAnimationGroup* expandGroup = new QParallelAnimationGroup;
    QPropertyAnimation* nextExpand = new QPropertyAnimation(nextWidget, "geometry", expandGroup);
    nextExpand->setDuration(m_duration/2);
    nextExpand->setStartValue(nextStart);
    nextExpand->setEndValue(nextEnd);
    nextExpand->setEasingCurve(QEasingCurve::OutQuad);
    expandGroup->addAnimation(nextExpand);

    sequence->addAnimation(collapseGroup);
    sequence->addAnimation(expandGroup);

    connect(sequence, &QSequentialAnimationGroup::finished, this, [this, index]() {
        QStackedWidget::setCurrentIndex(index);
        m_isAnimating = false;
        emit animationFinished();
    });

    sequence->start(QAbstractAnimation::DeleteWhenStopped);
}

void TStackedWidget::crossFadeAnimation(int index)
{
    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget = widget(index);

    if (!currentWidget || !nextWidget) {
        m_isAnimating = false;
        return;
    }

    // Prepare effects
    QGraphicsOpacityEffect* currentEffect = new QGraphicsOpacityEffect(currentWidget);
    QGraphicsOpacityEffect* nextEffect = new QGraphicsOpacityEffect(nextWidget);
    currentWidget->setGraphicsEffect(currentEffect);
    nextWidget->setGraphicsEffect(nextEffect);

    nextWidget->show();
    nextWidget->raise();
    nextWidget->setGeometry(currentWidget->geometry());

    // Initial states
    currentEffect->setOpacity(1.0);
    nextEffect->setOpacity(0.0);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // Current widget fade out
    QPropertyAnimation* currentFade = new QPropertyAnimation(currentEffect, "opacity", group);
    currentFade->setDuration(m_duration);
    currentFade->setStartValue(1.0);
    currentFade->setEndValue(0.0);
    currentFade->setEasingCurve(m_curve);

    // Next widget fade in
    QPropertyAnimation* nextFade = new QPropertyAnimation(nextEffect, "opacity", group);
    nextFade->setDuration(m_duration);
    nextFade->setStartValue(0.0);
    nextFade->setEndValue(1.0);
    nextFade->setEasingCurve(m_curve);

    connect(group, &QParallelAnimationGroup::finished, this, [this, index, currentWidget, nextWidget]() {
        currentWidget->setGraphicsEffect(nullptr);
        nextWidget->setGraphicsEffect(nullptr);
        QStackedWidget::setCurrentIndex(index);
        m_isAnimating = false;
        emit animationFinished();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void TStackedWidget::rotateAnimation(int index)
{
    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget = widget(index);

    if (!currentWidget || !nextWidget || currentWidget == nextWidget) {
        return;
    }

    // Prepare effects
    QGraphicsOpacityEffect* currentEffect = new QGraphicsOpacityEffect(currentWidget);
    QGraphicsOpacityEffect* nextEffect = new QGraphicsOpacityEffect(nextWidget);
    currentWidget->setGraphicsEffect(currentEffect);
    nextWidget->setGraphicsEffect(nextEffect);

    // Position and show next widget
    nextWidget->setGeometry(currentWidget->geometry());
    nextWidget->show();
    nextWidget->raise();

    // Initial states
    currentEffect->setOpacity(1.0);
    nextEffect->setOpacity(0.0);

    // Calculate direction
    int xOffset = (index > currentIndex()) ? width() : -width();

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // Current widget slide + fade
    QPropertyAnimation* currentSlide = new QPropertyAnimation(currentWidget, "pos", group);
    currentSlide->setDuration(m_duration);
    currentSlide->setStartValue(QPoint(0, 0));
    currentSlide->setEndValue(QPoint(-xOffset/2, 0));
    currentSlide->setEasingCurve(QEasingCurve::InBack);

    QPropertyAnimation* currentFade = new QPropertyAnimation(currentEffect, "opacity", group);
    currentFade->setDuration(m_duration);
    currentFade->setStartValue(1.0);
    currentFade->setEndValue(0.0);

    // Next widget slide + fade
    QPropertyAnimation* nextSlide = new QPropertyAnimation(nextWidget, "pos", group);
    nextSlide->setDuration(m_duration);
    nextSlide->setStartValue(QPoint(xOffset, 0));
    nextSlide->setEndValue(QPoint(0, 0));
    nextSlide->setEasingCurve(QEasingCurve::OutBack);

    QPropertyAnimation* nextFade = new QPropertyAnimation(nextEffect, "opacity", group);
    nextFade->setDuration(m_duration);
    nextFade->setStartValue(0.0);
    nextFade->setEndValue(1.0);

    connect(group, &QParallelAnimationGroup::finished, this, [this, index, currentWidget, nextWidget]() {
        QStackedWidget::setCurrentIndex(index);
        currentWidget->move(0, 0); // Reset position
        nextWidget->move(0, 0);   // Reset position
        cleanupAfterAnimation();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}
void TStackedWidget::zoomFadeAnimation(int index)
{
    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget = widget(index);

    if (!currentWidget || !nextWidget || currentWidget == nextWidget) {
        return;
    }

    // Prepare effects with size animation
    QGraphicsOpacityEffect* currentEffect = new QGraphicsOpacityEffect(currentWidget);
    QGraphicsOpacityEffect* nextEffect = new QGraphicsOpacityEffect(nextWidget);
    currentWidget->setGraphicsEffect(currentEffect);
    nextWidget->setGraphicsEffect(nextEffect);

    // Position and show next widget
    nextWidget->setGeometry(currentWidget->geometry());
    nextWidget->show();
    nextWidget->raise();

    // Initial states
    currentEffect->setOpacity(1.0);
    nextEffect->setOpacity(0.0);

    // Create animation group
    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // Current widget fade out + "zoom out" (simulated by growing)
    QPropertyAnimation* currentSize = new QPropertyAnimation(currentWidget, "size", group);
    currentSize->setDuration(m_duration);
    currentSize->setStartValue(currentWidget->size());
    currentSize->setEndValue(currentWidget->size() * 1.2);
    currentSize->setEasingCurve(QEasingCurve::InBack);

    QPropertyAnimation* currentFade = new QPropertyAnimation(currentEffect, "opacity", group);
    currentFade->setDuration(m_duration);
    currentFade->setStartValue(1.0);
    currentFade->setEndValue(0.0);

    // Next widget fade in + "zoom in" (simulated by shrinking from large)
    nextWidget->resize(currentWidget->size() * 1.2);

    QPropertyAnimation* nextSize = new QPropertyAnimation(nextWidget, "size", group);
    nextSize->setDuration(m_duration);
    nextSize->setStartValue(nextWidget->size());
    nextSize->setEndValue(currentWidget->size());
    nextSize->setEasingCurve(QEasingCurve::OutBack);

    QPropertyAnimation* nextFade = new QPropertyAnimation(nextEffect, "opacity", group);
    nextFade->setDuration(m_duration);
    nextFade->setStartValue(0.0);
    nextFade->setEndValue(1.0);

    // Center widgets during animation
    QPropertyAnimation* currentPos = new QPropertyAnimation(currentWidget, "pos", group);
    currentPos->setDuration(m_duration);
    currentPos->setStartValue(currentWidget->pos());
    currentPos->setEndValue(QPoint(
        currentWidget->x() - (currentWidget->width() * 0.1),
        currentWidget->y() - (currentWidget->height() * 0.1)
        ));

    QPropertyAnimation* nextPos = new QPropertyAnimation(nextWidget, "pos", group);
    nextPos->setDuration(m_duration);
    nextPos->setStartValue(QPoint(
        nextWidget->x() - (nextWidget->width() * 0.1),
        nextWidget->y() - (nextWidget->height() * 0.1)
        ));
    nextPos->setEndValue(currentWidget->pos());

    connect(group, &QParallelAnimationGroup::finished, this, [this, index, currentWidget, nextWidget]() {
        QStackedWidget::setCurrentIndex(index);
        currentWidget->setGraphicsEffect(nullptr);
        nextWidget->setGraphicsEffect(nullptr);
        currentWidget->resize(this->size()); // Reset size
        nextWidget->resize(this->size());    // Reset size
        cleanupAfterAnimation();

    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void TStackedWidget::setCurve(QEasingCurve curve)
{
    m_curve = curve;
}

void TStackedWidget::setAnimationDuration(int duration)
{
    m_duration = qMax(166, duration); // Minimum 50ms duration
}

void TStackedWidget::setAnimationType(Type type)
{
    m_type = type;
}

void TStackedWidget::setBlurEffectEnabled(bool enabled)
{
    m_blurEnabled = enabled;
}


void TStackedWidget::cleanupAfterAnimation()
{
    m_isAnimating=false;
    emit animationFinished();

    update();
}

