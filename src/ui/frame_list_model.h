#pragma once

#include "domain/pattern.h"

#include <QAbstractListModel>
#include <QIcon>
#include <QPalette>

#include <functional>

namespace vibmory
{
// A projection of the document. Reordering is committed through the editor's
// command boundary before updating the projection; the model owns no session.
class FrameListModel final : public QAbstractListModel
{
    Q_OBJECT
public:
    using MoveCommand = std::function<bool(std::uint64_t frameId, int target,
                                           std::uint64_t revision)>;
    explicit FrameListModel(MoveCommand move, QObject* parent = nullptr);
    void setDocument(const PatternDocument& document, std::uint64_t revision);
    void setPalette(const QPalette& palette);
    [[nodiscard]] bool moving() const
    {
        return moving_;
    }
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                         int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                      int column, const QModelIndex& parent) override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                  const QModelIndex& destinationParent,
                  int destinationChild) override;
    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }
    Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

signals:
    void documentReordered();

private:
    struct Entry
    {
        std::uint64_t id;
        std::vector<CellStimulus> cells;
        QIcon thumbnail;
    };
    [[nodiscard]] int sourceRow(const QMimeData* data) const;
    [[nodiscard]] bool validMove(int source, int destination) const;

    MoveCommand move_;
    QByteArray dragToken_;
    std::vector<Entry> entries_;
    GridShape shape_;
    QPalette palette_;
    std::uint64_t revision_ = 0;
    bool moving_ = false;
};
} // namespace vibmory
