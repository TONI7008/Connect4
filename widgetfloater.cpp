#include "widgetfloater.h"
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QSequentialAnimationGroup>
#include <QScreen>

WidgetFloater::WidgetFloater(QWidget *target, QObject *parent)
    : QObject(parent)
    , m_target(target)
    , m_floatAmount(10)
    , m_duration(4000)
    , m_randomDelay(false)
    , m_pauseDuration(0)
    , m_floatOffset(0)
    , m_autoReposition(true)
    , m_repositionDelay(300)
    , m_repositionTimer(nullptr)
    , m_pendingReposition(false)
    , m_isFloating(false)
    , m_currentCycle(0)
    , m_floatUpAnimation(nullptr)
    , m_floatDownAnimation(nullptr)
    , m_floatSequence(nullptr)
    , m_parentEventFilterInstalled(false)
{
    if (!m_target) {
        qWarning() << "WidgetFloater: No target widget provided!";
        return;
    }
    
    // Get parent widget
    m_parent = m_target->parentWidget();
    
    // Set default easing curve to match CSS ease-in-out
    m_easingCurve.setType(QEasingCurve::InOutQuad);
    
    // Save the original position
    saveOriginalPosition();
    
    // Setup animations
    setupAnimations();
    
    // Install event filter on parent
    installParentEventFilter();
    
    // Create reposition timer
    m_repositionTimer = new QTimer(this);
    m_repositionTimer->setSingleShot(true);
    m_repositionTimer->setInterval(m_repositionDelay);
    connect(m_repositionTimer, &QTimer::timeout, this, &WidgetFloater::delayedReposition);
}

WidgetFloater::~WidgetFloater()
{
    stopFloating();
    
    if (m_parent && m_parentEventFilterInstalled) {
        m_parent->removeEventFilter(this);
    }
    m_floatSequence->deleteLater();
    m_floatDownAnimation->deleteLater();
    m_floatUpAnimation->deleteLater();
    m_repositionTimer->deleteLater();
}

void WidgetFloater::installParentEventFilter()
{
    if (!m_parent || m_parentEventFilterInstalled) return;
    
    m_parent->installEventFilter(this);
    m_parentEventFilterInstalled = true;
}

bool WidgetFloater::eventFilter(QObject *obj, QEvent *event)
{
    if (!m_autoReposition || !isValid()) {
        return QObject::eventFilter(obj, event);
    }
    
    // Check if this is the parent widget
    if (obj == m_parent) {
        switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Move:
                // Parent moved or resized, schedule reposition
                if (!m_pendingReposition) {
                    m_pendingReposition = true;
                    QTimer::singleShot(0, this, &WidgetFloater::onParentResized);
                }
                break;
                
            case QEvent::Show:
                // Parent shown, update position
                QTimer::singleShot(100, this, &WidgetFloater::onParentResized);
                break;
                
            default:
                break;
        }
    }
    
    // Also watch the target widget for geometry changes
    if (obj == m_target && event->type() == QEvent::Move) {
        // Target was moved manually, update original position
        if (!m_isFloating) {
            saveOriginalPosition();
        }
    }
    
    return QObject::eventFilter(obj, event);
}

void WidgetFloater::saveOriginalPosition()
{
    if (!isValid()) return;
    
    m_originalPosition = m_target->pos();
    m_lastKnownPosition = m_originalPosition;
    m_floatOffset = 0;
}

void WidgetFloater::updateOriginalPosition()
{
    if (!isValid()) return;
    
    m_originalPosition = m_target->pos();
    m_lastKnownPosition = m_originalPosition;
}

void WidgetFloater::setOriginalPosition(const QPoint& pos)
{
    m_originalPosition = pos;
    m_lastKnownPosition = pos;
    
    if (m_isFloating) {
        // Restart animation with new position
        stopFloating();
        startFloating();
    }
}

void WidgetFloater::onParentResized()
{
    if (!isValid()) return;
    
    // Cancel any pending reposition
    if (m_repositionTimer->isActive()) {
        m_repositionTimer->stop();
    }
    
    // Start the reposition timer
    m_repositionTimer->start();
}

void WidgetFloater::delayedReposition()
{
    if (!isValid()) return;
    if(m_parent) m_parent->updateGeometry(); // Ensure parent layout is updated before repositioning
    //m_parent->layout()->activate(); // Activate layout to get correct positions
    m_pendingReposition = false;
    
    // Get the new position from layout
    QPoint newPos = m_target->pos();
    
    // Check if position actually changed
    if (newPos != m_lastKnownPosition) {
        bool wasFloating = m_isFloating;
        
        // Stop floating temporarily
        if (wasFloating) {
            stopFloating();
        }
        
        // Update original position
        m_originalPosition = newPos;
        m_lastKnownPosition = newPos;
        
        // Move widget to new position (without animation)
        m_target->setGeometry(QRect(newPos, m_target->size()));
        //m_target->move(newPos);
        
        // Restart floating if it was active
        if (wasFloating) {
            // Small delay to ensure layout is stable
            QTimer::singleShot(20, this, &WidgetFloater::startFloating);
            //startFloating();
        }
        
        emit positionUpdated();
    }
}

void WidgetFloater::setupAnimations()
{
    if (!isValid()) return;
    
    // Clean up existing animations
    if (m_floatSequence) {
        m_floatSequence->stop();
        delete m_floatSequence;
        m_floatSequence = nullptr;
    }
    
    // Create float up animation
    m_floatUpAnimation = new QPropertyAnimation(this, "floatOffset");
    m_floatUpAnimation->setDuration(m_duration / 2);
    m_floatUpAnimation->setStartValue(0);
    m_floatUpAnimation->setEndValue(-m_floatAmount);
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

void WidgetFloater::updateWidgetPosition()
{
    if (!isValid()) return;
    
    QPoint newPos = m_originalPosition;
    newPos.setY(newPos.y() + m_floatOffset);
    m_target->move(newPos);
}

void WidgetFloater::setFloatOffset(float offset)
{
    if (!isValid() || m_floatOffset == offset) return;
    
    m_floatOffset = offset;
    updateWidgetPosition();
}

void WidgetFloater::startFloating()
{
    if (!isValid() || m_isFloating) return;
    
    // Ensure we have the latest position
    saveOriginalPosition();
    
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
    
    // Before starting next cycle, check if position changed
    if (m_autoReposition && isValid()) {
        QPoint currentPos = m_target->pos();
        if (currentPos != m_lastKnownPosition) {
            // Position changed, update original position
            m_originalPosition = currentPos;
            m_lastKnownPosition = currentPos;
        }
    }
    
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
        setupAnimations();
        if (m_isFloating) {
            startFloating();
        }
    }
}

void WidgetFloater::setDuration(int milliseconds)
{
    if (milliseconds != m_duration && milliseconds > 0) {
        m_duration = milliseconds;
        setupAnimations();
        if (m_isFloating) {
            startFloating();
        }
    }
}

void WidgetFloater::setEasingCurve(const QEasingCurve& curve)
{
    m_easingCurve = curve;
    setupAnimations();
    if (m_isFloating) {
        startFloating();
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

void WidgetFloater::setAutoReposition(bool enabled)
{
    m_autoReposition = enabled;
}

void WidgetFloater::setRepositionDelay(int milliseconds)
{
    m_repositionDelay = qMax(50, milliseconds);
    if (m_repositionTimer) {
        m_repositionTimer->setInterval(m_repositionDelay);
    }
}

bool WidgetFloater::isFloating() const
{
    return m_isFloating;
}