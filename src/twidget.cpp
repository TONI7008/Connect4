#include "twidget.h"

TWidget::TWidget(QWidget *parent)
    : QWidget(parent), enableBackground(false) {

}

TWidget::~TWidget() {
}

void TWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        emit rightClicked(event->pos());
    }else if(event->button() == Qt::LeftButton){
        emit clicked(event->pos().toPointF());
    }
    QWidget::mousePressEvent(event);

}

void TWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(event->pos());
    }
}


void TWidget::paintEvent(QPaintEvent *event) {
    //QWidget::paintEvent(event);

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    // Activate antianaliasing for smoothness
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // Pour les transformations d'images plus douces

    // setup rounded corner
    QPainterPath path;
    path.addRoundedRect(rect(), m_bRadius, m_bRadius);
    painter.setClipPath(path);

    // Draw all
    if (enableBackground) {
        painter.drawPixmap(0, 0, width(), height(), backgroundImage);
    }
    QWidget::paintEvent(event);
}


void TWidget::setEnableBackground(bool set) {
    if (enableBackground != set) {
        enableBackground = set;
        update();
    }
}

void TWidget::setBackgroundImage(QString image) {
    QFile image_file(image);
    if (!image_file.exists()) return;
    backgroundImage = QPixmap(image);
    repaint();
}


void TWidget::setBorderRadius(short r)
{
    m_bRadius=r;
    repaint();
}

short TWidget::borderRadius()
{
    return m_bRadius;
}

void TWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    emit resizing();
}
