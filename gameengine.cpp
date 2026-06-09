#include "gameengine.h"
#include "ui_gameengine.h"
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

#define CURRENT_VERSION "26.5"  // Used for update checks — update as needed for testing

GameEngine::GameEngine(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GameEngine)
{
    ui->setupUi(this);

    // ── Frame styling ────────────────────────────────────────────────────────
    ui->c4Widget->setBorderRadius(15);
    ui->bottomWidget->setBorderRadius(25);
    ui->bottomWidget->setCornerStyle(bWidget::CornerStyle::Default);
    ui->bottomWidget->setTheme(bWidget::Theme::Light);
    ui->bottomWidget->setBlurMode(bWidget::BlurMode::Gaussian);
    ui->bottomWidget->setBlurRadius(150);
    ui->bottomWidget->setTranslucency(0.7);
    ui->topWidget->setTheme(bWidget::Theme::Light);
    ui->topWidget->setBlurMode(bWidget::BlurMode::Gaussian);
    ui->topWidget->setBlurRadius(150);
    ui->topWidget->setTranslucency(0.7);

    ui->topWidget->setBorderRadius(25);
    ui->topWidget->setCornerStyle(bWidget::CornerStyle::Default);
    ui->aiButton->setCheckable(true);

    // ── Player indicator circles ─────────────────────────────────────────────
    ui->circular1->setColor(QColor(250, 29, 71));
    ui->circular2->setColor(QColor(253, 254, 68));
    ui->circular2_4->setColor(QColor(253, 254, 68));
    ui->circular2_6->setColor(QColor(253, 254, 68));
    ui->circular2_3->setColor(QColor(250, 29, 71));
    ui->circular2_5->setColor(QColor(250, 29, 71));

    // ── Stacked widget ───────────────────────────────────────────────────────
    ui->stackedWidget->setCurrentWidget(ui->home);
    ui->stackedWidget->setAnimationDuration(500);
    ui->stackedWidget->setAnimationType(TStackedWidget::VerticalSlide);

    // ── Logo floater ─────────────────────────────────────────────────────────
    m_floater = new WidgetFloater(ui->holderFrame, this);
    m_floater->setFloatAmount(25);
    m_floater->setDuration(3100);
    m_floater->setAutoReposition(true);
    m_floater->setRepositionDelay(50);

    ui->logoFrame->setBackgroundColor(QColor(255, 255, 255, 200));
    ui->logoFrame->setBorderRadius(20);
    ui->holderFrame->setEnableBackground(false);
    ui->holderFrame->setBorder(false);

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(ui->logoFrame);
    shadow->setBlurRadius(15);
    shadow->setOffset(5, 5);
    shadow->setColor(QColor(250, 250, 250, 150));
    ui->logoFrame->setGraphicsEffect(shadow);

    // ── Winner overlay (parented to game page — stays visible over the board) ─
    m_overlay = new WinnerOverlay(ui->game);
    m_overlay->hide();
    connect(m_overlay, &WinnerOverlay::playAgain, this, &GameEngine::reset);
    connect(m_overlay, &WinnerOverlay::quit,      this, &GameEngine::close);

    // ── App updater ──────────────────────────────────────────────────────────
    // ⚠ Replace "YOUR_GITHUB_USER" / "connect4" with your real repo.
    m_updater = new AppUpdater("TONI7008", "connect4", CURRENT_VERSION, this);
    connect(m_updater, &AppUpdater::updateAvailable,  this, &GameEngine::onUpdateAvailable);
    connect(m_updater, &AppUpdater::downloadProgress, this, &GameEngine::onDownloadProgress);
    connect(m_updater, &AppUpdater::downloadFinished, this, &GameEngine::onDownloadFinished);
    QTimer::singleShot(2000, m_updater, &AppUpdater::checkForUpdates);

    // ── Initial piece ────────────────────────────────────────────────────────
    createPiece();

    // ── Connections ──────────────────────────────────────────────────────────
    connect(ui->startgameButton, &QPushButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentWidget(ui->game);
        m_floater->stopFloating();
    });
    QTimer::singleShot(600, this, [this]() {
        m_floater->captureLayoutPosition();
        if (!m_floater->isFloating()) m_floater->startFloating();
    });

    connect(ui->c4Widget, &Connect_4::Clicked, this, &GameEngine::onBoardClicked);

    connect(ui->c4Widget, &Connect_4::mouseMoved, this, [this](QPoint posn) {
        if (!m_piece || m_gameOver) return;
        m_piece->move(posn.x() - m_piece->width() / 2, m_piece->y());
    });

    connect(ui->c4Widget, &Connect_4::diameterChanged, this, [this](short d) {
        if (m_piece) {
            m_piece->resize(d, d);
            m_piece->move(m_piece->x(),
                          ui->c4Widget->y() - (int)ui->c4Widget->holeDiameter() - 1);
        }
    });

    connect(ui->quitButton,  &QPushButton::clicked, this, &GameEngine::close);
    connect(ui->resetbutton, &QPushButton::clicked, this, &GameEngine::reset);
    connect(ui->aiButton,    &QPushButton::clicked, this, &GameEngine::enableAi);

    connect(ui->difficultySlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_ai) m_ai->setDifficulty(value);
    });

    updateGeometry();
}

GameEngine::~GameEngine()
{
    delete ui;
    delete m_ai;
    delete m_floater;
    cleanupPieces();
}

// ── Resize ────────────────────────────────────────────────────────────────────

void GameEngine::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    short w = event->size().width();
    short h = event->size().height();
    ui->c4Widget->setMinimumSize(w * 500 / 700, h * 430 / 596);

    // Keep the overlay covering the full game page
    if (m_overlay && m_overlay->parentWidget())
        m_overlay->setGeometry(m_overlay->parentWidget()->rect());
}

// ── Core turn logic ───────────────────────────────────────────────────────────

void GameEngine::onBoardClicked(QPointF pos_, bool fromAI)
{
    if (m_gameOver)                                          return;
    if (fromAI && !m_aiMode)                                 return;
    if (!fromAI && m_aiMode && m_currentPlayer == Piece::p2) return;

    // Determine which board cell was clicked so we can use checkWinAt later.
    // The connect_4 widget already validated and marked the board before emitting.
    // Reverse-map the position back to (row, col):
    int col = static_cast<int>(pos_.x()) / (ui->c4Widget->width()  / Connect_4::Cols);
    int row = static_cast<int>(pos_.y()) / (ui->c4Widget->height() / Connect_4::Rows);
    // Clamp
    col = qBound(0, col, Connect_4::Cols - 1);
    row = qBound(0, row, Connect_4::Rows - 1);

    // Remember which player just played (before we switch)
    Piece::Player justPlayed = m_currentPlayer;
    int lastRow = row, lastCol = col;

    // Switch turn
    m_currentPlayer = (m_currentPlayer == Piece::p1) ? Piece::p2 : Piece::p1;
    ui->c4Widget->setCurrentPlayer(m_currentPlayer);
    setTurnLabel(m_currentPlayer);

    dropPiece(m_piece, pos_, 500);
    createPiece();

    // After drop animation, check exactly the placed cell — no ambiguity.
    QTimer::singleShot(560, this, [this, lastRow, lastCol, justPlayed]() {
        verifyWinnerAt(lastRow, lastCol, justPlayed);
    });

    // Trigger AI if it is now AI's turn
    if (m_aiMode && m_currentPlayer == Piece::p2 && !m_gameOver) {
        QTimer::singleShot(120, this, [this]() {
            if (m_ai && !m_gameOver && !m_ai->isBusy())
                m_ai->makeMove();
        });
    }
}

// ── Winner check (anchored at last-placed cell) ───────────────────────────────

void GameEngine::verifyWinnerAt(int row, int col, Piece::Player justPlayed)
{
    if (m_gameOver) return;

    Piece::Player winner = ui->c4Widget->checkWinAt(row, col);

    if (winner != Piece::px) {
        m_gameOver = true;
        if (winner == Piece::p1)
            ui->player1Score->setText(QString::number(ui->player1Score->text().toInt() + 1));
        else
            ui->player2Score->setText(QString::number(ui->player2Score->text().toInt() + 1));

        QTimer::singleShot(500, this, [this, winner]() { showResult(winner); });
        return;
    }

    if (ui->c4Widget->isBoardFull()) {
        m_gameOver = true;
        QTimer::singleShot(500, this, [this]() { showResult(Piece::px); });
    }
}

void GameEngine::showResult(Piece::Player winner)
{
    if (m_overlay) {
        m_overlay->setGeometry(ui->game->rect());
        m_overlay->raise();
        m_overlay->showResult(winner);
    }
}

// ── Reset ─────────────────────────────────────────────────────────────────────

void GameEngine::reset()
{
    m_gameOver      = false;
    m_currentPlayer = Piece::p1;

    if (m_overlay) m_overlay->hide();

    // Remove all dropped pieces, keep the hover piece
    for (Piece* p : m_pieces) {
        if (p && p != m_piece) {
            p->hide();
            p->deleteLater();
        }
    }
    m_pieces.clear();

    if (m_piece) {
        m_piece->setPlayer(Piece::p1);
        m_piece->move(ui->c4Widget->x() + (int)ui->c4Widget->holeDiameter(),
                      ui->c4Widget->y() - (int)ui->c4Widget->holeDiameter() - 1);
        m_piece->show();
        m_pieces.append(m_piece);
    }

    // Recreate AI to clear its state
    if (m_ai) {
        delete m_ai;
        m_ai = new AI(ui->c4Widget);
        m_ai->setDifficulty(ui->difficultySlider->value());
    }

    setTurnLabel(Piece::p1);
    ui->c4Widget->reset();
    update();
}

// ── AI enable ────────────────────────────────────────────────────────────────

void GameEngine::enableAi(bool enable)
{
    m_aiMode = enable;

    if (enable) {
        if (!m_ai) {
            m_ai = new AI(ui->c4Widget);
            m_ai->setDifficulty(ui->difficultySlider->value());
        }
        ui->aiLabel->setText("AI");

        // Mid-game activation: if it is already p2's turn, kick the AI now.
        if (!m_gameOver && m_currentPlayer == Piece::p2) {
            QTimer::singleShot(200, this, [this]() {
                if (m_ai && m_aiMode && !m_gameOver
                        && m_currentPlayer == Piece::p2 && !m_ai->isBusy())
                    m_ai->makeMove();
            });
        }
    } else {
        ui->aiLabel->setText("PLAYER 2");
        delete m_ai;
        m_ai = nullptr;
    }
}

// ── UI helpers ────────────────────────────────────────────────────────────────

void GameEngine::setTurnLabel(Piece::Player player)
{
    if (player == Piece::p1) {
        ui->cplayerLabel->setStyleSheet(
            "QLabel{ background:rgb(250,29,71); border-radius:8px;"
            " font-weight:bold; color:white; }");
        ui->cplayerLabel->setText("RED'S TURN");
    } else {
        ui->cplayerLabel->setStyleSheet(
            "QLabel{ background:rgb(253,254,68); border-radius:8px;"
            " font-weight:bold; color:black; }");
        ui->cplayerLabel->setText(m_aiMode ? "AI'S TURN" : "YELLOW'S TURN");
    }
}

void GameEngine::cleanupPieces()
{
    for (Piece* p : m_pieces)
        if (p) { p->hide(); delete p; }
    m_pieces.clear();
    m_piece = nullptr;
}

// ── Piece management ──────────────────────────────────────────────────────────

void GameEngine::createPiece()
{
    m_piece = new Piece(ui->c4Widget);
    m_piece->raise();
    m_piece->setPlayer(m_currentPlayer);
    m_piece->resize((int)ui->c4Widget->holeDiameter(),
                    (int)ui->c4Widget->holeDiameter() + 4);
    m_piece->move(ui->c4Widget->x() + (int)ui->c4Widget->holeDiameter(),
                  ui->c4Widget->y() - (int)ui->c4Widget->holeDiameter() - 1);
    m_piece->hide();
    m_pieces.append(m_piece);
}

QPropertyAnimation* GameEngine::dropPiece(Piece* piece,
                                           const QPointF& finalCenter,
                                           int duration)
{
    if (!piece) return nullptr;

    piece->show();
    QRect startRect = piece->geometry();
    QRect endRect(finalCenter.toPoint(), piece->size());

    auto *anim = new QPropertyAnimation(piece, "geometry");
    anim->setDuration(duration);
    anim->setStartValue(startRect);
    anim->setEndValue(endRect);

    QEasingCurve curve(QEasingCurve::OutBounce);
    curve.setAmplitude(1.5);
    curve.setPeriod(0.35);
    anim->setEasingCurve(curve);

    this->raise();
    piece->lower();

    connect(anim, &QPropertyAnimation::finished, this, [this, piece, finalCenter]() {
        piece->raise();
        ui->c4Widget->linkPieceToSlot(piece, finalCenter);
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

// ── Updater UI slots ──────────────────────────────────────────────────────────

void GameEngine::onUpdateAvailable(const QString &ver, const QUrl &/*url*/)
{
    // Wire these to whatever widgets you add to the .ui for the updater page.
    // Example assumes you have: updaterLabel (QLabel), updateProgressBar (QProgressBar),
    // downloadUpdateButton (QPushButton).
    if (ui->updaterLabel) {
        ui->updaterLabel->setText(QString("v%1 available!").arg(ver));
        ui->updaterLabel->setVisible(true);
    }
    if (ui->updateProgressBar)
        ui->updateProgressBar->setVisible(false);
    if (ui->downloadUpdateButton) {
        ui->downloadUpdateButton->setVisible(true);
        connect(ui->downloadUpdateButton, &QPushButton::clicked,
                m_updater, &AppUpdater::startDownload, Qt::UniqueConnection);
    }
}

void GameEngine::onDownloadProgress(int percent)
{
    if (ui->updateProgressBar) {
        ui->updateProgressBar->setVisible(true);
        ui->updateProgressBar->setValue(percent);
    }
    if (ui->updaterLabel)
        ui->updaterLabel->setText(QString("Downloading… %1%").arg(percent));
}

void GameEngine::onDownloadFinished(const QString &path)
{
    if (ui->updaterLabel)
        ui->updaterLabel->setText("Download complete! Launching installer…");
    if (ui->updateProgressBar)
        ui->updateProgressBar->setValue(100);

    QTimer::singleShot(800, this, [this, path]() {
        m_updater->launchInstaller(path);
    });
}
