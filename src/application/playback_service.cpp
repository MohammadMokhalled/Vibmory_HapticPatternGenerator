#include "application/playback_service.h"

#include <algorithm>

namespace vibmory
{
PlaybackService::PlaybackService(PlaybackBackend &backend,
                                 const MonotonicClock &clock)
    : backend_(backend), clock_(clock)
{
}

PlaybackService::~PlaybackService()
{
    stop();
}

Result<void> PlaybackService::start(DocumentSnapshot snapshot)
{
    if(state_ == PlaybackState::Playing || state_ == PlaybackState::Starting)
        return Error{ErrorCode::Busy, "Playback is already active."};
    if(auto result = snapshot.document.validateOutput(); !result)
        return result;
    state_ = PlaybackState::Starting;
    error_.reset();
    snapshot_ = std::move(snapshot);
    visualFrame_ = 0;
    auto result = backend_.start(*snapshot_);
    if(!result)
    {
        fail(result.error());
        return result;
    }
    startedAt_ = clock_.microseconds();
    state_ = PlaybackState::Playing;
    return {};
}

void PlaybackService::stop() noexcept
{
    if(state_ == PlaybackState::Playing || state_ == PlaybackState::Starting)
    {
        state_ = PlaybackState::Stopping;
        backend_.stop();
    }
    state_ = PlaybackState::Idle;
    snapshot_.reset();
    visualFrame_ = 0;
}

void PlaybackService::fail(Error error)
{
    backend_.stop();
    state_ = PlaybackState::Error;
    error_ = std::move(error);
    snapshot_.reset();
    visualFrame_ = 0;
}

void PlaybackService::tick()
{
    if(state_ != PlaybackState::Playing || !snapshot_)
        return;
    const auto &document = snapshot_->document;
    const auto &settings = document.settings();
    auto position = backend_.processedSampleFrames();
    if(!position)
    {
        const auto now = clock_.microseconds();
        const auto elapsed = now >= startedAt_ ? now - startedAt_ : 0;
        const auto bounded = std::min(
            elapsed, static_cast<std::uint64_t>(settings.durationMs) * 1000);
        position = bounded * settings.sampleRate / 1000000;
    }
    if(*position >= settings.sampleFrames())
    {
        stop();
        return;
    }
    visualFrame_ = static_cast<std::size_t>(
        (*position * settings.frameRate / settings.sampleRate) %
        document.frameCount());
}
} // namespace vibmory
