#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QWidget>
#include <QPropertyAnimation>
#include "piece.h"
#include"ai.h"
#include "widgetfloater.h"

namespace Ui {
class GameEngine;
}

class GameEngine : public QWidget
{
    Q_OBJECT

public:
    explicit GameEngine(QWidget *parent = nullptr);
    ~GameEngine();
protected:
    void resizeEvent(QResizeEvent* event) override;
private:
    Ui::GameEngine *ui;
    Piece* m_piece=nullptr;
    AI* m_ai=nullptr;
    WidgetFloater* m_floater=nullptr;

    bool ai_mode=false;

    QPropertyAnimation *dropPiece(Piece *piece, const QPointF &finalCenter, int duration);
    void createPiece();

    QVector<Piece*> m_pieces;
    void reset();
    void verifyWinner();
    
    void enableAi(bool enable);

    Piece::Player m_currentPlayer=Piece::p1;
};

#endif // GAMEENGINE_H
