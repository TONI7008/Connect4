#ifndef BWIDGET_H
#define BWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QPointer>
#include <QScreen>
#include <QFutureWatcher>
#include <QPainterPath>

class QMouseEvent;

// bWidget: a translucent, frosted-glass-style widget.
//
// Background paint pipeline (all ops on the correct thread):
//
//   Main thread                         Background thread
//   ──────────────────────────────      ────────────────────────────────────
//   grabBehind()                  →     [Qt widget ops — must stay here]
//   QImage source = grab.toImage()
//   QtConcurrent::run(...)        ──►   downscale → Gaussian blur → upscale
//                                       (pure pixel math, no Qt UI calls)
//   m_blurWatcher.finished()      ◄──   returns QImage
//   QPixmap::fromImage(result)          [QPixmap creation — must be here]
//   update()                            [triggers paintEvent — must be here]
//
// Child widgets (labels, icons, …) are painted by Qt on top of this
// backdrop and are completely unaffected by the blur — they stay crisp.

class bWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int blurRadius READ blurRadius WRITE setBlurRadius)
    Q_PROPERTY(QColor tintColor READ tintColor WRITE setTintColor)
    Q_PROPERTY(qreal translucency READ translucency WRITE setTranslucency)
    Q_PROPERTY(int cornerRadius READ cornerRadius WRITE setCornerRadius)
    Q_PROPERTY(Theme       theme       READ theme       WRITE setTheme)
    Q_PROPERTY(bool        hslEnabled  READ hslEnabled  WRITE setHslEnabled)
    Q_PROPERTY(BlurMode    blurMode    READ blurMode    WRITE setBlurMode)
    Q_PROPERTY(CornerStyle cornerStyle READ cornerStyle WRITE setCornerStyle)
    Q_PROPERTY(QColor      borderColor READ borderColor WRITE setBorderColor)

public:
    // Matches the two UIBlurEffectStyle materials from iOS:
    //   Light → bright milk-glass  (luminance boost + white tint overlay)
    //   Dark  → dark smoked-glass  (saturation shift + dark composite passes)
    enum class Theme { Light, Dark };
    Q_ENUM(Theme)

    // Selects the kernel used to blur the captured backdrop.
    //   Gaussian — true separable Gaussian (Apple-accurate, smooth falloff)
    //   Stack    — two box-blur passes = tent kernel (fast, visually similar to Gaussian)
    //   Box      — single uniform kernel pass (fastest, blockier at low radii)
    //   Lens     — circular disk kernel (flat aperture, hard-edged bokeh)
    enum class BlurMode { Gaussian, Stack, Box, Lens };
    Q_ENUM(BlurMode)

    // Controls which corners are rounded (radius set by setCornerRadius / setBorderRadius).
    // Mirrors TWidget::CornerStyle so the two widget families share the same API.
    enum class CornerStyle {
        Default,     // All four corners rounded
        TopOnly,     // Top-left + top-right rounded, bottom sharp
        BottomOnly,  // Bottom-left + bottom-right rounded, top sharp
        LeftOnly,    // Top-left + bottom-left rounded, right sharp
        RightOnly,   // Top-right + bottom-right rounded, left sharp
        BottomLeft,  // Bottom-left corner only
        BottomRight, // Bottom-right corner only
        None         // All corners sharp (plain rectangle)
    };
    Q_ENUM(CornerStyle)

    explicit bWidget(QWidget *parent = nullptr);

    int blurRadius() const { return m_blurRadius; }
    void setBlurRadius(int radius);

    // The hue/strength of the frost overlay.  Its alpha is the *maximum*
    // opacity the tint can reach (scaled further by setTranslucency).
    // setTheme() resets this to the canonical theme colour; call setTintColor()
    // afterward if you want to override it.
    QColor tintColor() const { return m_tintColor; }
    void setTintColor(const QColor &color);

    // How much of the blurred backdrop shows through the frost layer.
    //   0.0 → fully frosted / opaque   (tint at configured alpha)
    //   1.0 → fully see-through        (no tint, just the blur)
    qreal translucency() const { return m_translucency; }
    void setTranslucency(qreal amount);

    int cornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(int radius);

    // Applies a preset material style — sets the HSL pre-processing mode and
    // resets the tint colour to the canonical value for that style.
    Theme theme() const { return m_theme; }
    void setTheme(Theme theme);

    // Toggle the HSL luminance/saturation pre-processing that runs before the
    // blur.  Disable to compare raw-blur vs frosted-glass appearance.
    bool hslEnabled() const { return m_hslEnabled; }
    void setHslEnabled(bool enabled);

    BlurMode blurMode() const { return m_blurMode; }
    void setBlurMode(BlurMode mode);

    // Corner shape — which corners are rounded (see enum above).
    CornerStyle cornerStyle() const { return m_cornerStyle; }
    void setCornerStyle(CornerStyle style);

    // CSS-style alias for setCornerRadius — both set the same value.
    void setBorderRadius(int radius) { setCornerRadius(radius); }

    // Enable / disable the border stroke (default: enabled).
    bool borderEnabled() const { return m_enableBorder; }
    void setBorder(bool enabled);

    // Colour of the border stroke.  Default: rgba(255,255,255,50).
    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor &color);

signals:
    void clicked(const QPointF &pos);
    void rightClicked(const QPoint &pos);
    void doubleClicked(const QPoint &pos);
    void resizing();

public slots:
    // Re-grabs and re-blurs the region behind the widget.
    // Called automatically on move / resize / show; also safe to call
    // manually when you know the content behind the widget has changed.
    void refreshBackground();

protected:
    void paintEvent(QPaintEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QPixmap      grabBehind() const;    // must run on the main thread
    void         scheduleRefresh();     // debounces via m_refreshTimer
    QPainterPath buildClipPath() const; // constructs path from current style + radius

    // ── appearance ───────────────────────────────────────────────────────────
    int         m_blurRadius   = 32;
    int         m_cornerRadius = 24;
    QColor      m_tintColor    = QColor(255, 255, 255, 102);  // Light theme default
    qreal       m_translucency = 0.35;
    Theme       m_theme        = Theme::Light;
    bool        m_hslEnabled   = true;
    BlurMode    m_blurMode     = BlurMode::Gaussian;
    CornerStyle m_cornerStyle  = CornerStyle::Default;
    QColor      m_borderColor  = QColor(255, 255, 255, 50);
    bool        m_enableBorder = true;

    // ── background render state ───────────────────────────────────────────────
    QPixmap m_background;                    // latest blurred result for paintEvent
    QFutureWatcher<QImage> m_blurWatcher;    // tracks the off-thread blur task
    bool    m_blurPending = false;           // refresh was requested while busy
    qreal   m_pendingDpr  = 1.0;            // device-pixel ratio of the in-flight grab

    // ── debounce timer ────────────────────────────────────────────────────────
    QPointer<class QTimer> m_refreshTimer;
};

#endif // BWIDGET_H
