#include "dynamicframeassistant.h"
#include <QResizeEvent>
#include <QLayout>
#include <QDebug>

DynamicFrameAssistant::DynamicFrameAssistant(QWidget* parent) :
    QObject(parent),
    m_parentWidget(parent)
{
    if (m_parentWidget) {
        m_eventFilter = new LambdaEventFilter(
            [this](QObject*, QEvent* event) {
                if (event->type() == QEvent::Resize && !childWidgetBlocked()) {
                    this->handleResize();
                }
                return false;
            },
            this
            );
        m_parentWidget->installEventFilter(m_eventFilter);
    }
}

DynamicFrameAssistant::~DynamicFrameAssistant()
{
    if (m_eventFilter && m_parentWidget) {
        m_parentWidget->removeEventFilter(m_eventFilter);
    }

    // Clean up all animations
    for (auto& info : m_childWidgets) {
        if (info.animation) {
            info.animation->stop();
            delete info.animation;
        }
    }
}

void DynamicFrameAssistant::setupWidgetAnimation(QWidget* widget)
{
    if (!m_childWidgets.contains(widget)) return;

    ChildWidgetInfo& info = m_childWidgets[widget];
    if (!info.animation) {
        info.animation = new QPropertyAnimation(widget, "geometry", this);
        info.animation->setEasingCurve(QEasingCurve::OutQuad);
        connect(info.animation, &QPropertyAnimation::finished, this, [this, widget]() {
            if (m_childWidgets.contains(widget)) {
                emit animationFinished(widget, m_childWidgets[widget].visible);
            }
        });
    }
}

void DynamicFrameAssistant::addChildWidget(QWidget* widget, Qt::Alignment alignment, int margin)
{
    if (!widget) return;

    ChildWidgetInfo info;
    info.widget = widget;
    info.alignment = alignment;
    info.margin = margin;
    info.visible = true;
    info.animation = nullptr;

    if (widget->parentWidget() != m_parentWidget) {
        widget->setParent(m_parentWidget);
    }

    m_childWidgets.insert(widget, info);
    setupWidgetAnimation(widget);
    updateWidgetPosition(widget);

    update();
}

void DynamicFrameAssistant::removeChildWidget(QWidget* widget)
{
    if (!widget || !m_childWidgets.contains(widget)) return;

    if (m_childWidgets[widget].animation) {
        m_childWidgets[widget].animation->stop();
        delete m_childWidgets[widget].animation;
    }

    m_childWidgets.remove(widget);
}

void DynamicFrameAssistant::popAll(bool show, int duration)
{
    for (auto& info : m_childWidgets) {
        popWidget(info.widget, show, duration);
    }
}


void DynamicFrameAssistant::setBlock(bool newBlock)
{
    m_block = newBlock;
}

void DynamicFrameAssistant::update()
{
    updatePositions();
}

void DynamicFrameAssistant::setParentWidget(QWidget* widget)
{
    if (m_eventFilter && m_parentWidget) {
        m_parentWidget->removeEventFilter(m_eventFilter);
    }

    m_parentWidget = widget;

    if (m_parentWidget) {
        if (!m_eventFilter) {
            m_eventFilter = new LambdaEventFilter(
                [this](QObject*, QEvent* event) {
                    if (event->type() == QEvent::Resize && !childWidgetBlocked()) {
                        this->handleResize();
                    }
                    return false;
                },
                this
                );
        }
        m_parentWidget->installEventFilter(m_eventFilter);

        // Update parent for all child widgets
        for (auto& info : m_childWidgets) {
            if (info.widget) {
                info.widget->setParent(m_parentWidget);
            }
        }

        updatePositions();
    }
}

void DynamicFrameAssistant::setChildWidgetAlignment(QWidget* widget, Qt::Alignment alignment)
{
    if (!widget || !m_childWidgets.contains(widget)) return;

    m_childWidgets[widget].alignment = alignment;
    updateWidgetPosition(widget);
}

void DynamicFrameAssistant::setChildWidgetMargin(QWidget* widget, int margin)
{
    if (!widget || !m_childWidgets.contains(widget)) return;

    m_childWidgets[widget].margin = margin;
    updateWidgetPosition(widget);
}

bool DynamicFrameAssistant::childWidgetBlocked() const
{
    return m_block;
}

void DynamicFrameAssistant::setChildWidgetBlock(bool newBlock)
{
    m_block = newBlock;
}

void DynamicFrameAssistant::popWidget(QWidget* widget, bool show, int duration)
{
    if (!widget || !m_childWidgets.contains(widget) || show == m_childWidgets[widget].visible) return;

    ChildWidgetInfo& info = m_childWidgets[widget];

    // Ensure animation is set up
    if (!info.animation) {
        setupWidgetAnimation(widget);
    }

    if (show) {
        // Pop in (show)
        info.widget->show();
        info.widget->raise();
        QRect endRect(info.originalPosition, info.widget->size());
        QRect startRect(endRect.topLeft(), endRect.size());
        startRect.moveTopLeft(calculateHiddenPosition(info.widget, info.alignment));

        info.animation->setDuration(duration);
        info.animation->setStartValue(startRect);
        info.animation->setEndValue(endRect);
    } else {
        // Pop out (hide)
        QRect startRect(info.widget->pos(), info.widget->size());
        QRect endRect(startRect.topLeft(), startRect.size());
        endRect.moveTopLeft(calculateHiddenPosition(info.widget, info.alignment));

        info.animation->setDuration(duration);
        info.animation->setStartValue(startRect);
        info.animation->setEndValue(endRect);
    }

    info.animation->start();
    info.visible = show;
}

QPoint DynamicFrameAssistant::calculateHiddenPosition(QWidget* widget, const Qt::Alignment& alignment)
{
    if (!m_parentWidget || !widget) return QPoint();

    QPoint hiddenPos = m_childWidgets[widget].originalPosition;
    QSize childSize = widget->size();

    // Calculate hidden position based on alignment
    if (alignment & Qt::AlignTop) {
        hiddenPos.setY(-childSize.height()-m_defaultMargin);
    } else if (alignment & Qt::AlignBottom) {
        hiddenPos.setY(m_parentWidget->rect().height());
    }

    if (alignment & Qt::AlignLeft) {
        hiddenPos.setX(-childSize.width()-m_defaultMargin);
    } else if (alignment & Qt::AlignRight) {
        hiddenPos.setX(m_parentWidget->rect().width());
    }

    return hiddenPos;
}

QPoint DynamicFrameAssistant::calculateVisiblePosition(QWidget* widget, const Qt::Alignment& alignment, int margin)
{
    if (!m_parentWidget || !widget) return QPoint();

    QRect availableRect;

    QSize childSize = widget->size();
    QPoint newPos;

    if (alignment & Qt::AlignLeft) {
        newPos.setX(availableRect.left() + margin);
    } else if (alignment & Qt::AlignRight) {
        newPos.setX(availableRect.right() - childSize.width() - margin);
    } else {
        newPos.setX(availableRect.left() + (availableRect.width() - childSize.width()) / 2);
    }

    if (alignment & Qt::AlignTop) {
        newPos.setY(availableRect.top() + margin);
    } else if (alignment & Qt::AlignBottom) {
        newPos.setY(availableRect.bottom() - childSize.height() - margin);
    } else {
        newPos.setY(availableRect.top() + (availableRect.height() - childSize.height()) / 2);
    }

    return newPos;
}

void DynamicFrameAssistant::updatePositions()
{
    for (auto& info : m_childWidgets) {
        updateWidgetPosition(info.widget);
    }
}

void DynamicFrameAssistant::updateWidgetPosition(QWidget* widget)
{
    if (!m_parentWidget || !widget || !m_childWidgets.contains(widget)) return;

    ChildWidgetInfo& info = m_childWidgets[widget];
    info.originalPosition = calculateVisiblePosition(widget, info.alignment, info.margin);

    if (info.visible) {
        widget->move(info.originalPosition);
    }else{
        widget->move(calculateHiddenPosition(widget,info.alignment));
    }
}

void DynamicFrameAssistant::handleResize()
{

    updatePositions();
}
