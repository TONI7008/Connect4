#ifndef WIDGETFLOATER_H
#define WIDGETFLOATER_H

#include <QObject>
#include <QWidget>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QTimer>

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
    void setOriginalPosition(const QPoint& pos){
        m_originalPosition = pos;
        setFloatOffset(0); // Reset offset to ensure correct positioning
    }
    
    // Customization methods
    void setFloatAmount(int pixels);
    void setDuration(int milliseconds);
    void setEasingCurve(const QEasingCurve& curve);
    void setRandomDelay(bool enabled);
    void setPauseBetweenCycles(int milliseconds);
    
    // Status
    bool isFloating() const;
    int floatAmount() const { return m_floatAmount; }
    int duration() const { return m_duration; }
    
signals:
    void floatingStarted();
    void floatingStopped();
    void cycleCompleted();

private slots:
    void onAnimationFinished();
    void startNextCycle();

private:
    // Target widget
    QWidget *m_target=nullptr;
    
    // Animation properties
    QPropertyAnimation *m_floatUpAnimation=nullptr;
    QPropertyAnimation *m_floatDownAnimation=nullptr;
    QSequentialAnimationGroup *m_floatSequence=nullptr;
    QEasingCurve m_easingCurve;
    
    // Configuration
    int m_floatAmount;
    int m_duration;
    bool m_randomDelay;
    int m_pauseDuration;
    QPoint m_originalPosition;
    float m_floatOffset;
    
    // State
    bool m_isFloating;
    int m_currentCycle;
    
    // Helper methods
    void setupAnimations();
    void saveOriginalPosition();
    float floatOffset() const { return m_floatOffset; }
    void setFloatOffset(float offset);
};

#endif // WIDGETFLOATER_H