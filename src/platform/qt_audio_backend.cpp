#include "platform/qt_audio_backend.h"

#include "audio/pcm_renderer.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>

#include <algorithm>
#include <cstring>

namespace vibmory
{
class PcmStream final : public QIODevice
{
        public:
    explicit PcmStream(PcmRenderer renderer) : renderer_(std::move(renderer))
    {
        open(QIODevice::ReadOnly);
    }
    bool isSequential() const override
    {
        return true;
    }
    bool atEnd() const override
    {
        return offset_ == bytes_.size() &&
               renderer_.position() == renderer_.totalFrames() &&
               QIODevice::bytesAvailable() == 0;
    }
    qint64 bytesAvailable() const override
    {
        return static_cast<qint64>(
                   (renderer_.totalFrames() - renderer_.position()) *
                       renderer_.channelCount() * 2 +
                   bytes_.size() - offset_) +
               QIODevice::bytesAvailable();
    }

        protected:
    qint64 readData(char *destination, qint64 maxSize) override
    {
        if(maxSize <= 0)
            return 0;
        std::size_t written = 0;
        const auto requested = static_cast<std::size_t>(maxSize);
        while(written < requested)
        {
            if(offset_ == bytes_.size())
            {
                if(renderer_.position() == renderer_.totalFrames())
                    break;
                std::vector<std::int16_t> samples(4096 *
                                                  renderer_.channelCount());
                const auto result = renderer_.render(samples);
                if(!result)
                    return -1;
                bytes_ = littleEndianPcm(std::span(samples).first(
                    result.value() * renderer_.channelCount()));
                offset_ = 0;
            }
            const auto count =
                std::min(requested - written, bytes_.size() - offset_);
            std::memcpy(destination + written, bytes_.data() + offset_, count);
            offset_ += count;
            written += count;
        }
        return static_cast<qint64>(written);
    }
    qint64 writeData(const char *, qint64) override
    {
        return -1;
    }

        private:
    PcmRenderer renderer_;
    std::vector<std::byte> bytes_;
    std::size_t offset_ = 0;
};

QtAudioBackend::QtAudioBackend(QObject *parent)
    : QObject(parent), device_(QMediaDevices::defaultAudioOutput())
{
    connect(&devices_, &QMediaDevices::audioOutputsChanged, this, [this] {
        if(!sink_)
            return;
        const auto outputs = QMediaDevices::audioOutputs();
        const auto found = std::find_if(
            outputs.begin(), outputs.end(),
            [this](const auto &output) { return output.id() == device_.id(); });
        if(found == outputs.end())
            emit failed(tr("The selected audio device was disconnected."),
                        generation_);
    });
}

QtAudioBackend::~QtAudioBackend()
{
    stop();
}

std::vector<AudioDeviceDescription> QtAudioBackend::devices() const
{
    std::vector<AudioDeviceDescription> result;
    for(const auto &device : QMediaDevices::audioOutputs())
        result.push_back({device.id().toHex().toStdString(),
                          device.description().toStdString()});
    return result;
}

Result<void> QtAudioBackend::selectDeviceId(const std::string &id)
{
    for(const auto &device : QMediaDevices::audioOutputs())
        if(device.id().toHex().toStdString() == id)
        {
            selectDevice(device);
            return {};
        }
    return Error{ErrorCode::Device,
                 "The selected output device is no longer available."};
}

void QtAudioBackend::selectDevice(QAudioDevice device)
{
    stop();
    device_ = std::move(device);
}

Result<void> QtAudioBackend::start(const DocumentSnapshot &snapshot)
{
    stop();
    if(device_.isNull())
        return Error{ErrorCode::Device, "No audio output device is available. "
                                        "WAV export remains available."};
    auto renderer = PcmRenderer::create(snapshot.document);
    if(!renderer)
        return renderer.error();
    QAudioFormat format;
    format.setSampleRate(
        static_cast<int>(snapshot.document.settings().sampleRate));
    format.setChannelCount(
        static_cast<int>(snapshot.document.shape().cellCount()));
    format.setSampleFormat(QAudioFormat::Int16);
    if(!device_.isFormatSupported(format))
        return Error{ErrorCode::Unsupported,
                     "The selected device does not support the project's exact "
                     "sample rate/channel count. "
                     "Choose a compatible device or export WAV; channels are "
                     "never silently downmixed."};
    sampleRate_ = snapshot.document.settings().sampleRate;
    stream_ = std::make_unique<PcmStream>(std::move(renderer.value()));
    sink_ = std::make_unique<QAudioSink>(device_, format);
    sink_->setBufferSize(static_cast<qsizetype>(
        sampleRate_ * snapshot.document.shape().cellCount() * 2 / 10));
    connect(sink_.get(), &QAudioSink::stateChanged, this,
            [this, generation = generation_](QAudio::State state) {
                if(!sink_ || generation != generation_)
                    return;
                if(state == QAudio::StoppedState &&
                   sink_->error() != QAudio::NoError)
                    emit failed(tr("Audio output failed (backend error %1).")
                                    .arg(static_cast<int>(sink_->error())),
                                generation);
                else if(state == QAudio::IdleState && stream_ &&
                        stream_->atEnd())
                    emit finished(generation);
            });
    sink_->start(stream_.get());
    if(sink_->error() != QAudio::NoError)
    {
        const auto code = static_cast<int>(sink_->error());
        stop();
        return Error{ErrorCode::Device,
                     "Unable to start the audio output (error " +
                         std::to_string(code) + ")."};
    }
    return {};
}

void QtAudioBackend::stop() noexcept
{
    ++generation_;
    if(sink_)
    {
        sink_->disconnect(this);
        sink_->stop();
        sink_.reset();
    }
    stream_.reset();
}

std::optional<std::uint64_t> QtAudioBackend::processedSampleFrames() const
{
    if(!sink_)
        return std::nullopt;
    return static_cast<std::uint64_t>(
               std::max<qint64>(0, sink_->processedUSecs())) *
           sampleRate_ / 1000000;
}
} // namespace vibmory
