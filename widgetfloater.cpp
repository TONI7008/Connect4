#include "widgetfloater.h"
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QSequentialAnimationGroup>
#include <QScreen>
#include <QLayout>

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
    , m_positionCaptured(false)
{
    if (!m_target) {
        qWarning() << "WidgetFloater: No target widget provided!";
        return;
    }

    m_parent = m_target->parentWidget();
    m_easingCurve.setType(QEasingCurve::InOutQuad);

    // DON'T capture position yet — the widget hasn't been laid out.
    // We defer until the parent has shown and the layout has settled.

    setupAnimations();
    installParentEventFilter();

    m_repositionTimer = new QTimer(this);
    m_repositionTimer->setSingleShot(true);
    m_repositionTimer->setInterval(m_repositionDelay);
    connect(m_repositionTimer, &QTimer::timeout, this, &WidgetFloater::delayedReposition);
}

WidgetFloater::~WidgetFloater()
{
    stopFloating();

    if (m_parent && m_parentEventFilterInstalled)
        m_parent->removeEventFilter(this);

    // Children of this QObject — Qt will delete them, but stop first to be safe
    if (m_floatSequence) { m_floatSequence->stop(); }
    delete m_floatSequence;
    delete m_repositionTimer;
}

void WidgetFloater::installParentEventFilter()
{
    if (!m_parent || m_parentEventFilterInstalled) return;
    m_parent->installEventFilter(this);
    m_parentEventFilterInstalled = true;
}

bool WidgetFloater::eventFilter(QObject *obj, QEvent *event)
{
    if (!isValid())
        return QObject::eventFilter(obj, event);

    if (obj == m_parent) {
        switch (event->type()) {
        case QEvent::Show:
            // Parent first shown: capture position after layout finishes
            if (!m_positionCaptured) {
                QTimer::singleShot(50, this, [this]() {
                    if (!isValid()) return;
                    captureLayoutPosition();
                });
            } else if (m_autoReposition) {
                scheduleReposition();
            }
            break;

        case QEvent::Resize:
            if (m_autoReposition)
                scheduleReposition();
            break;

        case QEvent::Move:
            // Only reposition on Move if position has actually been captured
            if (m_autoReposition && m_positionCaptured)
                scheduleReposition();
            break;

        default:
            break;
        }
    }

    return QObject::eventFilter(obj, event);
}

// ── Position capture ──────────────────────────────────────────────────────────

void WidgetFloater::captureLayoutPosition()
{
    if (!isValid()) return;

    // Force the layout to settle
    if (m_parent && m_parent->layout())
        m_parent->layout()->activate();

    QPoint pos = m_target->pos();

    // Sanity check: if still at (0,0) and parent has a layout, wait a bit more
    if (pos.isNull() && m_parent && m_parent->layout()) {
        QTimer::singleShot(80, this, [this]() { captureLayoutPosition(); });
        return;
    }

    m_originalPosition  = pos;
    m_lastKnownPosition = pos;
    m_floatOffset       = 0;
    m_positionCaptured  = true;
}

void WidgetFloater::saveOriginalPosition()
{
    if (!isValid()) return;
    m_originalPosition  = m_target->pos();
    m_lastKnownPosition = m_originalPosition;
    m_floatOffset       = 0;
    m_positionCaptured  = true;
}

void WidgetFloater::updateOriginalPosition()
{
    if (!isValid()) return;
    m_originalPosition  = m_target->pos();
    m_lastKnownPosition = m_originalPosition;
    m_positionCaptured  = true;
}

void WidgetFloater::setOriginalPosition(const QPoint& pos)
{
    m_originalPosition  = pos;
    m_lastKnownPosition = pos;
    m_positionCaptured  = true;

    if (m_isFloating) {
        stopFloating();
        startFloating();
    }
}

// ── Reposition ────────────────────────────────────────────────────────────────

void WidgetFloater::scheduleReposition()
{
    if (!m_pendingReposition) {
        m_pendingReposition = true;
        m_repositionTimer->start();
    }
}

void WidgetFloater::onParentResized()
{
    scheduleReposition();
}

void WidgetFloater::delayedReposition()
{
    if (!isValid()) return;
    m_pendingReposition = false;

    if (!m_positionCaptured) {
        captureLayoutPosition();
        return;
    }

    if (m_parent && m_parent->layout())
        m_parent->layout()->activate();

    QPoint newPos = m_target->pos();

    if (newPos != m_lastKnownPosition) {
        bool wasFloating = m_isFloating;

        if (wasFloating) stopFloating();

        m_originalPosition  = newPos;
        m_lastKnownPosition = newPos;

        // Only move if not currently floating (otherwise updateWidgetPosition handles it)
        if (!wasFloating)
            m_target->move(newPos);

        if (wasFloating)
            QTimer::singleShot(20, this, &WidgetFloater::startFloating);

        emit positionUpdated();
    }
}

// ── Animations ────────────────────────────────────────────────────────────────

void WidgetFloater::setupAnimations()
{
    if (!isValid()) return;

    if (m_floatSequence) {
        m_floatSequence->stop();
        delete m_floatSequence;
        m_floatSequence = nullptr;
        m_floatUpAnimation   = nullptr;
        m_floatDownAnimation = nullptr;
    }

    m_floatUpAnimation = new QPropertyAnimation(this, "floatOffset", this);
    m_floatUpAnimation->setDuration(m_duration / 2);
    m_floatUpAnimation->setStartValue(0.0f);
    m_floatUpAnimation->setEndValue((float)-m_floatAmount);
    m_floatUpAnimation->setEasingCurve(m_easingCurve);

    m_floatDownAnimation = new QPropertyAnimation(this, "floatOffset", this);
    m_floatDownAnimation->setDuration(m_duration / 2);
    m_floatDownAnimation->setStartValue((float)-m_floatAmount);
    m_floatDownAnimation->setEndValue(0.0f);
    m_floatDownAnimation->setEasingCurve(m_easingCurve);

    m_floatSequence = new QSequentialAnimationGroup(this);
    m_floatSequence->addAnimation(m_floatUpAnimation);
    m_floatSequence->addAnimation(m_floatDownAnimation);

    connect(m_floatSequence, &QSequentialAnimationGroup::finished,
            this, &WidgetFloater::onAnimationFinished);
}

void WidgetFloater::updateWidgetPosition()
{
    if (!isValid() || !m_positionCaptured) return;
    QPoint newPos = m_originalPosition;
    newPos.setY(newPos.y() + (int)m_floatOffset);
    m_target->move(newPos);
}

void WidgetFloater::setFloatOffset(float offset)
{
    if (!isValid() || m_floatOffset == offset) return;
    m_floatOffset = offset;
    updateWidgetPosition();
}

// ── Floating control ──────────────────────────────────────────────────────────

void WidgetFloater::startFloating()
{
    if (!isValid() || m_isFloating) return;

    if (!m_positionCaptured) {
        // Position not ready yet; capture it now and re-call once done
        captureLayoutPosition();
        QTimer::singleShot(100, this, &WidgetFloater::startFloating);
        return;
    }

    // Refresh position in case layout changed since last capture
    QPoint currentPos = m_target->pos();
    if (currentPos != m_originalPosition) {
        m_originalPosition  = currentPos;
        m_lastKnownPosition = currentPos;
    }

    m_isFloating   = true;
    m_currentCycle = 0;

    if (m_floatSequence) {
        m_floatSequence->start();
        emit floatingStarted();
    }
}

void WidgetFloater::stopFloating()
{
    if (!m_isFloating) return;
    m_isFloating = false;

    if (m_floatSequence)
        m_floatSequence->stop();

    setFloatOffset(0);
    emit floatingStopped();
}

void WidgetFloater::setFloatingEnabled(bool enabled)
{
    enabled ? startFloating() : stopFloating();
}

void WidgetFloater::onAnimationFinished()
{
    if (!m_isFloating) return;
    m_currentCycle++;
    emit cycleCompleted();

    if (m_pauseDuration > 0)
        QTimer::singleShot(m_pauseDuration, this, &WidgetFloater::startNextCycle);
    else
        startNextCycle();
}

void WidgetFloater::startNextCycle()
{
    if (!m_isFloating) return;

    // Sync position if layout moved the widget between cycles
    if (m_autoReposition && isValid()) {
        QPoint currentPos = m_target->pos();
        if (currentPos != m_originalPosition && currentPos != m_lastKnownPosition) {
            m_originalPosition  = currentPos;
            m_lastKnownPosition = currentPos;
        }
    }

    if (m_randomDelay) {
        int delay = QRandomGenerator::global()->bounded(1000);
        QTimer::singleShot(delay, this, [this]() {
            if (m_isFloating && m_floatSequence)
                m_floatSequence->start();
        });
    } else {
        if (m_floatSequence)
            m_floatSequence->start();
    }
}

// ── Setters ───────────────────────────────────────────────────────────────────

void WidgetFloater::setFloatAmount(int pixels)
{
    if (pixels > 0 && pixels != m_floatAmount) {
        m_floatAmount = pixels;
        setupAnimations();
        if (m_isFloating) { stopFloating(); startFloating(); }
    }
}

void WidgetFloater::setDuration(int milliseconds)
{
    if (milliseconds > 0 && milliseconds != m_duration) {
        m_duration = milliseconds;
        setupAnimations();
        if (m_isFloating) { stopFloating(); startFloating(); }
    }
}

void WidgetFloater::setEasingCurve(const QEasingCurve& curve)
{
    m_easingCurve = curve;
    setupAnimations();
    if (m_isFloating) { stopFloating(); startFloating(); }
}

void WidgetFloater::setRandomDelay(bool enabled)         { m_randomDelay = enabled; }
void WidgetFloater::setPauseBetweenCycles(int ms)        { m_pauseDuration = qMax(0, ms); }
void WidgetFloater::setAutoReposition(bool enabled)      { m_autoReposition = enabled; }

void WidgetFloater::setRepositionDelay(int milliseconds)
{
    m_repositionDelay = qMax(50, milliseconds);
    if (m_repositionTimer)
        m_repositionTimer->setInterval(m_repositionDelay);
}

bool WidgetFloater::isFloating() const { return m_isFloating; }
