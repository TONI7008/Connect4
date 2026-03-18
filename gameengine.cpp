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

    ui->circular2_4->setColor(QColor(253,254,68));
    ui->circular2_6->setColor(QColor(253,254,68));

    ui->circular2_3->setColor(QColor(250,29,71));
    ui->circular2_5->setColor(QColor(250,29,71));

    ui->stackedWidget->setCurrentWidget(ui->home);
    ui->stackedWidget->setAnimationDuration(500);
    ui->stackedWidget->setAnimationType(TStackedWidget::VerticalSlide);

    ui->winnerFrame->setBorderRadius(50);
    ui->winnerFrame->setEnableBackground(true);
    ui->winnerFrame->setBorder(true);
    ui->winnerFrame->setBorderSize(5);

    m_floater = new WidgetFloater(ui->holderFrame, this);
    m_floater->setFloatAmount(25);
    m_floater->setDuration(3100);
    m_floater->setAutoReposition(true);
    m_floater->setRepositionDelay(20);
    

    ui->logoFrame->setBackgroundColor(QColor(255,255,255,200));
    ui->logoFrame->setBorderRadius(20);

    ui->holderFrame->setEnableBackground(false);
    ui->holderFrame->setBorder(false);

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(ui->logoFrame);
    shadow->setBlurRadius(15);
    shadow->setOffset(5, 5);
    shadow->setColor(QColor(250,250,250,150));
    ui->logoFrame->setGraphicsEffect(shadow);
    
   
    createPiece();

    connect(ui->startgameButton,&QPushButton::clicked,this,[this](){
        ui->stackedWidget->setCurrentWidget(ui->game);
    });

    connect(ui->c4Widget,&Connect_4::mouseMoved,this,[this](QPoint posn){
        if(!m_piece) return;
        //QPoint pos=ui->c4Widget->mapTo(this,posn);
        QPoint pos = posn;
        short w=m_piece->width();

        short x=pos.x()-w/2;
        m_piece->move(x,m_piece->y());
    });

    connect(ui->c4Widget,&Connect_4::Clicked,this,[this](QPointF pos_,bool fromAI){
        if(!m_piece) return;
        if(fromAI && !ai_mode) return; // Ignore AI moves if AI mode is not enabled
        // Prevent player from making a move while AI is thinking
        if(!fromAI && ai_mode && m_currentPlayer==Piece::p2) return;

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
        dropPiece(m_piece,pos_,500);
        createPiece();

        if(ai_mode && !fromAI){
            QTimer::singleShot(100,this,[this](){
                if(m_ai){
                    m_ai->makeMove();
                }
            });
        }

        verifyWinner();
        
    });

    connect(ui->c4Widget,&Connect_4::diameterChanged,this,[this](short d){
        m_piece->resize(d,d);
        m_piece->move(m_piece->x(),ui->c4Widget->y()-ui->c4Widget->holeDiameter()-1);
    });

    connect(ui->restartButton,&QPushButton::clicked,this,[this](){
        ui->stackedWidget->setCurrentWidget(ui->game);
        reset();
    });
    connect(ui->quitButton,&QPushButton::clicked,this,&GameEngine::close);
    connect(ui->resetbutton,&QPushButton::clicked,this,&GameEngine::reset);
    connect(ui->aiButton,&QPushButton::clicked,this,&GameEngine::enableAi);
    connect(ui->difficultySlider,&QSlider::valueChanged,this,[this](int value){
        if(m_ai){
            m_ai->setDifficulty(value);
        }
    });

    updateGeometry();
    m_floater->startFloating();

}


void GameEngine::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    short w = event->size().width();
    short h = event->size().height();
    
    ui->c4Widget->setMinimumSize(w*500/700,h*430/596);

}

void GameEngine::reset(){

    for (auto* element : std::as_const(m_pieces)){
        if(element==m_piece) continue;

        element->hide();
        element->deleteLater();
        m_pieces.removeOne(element);
    }
    m_currentPlayer=Piece::p1;
    if(m_piece){
        m_piece->setPlayer(m_currentPlayer);
        m_piece->move(ui->c4Widget->x()+ui->c4Widget->holeDiameter(),ui->c4Widget->y()-ui->c4Widget->holeDiameter()-1);
    }
    if(m_ai){
        delete m_ai;
        m_ai=nullptr;
    }
    ui->cplayerLabel->setText("RED'S TURN");
    QString style = ui->cplayerLabel->styleSheet();
    style.replace("rgb(253,254,68)","rgb(250,29,71)");
    style.replace("black","white");
    ui->cplayerLabel->setStyleSheet(style);
    ui->c4Widget->reset();

    update();
}

GameEngine::~GameEngine()
{
    delete ui;
    delete m_ai;
    if(m_piece){
        delete m_piece;
    }
    if (m_floater) {
        delete m_floater;
    }
}

QPropertyAnimation* GameEngine::dropPiece(Piece* piece,
                              const QPointF& finalCenter,
                              int duration)
{
    if (!piece)
        return nullptr;

    piece->show();
    QSize s = piece->size();

    QRect startRect = piece->geometry();

    QRect endRect(
        finalCenter.toPoint(),
        QSize(s.width(), s.height())
    );

    QPropertyAnimation* anim =
        new QPropertyAnimation(piece, "geometry");

    anim->setDuration(duration);
    anim->setStartValue(startRect);
    anim->setEndValue(endRect);

    QEasingCurve curve(QEasingCurve::OutBounce);
    curve.setAmplitude(1.5);
    curve.setPeriod(0.35);

    anim->setEasingCurve(curve);

    this->raise();
    piece->lower();

    connect(anim, &QPropertyAnimation::finished, this, [this,piece,finalCenter]() {
        piece->raise();
        ui->c4Widget->linkPieceToSlot(piece,finalCenter);
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);

    return anim;
}

void GameEngine::createPiece()
{
    m_piece=nullptr;
    m_piece = new Piece(ui->c4Widget);
    m_piece->raise();
    m_piece->setPlayer(Piece::p1);
    short margin=5;
    m_piece->resize(ui->c4Widget->holeDiameter(),ui->c4Widget->holeDiameter()+4);
    m_piece->move(ui->c4Widget->x()+ui->c4Widget->holeDiameter(),ui->c4Widget->y()-ui->c4Widget->holeDiameter()-1);

    m_piece->hide();

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

void GameEngine::verifyWinner()
{
    Piece::Player winner = ui->c4Widget->verifyWinner();
    short winnerCount=0;
        
    if (winner != Piece::px) {
        switch (winner)
        {
        case Piece::p1:
            ui->winnerFrame->setBorderColor(QColor(250,29,71));
            ui->winnerLabel->setText("RED'S WINS!");
            winnerCount = ui->player1Score->text().toInt();
            ui->player1Score->setText(QString::number(winnerCount+1));
            break;
        case Piece::p2:
            ui->winnerFrame->setBorderColor(QColor(253,254,68));
            ui->winnerLabel->setText("YELLOW'S WINS!");
            winnerCount = ui->player2Score->text().toInt();
            ui->player2Score->setText(QString::number(winnerCount+1));
            break;
        
        default:
            break;
        }

        QTimer::singleShot(1000, this, [this]() {
            ui->stackedWidget->setCurrentWidget(ui->winnerPage);
        });
    }
}
