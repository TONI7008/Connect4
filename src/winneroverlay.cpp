#include "winneroverlay.h"
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

WinnerOverlay::WinnerOverlay(QWidget *parent)
    : QWidget(parent)
{
    // Fill the whole parent, sit on top
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Widget);

    buildUi();

    // Background fade effect applied to this widget
    m_bgFade = new QGraphicsOpacityEffect(this);
    m_bgFade->setOpacity(0.0);
    setGraphicsEffect(m_bgFade);

    hide();
}

WinnerOverlay::~WinnerOverlay()
{
    if (m_bgAnim)   { m_bgAnim->stop(); delete m_bgAnim; }
    if (m_cardAnim) { m_cardAnim->stop(); delete m_cardAnim; }
    if (m_group)    { m_group->stop();   delete m_group; }
}

// ── UI layout ──────────────────────────────────────────────────────────────────

void WinnerOverlay::buildUi()
{
    // ── Card ──────────────────────────────────────────────────────────────────
    m_card = new QWidget(this);
    m_card->setFixedSize(320, 240);
    m_card->setAttribute(Qt::WA_TranslucentBackground);
    m_card->setStyleSheet(R"(
        QWidget {
            background: rgba(15, 15, 30, 200);
            border-radius: 24px;
            border: 2px solid rgba(255,255,255,40);
        }
    )");

    // ── Emoji (large) ─────────────────────────────────────────────────────────
    m_emoji = new QLabel(m_card);
    m_emoji->setAlignment(Qt::AlignCenter);
    m_emoji->setStyleSheet("QLabel { background:transparent; border:none; font-size:48px; }");

    // ── Title ─────────────────────────────────────────────────────────────────
    m_title = new QLabel(m_card);
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setStyleSheet(
        "QLabel { background:transparent; border:none; "
        "color:white; font-size:22px; font-weight:bold; }");
    m_title->setWordWrap(true);

    // ── Subtitle ──────────────────────────────────────────────────────────────
    m_sub = new QLabel(m_card);
    m_sub->setAlignment(Qt::AlignCenter);
    m_sub->setStyleSheet(
        "QLabel { background:transparent; border:none; "
        "color:rgba(200,200,200,200); font-size:13px; }");

    // ── Buttons ───────────────────────────────────────────────────────────────
    const QString btnBase = R"(
        QPushButton {
            border-radius: 12px;
            padding: 8px 20px;
            font-weight: bold;
            font-size: 13px;
            border: none;
        }
    )";

    m_again = new QPushButton("▶  Play Again", m_card);
    m_again->setStyleSheet(btnBase +
        "QPushButton { background: rgb(37,99,235); color: white; }");
    m_again->setCursor(Qt::PointingHandCursor);

    m_quitBtn = new QPushButton("✕  Quit", m_card);
    m_quitBtn->setStyleSheet(btnBase +
        "QPushButton { background: rgba(180,40,40,200); color: white; }");
    m_quitBtn->setCursor(Qt::PointingHandCursor);

    // ── Card layout ───────────────────────────────────────────────────────────
    auto *vl = new QVBoxLayout(m_card);
    vl->setContentsMargins(20, 20, 20, 20);
    vl->setSpacing(8);
    vl->addWidget(m_emoji);
    vl->addWidget(m_title);
    vl->addWidget(m_sub);
    vl->addSpacing(10);

    auto *hl = new QHBoxLayout;
    hl->setSpacing(12);
    hl->addWidget(m_again);
    hl->addWidget(m_quitBtn);
    vl->addLayout(hl);

    // ── Signals ───────────────────────────────────────────────────────────────
    connect(m_again,   &QPushButton::clicked, this, [this]() {
        hideResult();
        emit playAgain();
    });
    connect(m_quitBtn, &QPushButton::clicked, this, [this]() {
        hideResult();
        emit quit();
    });
}

// ── Public API ────────────────────────────────────────────────────────────────

void WinnerOverlay::showResult(Piece::Player winner)
{
    if (m_showing) return;
    m_showing = true;

    // Fill parent geometry
    if (parentWidget())
        setGeometry(parentWidget()->rect());

    // Configure content
    if (winner == Piece::px) {
        // Draw
        m_emoji->setText("🤝");
        m_title->setText("It's a Draw!");
        m_title->setStyleSheet(
            "QLabel { background:transparent; border:none; "
            "color: rgb(200,200,200); font-size:22px; font-weight:bold; }");
        m_sub->setText("Well played by both sides.");
        m_card->setStyleSheet(m_card->styleSheet().replace(
            "rgba(15, 15, 30, 200)", "rgba(30,30,30,210)"));
    } else if (winner == Piece::p1) {
        m_emoji->setText("🔴");
        m_title->setText("Red Wins!");
        m_title->setStyleSheet(
            "QLabel { background:transparent; border:none; "
            "color: rgb(250,80,100); font-size:22px; font-weight:bold; }");
        m_sub->setText("Player 1 connects four!");
        m_card->setStyleSheet(QString(R"(
            QWidget {
                background: rgba(80, 10, 20, 210);
                border-radius: 24px;
                border: 2px solid rgba(250,29,71,120);
            }
        )"));
    } else {
        m_emoji->setText("🟡");
        m_title->setText("Yellow Wins!");
        m_title->setStyleSheet(
            "QLabel { background:transparent; border:none; "
            "color: rgb(253,220,30); font-size:22px; font-weight:bold; }");
        m_sub->setText("Player 2 connects four!");
        m_card->setStyleSheet(QString(R"(
            QWidget {
                background: rgba(60, 55, 0, 220);
                border-radius: 24px;
                border: 2px solid rgba(253,254,68,120);
            }
        )"));
    }

    center();
    show();
    raise();
    animateIn();
}

void WinnerOverlay::hideResult()
{
    if (!m_showing) return;
    animateOut();
}

// ── Painting ──────────────────────────────────────────────────────────────────

void WinnerOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // Semi-transparent dark scrim — lets the board show through so winning
    // pieces remain visible.
    p.fillRect(rect(), QColor(0, 0, 0, qRound(160 * m_opacity)));
}

void WinnerOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    center();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void WinnerOverlay::center()
{
    if (!m_card) return;
    m_card->move((width()  - m_card->width())  / 2,
                 (height() - m_card->height()) / 2);
}

qreal WinnerOverlay::opacity() const  { return m_opacity; }

void WinnerOverlay::setOpacity(qreal v)
{
    m_opacity = qBound(0.0, v, 1.0);
    if (m_bgFade) m_bgFade->setOpacity(m_opacity);
    update(); // repaint scrim
}

void WinnerOverlay::animateIn()
{
    // Delete old group if any
    if (m_group) { m_group->stop(); delete m_group; }

    m_group = new QParallelAnimationGroup(this);

    // Fade in background scrim
    auto *fadeIn = new QPropertyAnimation(this, "opacity", m_group);
    fadeIn->setDuration(300);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutQuad);

    // Slide card up from slightly below center
    auto *slide = new QPropertyAnimation(m_card, "pos", m_group);
    slide->setDuration(350);
    QPoint target((width()  - m_card->width())  / 2,
                  (height() - m_card->height()) / 2);
    slide->setStartValue(target + QPoint(0, 40));
    slide->setEndValue(target);
    slide->setEasingCurve(QEasingCurve::OutBack);

    m_group->addAnimation(fadeIn);
    m_group->addAnimation(slide);
    m_group->start();
}

void WinnerOverlay::animateOut()
{
    if (m_group) { m_group->stop(); delete m_group; }

    m_group = new QParallelAnimationGroup(this);

    auto *fadeOut = new QPropertyAnimation(this, "opacity", m_group);
    fadeOut->setDuration(200);
    fadeOut->setStartValue(m_opacity);
    fadeOut->setEndValue(0.0);

    auto *slide = new QPropertyAnimation(m_card, "pos", m_group);
    slide->setDuration(200);
    slide->setStartValue(m_card->pos());
    slide->setEndValue(m_card->pos() + QPoint(0, 30));

    m_group->addAnimation(fadeOut);
    m_group->addAnimation(slide);

    connect(m_group, &QParallelAnimationGroup::finished, this, [this]() {
        m_showing = false;
        hide();
    });

    m_group->start();
}
