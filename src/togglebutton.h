// togglebutton.h
#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#include <QPushButton>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPainterPath>

class ToggleButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset)
    Q_PROPERTY(qreal animationProgress READ animationProgress WRITE setAnimationProgress)

    // Design properties
    Q_PROPERTY(QColor activeColor READ activeColor WRITE setActiveColor)
    Q_PROPERTY(QColor inactiveColor READ inactiveColor WRITE setInactiveColor)
    Q_PROPERTY(QColor thumbColor READ thumbColor WRITE setThumbColor)
    Q_PROPERTY(int animationDuration READ animationDuration WRITE setAnimationDuration)
    Q_PROPERTY(qreal cornerRadiusRatio READ cornerRadiusRatio WRITE setCornerRadiusRatio)

public:
    explicit ToggleButton(QWidget *parent = nullptr);
    ~ToggleButton();

    // State
    int offset() const { return m_offset; }

    // Color properties
    QColor activeColor() const { return m_activeColor; }
    QColor inactiveColor() const { return m_inactiveColor; }
    QColor thumbColor() const { return m_thumbColor; }
    QColor disabledColor() const { return m_disabledColor; }

    // Animation properties
    int animationDuration() const { return m_animationDuration; }
    qreal animationProgress() const { return m_animationProgress; }
    qreal cornerRadiusRatio() const { return m_cornerRadiusRatio; }

    QSize sizeHint() const override;

    qreal borderRatio() const;
    void setBorderRatio(qreal newBorderRatio);

    bool borderEnabled() const;
    void setBorderEnabled(bool newBorderEnabled);

public slots:
    void setChecked(bool);

    void setActiveColor(const QColor &color);
    void setInactiveColor(const QColor &color);
    void setThumbColor(const QColor &color);
    void setDisabledColor(const QColor &color);
    void setAnimationDuration(int duration);
    void setEasing(QEasingCurve::Type easing);
    void setCornerRadiusRatio(qreal ratio);
    void setOffset(int offset);

signals:
    void animationFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:

    bool m_hovered = false;
    bool m_borderEnabled=false;
    int m_offset = 0;

    // Colors
    QColor m_activeColor {71, 158, 245};
    QColor m_inactiveColor {180, 180, 180,180};
    QColor m_thumbColor {Qt::white};
    QColor m_disabledColor {160, 160, 160,180};
    QColor m_hoverColor {200, 200, 200,250};

    // Animation
    qreal m_animationProgress = 0.0;
    int m_animationDuration = 150;
    QEasingCurve::Type m_easingCurve = QEasingCurve::OutBack;

    // Dimensions (now ratios)
    qreal m_cornerRadiusRatio = 0.49; // Ratio of height
    qreal m_thumbRadiusRatio = 0.415;  // Ratio of height
    qreal m_paddingRatio = 0.069;      // Ratio of height

    qreal m_borderRatio = 0.08;
    // Animations
    QPropertyAnimation *m_positionAnimation=nullptr;
    QPropertyAnimation *m_colorAnimation=nullptr;

    // Helper methods
    void initAnimations();
    void updateAppearance();
    void startAnimations();
    void setAnimationProgress(qreal progress);

    QColor currentBackgroundColor() const;
    int calculateEndPosition() const;

    // Calculated properties
    int thumbRadius() const;
    int padding() const;
    int cornerRadius() const;
};

#endif // TOGGLEBUTTON_H
