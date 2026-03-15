#include "piece.h"

Piece::Piece(QWidget* parent) : CircularWidget(parent) {

    updateColor();
}

Piece::~Piece(){

}

void Piece::setPlayer(Player pl)
{
    m_player=pl;
    updateColor();
}


Piece::Player Piece::player()
{
    return m_player;
}

void Piece::updateColor(){
    if(m_player==p1){
        m_color=QColor(250,29,71); // red color
    }else if(m_player==p2){
        m_color=QColor(253,254,68); // yellow color
    }else{
        m_color=Qt::gray; // gray color
    }
}
