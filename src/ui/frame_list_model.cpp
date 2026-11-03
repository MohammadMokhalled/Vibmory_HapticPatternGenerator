#include "ui/frame_list_model.h"

#include "ui/grid_widget.h"

#include <QGuiApplication>
#include <QMimeData>
#include <QPainter>
#include <QScopedValueRollback>
#include <QUuid>

#include <algorithm>

namespace vibmory
{
namespace
{
constexpr auto frameMimeType = "application/x-vibmory-frame";

QIcon thumbnail(GridShape shape, const std::vector<CellStimulus>& cells,
                const QPalette& palette)
{
    QPixmap pixmap(80, 48);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    for (std::uint32_t row = 0; row < shape.rows; ++row)
        for (std::uint32_t column = 0; column < shape.columns; ++column)
        {
            const auto bounds = GridGeometry::cellRect(QRectF(pixmap.rect()),
                                                       shape, {row, column});
            const auto cell =
                cells[static_cast<std::size_t>(row) * shape.columns + column];
            painter.fillRect(bounds, cellFillColor(cell, palette));
            painter.setPen(palette.mid().color());
            painter.drawRect(bounds.adjusted(0, 0, -0.5, -0.5));
        }
    return QIcon(pixmap);
}
} // namespace

FrameListModel::FrameListModel(MoveCommand move, QObject* parent)
    : QAbstractListModel(parent), move_(std::move(move)),
      dragToken_(QUuid::createUuid().toByteArray(QUuid::WithoutBraces)),
      palette_(QGuiApplication::palette())
{
}

int FrameListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(entries_.size());
}

QVariant FrameListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0 ||
        index.row() < 0 || index.row() >= rowCount())
        return {};
    const auto& entry = entries_[static_cast<std::size_t>(index.row())];
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::AccessibleTextRole:
        return tr("Frame %1").arg(index.row() + 1);
    case Qt::DecorationRole:
        return entry.thumbnail;
    case Qt::UserRole:
        return QVariant::fromValue(static_cast<qulonglong>(entry.id));
    case Qt::ToolTipRole:
        return tr("Frame %1 · ID %2\nDrag between frames to reorder. You can "
                  "also use Move earlier and Move later.")
            .arg(index.row() + 1)
            .arg(entry.id);
    default:
        return {};
    }
}

Qt::ItemFlags FrameListModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;
    // Drops are accepted between rows, so there is no ambiguous drop-on action.
    return QAbstractListModel::flags(index) | Qt::ItemIsDragEnabled;
}

void FrameListModel::setDocument(const PatternDocument& document,
                                 std::uint64_t revision)
{
    revision_ = revision;
    bool reset =
        entries_.size() != document.frameCount() || shape_ != document.shape();
    for (std::size_t index = 0; !reset && index < entries_.size(); ++index)
        reset = entries_[index].id != document.frameId(index);
    if (reset)
    {
        std::vector<Entry> entries;
        entries.reserve(document.frameCount());
        for (std::size_t index = 0; index < document.frameCount(); ++index)
            entries.push_back(
                {document.frameId(index), document.frame(index),
                 thumbnail(document.shape(), document.frame(index), palette_)});
        beginResetModel();
        entries_ = std::move(entries);
        shape_ = document.shape();
        endResetModel();
        return;
    }
    for (std::size_t row = 0; row < entries_.size(); ++row)
        if (entries_[row].cells != document.frame(row))
        {
            entries_[row].cells = document.frame(row);
            entries_[row].thumbnail =
                thumbnail(shape_, entries_[row].cells, palette_);
            const auto changed = index(static_cast<int>(row));
            emit dataChanged(changed, changed, {Qt::DecorationRole});
        }
}

void FrameListModel::setPalette(const QPalette& palette)
{
    if (palette_ == palette)
        return;
    palette_ = palette;
    for (auto& entry : entries_)
        entry.thumbnail = thumbnail(shape_, entry.cells, palette_);
    if (!entries_.empty())
        emit dataChanged(index(0), index(rowCount() - 1), {Qt::DecorationRole});
}

QStringList FrameListModel::mimeTypes() const
{
    return {QString::fromLatin1(frameMimeType)};
}

QMimeData* FrameListModel::mimeData(const QModelIndexList& indexes) const
{
    auto* result = new QMimeData;
    if (indexes.size() == 1 && indexes.front().isValid() &&
        indexes.front().model() == this && indexes.front().column() == 0 &&
        indexes.front().row() >= 0 && indexes.front().row() < rowCount())
    {
        const auto id =
            entries_[static_cast<std::size_t>(indexes.front().row())].id;
        result->setData(
            frameMimeType,
            dragToken_ + ':' + QByteArray::number(static_cast<qulonglong>(id)) +
                ':' + QByteArray::number(static_cast<qulonglong>(revision_)));
    }
    return result;
}

int FrameListModel::sourceRow(const QMimeData* data) const
{
    if (!data || !data->hasFormat(frameMimeType))
        return -1;
    const auto payload = data->data(frameMimeType);
    if (payload.size() > 100)
        return -1;
    const auto fields = payload.split(':');
    if (fields.size() != 3 || fields[0] != dragToken_)
        return -1;
    bool validId = false, validRevision = false;
    const auto id = fields[1].toULongLong(&validId);
    const auto revision = fields[2].toULongLong(&validRevision);
    if (!validId || !validRevision || revision != revision_)
        return -1;
    const auto found =
        std::find_if(entries_.begin(), entries_.end(),
                     [id](const auto& entry) { return entry.id == id; });
    return found == entries_.end() ? -1
                                   : static_cast<int>(found - entries_.begin());
}

bool FrameListModel::validMove(int source, int destination) const
{
    return !moving_ && source >= 0 && source < rowCount() && destination >= 0 &&
           destination <= rowCount() && destination != source &&
           destination != source + 1;
}

bool FrameListModel::canDropMimeData(const QMimeData* data,
                                     Qt::DropAction action, int row, int column,
                                     const QModelIndex& parent) const
{
    return action == Qt::MoveAction && !parent.isValid() && column <= 0 &&
           validMove(sourceRow(data), row == -1 ? rowCount() : row);
}

bool FrameListModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                  int row, int column,
                                  const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;
    return moveRows({}, sourceRow(data), 1, {}, row == -1 ? rowCount() : row);
}

bool FrameListModel::moveRows(const QModelIndex& sourceParent, int source,
                              int count, const QModelIndex& destinationParent,
                              int destination)
{
    if (sourceParent.isValid() || destinationParent.isValid() || count != 1 ||
        !validMove(source, destination) || !move_)
        return false;
    const QScopedValueRollback<bool> guard(moving_, true);
    const auto target = destination > source ? destination - 1 : destination;
    // The command must not refresh the view reentrantly. The signal below
    // publishes the completed transaction to its owner after model
    // notification.
    if (!move_(entries_[static_cast<std::size_t>(source)].id, target,
               revision_))
        return false;
    if (beginMoveRows({}, source, source, {}, destination))
    {
        if (source < target)
            std::rotate(entries_.begin() + source,
                        entries_.begin() + source + 1,
                        entries_.begin() + target + 1);
        else
            std::rotate(entries_.begin() + target, entries_.begin() + source,
                        entries_.begin() + source + 1);
        endMoveRows();
        emit dataChanged(
            index(std::min(source, target)), index(std::max(source, target)),
            {Qt::DisplayRole, Qt::AccessibleTextRole, Qt::ToolTipRole});
    }
    emit documentReordered();
    return true;
}
} // namespace vibmory
