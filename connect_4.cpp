#include "connect_4.h"
#include <QGridLayout>


Connect_4::Connect_4(QWidget *parent)
    : TWidget(parent)
{

    setEnableBackground(true);
    setBackgroundImage("/home/hacker/Documents/C++/Connect4/Images/Melamine-wood-005.png");

    connect(this, &Connect_4::clicked, [this](const QPointF& pos) {
        // Handle click events here
        // You can determine which column was clicked based on the x-coordinate of pos
        int col = pos.x() / (width() / Cols);
        int row = pos.y() / (height() / Rows);

        if (isValidMove(row, col)) {
            //qDebug() << "Clicked on column:" << col << "row:" << row;
            board[row][col].player = m_currentPlayer; // Mark the slot as occupied by the current player
            emit Clicked(board[row][col].center);
            
        }
    });

    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setAttribute(Qt::WA_TranslucentBackground);

}

Connect_4::~Connect_4()
{
    
}

bool Connect_4::checkWin(int row, int col, Piece::Player player)
{
   QPair<short,short> directions[] = {
        {0, 1},   // Horizontal
        {1, 0},   // Vertical
        {1, 1},   // Diagonal down-right
        {1, -1}   // Diagonal down-left
    };

    for (const auto& dir : directions) {
        int count = 1; // Count the current piece

        count += countInDirection(row, col, player, dir); // Count in  direction

        if (count >= 4) {
            return true; // Win condition met
        }
    }

    return false; // No win found
}

int Connect_4::countInDirection(int row, int col, Piece::Player player, const QPair<int, int>& dir)
{
    int count = 0;

    // Check in the positive direction
    for (int i = 1; i < 4; ++i) {
        int r = row + dir.first * i;
        int c = col + dir.second * i;
        if (r >= 0 && r < Rows && c >= 0 && c < Cols && board[r][c].player == player) {
            count++;
        } else {
            break;
        }
    }

    // Check in the negative direction
    for (int i = 1; i < 4; ++i) {
        int r = row - dir.first * i;
        int c = col - dir.second * i;
        if (r >= 0 && r < Rows && c >= 0 && c < Cols && board[r][c].player == player) {
            count++;
        } else {
            break;
        }
    }

    return count;
}

bool Connect_4::isValidMove(int row, int col)
{
    if (row < 0 || row >= Rows || col < 0 || col >= Cols)
        return false;

    if (board[row][col].player != Piece::px)
        return false;

    return (row == Rows - 1) ||
           (board[row + 1][col].player != Piece::px);
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
    boardPath.addRoundedRect(rect(),height()/15,height()/15);

    for (int r = 0; r < Rows; ++r) {
        for (int c = 0; c < Cols; ++c) {

            double x = Margin + c * (holeW + Margin) + (holeW - m_currentDiameter) / 2.0;
            double y = Margin + r * (holeH + Margin) + (holeH - m_currentDiameter) / 2.0;

            board[r][c].center = QPointF(x, y);
            boardPath.addEllipse(QRectF(x, y, m_currentDiameter, m_currentDiameter));
            
            if(board[r][c].player != Piece::px){
               if(board[r][c].piece){
                    QRectF pieceRect(board[r][c].center, QSizeF(m_currentDiameter, m_currentDiameter));
                    board[r][c].piece->setGeometry(pieceRect.toRect());
                }
            }

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

void Connect_4::printBoardState() const
{
    qDebug() << "Current Board State:";
    for (int r = 0; r < Rows; ++r) {
        QString rowStr;
        for (int c = 0; c < Cols; ++c) {
            switch (board[r][c].player) {
                case Piece::p1:
                    rowStr += "-P1-";
                    break;
                case Piece::p2:
                    rowStr += "-P2-";
                    break;
                case Piece::px:
                    rowStr += "-Em-";
                    break;
            }
        }
        qDebug() << rowStr;
    }
}
Piece::Player Connect_4::verifyWinner()
{
    for (int r = 0; r < Rows; ++r) {
        for (int c = 0; c < Cols; ++c) {
            if (board[r][c].player != Piece::px) {
                if (checkWin(r, c, board[r][c].player)) {
                    return board[r][c].player; // Return the winner
                }
            }
        }
    }
    return Piece::px; // No winner
}
void Connect_4::linkPieceToSlot(Piece* _piece,QPointF pos){
    if(!_piece) return;
    int col = pos.x() / (width() / Cols);
    int row = pos.y() / (height() / Rows);

    board[row][col].piece = _piece;
}

void Connect_4::updatePositions(){
   qDebug() << "w :" << width() << " h :" << height();
   
   double holeW = (double)(width() - (Margin * (Cols + 1))) / Cols;
   double holeH = (double)(height() - (Margin * (Rows + 1))) / Rows;
   
   for (int r = 0; r < Rows; ++r) {
       for (int c = 0; c < Cols; ++c) {
           
           double x = Margin + c * (holeW + Margin) + (holeW - m_currentDiameter) / 2.0;
           double y = Margin + r * (holeH + Margin) + (holeH - m_currentDiameter) / 2.0;
           
           if(board[r][c].player != Piece::px){
               if(board[r][c].piece){
                    qDebug() << "x :" << x << " y :" << y;
                    QRectF pieceRect(board[r][c].center, QSizeF(m_currentDiameter, m_currentDiameter));
                    board[r][c].piece->setGeometry(pieceRect.toRect());
                }
            }
        }
    }
}

void Connect_4::reset(){
    for (int r = 0; r < Rows; ++r) {
        for (int c = 0; c < Cols; ++c) {
            board[r][c].player = Piece::px;
            if(board[r][c].piece){
                board[r][c].piece = nullptr;
            }
        }
    }
}