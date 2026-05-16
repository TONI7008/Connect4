#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include "piece.h"
#include "ai.h"
#include "widgetfloater.h"
#include "winneroverlay.h"
#include "appupdater.h"

namespace Ui { class GameEngine; }

class GameEngine : public QWidget
{
    Q_OBJECT

public:
    explicit GameEngine(QWidget *parent = nullptr);
    ~GameEngine();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onBoardClicked(QPointF pos, bool fromAI);
    void reset();
    void enableAi(bool enable);
    // Anchored winner check — always checks from the just-placed cell
    void verifyWinnerAt(int row, int col, Piece::Player justPlayed);
    void onUpdateAvailable(const QString &ver, const QUrl &url);
    void onDownloadProgress(int percent);
    void onDownloadFinished(const QString &path);

private:
    Ui::GameEngine  *ui             = nullptr;
    Piece           *m_piece        = nullptr;
    AI              *m_ai           = nullptr;
    WidgetFloater   *m_floater      = nullptr;
    WinnerOverlay   *m_overlay      = nullptr;
    AppUpdater      *m_updater      = nullptr;

    bool            m_aiMode        = false;
    bool            m_gameOver      = false;
    Piece::Player   m_currentPlayer = Piece::p1;

    QVector<Piece*> m_pieces;

    QPropertyAnimation* dropPiece(Piece* piece, const QPointF& finalCenter, int duration);
    void createPiece();
    void setTurnLabel(Piece::Player player);
    void cleanupPieces();
    void showResult(Piece::Player winner);
};

#endif // GAMEENGINE_H
