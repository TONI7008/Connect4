#include "aihelper.h"
#include <algorithm>
#include <QDebug>

AIHelper::AIHelper(QObject *parent)
    : QObject(parent)
{
    m_board.resize(ROWS, QVector<Piece::Player>(COLS, Piece::px));
}

void AIHelper::setBoardState(const QVector<QVector<Piece::Player>>& boardState)
{
    m_board = boardState;
}

bool AIHelper::isValidMove(int row, int col) const
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return false;

    if (m_board[row][col] != Piece::px)
        return false;

    return (row == ROWS - 1) ||
           (m_board[row + 1][col] != Piece::px);
}

int AIHelper::countInDirection(int row, int col, Piece::Player player, const std::pair<int, int>& direction) const
{
    if (m_board[row][col] != player)
        return 0;
    
    int count = 1;
    int r = row + direction.first;
    int c = col + direction.second;
    
    // Count in positive direction
    while (r >= 0 && r < ROWS && c >= 0 && c < COLS && m_board[r][c] == player) {
        count++;
        r += direction.first;
        c += direction.second;
    }
    
    // Count in negative direction
    r = row - direction.first;
    c = col - direction.second;
    while (r >= 0 && r < ROWS && c >= 0 && c < COLS && m_board[r][c] == player) {
        count++;
        r -= direction.first;
        c -= direction.second;
    }
    
    return count;
}

int AIHelper::minimax(int depth, bool isMaximizing, Piece::Player currentPlayer)
{
    if (depth == 0) {
        return evaluateBoard(currentPlayer);
    }
    
    Piece::Player opponent = (currentPlayer == Piece::p1) ? Piece::p2 : Piece::p1;
    
    if (isMaximizing) {
        int maxEval = -INF;
        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                if (isValidMove(row, col)) {
                    // Simulate the move
                    Piece::Player original = m_board[row][col];
                    m_board[row][col] = currentPlayer;
                    
                    int eval = minimax(depth - 1, false, opponent);
                    
                    // Undo the move
                    m_board[row][col] = original;
                    
                    maxEval = std::max(maxEval, eval);
                }
            }
        }
        return maxEval;
    } else {
        int minEval = INF;
        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                if (isValidMove(row, col)) {
                    // Simulate the move
                    Piece::Player original = m_board[row][col];
                    m_board[row][col] = currentPlayer;
                    
                    int eval = minimax(depth - 1, true, opponent);
                    
                    // Undo the move
                    m_board[row][col] = original;
                    
                    minEval = std::min(minEval, eval);
                }
            }
        }
        return minEval;
    }
}

int AIHelper::evaluateBoard(Piece::Player aiPlayer) const
{
    int score = 0;
    Piece::Player opponent = (aiPlayer == Piece::p1) ? Piece::p2 : Piece::p1;
    
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            if (m_board[row][col] == aiPlayer) {
                // Center column preference
                if (col == COLS / 2) {
                    score += 10;
                }
                
                // Lower row preference
                if (row > 4) {
                    score += 5;
                }
                
                // Line counts with different weights based on length
                int horizontal = countInDirection(row, col, aiPlayer, {0, 1});
                int vertical = countInDirection(row, col, aiPlayer, {1, 0});
                int diagDownRight = countInDirection(row, col, aiPlayer, {1, 1});
                int diagDownLeft = countInDirection(row, col, aiPlayer, {1, -1});
                
                // Weight longer sequences more heavily
                if (horizontal >= 4) score += 1000;
                else if (horizontal == 3) score += 100;
                else if (horizontal == 2) score += 10;
                
                if (vertical >= 4) score += 1000;
                else if (vertical == 3) score += 100;
                else if (vertical == 2) score += 10;
                
                if (diagDownRight >= 4) score += 1000;
                else if (diagDownRight == 3) score += 100;
                else if (diagDownRight == 2) score += 10;
                
                if (diagDownLeft >= 4) score += 1000;
                else if (diagDownLeft == 3) score += 100;
                else if (diagDownLeft == 2) score += 10;
                
            } else if (m_board[row][col] == opponent) {
                // Penalize opponent's strong positions
                int horizontal = countInDirection(row, col, opponent, {0, 1});
                int vertical = countInDirection(row, col, opponent, {1, 0});
                int diagDownRight = countInDirection(row, col, opponent, {1, 1});
                int diagDownLeft = countInDirection(row, col, opponent, {1, -1});
                
                // Heavier penalties for opponent's near-wins
                if (horizontal >= 4) score -= 1500;
                else if (horizontal == 3) score -= 150;
                else if (horizontal == 2) score -= 15;
                
                if (vertical >= 4) score -= 1500;
                else if (vertical == 3) score -= 150;
                else if (vertical == 2) score -= 15;
                
                if (diagDownRight >= 4) score -= 1500;
                else if (diagDownRight == 3) score -= 150;
                else if (diagDownRight == 2) score -= 15;
                
                if (diagDownLeft >= 4) score -= 1500;
                else if (diagDownLeft == 3) score -= 150;
                else if (diagDownLeft == 2) score -= 15;
            }
        }
    }
    return score;
}

void AIHelper::findBestMove(int difficulty, int &bestRow, int &bestCol)
{
    bestRow = -1;
    bestCol = -1;
    int bestEval = -INF;
    Piece::Player aiPlayer = Piece::p2; // AI always plays as p2
    
    short count = 0;
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            if (isValidMove(row, col)) {
                count++;
                // Simulate the move
                Piece::Player original = m_board[row][col];
                m_board[row][col] = aiPlayer;
                
                int eval = minimax(difficulty, false, Piece::p1); // Opponent is p1
                
                // Undo the move
                m_board[row][col] = original;
                
                if (eval > bestEval) {
                    bestEval = eval;
                    bestRow = row;
                    bestCol = col;
                }
            }
        }
    }

    if (bestRow != -1 && bestCol != -1) {
        emit moveFound(bestRow, bestCol);
    } else {
        emit error("No valid moves found");
    }
}

void AIHelper::printBoardState() const
{
    qDebug() << "Current Board State:";
    for (int r = 0; r < ROWS; ++r) {
        QString rowStr;
        for (int c = 0; c < COLS; ++c) {
            switch (m_board[r][c]) {
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