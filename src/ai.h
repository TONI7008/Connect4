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

    void setDifficulty(int level) { m_difficulty = qBound(1, level, 10); }
    bool isBusy() const { return m_busy; }

public slots:
    void makeMove();

signals:
    void moveMade(int row, int col);
    void aiError(const QString& message);

private slots:
    void onMoveFound(int row, int col);
    void onHelperError(const QString& message);

private:
    Connect_4 *m_game  = nullptr;
    int        m_difficulty = 4;
    bool       m_busy       = false;

    QPointer<AIHelper> m_helper;
    QPointer<QThread>  m_thread;
};

#endif // AI_H