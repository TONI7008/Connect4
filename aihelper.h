#ifndef AIHELPER_H
#define AIHELPER_H

#include <QObject>
#include <QVector>
#include "piece.h"

class AIHelper : public QObject
{
    Q_OBJECT
public:
    explicit AIHelper(QObject *parent = nullptr);

    void setBoardState(const QVector<QVector<Piece::Player>>& boardState);
    void findBestMove(int difficulty, int &bestRow, int &bestCol);

signals:
    void moveFound(int row, int col);
    void error(const QString& message);

private:
    QVector<QVector<Piece::Player>> m_board;

    // Returns the lowest empty row in a column, or -1 if full
    int dropRow(int col) const;

    bool isTerminal() const;
    bool checkWinFor(Piece::Player player) const;
    int  scoreWindow(const QVector<Piece::Player>& window, Piece::Player p) const;
    int  evaluateBoard() const;          // Always from p2 (AI) perspective

    // Alpha-beta minimax; maximizing = AI (p2) turn
    int minimax(int depth, int alpha, int beta, bool maximizing);

    static constexpr int INF  = 1'000'000;
    static constexpr int ROWS = 7;
    static constexpr int COLS = 6;

    // Column order: center-first for better pruning
    static const int COL_ORDER[6];
};

#endif // AIHELPER_H