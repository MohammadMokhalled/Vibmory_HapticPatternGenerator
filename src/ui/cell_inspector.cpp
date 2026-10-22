#include "ui/cell_inspector.h"

#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace vibmory
{
CellInspector::CellInspector(QWidget* parent) : QGroupBox(parent)
{
    setObjectName("cellInspector");
    auto* layout = new QVBoxLayout(this);
    auto valueEditor = [this, layout](const QString& label, const char* name,
                                      int maximum, QSpinBox*& spin,
                                      QSlider*& slider) {
        auto* caption = new QLabel(label);
        spin = new QSpinBox;
        spin->setObjectName(name);
        spin->setRange(0, maximum);
        spin->setKeyboardTracking(false);
        caption->setBuddy(spin);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, maximum);
        slider->setAccessibleName(label);
        auto* row = new QHBoxLayout;
        auto* minimum = new QPushButton(tr("Min"));
        auto* maxButton = new QPushButton(tr("Max"));
        minimum->setAccessibleName(tr("Minimum %1").arg(label));
        maxButton->setAccessibleName(tr("Maximum %1").arg(label));
        row->addWidget(minimum);
        row->addWidget(spin);
        row->addWidget(maxButton);
        layout->addWidget(caption);
        layout->addLayout(row);
        layout->addWidget(slider);
        connect(minimum, &QPushButton::clicked, spin,
                [spin] { spin->setValue(spin->minimum()); });
        connect(maxButton, &QPushButton::clicked, spin,
                [spin] { spin->setValue(spin->maximum()); });
        connect(spin, &QSpinBox::valueChanged, this, [this, slider](int value) {
            const QSignalBlocker blocker(slider);
            slider->setValue(value);
            emit stimulusEdited();
        });
        connect(slider, &QSlider::valueChanged, this, [this, spin](int value) {
            const QSignalBlocker blocker(spin);
            spin->setValue(value);
            emit stimulusEdited();
        });
        connect(slider, &QSlider::sliderPressed, this,
                &CellInspector::editGroupStarted);
        connect(slider, &QSlider::sliderReleased, this,
                &CellInspector::editGroupFinished);
        connect(spin, &QSpinBox::editingFinished, this,
                &CellInspector::editGroupFinished);
    };
    valueEditor(tr("&Frequency (Hz)"), "frequencySpinBox",
                static_cast<int>(maxFrequencyHz), frequency_, frequencySlider_);
    valueEditor(tr("&Amplitude (signed PCM peak)"), "amplitudeSpinBox",
                static_cast<int>(maxAmplitude), amplitude_, amplitudeSlider_);
    setCell({}, {}, maxFrequencyHz);
}

void CellInspector::setCell(std::optional<CellPosition> position,
                            std::optional<CellStimulus> value,
                            std::uint32_t frequencyLimit)
{
    setEnabled(position.has_value() && value.has_value());
    if (!position || !value)
    {
        setTitle(tr("Select a cell"));
        return;
    }
    const QSignalBlocker f(frequency_), a(amplitude_), fs(frequencySlider_),
        as(amplitudeSlider_);
    frequency_->setMaximum(static_cast<int>(frequencyLimit));
    frequencySlider_->setMaximum(static_cast<int>(frequencyLimit));
    frequency_->setValue(static_cast<int>(value->frequencyHz));
    amplitude_->setValue(static_cast<int>(value->amplitude));
    frequencySlider_->setValue(static_cast<int>(value->frequencyHz));
    amplitudeSlider_->setValue(static_cast<int>(value->amplitude));
    setTitle(
        tr("Cell R%1 C%2").arg(position->row + 1).arg(position->column + 1));
}

CellStimulus CellInspector::stimulus() const
{
    return {static_cast<std::uint32_t>(frequency_->value()),
            static_cast<std::uint32_t>(amplitude_->value())};
}
} // namespace vibmory
