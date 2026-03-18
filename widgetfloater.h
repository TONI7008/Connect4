#ifndef WIDGETFLOATER_H
#define WIDGETFLOATER_H

#include <QObject>
#include <QWidget>
#include <QPropertyAnimation>
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

    // Animation control
    void startFloating();
    void stopFloating();
    void setFloatingEnabled(bool enabled);
    
    // Customization methods
    void setFloatAmount(int pixels);
    void setDuration(int milliseconds);
    void setEasingCurve(const QEasingCurve& curve);
    void setRandomDelay(bool enabled);
    void setPauseBetweenCycles(int milliseconds);
    void setAutoReposition(bool enabled);
    void setRepositionDelay(int milliseconds);
    
    // Status
    bool isFloating() const;
    int floatAmount() const { return m_floatAmount; }
    int duration() const { return m_duration; }
    QPoint originalPosition() const { return m_originalPosition; }
    
    // Manual reposition (if needed)
    void updateOriginalPosition();
    void setOriginalPosition(const QPoint& pos);

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
    // Target widget
    QPointer<QWidget> m_target;
    QPointer<QWidget> m_parent;
    
    // Animation properties
    QPropertyAnimation *m_floatUpAnimation;
    QPropertyAnimation *m_floatDownAnimation;
    QSequentialAnimationGroup *m_floatSequence;
    QEasingCurve m_easingCurve;
    
    // Configuration
    int m_floatAmount;
    int m_duration;
    bool m_randomDelay;
    int m_pauseDuration;
    QPoint m_originalPosition;
    QPoint m_lastKnownPosition;
    float m_floatOffset;
    
    // Auto-reposition
    bool m_autoReposition;
    int m_repositionDelay;
    QTimer *m_repositionTimer;
    bool m_pendingReposition;
    
    // State
    bool m_isFloating;
    int m_currentCycle;
    bool m_parentEventFilterInstalled;
    
    // Helper methods
    void setupAnimations();
    void saveOriginalPosition();
    void installParentEventFilter();
    float floatOffset() const { return m_floatOffset; }
    void setFloatOffset(float offset);
    void updateWidgetPosition();
    bool isValid() const { return !m_target.isNull(); }
};

#endif // WIDGETFLOATER_H