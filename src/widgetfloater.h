#ifndef WIDGETFLOATER_H
#define WIDGETFLOATER_H

#include <QObject>
#include <QWidget>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QTimer>
#include <QPointer>

class WidgetFloater : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float floatOffset READ floatOffset WRITE setFloatOffset)

public:
    explicit WidgetFloater(QWidget *target, QObject *parent = nullptr);
    ~WidgetFloater();

    void startFloating();
    void stopFloating();
    void setFloatingEnabled(bool enabled);

    void setFloatAmount(int pixels);
    void setDuration(int milliseconds);
    void setEasingCurve(const QEasingCurve& curve);
    void setRandomDelay(bool enabled);
    void setPauseBetweenCycles(int milliseconds);
    void setAutoReposition(bool enabled);
    void setRepositionDelay(int milliseconds);

    bool  isFloating()        const;
    int   floatAmount()       const { return m_floatAmount; }
    int   duration()          const { return m_duration;    }
    QPoint originalPosition() const { return m_originalPosition; }

    void updateOriginalPosition();
    void setOriginalPosition(const QPoint& pos);

    // Call this manually if you know the layout has settled and want to
    // re-capture the widget's position (e.g. after a page switch animation).
    void captureLayoutPosition();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void floatingStarted();
    void floatingStopped();
    void cycleCompleted();
    void positionUpdated();

private slots:
    void onAnimationFinished();
    void startNextCycle();
    void onParentResized();
    void delayedReposition();

private:
    QPointer<QWidget> m_target;
    QPointer<QWidget> m_parent;

    QPropertyAnimation      *m_floatUpAnimation   = nullptr;
    QPropertyAnimation      *m_floatDownAnimation = nullptr;
    QSequentialAnimationGroup *m_floatSequence     = nullptr;
    QEasingCurve             m_easingCurve;

    int   m_floatAmount;
    int   m_duration;
    bool  m_randomDelay;
    int   m_pauseDuration;
    QPoint m_originalPosition;
    QPoint m_lastKnownPosition;
    float  m_floatOffset;

    bool  m_autoReposition;
    int   m_repositionDelay;
    QTimer *m_repositionTimer = nullptr;
    bool   m_pendingReposition;

    bool  m_isFloating;
    int   m_currentCycle;
    bool  m_parentEventFilterInstalled;
    bool  m_positionCaptured;   // ← NEW: true once we've read a valid layout pos

    void setupAnimations();
    void saveOriginalPosition();
    void installParentEventFilter();
    void scheduleReposition();            // ← NEW: debounce helper
    void updateWidgetPosition();

    float floatOffset() const   { return m_floatOffset; }
    void  setFloatOffset(float offset);
    bool  isValid()     const   { return !m_target.isNull(); }
};

#endif // WIDGETFLOATER_H
