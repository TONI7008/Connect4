#ifndef TWIDGET_H
#define TWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QFile>
#include <QShowEvent>

class TWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TWidget(QWidget *parent = nullptr);
    ~TWidget();
    void setEnableBackground(bool);
    void setBackgroundImage(QString);
    void setBorderRadius(short);
    short borderRadius();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent* event) override;

signals:
    void clicked(const QPointF& pos);
    void rightClicked(const QPoint& pos);
    void doubleClicked(const QPoint& pos);


    void resizing();

private:
    bool enableBackground;
    QPoint m_clickPos;
    short m_bRadius=0;
    QPixmap backgroundImage=QPixmap(":/Icons/fullblurbluesky.png");

};

#endif // TWIDGET_H
