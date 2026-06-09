#include "connect_4.h"
#include <QPainter>
#include <QMouseEvent>

// ── Constructor ───────────────────────────────────────────────────────────────

Connect_4::Connect_4(QWidget *parent)
    : TWidget(parent)
{
    setEnableBackground(true);
    setBackgroundImage("/home/hacker/Documents/C++/Connect4/Images/Melamine-wood-005.png");

    connect(this, &Connect_4::clicked, [this](const QPointF& pos) {
        int col = static_cast<int>(pos.x()) / (width()  / Cols);
        int row = static_cast<int>(pos.y()) / (height() / Rows);

        if (isValidMove(row, col)) {
            board[row][col].player = m_currentPlayer;
            emit Clicked(board[row][col].center);
        }
    });

    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setAttribute(Qt::WA_TranslucentBackground);
}

Connect_4::~Connect_4() {}

// ── Move validation ───────────────────────────────────────────────────────────

bool Connect_4::isValidMove(int row, int col)
{
    if (row < 0 || row >= Rows || col < 0 || col >= Cols) return false;
    if (board[row][col].player != Piece::px)               return false;
    // A cell is playable only if the cell below it is occupied (or it is the bottom row)
    return (row == Rows - 1) || (board[row + 1][col].player != Piece::px);
}

// ── Win detection ─────────────────────────────────────────────────────────────

// Count pieces of `player` stepping in direction (dr,dc) from (row,col),
// NOT including the starting cell.
int Connect_4::countDir(int row, int col, Piece::Player player,
                        int dr, int dc) const
{
    int count = 0;
    int r = row + dr;
    int c = col + dc;
    while (r >= 0 && r < Rows && c >= 0 && c < Cols
           && board[r][c].player == player)
    {
        ++count;
        r += dr;
        c += dc;
    }
    return count;
}

// Check all 4 axes through (row,col). If any gives >= 4 in a line,
// highlight those pieces and return true.
bool Connect_4::checkWinFromCell(int row, int col, Piece::Player player)
{
    // Each axis: (dr, dc) and its opposite (-dr, -dc)
    static const int axes[4][2] = {
        {0, 1},   // horizontal
        {1, 0},   // vertical
        {1, 1},   // diagonal ↘
        {1, -1}   // diagonal ↙
    };

    for (auto& ax : axes) {
        int dr = ax[0], dc = ax[1];

        int fwd = countDir(row, col, player,  dr,  dc);
        int bwd = countDir(row, col, player, -dr, -dc);
        int total = 1 + fwd + bwd;

        if (total >= 4) {
            // Highlight the placed cell
            if (board[row][col].piece)
                board[row][col].piece->setHighlighted(true);

            // Highlight forward chain
            for (int i = 1; i <= fwd; ++i) {
                int r = row + dr * i;
                int c = col + dc * i;
                if (board[r][c].piece)
                    board[r][c].piece->setHighlighted(true);
            }
            // Highlight backward chain
            for (int i = 1; i <= bwd; ++i) {
                int r = row - dr * i;
                int c = col - dc * i;
                if (board[r][c].piece)
                    board[r][c].piece->setHighlighted(true);
            }
            return true;
        }
    }
    return false;
}

// Preferred API: check only the cell that was just played.
// Always returns exactly the right 4 highlighted pieces.
Piece::Player Connect_4::checkWinAt(int row, int col)
{
    if (row < 0 || row >= Rows || col < 0 || col >= Cols) return Piece::px;
    Piece::Player p = board[row][col].player;
    if (p == Piece::px) return Piece::px;
    return checkWinFromCell(row, col, p) ? p : Piece::px;
}

// Full-board scan fallback (safe to call any time).
Piece::Player Connect_4::verifyWinner()
{
    for (int r = 0; r < Rows; ++r)
        for (int c = 0; c < Cols; ++c)
            if (board[r][c].player != Piece::px)
                if (checkWinFromCell(r, c, board[r][c].player))
                    return board[r][c].player;
    return Piece::px;
}

bool Connect_4::isBoardFull() const
{
    for (int c = 0; c < Cols; ++c)
        if (board[0][c].player == Piece::px) return false;
    return true;
}

// ── Slot linkage ──────────────────────────────────────────────────────────────

void Connect_4::linkPieceToSlot(Piece* _piece, QPointF pos)
{
    if (!_piece) return;
    int col = static_cast<int>(pos.x()) / (width()  / Cols);
    int row = static_cast<int>(pos.y()) / (height() / Rows);
    if (row >= 0 && row < Rows && col >= 0 && col < Cols)
        board[row][col].piece = _piece;
}

void Connect_4::reset()
{
    for (int r = 0; r < Rows; ++r)
        for (int c = 0; c < Cols; ++c) {
            board[r][c].player = Piece::px;
            board[r][c].piece  = nullptr;
        }
    m_currentPlayer = Piece::p1;
    update();
}

// ── Painting ──────────────────────────────────────────────────────────────────

void Connect_4::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);

    double holeW = (double)(width()  - (Margin * (Cols + 1))) / Cols;
    double holeH = (double)(height() - (Margin * (Rows + 1))) / Rows;

    m_currentDiameter = std::max(minHoleDiameter, std::min(holeW, holeH));
    emit diameterChanged((short)m_currentDiameter);

    // Board rounded shape
    QPainterPath boardPath;
    qreal radius = height() / 15.0;
    boardPath.addRoundedRect(rect(), radius, radius);

    QVector<QRectF> holes;
    for (int r = 0; r < Rows; ++r) {
        for (int c = 0; c < Cols; ++c) {
            double x = Margin + c * (holeW + Margin) + (holeW - m_currentDiameter) / 2.0;
            double y = Margin + r * (holeH + Margin) + (holeH - m_currentDiameter) / 2.0;
            QRectF holeRect(x, y, m_currentDiameter, m_currentDiameter);
            holes.push_back(holeRect);

            board[r][c].center = QPointF(x, y);
            boardPath.addEllipse(holeRect);

            if (board[r][c].player != Piece::px && board[r][c].piece)
                board[r][c].piece->setGeometry(holeRect.toRect());
        }
    }
    boardPath.setFillRule(Qt::OddEvenFill);

    // ── Base gradient ───────────────────────────────────────────────────────
    QColor boardColor(20, 90, 200);
    QLinearGradient baseGrad(0, 0, width(), height());
    baseGrad.setColorAt(0.0, boardColor.lighter(130));
    baseGrad.setColorAt(0.4, boardColor);
    baseGrad.setColorAt(1.0, boardColor.darker(180));
    p.fillPath(boardPath, baseGrad);

    // ── Top highlight ────────────────────────────────────────────────────────
    QLinearGradient topLight(0, 0, 0, height() * 0.5);
    topLight.setColorAt(0.0, QColor(250,250,250,80));
    topLight.setColorAt(1.0, QColor(250,250,250,0));
    p.fillPath(boardPath, topLight);

    // ── Hole shading ─────────────────────────────────────────────────────────
    for (const QRectF& hole : holes) {
        QPointF center = hole.center();
        qreal   r2     = hole.width() / 2.0;

        // Inner depth shadow
        QRadialGradient shadowGrad(center, r2);
        shadowGrad.setColorAt(0.6, QColor(0,0,0,0));
        shadowGrad.setColorAt(1.0, QColor(0,0,0,140));
        p.setBrush(shadowGrad);
        p.drawEllipse(hole);

        // Inner rim light
        QRectF inner = hole.adjusted(1,1,-1,-1);
        QColor rimHighlight = boardColor.lighter(160);
        rimHighlight.setAlpha(120);
        QRadialGradient rimLight(center - QPointF(r2*0.3, r2*0.3), r2);
        rimLight.setColorAt(0.0, rimHighlight);
        rimLight.setColorAt(1.0, QColor(255,255,255,0));
        p.setBrush(rimLight);
        p.drawEllipse(inner);

        // Outer rim shadow
        QColor rimShadow = boardColor.darker(200);
        rimShadow.setAlpha(120);
        QPen rimPen(rimShadow, 2);
        rimPen.setCosmetic(true);
        p.setPen(rimPen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(hole);
        p.setPen(Qt::NoPen);
    }

    // ── Board outer edge ─────────────────────────────────────────────────────
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
    return QSize(Cols * (m_currentDiameter + Margin) + Margin,
                 Rows * (m_currentDiameter + Margin) + Margin);
}
QSize Connect_4::minimumSizeHint() const { return sizeHint(); }

void Connect_4::printBoardState() const
{
    qDebug() << "Board state:";
    for (int r = 0; r < Rows; ++r) {
        QString row;
        for (int c = 0; c < Cols; ++c)
            row += board[r][c].player==Piece::p1 ? "-P1-" :
                   board[r][c].player==Piece::p2 ? "-P2-" : "-Em-";
        qDebug() << row;
    }
}
