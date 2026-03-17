#include "ai.h"
#include <QThread>
#include <QDebug>

AI::AI(Connect_4 *game, QObject *parent)
    : QObject(parent)
    , m_game(game)
{
    
}

AI::~AI()
{
      
    if (m_helper) {
        m_helper->deleteLater();
    }
}

void AI::makeMove()
{
    if (!m_game) {
        qWarning() << "AI: Cannot make move - game is null";
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

    QThread* m_aiThread = new QThread(nullptr);
    m_helper = new AIHelper(nullptr);
    m_helper->setBoardState(currentBoard);

    connect(m_aiThread, &QThread::started, [this, currentBoard]() {
        int bestRow, bestCol;
        m_helper->findBestMove(m_difficulty, bestRow, bestCol);
    });
        
    m_helper->moveToThread(m_aiThread);
    
    // Connect signals/slots
    connect(m_aiThread, &QThread::finished, this, [this, m_aiThread]() {
        m_helper->deleteLater();
        m_aiThread->deleteLater();
    });
    connect(m_helper, &AIHelper::moveFound, this, &AI::onMoveFound);
    connect(m_helper, &AIHelper::error, this, &AI::onHelperError);
    
    // Set thread priority to high for better AI performance
    m_aiThread->start(QThread::HighPriority);


}

void AI::onMoveFound(int row, int col)
{
    if (m_game && row != -1 && col != -1) {
        // Simulate a click on the Connect_4 widget at the center of the chosen slot
        QPointF movePos = m_game->board[row][col].center;
        m_game->board[row][col].player = Piece::p2; // Mark the slot as occupied by AI player
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