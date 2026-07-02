#pragma once

#include "dante/ui/TerminalController.hpp"

#include <QFont>
#include <QQuickPaintedItem>

namespace dante {

// Renderiza a grade do term::Screen de um TerminalController com QPainter. Recomputa cols/rows
// a partir do tamanho do item + métricas da fonte (e chama controller->resize). Encaminha teclas
// pro controller (sequências VT).
// ponytail: QPainter primeiro (PRD 4.6: comece simples, meça, migre p/ scene graph/atlas de
//           glifos se < 60fps). drawText por célula é suficiente p/ 80x24; batch por run de
//           atributo se o profiling pedir.
class TerminalView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(dante::TerminalController* controller READ controller WRITE setController NOTIFY
                   controllerChanged)

public:
    explicit TerminalView(QQuickItem* parent = nullptr);

    TerminalController* controller() const { return ctrl_; }
    void setController(TerminalController* c);

    void paint(QPainter* painter) override;

signals:
    void controllerChanged();

protected:
    void geometryChange(const QRectF& newGeom, const QRectF& oldGeom) override;
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    void recomputeGrid();

    TerminalController* ctrl_ = nullptr;
    QFont font_;
    qreal cellW_ = 8.0;
    qreal cellH_ = 16.0;
    qreal ascent_ = 12.0;
};

} // namespace dante
