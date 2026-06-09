#ifndef PIECE_H
#define PIECE_H

#include "circularwidget.h"
#include <QGraphicsDropShadowEffect>


class Piece : public CircularWidget
{
    Q_OBJECT
public:
    Piece(QWidget* parent=nullptr);
    ~Piece();
    enum Player {p1,p2,px};

    void setPlayer(Player);
    void setHighlighted(bool highlighted);
    Player player();


private:
    Player m_player;
    bool m_highlighted=false;

    QGraphicsDropShadowEffect* m_shadowEffect=nullptr;
    void updateColor();
};

#endif // PIECE_H
