#ifndef POPOUTFRAME_H
#define POPOUTFRAME_H

#include <QFrame>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QMouseEvent>

#include "tframe.h"


class PopoutFrame : public TFrame {
    Q_OBJECT
    Q_PROPERTY(int popoutDuration READ popoutDuration WRITE setPopoutDuration)
    Q_PROPERTY(qreal popoutScale READ popoutScale WRITE setPopoutScale)

public:
    enum PopoutDirection {
        Center = 0x0,
        Top = 0x1,
        Left = 0x2,
        Right = 0x4,
        Bottom = 0x8
    };
    Q_DECLARE_FLAGS(PopoutDirections, PopoutDirection)
    Q_FLAG(PopoutDirections)
    Q_ENUM(PopoutDirection)

    explicit PopoutFrame(QWidget* parent = nullptr);
    ~PopoutFrame();

    void popout();
    void popout(int duration, qreal scale);

    // Property getters
    int popoutDuration() const { return m_popoutDuration; }
    qreal popoutScale() const { return m_popoutScale; }
    PopoutDirections popoutDirection() const { return m_direction; }

    // Property setters
    void setPopoutDuration(int duration);
    void setPopoutScale(qreal scale);
    void setEasingCurve(QEasingCurve::Type curve);
    void setPopoutColor(const QColor& color);
    void setPopoutMaxWidth(int maxWidth);
    void setPopoutMaxHeight(int maxHeight);
    void setPopoutDirection(PopoutDirections direction);
    void setMaximumSize(const QSize &size);

    qreal widthExpansionFactor() const;
    void setWidthExpansionFactor(qreal newWidthExpansionFactor);

signals:
    void hoverStarted();
    void hoverEnded();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void startExpandAnimation();
    void startShrinkAnimation();
    QRect updateExpandedGeometry();

    QRect calculateExpandedGeometry(const QRect &baseGeometry);
    QPoint calculateExpandedPosition(const QRect &baseGeometry, const QSize &expandedSize);

    QRect originalGeometry;
    QSequentialAnimationGroup* animationGroup;
    QPropertyAnimation* expandAnim = nullptr;
    QPropertyAnimation* shrinkAnim = nullptr;
    QEasingCurve::Type currentCurve = QEasingCurve::OutQuad;
    QColor borderColor = Qt::transparent;
    int borderWidth = 4;
    int m_popoutMaxWidth = -1; // -1 means no limit
    int m_popoutMaxHeight = -1; // -1 means no limit
    bool isHovered = false;
    int m_popoutDuration = 400; // Default duration
    qreal m_popoutScale = 1.2; // Default scale
    qreal m_widthExpansionFactor = 3; // How much wider than height expansion
    PopoutDirections m_direction = Center; // Default direction
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PopoutFrame::PopoutDirections)

#endif // POPOUTFRAME_H
