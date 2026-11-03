#include "platform/device_profile_store.h"

#include "platform/project_store.h"
#include "platform/text_file.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

namespace vibmory
{
namespace
{
bool fieldsMatch(const QJsonObject& object, const QStringList& fields)
{
    if (object.size() != fields.size())
        return false;
    for (const auto& field : fields)
        if (!object.contains(field))
            return false;
    return true;
}

bool validInteger(const QJsonValue& value, double maximum)
{
    if (!value.isDouble())
        return false;
    const auto number = value.toDouble();
    return std::isfinite(number) && number >= 0 && number <= maximum &&
           std::floor(number) == number;
}
} // namespace

Result<DeviceProfile> loadDeviceProfile(const std::filesystem::path& path)
{
    const auto text = readTextFile(path, maxDeviceProfileBytes);
    if (!text)
        return text.error();
    const auto failure =
        [&](const std::string& message) -> Result<DeviceProfile> {
        return Error{ErrorCode::Parse,
                     toQString(path).toStdString() + ": " + message};
    };
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(
        QByteArray::fromStdString(text.value()), &error);
    if (error.error != QJsonParseError::NoError || !parsed.isObject())
        return failure("Invalid device profile JSON: " +
                       error.errorString().toStdString());
    const auto object = parsed.object();
    if (!fieldsMatch(object, {"format", "version", "name", "grid",
                              "outputDeviceId", "channels"}) ||
        object["format"] != "vibmory-device-profile" ||
        object["version"] != 1 || !object["name"].isString() ||
        !object["outputDeviceId"].isString() || !object["grid"].isObject() ||
        !object["channels"].isArray())
        return failure(
            "Unsupported device profile version, fields, or value types.");
    const auto grid = object["grid"].toObject();
    if (!fieldsMatch(grid, {"rows", "columns"}) ||
        !validInteger(grid["rows"], maxCells) ||
        !validInteger(grid["columns"], maxCells))
        return failure("Invalid profile grid dimensions.");
    DeviceProfile profile{object["name"].toString().toStdString(),
                          {static_cast<std::uint32_t>(grid["rows"].toInt()),
                           static_cast<std::uint32_t>(grid["columns"].toInt())},
                          {},
                          object["outputDeviceId"].toString().toStdString()};
    const auto channels = object["channels"].toArray();
    if (!profile.shape.validate() ||
        static_cast<std::size_t>(channels.size()) != profile.shape.cellCount())
        return failure("Profile channel count does not match its grid.");
    for (const auto& channel : channels)
    {
        if (!channel.isObject())
            return failure("Each channel must contain a cell index and gain.");
        const auto values = channel.toObject();
        if (!fieldsMatch(values, {"cell", "gain"}) ||
            !validInteger(values["cell"], maxCells - 1) ||
            !values["gain"].isDouble())
            return failure("Invalid channel fields or value types.");
        profile.routing.cells.push_back(
            static_cast<std::size_t>(values["cell"].toInt()));
        profile.routing.gains.push_back(values["gain"].toDouble());
    }
    if (auto result = profile.validate(); !result)
        return failure(result.error().message);
    return profile;
}

Result<void> saveDeviceProfile(const std::filesystem::path& path,
                               const DeviceProfile& profile)
{
    if (auto result = profile.validate(); !result)
        return result;
    if (QString::fromStdString(profile.name).toStdString() != profile.name ||
        QString::fromStdString(profile.outputDeviceId).toStdString() !=
            profile.outputDeviceId)
        return invalid("Device profile text must be valid UTF-8.");
    QJsonArray channels;
    for (std::size_t index = 0; index < profile.routing.cells.size(); ++index)
        channels.append(QJsonObject{
            {"cell", static_cast<int>(profile.routing.cells[index])},
            {"gain", profile.routing.gains[index]}});
    const QJsonObject object{
        {"format", "vibmory-device-profile"},
        {"version", 1},
        {"name", QString::fromStdString(profile.name)},
        {"grid",
         QJsonObject{{"rows", static_cast<int>(profile.shape.rows)},
                     {"columns", static_cast<int>(profile.shape.columns)}}},
        {"outputDeviceId", QString::fromStdString(profile.outputDeviceId)},
        {"channels", channels}};
    return writeTextFileAtomically(
        path, QJsonDocument(object).toJson().toStdString());
}
} // namespace vibmory
