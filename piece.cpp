#include "piece.h"

Piece::Piece(QWidget* parent) : CircularWidget(parent) {

    updateColor();
    m_shadowEffect = new QGraphicsDropShadowEffect(this);
    m_shadowEffect->setBlurRadius(0);
    m_shadowEffect->setOffset(0, 0);
    setGraphicsEffect(m_shadowEffect);
}

Piece::~Piece(){
    setGraphicsEffect(nullptr); // Qt automatically deletes the effect when the widget is destroyed, so we just need to disassociate it.
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
        m_color=QColor(250, 220, 40); // yellow color
    }else{
        m_color=Qt::gray; // gray color
    }
}

void Piece::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    if(m_highlighted){
        m_shadowEffect->setBlurRadius(15);
        m_shadowEffect->setOffset(5, 5);
        if(m_player==p1){
            m_shadowEffect->setColor(QColor(250,220,40,180));
        }else if(m_player==p2){ 
            m_shadowEffect->setColor(QColor(250,29,71,180));
        }
    }else{
        m_shadowEffect->setBlurRadius(0);
        m_shadowEffect->setOffset(0, 0);
        m_shadowEffect->setColor(Qt::transparent);
    }
    update();
}