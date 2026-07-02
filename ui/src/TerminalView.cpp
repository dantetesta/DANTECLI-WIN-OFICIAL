#include "dante/ui/TerminalView.hpp"

#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

namespace dante {

namespace {

// Paleta xterm-256: 16 básicas + cubo 6x6x6 + rampa cinza.
QColor xtermColor(int idx) {
    static const QColor base16[16] = {
        QColor(0x00, 0x00, 0x00), QColor(0x80, 0x00, 0x00), QColor(0x00, 0x80, 0x00),
        QColor(0x80, 0x80, 0x00), QColor(0x00, 0x00, 0x80), QColor(0x80, 0x00, 0x80),
        QColor(0x00, 0x80, 0x80), QColor(0xC0, 0xC0, 0xC0), QColor(0x80, 0x80, 0x80),
        QColor(0xFF, 0x00, 0x00), QColor(0x00, 0xFF, 0x00), QColor(0xFF, 0xFF, 0x00),
        QColor(0x00, 0x00, 0xFF), QColor(0xFF, 0x00, 0xFF), QColor(0x00, 0xFF, 0xFF),
        QColor(0xFF, 0xFF, 0xFF)};
    if (idx < 16) {
        return base16[idx];
    }
    if (idx < 232) {
        const int i = idx - 16;
        const auto chan = [](int v) { return v == 0 ? 0 : 55 + v * 40; };
        return QColor(chan((i / 36) % 6), chan((i / 6) % 6), chan(i % 6));
    }
    const int level = 8 + (idx - 232) * 10; // 232..255
    return QColor(level, level, level);
}

QColor toQColor(const term::Color& c, bool isBg) {
    switch (c.kind) {
    case term::Color::Kind::Default:
        return isBg ? QColor(0x0A, 0x0A, 0x0C) : QColor(0xD6, 0xD6, 0xDA);
    case term::Color::Kind::Indexed:
        return xtermColor(c.index);
    case term::Color::Kind::Rgb:
        return QColor(c.r, c.g, c.b);
    }
    return QColor(0xD6, 0xD6, 0xDA);
}

const QColor kDefaultBg(0x0A, 0x0A, 0x0C);
const QColor kCursor(0x2D, 0x6B, 0xF0);

} // namespace

TerminalView::TerminalView(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsInputMethod, true);
    font_.setFamilies({"Cascadia Mono", "Consolas", "Menlo", "DejaVu Sans Mono", "monospace"});
    font_.setStyleHint(QFont::Monospace);
    font_.setFixedPitch(true);
    font_.setPixelSize(14);
    const QFontMetricsF fm(font_);
    cellW_ = fm.horizontalAdvance(QChar('M'));
    cellH_ = fm.height();
    ascent_ = fm.ascent();
}

void TerminalView::setController(TerminalController* c) {
    if (ctrl_ == c) {
        return;
    }
    if (ctrl_ != nullptr) {
        disconnect(ctrl_, nullptr, this, nullptr);
    }
    ctrl_ = c;
    if (ctrl_ != nullptr) {
        connect(ctrl_, &TerminalController::screenChanged, this, [this] { update(); });
    }
    emit controllerChanged();
    recomputeGrid();
    update();
}

void TerminalView::recomputeGrid() {
    if (ctrl_ == nullptr || cellW_ <= 0 || cellH_ <= 0) {
        return;
    }
    const int cols = std::max(1, static_cast<int>(width() / cellW_));
    const int rows = std::max(1, static_cast<int>(height() / cellH_));
    if (cols != ctrl_->screen().cols() || rows != ctrl_->screen().rows()) {
        ctrl_->resize(cols, rows);
    }
}

void TerminalView::geometryChange(const QRectF& newGeom, const QRectF& oldGeom) {
    QQuickPaintedItem::geometryChange(newGeom, oldGeom);
    recomputeGrid();
    update();
}

void TerminalView::paint(QPainter* painter) {
    painter->fillRect(boundingRect(), kDefaultBg);
    if (ctrl_ == nullptr) {
        return;
    }
    const term::Screen& scr = ctrl_->screen();
    painter->setFont(font_);

    for (int r = 0; r < scr.rows(); ++r) {
        const qreal y = r * cellH_;
        for (int c = 0; c < scr.cols(); ++c) {
            const term::Cell& cell = scr.at(r, c);
            const qreal x = c * cellW_;

            const QColor bg = toQColor(cell.bg, /*isBg=*/true);
            if (bg != kDefaultBg) {
                painter->fillRect(QRectF(x, y, cellW_, cellH_), bg);
            }
            if (cell.ch != U' ' && cell.ch >= 0x20) {
                painter->setPen(toQColor(cell.fg, /*isBg=*/false));
                if (font_.bold() != cell.bold) {
                    font_.setBold(cell.bold);
                    painter->setFont(font_);
                }
                const char32_t cp = cell.ch;
                painter->drawText(QPointF(x, y + ascent_), QString::fromUcs4(&cp, 1));
            }
        }
    }

    // Cursor: bloco preenchido se focado, contorno caso contrário.
    const qreal cx = scr.cursorCol() * cellW_;
    const qreal cy = scr.cursorRow() * cellH_;
    const QRectF cur(cx, cy, cellW_, cellH_);
    if (hasActiveFocus()) {
        painter->fillRect(cur, QColor(kCursor.red(), kCursor.green(), kCursor.blue(), 140));
    } else {
        painter->setPen(kCursor);
        painter->drawRect(cur.adjusted(0.5, 0.5, -0.5, -0.5));
    }
}

void TerminalView::keyPressEvent(QKeyEvent* e) {
    if (ctrl_ != nullptr) {
        ctrl_->sendKey(e->key(), static_cast<int>(e->modifiers()), e->text());
        e->accept();
        return;
    }
    QQuickPaintedItem::keyPressEvent(e);
}

void TerminalView::mousePressEvent(QMouseEvent* e) {
    forceActiveFocus();
    update(); // redesenha o cursor (foco mudou)
    e->accept();
}

} // namespace dante
