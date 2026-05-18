#include "aihelper.h"
#include <algorithm>
#include <climits>
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

// ── Gravity helper ────────────────────────────────────────────────────────────
int AIHelper::dropRow(int col) const
{
    if (col < 0 || col >= COLS) return -1;
    for (int row = ROWS - 1; row >= 0; --row)
        if (m_board[row][col] == Piece::px)
            return row;
    return -1;
}

bool AIHelper::hasAnyMove() const
{
    for (int col = 0; col < COLS; ++col)
        if (dropRow(col) != -1) return true;
    return false;
}

// ── Win detection ─────────────────────────────────────────────────────────────
bool AIHelper::checkWinFor(Piece::Player player) const
{
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c <= COLS - 4; ++c)
            if (m_board[r][c]==player && m_board[r][c+1]==player &&
                m_board[r][c+2]==player && m_board[r][c+3]==player)
                return true;
    for (int c = 0; c < COLS; ++c)
        for (int r = 0; r <= ROWS - 4; ++r)
            if (m_board[r][c]==player && m_board[r+1][c]==player &&
                m_board[r+2][c]==player && m_board[r+3][c]==player)
                return true;
    for (int r = 0; r <= ROWS - 4; ++r)
        for (int c = 0; c <= COLS - 4; ++c)
            if (m_board[r][c]==player && m_board[r+1][c+1]==player &&
                m_board[r+2][c+2]==player && m_board[r+3][c+3]==player)
                return true;
    for (int r = 0; r <= ROWS - 4; ++r)
        for (int c = 3; c < COLS; ++c)
            if (m_board[r][c]==player && m_board[r+1][c-1]==player &&
                m_board[r+2][c-2]==player && m_board[r+3][c-3]==player)
                return true;
    return false;
}

bool AIHelper::isTerminal() const
{
    return checkWinFor(Piece::p1) || checkWinFor(Piece::p2) || !hasAnyMove();
}

// ── Board evaluation (always p2/AI perspective) ───────────────────────────────
int AIHelper::scoreWindow(const QVector<Piece::Player>& window, Piece::Player p) const
{
    Piece::Player opp = (p == Piece::p1) ? Piece::p2 : Piece::p1;
    int pCount   = window.count(p);
    int empCount = window.count(Piece::px);
    int oCount   = window.count(opp);

    if (pCount == 4)                  return  100'000;
    if (pCount == 3 && empCount == 1) return  50;
    if (pCount == 2 && empCount == 2) return  10;
    if (oCount == 3 && empCount == 1) return -80;
    if (oCount == 4)                  return -100'000;
    return 0;
}

int AIHelper::evaluateBoard() const
{
    Piece::Player ai = Piece::p2;
    int score = 0;

    for (int row = 0; row < ROWS; ++row)
        if (m_board[row][COLS / 2] == ai) score += 6;

    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c <= COLS - 4; ++c)
            score += scoreWindow({m_board[r][c],   m_board[r][c+1],
                                  m_board[r][c+2], m_board[r][c+3]}, ai);
    for (int c = 0; c < COLS; ++c)
        for (int r = 0; r <= ROWS - 4; ++r)
            score += scoreWindow({m_board[r][c],   m_board[r+1][c],
                                  m_board[r+2][c], m_board[r+3][c]}, ai);
    for (int r = 0; r <= ROWS - 4; ++r)
        for (int c = 0; c <= COLS - 4; ++c)
            score += scoreWindow({m_board[r][c],     m_board[r+1][c+1],
                                  m_board[r+2][c+2], m_board[r+3][c+3]}, ai);
    for (int r = 0; r <= ROWS - 4; ++r)
        for (int c = 3; c < COLS; ++c)
            score += scoreWindow({m_board[r][c],     m_board[r+1][c-1],
                                  m_board[r+2][c-2], m_board[r+3][c-3]}, ai);
    return score;
}

// ── Alpha-beta minimax ────────────────────────────────────────────────────────
int AIHelper::minimax(int depth, int alpha, int beta, bool maximizing)
{
    // Always check terminal BEFORE generating children.
    if (checkWinFor(Piece::p2)) return  INF + depth; // AI wins — reward faster wins
    if (checkWinFor(Piece::p1)) return -INF - depth; // Human wins — punish
    if (!hasAnyMove())          return 0;             // Draw
    if (depth == 0)             return evaluateBoard();

    Piece::Player mover = maximizing ? Piece::p2 : Piece::p1;

    if (maximizing) {
        int best = INT_MIN;  // ← INT_MIN so even a losing move beats this sentinel
        for (int ci = 0; ci < COLS; ++ci) {
            int col = COL_ORDER[ci];
            int row = dropRow(col);
            if (row == -1) continue;

            m_board[row][col] = mover;
            best  = std::max(best, minimax(depth - 1, alpha, beta, false));
            m_board[row][col] = Piece::px;

            alpha = std::max(alpha, best);
            if (alpha >= beta) break;
        }
        return best;
    } else {
        int best = INT_MAX;  // ← INT_MAX symmetric sentinel
        for (int ci = 0; ci < COLS; ++ci) {
            int col = COL_ORDER[ci];
            int row = dropRow(col);
            if (row == -1) continue;

            m_board[row][col] = mover;
            best = std::min(best, minimax(depth - 1, alpha, beta, true));
            m_board[row][col] = Piece::px;

            beta = std::min(beta, best);
            if (alpha >= beta) break;
        }
        return best;
    }
}

// ── Entry point ───────────────────────────────────────────────────────────────
void AIHelper::findBestMove(int difficulty, int &bestRow, int &bestCol)
{
    bestRow = -1;
    bestCol = -1;

    // Guard: game already decided
    if (checkWinFor(Piece::p1) || checkWinFor(Piece::p2)) {
        emit error("Game already decided — no move needed");
        return;
    }

    // Guard: board full
    if (!hasAnyMove()) {
        emit error("Board is full — draw");
        return;
    }

    // ── 1. Win in one? ────────────────────────────────────────────────────
    for (int ci = 0; ci < COLS; ++ci) {
        int col = COL_ORDER[ci];
        int row = dropRow(col);
        if (row == -1) continue;
        m_board[row][col] = Piece::p2;
        bool win = checkWinFor(Piece::p2);
        m_board[row][col] = Piece::px;
        if (win) { emit moveFound(row, col); return; }
    }

    // ── 2. Block human win in one? ────────────────────────────────────────
    for (int ci = 0; ci < COLS; ++ci) {
        int col = COL_ORDER[ci];
        int row = dropRow(col);
        if (row == -1) continue;
        m_board[row][col] = Piece::p1;
        bool win = checkWinFor(Piece::p1);
        m_board[row][col] = Piece::px;
        if (win) { emit moveFound(row, col); return; }
    }

    // ── 3. Full alpha-beta search ─────────────────────────────────────────
    // THE FIX: sentinel must be lower than any value minimax can return.
    // minimax returns as low as  -INF - depth  (e.g. -1'000'008 at depth 8).
    // Using INT_MIN guarantees any real score beats the sentinel, so bestRow
    // is always updated on the first valid move — even in a totally lost position.
    int bestEval = INT_MIN;

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

    // This should never trigger now — bestRow is set on the very first
    // valid column since INT_MIN < any minimax return value.
    if (bestRow == -1) {
        qWarning() << "AIHelper: INT_MIN sentinel failed — using first valid col";
        for (int ci = 0; ci < COLS; ++ci) {
            int col = COL_ORDER[ci];
            int row = dropRow(col);
            if (row != -1) { bestRow = row; bestCol = col; break; }
        }
    }

    if (bestRow != -1)
        emit moveFound(bestRow, bestCol);
    else
        emit error("No valid moves found (should not happen)");
}
