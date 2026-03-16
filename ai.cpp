#include "ai.h"
#include <QThread>
#include <QDebug>

AI::AI(Connect_4 *game, QObject *parent)
    : QObject(parent)
    , m_game(game)
{
    setupThread();
}

AI::~AI()
{
    m_aiThread.quit();
    m_aiThread.wait();
    
    if (m_helper) {
        m_helper->deleteLater();
    }
}

void AI::setupThread()
{
    // Create helper and move to thread
    m_helper = new AIHelper();
    m_helper->moveToThread(&m_aiThread);
    
    // Connect signals/slots
    connect(&m_aiThread, &QThread::finished, m_helper, &QObject::deleteLater);
    connect(m_helper, &AIHelper::moveFound, this, &AI::onMoveFound);
    connect(m_helper, &AIHelper::error, this, &AI::onHelperError);
    
    // Set thread priority to high for better AI performance
    m_aiThread.start(QThread::HighPriority);
}

void AI::makeMove()
{
    if (!m_helper || !m_game) {
        qWarning() << "AI: Cannot make move - helper or game is null";
        return;
    }
    
    // Capture the current board state
    QVector<QVector<Piece::Player>> currentBoard;
    currentBoard.resize(Connect_4::Rows);
    
    for (int row = 0; row < Connect_4::Rows; ++row) {
        currentBoard[row].resize(Connect_4::Cols);
        for (int col = 0; col < Connect_4::Cols; ++col) {
            currentBoard[row][col] = m_game->board[row][col].player;
        }
    }
    
    // Use QMetaObject::invokeMethod to call the helper in its thread
    QMetaObject::invokeMethod(m_helper, [this, currentBoard]() {
        m_helper->setBoardState(currentBoard);
        
        int bestRow, bestCol;
        m_helper->findBestMove(m_difficulty, bestRow, bestCol);
        
        // The result will be emitted via moveFound signal
    }, Qt::QueuedConnection);
}

void AI::onMoveFound(int row, int col)
{
    if (m_game && row != -1 && col != -1) {
        // Simulate a click on the Connect_4 widget at the center of the chosen slot
        QPointF movePos = m_game->board[row][col].center;
        emit m_game->Clicked(movePos, true); // Emit the clicked signal with the position and indicate it's from AI
    }
}

void AI::onHelperError(const QString& message)
{
    qWarning() << "AI Helper error:" << message;
}

void AI::onHelperFinished()
{
    // Clean up if needed
    qDebug() << "AI Helper finished computation";
}