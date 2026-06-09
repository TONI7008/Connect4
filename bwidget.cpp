#include "bwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QShowEvent>
#include <QTimer>
#include <QtConcurrent>

#include <cmath>
#include <vector>

// ─── blur + HSL primitives (pure functions — safe on any thread) ──────────────

namespace {

// ── HSL helpers ───────────────────────────────────────────────────────────────

static float hue2rgb(float p, float q, float t)
{
    if (t < 0.f) t += 1.f;
    if (t > 1.f) t -= 1.f;
    if (t < 1.f/6.f) return p + (q - p) * 6.f * t;
    if (t < 0.5f)    return q;
    if (t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6.f;
    return p;
}

// Per-pixel HSL adjustment — ports lightBlur()/darkBlur() from Max Zeryck's
// Scriptable widget-blur script into C++.
// Applied BEFORE the blur so the diffusion spreads the pre-processed colours.
QImage applyHslAdjustment(QImage img, bool lightMode)
{
    if (img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);

    const int w = img.width(), h = img.height();
    for (int y = 0; y < h; ++y) {
        QRgb *row = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            const QRgb px = row[x];
            const int alpha = qAlpha(px);
            if (alpha == 0) continue;

            const float r = qRed(px)   / 255.f;
            const float g = qGreen(px) / 255.f;
            const float b = qBlue(px)  / 255.f;

            const float mn = std::min({r, g, b});
            const float mx = std::max({r, g, b});
            float hue = 0.f, sat = 0.f;
            float lum = (mx + mn) * 0.5f;

            if (mx > mn) {
                const float d = mx - mn;
                sat = (lum > 0.5f) ? d / (2.f - mx - mn) : d / (mx + mn);
                if      (mx == r) hue = (g - b) / d + (g < b ? 6.f : 0.f);
                else if (mx == g) hue = (b - r) / d + 2.f;
                else              hue = (r - g) / d + 4.f;
                hue /= 6.f;
            }

            if (lightMode) {
                float lumCalc = (lum > 0.f) ? (0.35f + 0.3f / lum) : 3.3f;
                lumCalc = qBound(1.f, lumCalc, 3.3f);
                lum = qMin(1.f, lum * lumCalc);
                sat = qMin(1.f, sat * (2.f * sat * lum) * 1.5f);
            } else {
                sat = qMin(1.f, sat * (1.f - lum) * 3.f);
            }

            float nr, ng, nb;
            if (sat < 1e-6f) {
                nr = ng = nb = lum;
            } else {
                const float q2 = (lum < 0.5f) ? lum*(1.f+sat) : lum+sat-lum*sat;
                const float p2 = 2.f * lum - q2;
                nr = hue2rgb(p2, q2, hue + 1.f/3.f);
                ng = hue2rgb(p2, q2, hue);
                nb = hue2rgb(p2, q2, hue - 1.f/3.f);
            }

            row[x] = qRgba(qBound(0, int(nr*255.f+.5f), 255),
                           qBound(0, int(ng*255.f+.5f), 255),
                           qBound(0, int(nb*255.f+.5f), 255),
                           alpha);
        }
    }
    return img;
}

// ── Gaussian blur ─────────────────────────────────────────────────────────────

std::vector<qreal> gaussianKernel(qreal sigma)
{
    const int r = qMax(1, int(std::ceil(sigma * 3.0)));
    std::vector<qreal> k(2*r+1);
    qreal sum = 0;
    for (int i = -r; i <= r; ++i) { k[i+r] = std::exp(-(i*i)/(2*sigma*sigma)); sum += k[i+r]; }
    for (qreal &w : k) w /= sum;
    return k;
}

QImage separableGaussianBlur(const QImage &source, qreal sigma)
{
    if (sigma <= 0.0) return source;
    const QImage src = source.format() == QImage::Format_ARGB32_Premultiplied
                       ? source : source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const int w = src.width(), h = src.height();
    if (!w || !h) return src;
    const auto k = gaussianKernel(sigma);
    const int r = int(k.size()/2);
    const QRgb *sb = reinterpret_cast<const QRgb*>(src.constBits());

    QImage horiz(w, h, QImage::Format_ARGB32_Premultiplied);
    QRgb *hb = reinterpret_cast<QRgb*>(horiz.bits());
    for (int y = 0; y < h; ++y) {
        const QRgb *row = sb + y*w; QRgb *out = hb + y*w;
        for (int x = 0; x < w; ++x) {
            qreal a=0,rv=0,g=0,b=0;
            for (int i=-r;i<=r;++i){const QRgb p=row[qBound(0,x+i,w-1)];const qreal wt=k[i+r];a+=wt*qAlpha(p);rv+=wt*qRed(p);g+=wt*qGreen(p);b+=wt*qBlue(p);}
            out[x]=qRgba(int(rv+.5),int(g+.5),int(b+.5),int(a+.5));
        }
    }
    QImage result(w, h, QImage::Format_ARGB32_Premultiplied);
    QRgb *rb = reinterpret_cast<QRgb*>(result.bits());
    for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; ++y) {
            qreal a=0,rv=0,g=0,b=0;
            for (int i=-r;i<=r;++i){const QRgb p=hb[qBound(0,y+i,h-1)*w+x];const qreal wt=k[i+r];a+=wt*qAlpha(p);rv+=wt*qRed(p);g+=wt*qGreen(p);b+=wt*qBlue(p);}
            rb[y*w+x]=qRgba(int(rv+.5),int(g+.5),int(b+.5),int(a+.5));
        }
    return result;
}

// ── Box blur ──────────────────────────────────────────────────────────────────
// Simple uniform kernel via sliding-window running sum. O(n) regardless of radius.

static void boxRowH(const QRgb *src, QRgb *dst, int w, int radius)
{
    const int sz = 2*radius+1;
    int rs=0,gs=0,bs=0,as=0;
    for (int i=-radius;i<=radius;++i){const QRgb p=src[qBound(0,i,w-1)];rs+=qRed(p);gs+=qGreen(p);bs+=qBlue(p);as+=qAlpha(p);}
    for (int x=0;x<w;++x){
        dst[x]=qRgba(rs/sz,gs/sz,bs/sz,as/sz);
        const QRgb po=src[qBound(0,x-radius,  w-1)];
        const QRgb pi=src[qBound(0,x+radius+1,w-1)];
        rs+=qRed(pi)-qRed(po); gs+=qGreen(pi)-qGreen(po);
        bs+=qBlue(pi)-qBlue(po); as+=qAlpha(pi)-qAlpha(po);
    }
}

static void boxColV(const QRgb *src, QRgb *dst, int w, int h, int x, int radius)
{
    const int sz = 2*radius+1;
    int rs=0,gs=0,bs=0,as=0;
    for (int i=-radius;i<=radius;++i){const QRgb p=src[qBound(0,i,h-1)*w+x];rs+=qRed(p);gs+=qGreen(p);bs+=qBlue(p);as+=qAlpha(p);}
    for (int y=0;y<h;++y){
        dst[y*w+x]=qRgba(rs/sz,gs/sz,bs/sz,as/sz);
        const QRgb po=src[qBound(0,y-radius,  h-1)*w+x];
        const QRgb pi=src[qBound(0,y+radius+1,h-1)*w+x];
        rs+=qRed(pi)-qRed(po); gs+=qGreen(pi)-qGreen(po);
        bs+=qBlue(pi)-qBlue(po); as+=qAlpha(pi)-qAlpha(po);
    }
}

QImage boxBlur(const QImage &src, int radius)
{
    const QImage s = src.format()==QImage::Format_ARGB32 ? src
                     : src.convertToFormat(QImage::Format_ARGB32);
    const int w=s.width(), h=s.height();
    const QRgb *sb = reinterpret_cast<const QRgb*>(s.constBits());

    QImage horiz(w, h, QImage::Format_ARGB32);
    QRgb *hb = reinterpret_cast<QRgb*>(horiz.bits());
    for (int y=0;y<h;++y) boxRowH(sb+y*w, hb+y*w, w, radius);

    QImage result(w, h, QImage::Format_ARGB32);
    QRgb *rb = reinterpret_cast<QRgb*>(result.bits());
    for (int x=0;x<w;++x) boxColV(hb, rb, w, h, x, radius);
    return result;
}

// ── Stack blur ────────────────────────────────────────────────────────────────
// Two consecutive box-blur passes produce a tent (triangle) kernel — this is
// mathematically equivalent to the StackBlur algorithm by Mario Klingemann
// and the same kernel used in the Scriptable widget-blur script.

QImage stackBlur(const QImage &src, int radius)
{
    return boxBlur(boxBlur(src, radius), radius);
}

// ── Lens blur ─────────────────────────────────────────────────────────────────
// Circular disk (flat) kernel — simulates a perfect circular camera aperture.
// Produces hard-edged bokeh rings instead of the smooth Gaussian falloff.
// Non-separable: O(n * π*r²). Fast enough since it runs on the downscaled image.

QImage lensBlur(const QImage &src, int radius)
{
    const QImage s = src.format()==QImage::Format_ARGB32 ? src
                     : src.convertToFormat(QImage::Format_ARGB32);
    const int w=s.width(), h=s.height();

    std::vector<std::pair<int,int>> disk;
    disk.reserve(int(M_PI * radius * radius));
    for (int dy=-radius;dy<=radius;++dy)
        for (int dx=-radius;dx<=radius;++dx)
            if (dx*dx+dy*dy <= radius*radius)
                disk.emplace_back(dx, dy);
    if (disk.empty()) return s;

    const float inv = 1.f / float(disk.size());
    const QRgb *sb = reinterpret_cast<const QRgb*>(s.constBits());
    QImage result(w, h, QImage::Format_ARGB32);
    QRgb *rb = reinterpret_cast<QRgb*>(result.bits());

    for (int y=0;y<h;++y)
        for (int x=0;x<w;++x) {
            float rv=0,g=0,b=0,a=0;
            for (auto [dx,dy] : disk) {
                const QRgb p = sb[qBound(0,y+dy,h-1)*w + qBound(0,x+dx,w-1)];
                rv+=qRed(p); g+=qGreen(p); b+=qBlue(p); a+=qAlpha(p);
            }
            rb[y*w+x]=qRgba(int(rv*inv+.5f),int(g*inv+.5f),
                             int(b*inv+.5f), int(a*inv+.5f));
        }
    return result;
}

// ── Full frosted-glass pipeline ───────────────────────────────────────────────
//
//  1. Downscale 0.2×  — fine detail gone, huge speed gain
//  2. HSL adjust      — lightBlur / darkBlur pre-processing (optional)
//  3. Chosen blur     — Gaussian / Stack / Box / Lens on the tiny image
//  4. Upscale         — smooth bicubic upscale adds further softening
//
// No Qt widget calls — safe to run on a QtConcurrent background thread.

QImage computeBlur(QImage source, int blurRadius,
                   bool lightMode, bool hslEnabled,
                   bWidget::BlurMode blurMode)
{
    if (source.isNull() || blurRadius <= 0) return source;

    constexpr qreal downscale = 0.2;
    const QSize shrunkSize =
        (QSizeF(source.size()) * downscale).toSize().expandedTo(QSize(2, 2));

    const QImage shrunk =
        source.scaled(shrunkSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    const QImage adjusted = hslEnabled ? applyHslAdjustment(shrunk, lightMode) : shrunk;

    const int smallRadius = qMax(1, qRound(blurRadius * downscale));

    QImage blurred;
    switch (blurMode) {
    case bWidget::BlurMode::Stack:
        blurred = stackBlur(adjusted, smallRadius);
        break;
    case bWidget::BlurMode::Box:
        blurred = boxBlur(adjusted, smallRadius);
        break;
    case bWidget::BlurMode::Lens:
        blurred = lensBlur(adjusted, smallRadius);
        break;
    default: {   // Gaussian
        const qreal sigma = qMax<qreal>(0.6, smallRadius / 3.0);
        blurred = separableGaussianBlur(
            adjusted.convertToFormat(QImage::Format_ARGB32_Premultiplied), sigma);
        break;
    }
    }

    return blurred.scaled(source.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

} // namespace

// ─── bWidget ──────────────────────────────────────────────────────────────────

bWidget::bWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    if (!parent)
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(50);
    connect(m_refreshTimer, &QTimer::timeout, this, &bWidget::refreshBackground);

    connect(&m_blurWatcher, &QFutureWatcher<QImage>::finished, this, [this] {
        if (m_blurWatcher.isCanceled()) return;
        QImage result = m_blurWatcher.result();
        result.setDevicePixelRatio(m_pendingDpr);
        m_background = QPixmap::fromImage(result);
        update();
        if (m_blurPending) { m_blurPending = false; refreshBackground(); }
    });
}

// ─── setters ──────────────────────────────────────────────────────────────────

void bWidget::setBlurRadius(int radius)
{
    radius = qMax(0, radius);
    if (m_blurRadius == radius) return;
    m_blurRadius = radius;
    scheduleRefresh();
}

void bWidget::setTintColor(const QColor &color)
{
    if (m_tintColor == color) return;
    m_tintColor = color;
    update();
}

void bWidget::setTranslucency(qreal amount)
{
    amount = qBound<qreal>(0.0, amount, 1.0);
    if (qFuzzyCompare(m_translucency, amount)) return;
    m_translucency = amount;
    update();
}

void bWidget::setCornerRadius(int radius)
{
    radius = qMax(0, radius);
    if (m_cornerRadius == radius) return;
    m_cornerRadius = radius;
    update();
}

void bWidget::setTheme(Theme theme)
{
    if (m_theme == theme) return;
    m_theme = theme;
    m_tintColor = (theme == Theme::Light) ? QColor(255, 255, 255, 102)
                                          : QColor(55,  55,  55,  102);
    scheduleRefresh();
}

void bWidget::setHslEnabled(bool enabled)
{
    if (m_hslEnabled == enabled) return;
    m_hslEnabled = enabled;
    scheduleRefresh();
}

void bWidget::setBlurMode(BlurMode mode)
{
    if (m_blurMode == mode) return;
    m_blurMode = mode;
    scheduleRefresh();
}

void bWidget::setCornerStyle(CornerStyle style)
{
    if (m_cornerStyle == style) return;
    m_cornerStyle = style;
    update();
}

void bWidget::setBorderColor(const QColor &color)
{
    if (m_borderColor == color) return;
    m_borderColor = color;
    update();
}

void bWidget::setBorder(bool enabled)
{
    if (m_enableBorder == enabled) return;
    m_enableBorder = enabled;
    update();
}

// ─── clip path (ported from TWidget::paintEvent switch) ──────────────────────

QPainterPath bWidget::buildClipPath() const
{
    const QRect r   = rect();
    const int   rad = m_cornerRadius;
    QPainterPath path;

    switch (m_cornerStyle) {
    case CornerStyle::TopOnly:
        path.moveTo(r.bottomLeft());
        path.lineTo(r.topLeft() + QPoint(0, rad));
        path.arcTo(QRect(r.topLeft(), QSize(2*rad, 2*rad)), 180, -90);
        path.lineTo(r.topRight() - QPoint(rad, 0));
        path.arcTo(QRect(r.topRight() - QPoint(2*rad, 0), QSize(2*rad, 2*rad)), 90, -90);
        path.lineTo(r.bottomRight());
        path.lineTo(r.bottomLeft());
        break;

    case CornerStyle::BottomOnly:
        path.moveTo(r.topLeft());
        path.lineTo(r.bottomLeft() - QPoint(0, rad));
        path.arcTo(QRect(r.bottomLeft() - QPoint(0, 2*rad), QSize(2*rad, 2*rad)), 180, 90);
        path.lineTo(r.bottomRight() - QPoint(rad, 0));
        path.arcTo(QRect(r.bottomRight() - QPoint(2*rad, 2*rad), QSize(2*rad, 2*rad)), 270, 90);
        path.lineTo(r.topRight());
        path.lineTo(r.topLeft());
        break;

    case CornerStyle::LeftOnly:
        path.moveTo(r.topRight());
        path.lineTo(r.topLeft() + QPoint(rad, 0));
        path.arcTo(QRect(r.topLeft(), QSize(2*rad, 2*rad)), 90, 90);
        path.lineTo(r.bottomLeft() - QPoint(0, rad));
        path.arcTo(QRect(r.bottomLeft() - QPoint(0, 2*rad), QSize(2*rad, 2*rad)), 180, 90);
        path.lineTo(r.bottomRight());
        path.lineTo(r.topRight());
        break;

    case CornerStyle::RightOnly:
        path.moveTo(r.topLeft());
        path.lineTo(r.topRight() - QPoint(rad, 0));
        path.arcTo(QRect(r.topRight() - QPoint(2*rad, 0), QSize(2*rad, 2*rad)), 90, -90);
        path.lineTo(r.bottomRight() - QPoint(0, rad));
        path.arcTo(QRect(r.bottomRight() - QPoint(2*rad, 2*rad), QSize(2*rad, 2*rad)), 0, -90);
        path.lineTo(r.bottomLeft());
        path.lineTo(r.topLeft());
        break;

    case CornerStyle::BottomLeft:
        path.moveTo(r.topRight());
        path.lineTo(r.topLeft());
        path.lineTo(r.bottomLeft() - QPoint(0, rad));
        path.arcTo(QRect(r.bottomLeft() - QPoint(0, 2*rad), QSize(2*rad, 2*rad)), 180, 90);
        path.lineTo(r.bottomRight());
        path.lineTo(r.topRight());
        break;

    case CornerStyle::BottomRight:
        path.moveTo(r.topLeft());
        path.lineTo(r.topRight());
        path.lineTo(r.bottomRight() - QPoint(0, rad));
        path.arcTo(QRect(r.bottomRight() - QPoint(2*rad, 2*rad), QSize(2*rad, 2*rad)), 0, -90);
        path.lineTo(r.bottomLeft());
        path.lineTo(r.topLeft());
        break;

    case CornerStyle::None:
        path.addRect(r);
        break;

    case CornerStyle::Default:
    default:
        path.addRoundedRect(r, rad, rad);
        break;
    }
    return path;
}

// ─── scheduling ───────────────────────────────────────────────────────────────

void bWidget::scheduleRefresh()
{
    if (m_refreshTimer) m_refreshTimer->start();
}

// ─── grab + async blur ────────────────────────────────────────────────────────

QPixmap bWidget::grabBehind() const
{
    auto *self = const_cast<bWidget *>(this);
    const bool vis = self->isVisible();
    QPixmap grabbed;
    if (QWidget *src = parentWidget()) {
        const QRect region(mapTo(src, QPoint(0,0)), size());
        if (vis) self->setVisible(false);
        grabbed = src->grab(region);
        if (vis) self->setVisible(true);
    } else if (QScreen *scr = screen()) {
        const QRect geo(mapToGlobal(QPoint(0,0)), size());
        const QPoint pos = geo.topLeft() - scr->geometry().topLeft();
        if (vis) self->setVisible(false);
        grabbed = scr->grabWindow(0, pos.x(), pos.y(), width(), height());
        if (vis) self->setVisible(true);
    }
    return grabbed;
}

void bWidget::refreshBackground()
{
    if (!isVisible() || size().isEmpty()) return;
    if (m_blurWatcher.isRunning()) { m_blurPending = true; return; }
    m_blurPending = false;

    const QPixmap grabbed = grabBehind();
    if (grabbed.isNull()) { m_background = {}; update(); return; }

    const QImage source =
        grabbed.toImage().convertToFormat(QImage::Format_ARGB32).copy();
    m_pendingDpr = grabbed.devicePixelRatio();

    // Capture everything by value — no `this` access from the background thread.
    const int       blurRadius = m_blurRadius;
    const bool      lightMode  = (m_theme == Theme::Light);
    const bool      hslEnabled = m_hslEnabled;
    const BlurMode  blurMode   = m_blurMode;

    m_blurWatcher.setFuture(
        QtConcurrent::run([source, blurRadius, lightMode, hslEnabled, blurMode]() -> QImage {
            return computeBlur(source, blurRadius, lightMode, hslEnabled, blurMode);
        })
    );
}

// ─── paint ────────────────────────────────────────────────────────────────────

void bWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QPainterPath clip = buildClipPath();
    painter.setClipPath(clip);

    if (!m_background.isNull())
        painter.drawPixmap(rect(), m_background);
    else
        painter.fillRect(rect(), m_tintColor.darker(150));

    const qreal frost = 1.0 - m_translucency;

    if (m_theme == Theme::Dark) {
        painter.setCompositionMode(QPainter::CompositionMode_HardLight);
        painter.fillPath(clip, QColor(55,55,55, qRound(51  * frost)));
        painter.setCompositionMode(QPainter::CompositionMode_SoftLight);
        painter.fillPath(clip, QColor(55,55,55, qRound(255 * frost)));
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    QColor tint = m_tintColor;
    tint.setAlphaF(tint.alphaF() * frost);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.fillPath(clip, tint);

    painter.setClipping(false);
    painter.setBrush(Qt::NoBrush);
    if (m_enableBorder && m_borderColor.alpha() > 0) {
        QPen pen(m_borderColor, 1);
        pen.setJoinStyle(Qt::MiterJoin);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawPath(clip);
    }
}

// ─── events ───────────────────────────────────────────────────────────────────

void bWidget::moveEvent(QMoveEvent *e)    { QWidget::moveEvent(e);   scheduleRefresh(); }
void bWidget::showEvent(QShowEvent *e)    { QWidget::showEvent(e);   scheduleRefresh(); }

void bWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    scheduleRefresh();
    emit resizing();
}

void bWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        emit rightClicked(event->pos());
    else if (event->button() == Qt::LeftButton)
        emit clicked(event->position());
    QWidget::mousePressEvent(event);
}

void bWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit doubleClicked(event->pos());
    QWidget::mouseDoubleClickEvent(event);
}
