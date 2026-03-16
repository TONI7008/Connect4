#ifndef TSTACKEDWIDGET_H
#define TSTACKEDWIDGET_H

#include <QStackedWidget>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QQueue>
#include <QElapsedTimer>
#include <QPainter>

class TStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit TStackedWidget(QWidget* parent = nullptr);
    ~TStackedWidget();

    enum Type {
        SlideFade,       // TypeI - Slide with fade
        HorizontalSlide, // TypeII - Horizontal slide
        VerticalSlide,    // TypeIII - Vertical slide
        AccordionVSlide,
        CrossFade,
        RotateAnim,
        ZoomFade

    };

    void setCurrentIndex(int index, bool immediate = false);
    void setCurrentWidget(QWidget *w, bool immediate = false);
    void setCurve(QEasingCurve curve);
    void setAnimationDuration(int duration);
    void setAnimationType(Type type);
    void setBlurEffectEnabled(bool enabled);
    void addWidget(QWidget* widget);

    // New smoothness control methods
    void setFadeOpacityRange(qreal start, qreal end);
    void setAnimationQuality(QPainter::RenderHint hint);

signals:
    void animationStarted();
    void animationFinished();

private:
    struct AnimationRequest {
        int index;
        bool immediate;
    };

    QEasingCurve m_curve = QEasingCurve::OutBack;
    int m_duration = 300;
    bool m_isAnimating = false;
    Type m_type = SlideFade;
    bool m_blurEnabled = false;
    qreal m_fadeStartOpacity = 1.0;
    qreal m_fadeEndOpacity = 0.0;
    QPainter::RenderHint m_renderHint = QPainter::Antialiasing;

    void executeAnimation(int index, bool immediate);
    void slideFadeAnimation(int index);
    void horizontalSlideAnimation(int index);
    void verticalSlideAnimation(int index);
    void cleanupAfterAnimation();
    void accordionAnimation(int index);
    void crossFadeAnimation(int index);
    void rotateAnimation(int index);
    void zoomFadeAnimation(int index);
};

#endif // TSTACKEDWIDGET_H
