#include "ui/grid_widget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

namespace vibmory
{
std::optional<CellPosition> GridGeometry::cellAt(QRectF bounds, GridShape shape,
                                                 QPointF point)
{
    if (bounds.width() <= 0 || bounds.height() <= 0 || !shape.validate() ||
        point.x() < bounds.x() || point.y() < bounds.y() ||
        point.x() >= bounds.x() + bounds.width() ||
        point.y() >= bounds.y() + bounds.height())
        return std::nullopt;
    return CellPosition{
        std::min(shape.rows - 1,
                 static_cast<std::uint32_t>((point.y() - bounds.y()) *
                                            shape.rows / bounds.height())),
        std::min(shape.columns - 1,
                 static_cast<std::uint32_t>((point.x() - bounds.x()) *
                                            shape.columns / bounds.width()))};
}

QRectF GridGeometry::cellRect(QRectF bounds, GridShape shape, CellPosition cell)
{
    if (!shape.validate() || bounds.width() <= 0 || bounds.height() <= 0 ||
        cell.row >= shape.rows || cell.column >= shape.columns)
        return {};
    const double width = bounds.width() / shape.columns;
    const double height = bounds.height() / shape.rows;
    return {bounds.x() + cell.column * width, bounds.y() + cell.row * height,
            width, height};
}

GridWidget::GridWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("gridWidget");
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(tr("Actuator grid"));
    setAccessibleDescription(
        tr("Use arrow keys to select a cell. Escape clears selection. Stimulus "
           "values appear in the inspector."));
    setToolTip(tr("Select a cell to edit frequency and amplitude. Arrow keys "
                  "move selection."));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void GridWidget::setFrame(const PatternDocument& document, std::size_t frame,
                          std::optional<CellPosition> selection)
{
    if (shape_ == document.shape() && values_ == document.frame(frame) &&
        selection_ == selection)
        return;
    shape_ = document.shape();
    values_ = document.frame(frame);
    selection_ = selection;
    update();
}

void GridWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    if (values_.size() != shape_.cellCount())
        return;
    for (std::uint32_t row = 0; row < shape_.rows; ++row)
        for (std::uint32_t column = 0; column < shape_.columns; ++column)
        {
            const CellPosition position{row, column};
            const auto cell =
                values_[static_cast<std::size_t>(row) * shape_.columns +
                        column];
            const auto bounds =
                GridGeometry::cellRect(QRectF(rect()), shape_, position);
            const auto hue =
                static_cast<int>(cell.frequencyHz * 200 / maxFrequencyHz);
            const auto saturation =
                static_cast<int>(cell.amplitude * 190 / maxAmplitude);
            const auto color = QColor::fromHsv(hue, saturation, 245);
            painter.fillRect(bounds.adjusted(1, 1, -1, -1), color);
            painter.setPen(palette().mid().color());
            painter.drawRect(bounds.adjusted(0, 0, -1, -1));
            if (bounds.width() > 95 && bounds.height() > 55)
            {
                painter.setPen(Qt::black);
                painter.drawText(bounds.adjusted(5, 5, -5, -5), Qt::AlignCenter,
                                 tr("R%1 C%2\n%3 Hz · %4")
                                     .arg(row + 1)
                                     .arg(column + 1)
                                     .arg(cell.frequencyHz)
                                     .arg(cell.amplitude));
            }
            if (selection_ == position)
            {
                painter.setPen(
                    QPen(palette().highlight().color(), hasFocus() ? 4 : 3));
                painter.drawRect(bounds.adjusted(2, 2, -3, -3));
            }
        }
}

void GridWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !isEnabled())
        return;
    const auto cell =
        GridGeometry::cellAt(QRectF(rect()), shape_, event->position());
    if (cell)
    {
        setFocus(Qt::MouseFocusReason);
        emit cellSelected(static_cast<int>(cell->row),
                          static_cast<int>(cell->column));
    }
}

void GridWidget::keyPressEvent(QKeyEvent* event)
{
    auto position = selection_.value_or(CellPosition{});
    switch (event->key())
    {
    case Qt::Key_Left:
        if (position.column > 0)
            --position.column;
        break;
    case Qt::Key_Right:
        if (position.column + 1 < shape_.columns)
            ++position.column;
        break;
    case Qt::Key_Up:
        if (position.row > 0)
            --position.row;
        break;
    case Qt::Key_Down:
        if (position.row + 1 < shape_.rows)
            ++position.row;
        break;
    case Qt::Key_Return:
    case Qt::Key_Space:
        break;
    case Qt::Key_Escape:
        emit selectionCleared();
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    emit cellSelected(static_cast<int>(position.row),
                      static_cast<int>(position.column));
}
} // namespace vibmory
