#pragma once

#include "application/playback_service.h"

#include <QAudioDevice>
#include <QElapsedTimer>
#include <QMediaDevices>
#include <QObject>

#include <memory>

class QAudioSink;

namespace vibmory
{
class PcmStream;

class QtClock final : public MonotonicClock
{
        public:
    QtClock()
    {
        timer_.start();
    }
    std::uint64_t microseconds() const override
    {
        return static_cast<std::uint64_t>(timer_.nsecsElapsed() / 1000);
    }

        private:
    QElapsedTimer timer_;
};

class QtAudioBackend final : public QObject, public PlaybackBackend
{
    Q_OBJECT
        public:
    explicit QtAudioBackend(QObject *parent = nullptr);
    ~QtAudioBackend() override;
    [[nodiscard]] Result<void> start(const DocumentSnapshot &snapshot) override;
    void stop() noexcept override;
    [[nodiscard]] std::optional<std::uint64_t> processedSampleFrames()
        const override;
    void selectDevice(QAudioDevice device);
    [[nodiscard]] QAudioDevice selectedDevice() const
    {
        return device_;
    }
    [[nodiscard]] std::vector<AudioDeviceDescription> devices() const override;
    [[nodiscard]] Result<void> selectDeviceId(const std::string &id) override;

        signals:
    void failed(const QString &message);
    void finished();

        private:
    QMediaDevices devices_;
    QAudioDevice device_;
    std::unique_ptr<PcmStream> stream_;
    std::unique_ptr<QAudioSink> sink_;
    std::uint32_t sampleRate_ = 48000;
};
} // namespace vibmory
