#pragma once

#include "domain/pattern.h"

#include <QGroupBox>

#include <optional>

class QSlider;
class QSpinBox;

namespace vibmory
{
class CellInspector final : public QGroupBox
{
    Q_OBJECT
public:
    explicit CellInspector(QWidget* parent = nullptr);
    void setCell(std::optional<CellPosition> position,
                 std::optional<CellStimulus> value,
                 std::uint32_t frequencyLimit);
    [[nodiscard]] CellStimulus stimulus() const;

signals:
    void stimulusEdited();
    void editGroupStarted();
    void editGroupFinished();

private:
    QSpinBox* frequency_ = nullptr;
    QSpinBox* amplitude_ = nullptr;
    QSlider* frequencySlider_ = nullptr;
    QSlider* amplitudeSlider_ = nullptr;
};
} // namespace vibmory
