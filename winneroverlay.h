#ifndef WINNEROVERLAY_H
#define WINNEROVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "piece.h"

/**
 * WinnerOverlay
 * -------------
 * A frameless, translucent widget that animates in on top of the game board
 * when a win or draw is detected. It shows the result, highlights the winning
 * pieces (already done by Connect_4), and offers Play Again / Quit buttons.
 *
 * Usage:
 *   auto* overlay = new WinnerOverlay(gamePageWidget);
 *   overlay->show(Piece::p1);          // or Piece::px for draw
 *   connect(overlay, &WinnerOverlay::playAgain, this, &GameEngine::reset);
 */
class WinnerOverlay : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit WinnerOverlay(QWidget *parent = nullptr);
    ~WinnerOverlay();

    void showResult(Piece::Player winner);   // Call to animate in
    void hideResult();                        // Animate out

    qreal opacity() const;
    void  setOpacity(qreal v);

signals:
    void playAgain();
    void quit();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    void animateIn();
    void animateOut();
    void center();

    QWidget                 *m_card    = nullptr;
    QLabel                  *m_emoji   = nullptr;
    QLabel                  *m_title   = nullptr;
    QLabel                  *m_sub     = nullptr;
    QPushButton             *m_again   = nullptr;
    QPushButton             *m_quitBtn = nullptr;

    QGraphicsOpacityEffect  *m_bgFade  = nullptr;
    QPropertyAnimation      *m_bgAnim  = nullptr;
    QPropertyAnimation      *m_cardAnim= nullptr;
    QParallelAnimationGroup *m_group   = nullptr;

    qreal  m_opacity = 0.2;
    bool   m_showing = false;
};

#endif // WINNEROVERLAY_H
