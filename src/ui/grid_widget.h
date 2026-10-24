#pragma once

#include "domain/pattern.h"

#include <QRectF>
#include <QWidget>

#include <optional>

class QColor;

namespace vibmory
{
[[nodiscard]] QColor stimulusColor(CellStimulus cell);

class GridGeometry
{
public:
    [[nodiscard]] static std::optional<CellPosition> cellAt(QRectF bounds,
                                                            GridShape shape,
                                                            QPointF point);
    [[nodiscard]] static QRectF cellRect(QRectF bounds, GridShape shape,
                                         CellPosition cell);
};

class GridWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit GridWidget(QWidget* parent = nullptr);
    void setFrame(const PatternDocument& document, std::size_t frame,
                  std::optional<CellPosition> selection);
    [[nodiscard]] QSize minimumSizeHint() const override
    {
        return {240, 180};
    }

signals:
    void cellSelected(int row, int column);
    void selectionCleared();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    GridShape shape_;
    std::vector<CellStimulus> values_;
    std::optional<CellPosition> selection_;
};
} // namespace vibmory
