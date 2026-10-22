#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace vibmory
{
enum class ErrorCode
{
    Validation,
    Parse,
    Io,
    Unsupported,
    Busy,
    Cancelled,
    Device
};

struct Error
{
    ErrorCode code;
    std::string message;
};

template<class T> class [[nodiscard]] Result
{
        public:
    Result(T value) : data_(std::move(value))
    {
    }
    Result(Error error) : data_(std::move(error))
    {
    }
    explicit operator bool() const
    {
        return std::holds_alternative<T>(data_);
    }
    T &value()
    {
        return std::get<T>(data_);
    }
    const T &value() const
    {
        return std::get<T>(data_);
    }
    const Error &error() const
    {
        return std::get<Error>(data_);
    }

        private:
    std::variant<T, Error> data_;
};

template<> class [[nodiscard]] Result<void>
{
        public:
    Result() = default;
    Result(Error error) : error_(std::move(error))
    {
    }
    explicit operator bool() const
    {
        return !error_;
    }
    const Error &error() const
    {
        return error_.value();
    }

        private:
    std::optional<Error> error_;
};

inline Error invalid(std::string message)
{
    return {ErrorCode::Validation, std::move(message)};
}
} // namespace vibmory
