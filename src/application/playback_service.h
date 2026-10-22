#pragma once

#include "domain/pattern.h"

#include <optional>

namespace vibmory
{
struct AudioDeviceDescription
{
    std::string id;
    std::string label;
};

class PlaybackBackend
{
        public:
    virtual ~PlaybackBackend() = default;
    [[nodiscard]] virtual Result<void> start(
        const DocumentSnapshot &snapshot) = 0;
    virtual void stop() noexcept = 0;
    [[nodiscard]] virtual std::optional<std::uint64_t> processedSampleFrames()
        const = 0;
    [[nodiscard]] virtual std::vector<AudioDeviceDescription> devices() const
    {
        return {};
    }
    [[nodiscard]] virtual Result<void> selectDeviceId(const std::string &)
    {
        return Error{ErrorCode::Unsupported,
                     "Device selection is unavailable for this backend."};
    }
};

class MonotonicClock
{
        public:
    virtual ~MonotonicClock() = default;
    [[nodiscard]] virtual std::uint64_t microseconds() const = 0;
};

enum class PlaybackState
{
    Idle,
    Starting,
    Playing,
    Stopping,
    Error
};

class PlaybackService
{
        public:
    PlaybackService(PlaybackBackend &backend, const MonotonicClock &clock);
    ~PlaybackService();
    PlaybackService(const PlaybackService &) = delete;
    PlaybackService &operator=(const PlaybackService &) = delete;
    [[nodiscard]] Result<void> start(DocumentSnapshot snapshot);
    void stop() noexcept;
    void fail(Error error);
    void tick();
    [[nodiscard]] PlaybackState state() const
    {
        return state_;
    }
    [[nodiscard]] std::size_t visualFrame() const
    {
        return visualFrame_;
    }
    [[nodiscard]] const std::optional<Error> &lastError() const
    {
        return error_;
    }
    [[nodiscard]] const std::optional<DocumentSnapshot> &snapshot() const
    {
        return snapshot_;
    }

        private:
    PlaybackBackend &backend_;
    const MonotonicClock &clock_;
    PlaybackState state_ = PlaybackState::Idle;
    std::optional<DocumentSnapshot> snapshot_;
    std::optional<Error> error_;
    std::uint64_t startedAt_ = 0;
    std::size_t visualFrame_ = 0;
};
} // namespace vibmory
