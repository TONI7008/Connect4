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

    WinResult result;
    for (const auto& dir : directions) {
        int count = 1; // Count the current piece

        result = countInDirection(row, col, player, dir);
        count += result.count; // Count in  direction

        if (count >= 4) {
            for(int i=0;i<=result.count;i++){
                int r = row + dir.first * i;
                int c = col + dir.second * i;
                
                if(r==row && c==col) continue; // Skip the original piece

                if (r >= 0 && r < Rows && c >= 0 && c < Cols && board[r][c].player == player) {
                    board[r][c].piece->setHighlighted(true);
                } else {
                    break;
                }
            }
            return true; // Win condition met
        }
    }

    return false; // No win found
}

Connect_4::WinResult Connect_4::countInDirection(int row, int col, Piece::Player player, const QPair<int, int>& dir)
{
    WinResult result;
    result.count = 0;

    // Check in the positive direction
    for (int i = 1; i < 4; ++i) {
        int r = row + dir.first * i;
        int c = col + dir.second * i;
        if (r >= 0 && r < Rows && c >= 0 && c < Cols && board[r][c].player == player) {
            result.count++;
            result.direction = dir;
        } else {
            break;
        }
    }

    return result;
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
    QWidget::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);

    double holeW = (double)(width() - (Margin * (Cols + 1))) / Cols;
    double holeH = (double)(height() - (Margin * (Rows + 1))) / Rows;

    m_currentDiameter = std::max(minHoleDiameter, std::min(holeW, holeH));
    emit diameterChanged((short)m_currentDiameter);

    //------------------------------------------------------
    // 1. BOARD BASE SHAPE
    //------------------------------------------------------
    QPainterPath boardPath;
    qreal radius = height() / 15.0;
    boardPath.addRoundedRect(rect(), radius, radius);

    //------------------------------------------------------
    // 2. HOLES (cut out)
    //------------------------------------------------------
    QVector<QRectF> holes;

    for (int r = 0; r < Rows; ++r) {
        for (int c = 0; c < Cols; ++c) {

            double x = Margin + c * (holeW + Margin) + (holeW - m_currentDiameter) / 2.0;
            double y = Margin + r * (holeH + Margin) + (holeH - m_currentDiameter) / 2.0;

            QRectF holeRect(x, y, m_currentDiameter, m_currentDiameter);
            holes.push_back(holeRect);

            board[r][c].center = QPointF(x, y);

            boardPath.addEllipse(holeRect);

            if(board[r][c].player != Piece::px && board[r][c].piece){
                board[r][c].piece->setGeometry(holeRect.toRect());
            }
        }
    }

    boardPath.setFillRule(Qt::OddEvenFill);

    //------------------------------------------------------
    // 3. BASE GRADIENT (plastic body)
    //------------------------------------------------------
    QColor boardColor(20, 90, 200); // Base board color light blue
    QColor base      = boardColor; // base board color
    QColor light     = base.lighter(130);   // top light
    QColor mid       = base;
    QColor dark      = base.darker(130);    // mid shadow
    QColor deepDark  = base.darker(180);    // edge shadow

    QLinearGradient baseGrad(0, 0, width(), height());
    baseGrad.setColorAt(0.0, light);
    baseGrad.setColorAt(0.4, mid);
    baseGrad.setColorAt(1.0, deepDark);

    p.fillPath(boardPath, baseGrad);

    //------------------------------------------------------
    // 4. GLOBAL LIGHT (top highlight)
    //------------------------------------------------------
    QLinearGradient topLight(0, 0, 0, height() * 0.5);
    topLight.setColorAt(0.0, QColor(250,250,250,80));
    topLight.setColorAt(1.0, QColor(250,250,250,0));

    p.fillPath(boardPath, topLight);

    //------------------------------------------------------
    // 5. HOLE DEPTH (adapted to boardColor)
    //------------------------------------------------------
    for (const QRectF& hole : holes)
    {
        QPointF c = hole.center();
        qreal r = hole.width() / 2.0;

        // Inner shadow (depth)
        QRadialGradient shadowGrad(c, r);
        shadowGrad.setColorAt(0.6, QColor(0,0,0,0));
        shadowGrad.setColorAt(1.0, QColor(0,0,0,140));

        p.setBrush(shadowGrad);
        p.drawEllipse(hole);

        // Inner rim highlight (color-aware, not pure white)
        QRectF inner = hole.adjusted(1,1,-1,-1);

        QColor rimHighlight = boardColor.lighter(160);
        rimHighlight.setAlpha(120);

        QRadialGradient rimLight(c - QPointF(r*0.3, r*0.3), r);
        rimLight.setColorAt(0.0, rimHighlight);
        rimLight.setColorAt(1.0, QColor(255,255,255,0));

        p.setBrush(rimLight);
        p.drawEllipse(inner);

        // Outer rim shadow (slightly tinted, not pure black)
        QColor rimShadow = boardColor.darker(200);
        rimShadow.setAlpha(120);

        QPen rimPen(rimShadow, 2);
        rimPen.setCosmetic(true);
        p.setPen(rimPen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(hole);

        p.setPen(Qt::NoPen);
    }

    //------------------------------------------------------
    // 6. OUTER EDGE (thickness, color-aware)
    //------------------------------------------------------
    QColor edgeColor = boardColor.darker(220);
    edgeColor.setAlpha(150);

    QPen edgePen(edgeColor, 3);
    edgePen.setCosmetic(true);
    p.setPen(edgePen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), radius, radius);

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