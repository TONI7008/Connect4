#include "connect_4.h"
#include <QGridLayout>


Connect_4::Connect_4(QWidget *parent)
    : TWidget(parent)
{

    setEnableBackground(true);
    setBackgroundImage("/home/hacker/Documents/C++/Connect4/Images/Melamine-wood-005.png");

    setStyleSheet("background-color:#2A180B; border:5px solid #3B2314;");


    connect(this, &Connect_4::clicked, [this](const QPointF& pos) {
        // Handle click events here
        // You can determine which column was clicked based on the x-coordinate of pos
        int col = pos.x() / (width() / Cols);
        int row = pos.y() / (height() / Rows);

        emit Clicked(board[row][col]);
    });

    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setAttribute(Qt::WA_TranslucentBackground);
    //setAttribute(Qt::WA_NoSystemBackground);
    //setAttribute();

}

Connect_4::~Connect_4()
{

}

void Connect_4::paintEvent(QPaintEvent *event)
{
    //TWidget::paintEvent(event);
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    // Compute available space
    double holeW = (double)(width() - (Margin * (Cols + 1))) / Cols;
    double holeH = (double)(height() - (Margin * (Rows + 1))) / Rows;

    m_currentDiameter = std::max(minHoleDiameter, std::min(holeW, holeH));
    emit diameterChanged((short)m_currentDiameter);


    //------------------------------------------------------
    // Build board shape with punched holes
    //------------------------------------------------------

    QPainterPath boardPath;
    boardPath.addRoundedRect(rect(),15,15);

    for (int r = 0; r < Rows; ++r) {
        for (int c = 0; c < Cols; ++c) {

            double x = Margin + c * (holeW + Margin) + (holeW - m_currentDiameter) / 2.0;
            double y = Margin + r * (holeH + Margin) + (holeH - m_currentDiameter) / 2.0;

            board[r][c]=QPointF(x,y);

            boardPath.addEllipse(QRectF(x, y, m_currentDiameter, m_currentDiameter));
        }
    }

    boardPath.setFillRule(Qt::OddEvenFill);

    //------------------------------------------------------
    // Draw board
    //------------------------------------------------------
    QLinearGradient gradient(0, 0, width() * 0.8, height());
    gradient.setColorAt(0.0, QColor(0, 71, 171));
    gradient.setColorAt(1.0, QColor(28, 169, 201));
    painter.setBrush(QColor(30,30,30,180)); // board color
    painter.drawPath(boardPath);

}


void Connect_4::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(event->pos());

    QWidget::mouseMoveEvent(event);
}

QSize Connect_4::sizeHint() const
{
    return QSize(Cols * (m_currentDiameter + Margin) + Margin, Rows * (m_currentDiameter + Margin) + Margin);
}

QSize Connect_4::minimumSizeHint() const
{
    return QSize(Cols * (m_currentDiameter + Margin) + Margin, Rows * (m_currentDiameter + Margin) + Margin);
}
