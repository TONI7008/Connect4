#ifndef CONNECT_4_H
#define CONNECT_4_H

#include "twidget.h"
#include "piece.h"
#include <QVector>
#include <QPair>

class Connect_4 : public TWidget
{
    Q_OBJECT

public:
    Connect_4(QWidget *parent = nullptr);
    ~Connect_4();

    static constexpr short Rows = 7;
    static constexpr short Cols = 6;

    double holeDiameter() const { return m_currentDiameter; }

    struct Slot {
        QPointF       center = QPointF(0.0, 0.0);
        Piece::Player player = Piece::px;
        Piece*        piece  = nullptr;
    };

    void          setCurrentPlayer(Piece::Player player) { m_currentPlayer = player; }
    Piece::Player currentPlayer() const                  { return m_currentPlayer;   }

    // Check for a winner anchored at the last-placed piece position.
    // Returns the winning player or Piece::px if no winner yet.
    // Call this immediately after updating board[row][col].player.
    Piece::Player checkWinAt(int row, int col);

    // Full-board scan (used after reset to be safe). Prefer checkWinAt.
    Piece::Player verifyWinner();

    bool isBoardFull() const;

    void linkPieceToSlot(Piece* piece, QPointF pos);
    void reset();

    struct WinResult {
        QPair<short,short> direction;
        short count = 0;
    };

    Slot board[Rows][Cols];

signals:
    void mouseMoved(QPoint);
    void diameterChanged(short);
    // fromAI = true when the AI placed the piece (GameEngine uses this to gate input)
    void Clicked(QPointF pos, bool fromAI = false);

protected:
    void paintEvent(QPaintEvent* event)     override;
    void mouseMoveEvent(QMouseEvent* event) override;
    QSize sizeHint()        const override;
    QSize minimumSizeHint() const override;

private:
    const  short  Margin          = 9;
    const  double minHoleDiameter = 40.0;
    double m_currentDiameter      = minHoleDiameter;

    Piece::Player m_currentPlayer = Piece::p1;

    bool      isValidMove(int row, int col);

    // Returns true and highlights all 4 winning pieces if there is a win
    // through (row,col) for player. Checks both directions of each axis.
    bool      checkWinFromCell(int row, int col, Piece::Player player);

    // Count consecutive pieces from (row,col) in direction dir (not counting
    // the starting cell itself).  Stops at board edge or different player.
    int countDir(int row, int col, Piece::Player player,
                 int dr, int dc) const;

    void printBoardState() const;

    friend class AI;
    friend class AIHelper;
};

#endif // CONNECT_4_H
