#ifndef AI_H
#define AI_H

#include "connect_4.h"
#include "aihelper.h"
#include <QThread>
#include <QPointer>

class AI : public QObject
{
    Q_OBJECT
public:
    explicit AI(Connect_4 *game, QObject *parent = nullptr);
    ~AI();
    
    void setDifficulty(int level) { m_difficulty = level; }
    
public slots:
    void makeMove();
    
private slots:
    void onMoveFound(int row, int col);
    void onHelperError(const QString& message);
    void onHelperFinished();
    
private:
    Connect_4 *m_game;
    int m_difficulty = 2; // 1: Easy, 5: Medium, 10: Hard
    
    // Thread and helper for AI computation
    QThread m_aiThread;
    QPointer<AIHelper> m_helper;
    
    void setupThread();
};

#endif // AI_H