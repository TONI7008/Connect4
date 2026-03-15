#ifndef PIECE_H
#define PIECE_H

#include "circularwidget.h"


class Piece : public CircularWidget
{
    Q_OBJECT
public:
    Piece(QWidget* parent=nullptr);
    ~Piece();
    enum Player {p1,p2,px};

    void setPlayer(Player);
    Player player();


private:
    Player m_player;
    void updateColor();
};

#endif // PIECE_H
