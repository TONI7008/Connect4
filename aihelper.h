#ifndef AIHELPER_H
#define AIHELPER_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include "connect_4.h"

class AIHelper : public QObject
{
    Q_OBJECT
public:
    explicit AIHelper(QObject *parent = nullptr);
    
    // Set the current board state
    void setBoardState(const QVector<QVector<Piece::Player>>& boardState);
    
    // Main AI functions
    void findBestMove(int difficulty, int &bestRow, int &bestCol);
    
signals:
    void moveFound(int row, int col);
    void error(const QString& message);
    
private:
    QVector<QVector<Piece::Player>> m_board;
    
    // Helper functions
    bool isValidMove(int row, int col) const;
    int countInDirection(int row, int col, Piece::Player player, const std::pair<int, int>& direction) const;
    int minimax(int depth, bool isMaximizing, Piece::Player currentPlayer);
    int evaluateBoard(Piece::Player aiPlayer) const;
    void printBoardState() const;
    
    // Constants
    static constexpr int INF = 1000000;
    static constexpr int ROWS = 7;
    static constexpr int COLS = 6;
};

#endif // AIHELPER_H