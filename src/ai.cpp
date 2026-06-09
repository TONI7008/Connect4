#include "ai.h"
#include <QDebug>

AI::AI(Connect_4 *game, QObject *parent)
    : QObject(parent)
    , m_game(game)
{
    
}

AI::~AI()
{
    // Stop the worker thread cleanly
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(2000);
    }
}

void AI::makeMove()
{
    if (!m_game || m_busy) return;
    m_busy = true;

    // Snapshot the board on the main thread
    QVector<QVector<Piece::Player>> boardSnapshot;
    boardSnapshot.resize(Connect_4::Rows);
    for (int r = 0; r < Connect_4::Rows; ++r) {
        boardSnapshot[r].resize(Connect_4::Cols);
        for (int c = 0; c < Connect_4::Cols; ++c)
            boardSnapshot[r][c] = m_game->board[r][c].player;
    }

    // Clean up any previous worker
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }

    QThread  *thread = new QThread(nullptr);
    AIHelper *helper = new AIHelper(nullptr);
    helper->setBoardState(boardSnapshot);

    m_thread = thread;
    m_helper = helper;

    helper->moveToThread(thread);

    // Wire signals before starting
    connect(helper, &AIHelper::moveFound, this, &AI::onMoveFound, Qt::QueuedConnection);
    connect(helper, &AIHelper::error,     this, &AI::onHelperError, Qt::QueuedConnection);

    // Start computation when thread starts
    connect(thread, &QThread::started, helper, [helper, this]() {
        int r = -1, c = -1;
        helper->findBestMove(m_difficulty, r, c);
    });

    // Cleanup after thread finishes
    connect(thread, &QThread::finished, helper, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start(QThread::HighPriority);
}

void AI::onMoveFound(int row, int col)
{
    m_busy = false;

    if (!m_game || row == -1 || col == -1) return;

    // Validate the move is still legal (board may have changed)
    if (m_game->board[row][col].player != Piece::px) {
        emit aiError("AI chose an occupied cell — skipping");
        return;
    }

    // Mark the slot and emit the signal so GameEngine handles animation
    // (GameEngine's Clicked handler will set the player on its side via setCurrentPlayer)
    QPointF movePos = m_game->board[row][col].center;

    // Pre-mark the board so the Connect_4 click handler doesn't reject it
    // and so verifyWinner works correctly after the piece drops
    m_game->board[row][col].player = Piece::p2;

    emit moveMade(row, col);          // tell GameEngine
    emit m_game->Clicked(movePos, true); // drive the animation / turn logic
}

void AI::onHelperError(const QString& message)
{
    m_busy = false;
    qWarning() << "AI error:" << message;
    emit aiError(message);
}