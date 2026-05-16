#include "aihelper.h"
#include <algorithm>
#include <QDebug>

// Search center columns first — greatly improves alpha-beta pruning
const int AIHelper::COL_ORDER[6] = {2, 3, 1, 4, 0, 5};

AIHelper::AIHelper(QObject *parent)
    : QObject(parent)
{
    m_board.resize(ROWS, QVector<Piece::Player>(COLS, Piece::px));
}

void AIHelper::setBoardState(const QVector<QVector<Piece::Player>>& boardState)
{
    m_board = boardState;
}

// Gravity: find the lowest empty row in a column
int AIHelper::dropRow(int col) const
{
    if (col < 0 || col >= COLS) return -1;
    for (int row = ROWS - 1; row >= 0; --row) {
        if (m_board[row][col] == Piece::px)
            return row;
    }
    return -1; // column full
}

// Score a 4-cell window from p's perspective
int AIHelper::scoreWindow(const QVector<Piece::Player>& window, Piece::Player p) const
{
    Piece::Player opp = (p == Piece::p1) ? Piece::p2 : Piece::p1;
    int pCount   = window.count(p);
    int empCount = window.count(Piece::px);
    int oCount   = window.count(opp);

    if (pCount == 4)               return  100'000; // Win!
    if (pCount == 3 && empCount == 1) return  50;
    if (pCount == 2 && empCount == 2) return  10;
    if (oCount == 3 && empCount == 1) return -80;   // Block urgently
    if (oCount == 4)               return -100'000; // Opponent wins
    return 0;
}

// Evaluate the whole board, always from p2 (AI) perspective
int AIHelper::evaluateBoard() const
{
    Piece::Player ai  = Piece::p2;
    int score = 0;

    // Center column bonus
    int centerCol = COLS / 2;
    for (int row = 0; row < ROWS; ++row) {
        if (m_board[row][centerCol] == ai) score += 6;
    }

    // Horizontal windows
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col <= COLS - 4; ++col) {
            QVector<Piece::Player> window = {
                m_board[row][col],
                m_board[row][col+1],
                m_board[row][col+2],
                m_board[row][col+3]
            };
            score += scoreWindow(window, ai);
        }
    }

    // Vertical windows
    for (int col = 0; col < COLS; ++col) {
        for (int row = 0; row <= ROWS - 4; ++row) {
            QVector<Piece::Player> window = {
                m_board[row][col],
                m_board[row+1][col],
                m_board[row+2][col],
                m_board[row+3][col]
            };
            score += scoreWindow(window, ai);
        }
    }

    // Diagonal down-right
    for (int row = 0; row <= ROWS - 4; ++row) {
        for (int col = 0; col <= COLS - 4; ++col) {
            QVector<Piece::Player> window = {
                m_board[row][col],
                m_board[row+1][col+1],
                m_board[row+2][col+2],
                m_board[row+3][col+3]
            };
            score += scoreWindow(window, ai);
        }
    }

    // Diagonal down-left
    for (int row = 0; row <= ROWS - 4; ++row) {
        for (int col = 3; col < COLS; ++col) {
            QVector<Piece::Player> window = {
                m_board[row][col],
                m_board[row+1][col-1],
                m_board[row+2][col-2],
                m_board[row+3][col-3]
            };
            score += scoreWindow(window, ai);
        }
    }

    return score;
}

bool AIHelper::checkWinFor(Piece::Player player) const
{
    // Horizontal
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c <= COLS - 4; ++c)
            if (m_board[r][c]==player && m_board[r][c+1]==player &&
                m_board[r][c+2]==player && m_board[r][c+3]==player)
                return true;
    // Vertical
    for (int c = 0; c < COLS; ++c)
        for (int r = 0; r <= ROWS - 4; ++r)
            if (m_board[r][c]==player && m_board[r+1][c]==player &&
                m_board[r+2][c]==player && m_board[r+3][c]==player)
                return true;
    // Diagonal down-right
    for (int r = 0; r <= ROWS - 4; ++r)
        for (int c = 0; c <= COLS - 4; ++c)
            if (m_board[r][c]==player && m_board[r+1][c+1]==player &&
                m_board[r+2][c+2]==player && m_board[r+3][c+3]==player)
                return true;
    // Diagonal down-left
    for (int r = 0; r <= ROWS - 4; ++r)
        for (int c = 3; c < COLS; ++c)
            if (m_board[r][c]==player && m_board[r+1][c-1]==player &&
                m_board[r+2][c-2]==player && m_board[r+3][c-3]==player)
                return true;
    return false;
}

bool AIHelper::isTerminal() const
{
    if (checkWinFor(Piece::p1) || checkWinFor(Piece::p2)) return true;
    // Draw: no valid columns left
    for (int col = 0; col < COLS; ++col)
        if (dropRow(col) != -1) return false;
    return true;
}

// Alpha-beta minimax — maximizing == AI (p2) moving
int AIHelper::minimax(int depth, int alpha, int beta, bool maximizing)
{
    // Terminal / depth check
    if (checkWinFor(Piece::p2)) return  INF + depth; // AI wins — prefer faster wins
    if (checkWinFor(Piece::p1)) return -INF - depth; // Human wins
    if (depth == 0 || isTerminal()) return evaluateBoard();

    Piece::Player mover = maximizing ? Piece::p2 : Piece::p1;

    if (maximizing) {
        int best = -INF;
        for (int ci = 0; ci < COLS; ++ci) {
            int col = COL_ORDER[ci];
            int row = dropRow(col);
            if (row == -1) continue;

            m_board[row][col] = mover;
            int val = minimax(depth - 1, alpha, beta, false);
            m_board[row][col] = Piece::px;

            best  = std::max(best, val);
            alpha = std::max(alpha, best);
            if (alpha >= beta) break; // Beta cut-off
        }
        return best;
    } else {
        int best = INF;
        for (int ci = 0; ci < COLS; ++ci) {
            int col = COL_ORDER[ci];
            int row = dropRow(col);
            if (row == -1) continue;

            m_board[row][col] = mover;
            int val = minimax(depth - 1, alpha, beta, true);
            m_board[row][col] = Piece::px;

            best = std::min(best, val);
            beta = std::min(beta, best);
            if (alpha >= beta) break; // Alpha cut-off
        }
        return best;
    }
}

void AIHelper::findBestMove(int difficulty, int &bestRow, int &bestCol)
{
    bestRow = -1;
    bestCol = -1;
    int bestEval = -INF - 1;

    // Immediate win / block check before full minimax
    // 1. Can AI win immediately?
    for (int ci = 0; ci < COLS; ++ci) {
        int col = COL_ORDER[ci];
        int row = dropRow(col);
        if (row == -1) continue;
        m_board[row][col] = Piece::p2;
        if (checkWinFor(Piece::p2)) {
            m_board[row][col] = Piece::px;
            bestRow = row; bestCol = col;
            emit moveFound(bestRow, bestCol);
            return;
        }
        m_board[row][col] = Piece::px;
    }

    // 2. Must we block player immediately?
    for (int ci = 0; ci < COLS; ++ci) {
        int col = COL_ORDER[ci];
        int row = dropRow(col);
        if (row == -1) continue;
        m_board[row][col] = Piece::p1;
        if (checkWinFor(Piece::p1)) {
            m_board[row][col] = Piece::px;
            bestRow = row; bestCol = col;
            emit moveFound(bestRow, bestCol);
            return;
        }
        m_board[row][col] = Piece::px;
    }

    // 3. Full alpha-beta search
    for (int ci = 0; ci < COLS; ++ci) {
        int col = COL_ORDER[ci];
        int row = dropRow(col);
        if (row == -1) continue;

        m_board[row][col] = Piece::p2;
        int eval = minimax(difficulty - 1, -INF, INF, false);
        m_board[row][col] = Piece::px;

        if (eval > bestEval) {
            bestEval = eval;
            bestRow  = row;
            bestCol  = col;
        }
    }

    if (bestRow != -1 && bestCol != -1) {
        emit moveFound(bestRow, bestCol);
    } else {
        emit error("No valid moves found");
    }
}