#include "gameengine.h"
#include "ui_gameengine.h"
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QRect>


GameEngine::GameEngine(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GameEngine)
{
    ui->setupUi(this);
    ui->c4Widget->setBorderRadius(15);

    ui->bottomFrame->setBorderRadius(25);
    ui->bottomFrame->setCornerStyle(TFrame::CornerStyle::TopOnly);
    ui->topFrame->setBorderRadius(25);
    ui->topFrame->setCornerStyle(TFrame::CornerStyle::BottomOnly);
    ui->aiButton->setCheckable(true);

    ui->circular1->setColor(QColor(250,29,71));
    ui->circular2->setColor(QColor(253,254,68));

    //ui->c4Widget->bord
    createPiece();

    connect(ui->c4Widget,&Connect_4::mouseMoved,this,[this](QPoint posn){
        if(!m_piece) return;
        QPoint pos=ui->c4Widget->mapTo(this,posn);
        short w=m_piece->width();

        short x=pos.x()-w/2;
        m_piece->move(x,m_piece->y());
    });

    connect(ui->c4Widget,&Connect_4::Clicked,this,[this](QPointF pos_,bool fromAI){
        if(!m_piece) return;
        if(fromAI && !ai_mode) return; // Ignore AI moves if AI mode is not enabled
        if(m_currentPlayer==Piece::p1 && fromAI) return; // Ignore player moves if AI mode is enabled

        QString style;
        switch (m_currentPlayer) {
        case Piece::p1:
            m_currentPlayer=Piece::p2;
            style = ui->cplayerLabel->styleSheet();
            style.replace("rgb(250,29,71)","rgb(253,254,68)");
            style.replace("white","black");
            ui->cplayerLabel->setStyleSheet(style);
            ui->cplayerLabel->setText("YELLOW'S TURN");
            break;
        case Piece::p2:
            m_currentPlayer=Piece::p1;
            style = ui->cplayerLabel->styleSheet();
            style.replace("rgb(253,254,68)","rgb(250,29,71)");
            style.replace("black","white");
            ui->cplayerLabel->setStyleSheet(style);
            ui->cplayerLabel->setText("RED'S TURN");
            break;
        default:
            break;
        }
        
        ui->c4Widget->setCurrentPlayer(m_currentPlayer);
        QPointF pos=ui->c4Widget->mapTo(this,pos_);
        dropPiece(m_piece,pos,500);
        createPiece();

        if(ai_mode){
    
            QTimer::singleShot(600,this,[this](){
                if(m_ai){
                    m_ai->makeMove();
                }
                    
            });

        }
        
    });

    connect(ui->c4Widget,&Connect_4::diameterChanged,this,[this](short d){
        m_piece->resize(d,d);
        m_piece->move(m_piece->x(),ui->c4Widget->y()-ui->c4Widget->holeDiameter()-1);
    });

    connect(ui->quitButton,&QPushButton::clicked,this,&GameEngine::close);
    connect(ui->resetbutton,&QPushButton::clicked,this,&GameEngine::reset);
    connect(ui->aiButton,&QPushButton::clicked,this,&GameEngine::enableAi);
    connect(ui->difficultySlider,&QSlider::valueChanged,this,[this](int value){
        if(m_ai){
            m_ai->setDifficulty(value);
        }
    });


}

void GameEngine::reset(){

    for (auto* element : std::as_const(m_pieces)){
        if(element==m_piece) continue;


        element->hide();
        element->deleteLater();
        m_pieces.removeOne(element);
    }
    update();
}

void GameEngine::verifyWinner()
{

}

GameEngine::~GameEngine()
{
    delete ui;
    delete m_ai;
    if(m_piece){
        delete m_piece;
    }
}

QPropertyAnimation* GameEngine::dropPiece(QWidget* piece,
                              const QPointF& finalCenter,
                              int duration)
{
    if (!piece)
        return nullptr;

    QSize s = piece->size();

    QRect startRect = piece->geometry();

    QRect endRect(
        finalCenter.x()+1 ,
        finalCenter.y(),
        s.width(),
        s.height()
    );

    QPropertyAnimation* anim =
        new QPropertyAnimation(piece, "geometry");

    anim->setDuration(duration);
    anim->setStartValue(startRect);
    anim->setEndValue(endRect);

    QEasingCurve curve(QEasingCurve::OutBounce);
    curve.setAmplitude(1.0);
    curve.setPeriod(0.35);

    anim->setEasingCurve(curve);

    this->raise();
    piece->lower();

    anim->start(QAbstractAnimation::DeleteWhenStopped);

    return anim;
}

void GameEngine::createPiece()
{
    m_piece=nullptr;
    m_piece = new Piece(this);
    m_piece->raise();
    m_piece->setPlayer(Piece::p1);
    short margin=5;
    m_piece->resize(ui->c4Widget->holeDiameter(),ui->c4Widget->holeDiameter()+4);
    m_piece->move(ui->c4Widget->x()+ui->c4Widget->holeDiameter(),ui->c4Widget->y()-ui->c4Widget->holeDiameter()-1);

    m_piece->show();

    m_piece->setPlayer(m_currentPlayer);

    m_pieces.append(m_piece);

}
void GameEngine::enableAi(bool enable)
{
    ai_mode = enable;
    if (enable) {
        if (!m_ai) {
            m_ai = new AI(ui->c4Widget);
            m_ai->setDifficulty(ui->difficultySlider->value());
        }
        ui->aiLabel->setText("AI");
       
    } else {
        ui->aiLabel->setText("PLAYER 2");
        delete m_ai;
        m_ai = nullptr;
    }
}