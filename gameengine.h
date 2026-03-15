#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QWidget>
#include <QPropertyAnimation>
#include "piece.h"

namespace Ui {
class GameEngine;
}

class GameEngine : public QWidget
{
    Q_OBJECT

public:
    explicit GameEngine(QWidget *parent = nullptr);
    ~GameEngine();

private:
    Ui::GameEngine *ui;
    Piece* m_piece=nullptr;

    QPropertyAnimation *dropPiece(QWidget *piece, const QPointF &finalCenter, int duration);
    void createPiece();

    QVector<Piece*> m_pieces;
    void reset();
    void verifyWinner();

    Piece::Player m_currentPlayer=Piece::p1;
};

#endif // GAMEENGINE_H
