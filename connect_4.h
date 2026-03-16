#ifndef CONNECT_4_H
#define CONNECT_4_H

#include "twidget.h"
#include "piece.h"
#include <QVector>
#include <QPair>

class Connect_4 : public TWidget
{
    Q_OBJECT

public:
    Connect_4(QWidget *parent = nullptr);
    ~Connect_4();

    double holeDiameter() const { return m_currentDiameter; }
    struct Slot
    {
        QPointF center =QPointF(0.0,0.0);
        Piece::Player player=Piece::px;

    };

    void setCurrentPlayer(Piece::Player player) { m_currentPlayer = player; }


signals:
    void mouseMoved(QPoint);
    void diameterChanged(short);
    void Clicked(QPointF pos,bool fromAI=false);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;


private:    

    const static short Rows = 7;
    const static short Cols = 6;
    const  short Margin = 8;
    const  double minHoleDiameter = 40.0;

    double m_currentDiameter = minHoleDiameter;

    Slot board[Rows][Cols];
    bool isValidMove(int row, int col);

    bool checkWin(int row, int col, Piece::Player player);
    int countInDirection(int row, int col, Piece::Player player, const QPair<int, int>& dir);

    Piece::Player m_currentPlayer = Piece::p1;

    void printBoardState() const;

    friend class AI;

};
#endif // CONNECT_4_H
