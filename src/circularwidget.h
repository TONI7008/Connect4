#ifndef CIRCULARWIDGET_H
#define CIRCULARWIDGET_H

#include <QObject>
#include <QWidget>

class CircularWidget : public QWidget
{
    Q_OBJECT
public:
    CircularWidget(QWidget* parent=nullptr);
    ~CircularWidget();

    void setColor(const QColor &newColor);

    

protected:
    void paintEvent(QPaintEvent* event) override;
    QColor m_color=Qt::green;


};

#endif // CIRCULARWIDGET_H
