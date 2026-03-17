#include "widgetfloater.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QSequentialAnimationGroup>

WidgetFloater::WidgetFloater(QWidget *target, QObject *parent)
    : QObject(parent)
    , m_target(target)
    , m_floatAmount(10)
    , m_duration(4000)  // 4 seconds for full cycle (matches CSS animation)
    , m_randomDelay(false)
    , m_pauseDuration(0)
    , m_isFloating(false)
    , m_currentCycle(0)
    , m_floatUpAnimation(nullptr)
    , m_floatDownAnimation(nullptr)
    , m_floatSequence(nullptr)
{
    if (!m_target) {
        qWarning() << "WidgetFloater: No target widget provided!";
        return;
    }
    
    // Set default easing curve to match CSS ease-in-out
    m_easingCurve.setType(QEasingCurve::InOutQuad);
    
    // Save the original position
    saveOriginalPosition();
    
    // Setup animations
    setupAnimations();
}

WidgetFloater::~WidgetFloater()
{
    stopFloating();

    if (m_floatUpAnimation) {
        m_floatUpAnimation->deleteLater();
    }
    if (m_floatDownAnimation) {
        m_floatDownAnimation->deleteLater();
    }
    if (m_floatSequence) {
        m_floatSequence->deleteLater();
    }
    

}

void WidgetFloater::saveOriginalPosition()
{
    if (m_target) {
        m_originalPosition = m_target->pos();
        m_floatOffset = 0;
    }
}

void WidgetFloater::setupAnimations()
{
    if (!m_target) return;
    
    // Clean up existing animations
    if (m_floatSequence) {
        m_floatSequence->stop();
        delete m_floatSequence;
    }
    
    // Create float up animation
    m_floatUpAnimation = new QPropertyAnimation(this, "floatOffset");
    m_floatUpAnimation->setDuration(m_duration / 2);  // Half for up, half for down
    m_floatUpAnimation->setStartValue(0);
    m_floatUpAnimation->setEndValue(-m_floatAmount);  // Negative for up
    m_floatUpAnimation->setEasingCurve(m_easingCurve);
    
    // Create float down animation
    m_floatDownAnimation = new QPropertyAnimation(this, "floatOffset");
    m_floatDownAnimation->setDuration(m_duration / 2);
    m_floatDownAnimation->setStartValue(-m_floatAmount);
    m_floatDownAnimation->setEndValue(0);
    m_floatDownAnimation->setEasingCurve(m_easingCurve);
    
    // Create sequence
    m_floatSequence = new QSequentialAnimationGroup(this);
    m_floatSequence->addAnimation(m_floatUpAnimation);
    m_floatSequence->addAnimation(m_floatDownAnimation);
    
    // Connect finished signal
    connect(m_floatSequence, &QSequentialAnimationGroup::finished, 
            this, &WidgetFloater::onAnimationFinished);
}

void WidgetFloater::setFloatOffset(float offset)
{
    if (!m_target) return;
    
    m_floatOffset = offset;
    // Update widget position
    m_target->move(m_originalPosition.x(), m_originalPosition.y() + offset);
}

void WidgetFloater::startFloating()
{
    if (!m_target || m_isFloating) return;
    
    // Reset to original position first
    setFloatOffset(0);
    
    m_isFloating = true;
    m_currentCycle = 0;
    
    // Start the animation
    if (m_floatSequence) {
        m_floatSequence->start();
        emit floatingStarted();
    }
}

void WidgetFloater::stopFloating()
{
    if (!m_isFloating) return;
    
    m_isFloating = false;
    
    if (m_floatSequence) {
        m_floatSequence->stop();
    }
    
    // Return to original position
    setFloatOffset(0);
    
    emit floatingStopped();
}

void WidgetFloater::setFloatingEnabled(bool enabled)
{
    if (enabled) {
        startFloating();
    } else {
        stopFloating();
    }
}

void WidgetFloater::onAnimationFinished()
{
    if (!m_isFloating) return;
    
    m_currentCycle++;
    emit cycleCompleted();
    
    // Handle pause between cycles if specified
    if (m_pauseDuration > 0) {
        QTimer::singleShot(m_pauseDuration, this, &WidgetFloater::startNextCycle);
    } else {
        startNextCycle();
    }
}

void WidgetFloater::startNextCycle()
{
    if (!m_isFloating) return;
    
    if (m_randomDelay) {
        // Add random delay between 0 and 1 second
        int randomDelay = QRandomGenerator::global()->bounded(1000);
        QTimer::singleShot(randomDelay, this, [this]() {
            if (m_isFloating && m_floatSequence) {
                m_floatSequence->start();
            }
        });
    } else {
        if (m_floatSequence) {
            m_floatSequence->start();
        }
    }
}

void WidgetFloater::setFloatAmount(int pixels)
{
    if (pixels != m_floatAmount && pixels > 0) {
        m_floatAmount = pixels;
        setupAnimations();  // Recreate animations with new values
        if (m_isFloating) {
            startFloating();  // Restart with new values
        }
    }
}

void WidgetFloater::setDuration(int milliseconds)
{
    if (milliseconds != m_duration && milliseconds > 0) {
        m_duration = milliseconds;
        setupAnimations();  // Recreate animations with new duration
        if (m_isFloating) {
            startFloating();  // Restart with new duration
        }
    }
}

void WidgetFloater::setEasingCurve(const QEasingCurve& curve)
{
    m_easingCurve = curve;
    setupAnimations();  // Recreate animations with new curve
    if (m_isFloating) {
        startFloating();  // Restart with new curve
    }
}

void WidgetFloater::setRandomDelay(bool enabled)
{
    m_randomDelay = enabled;
}

void WidgetFloater::setPauseBetweenCycles(int milliseconds)
{
    m_pauseDuration = qMax(0, milliseconds);
}

bool WidgetFloater::isFloating() const
{
    return m_isFloating;
}