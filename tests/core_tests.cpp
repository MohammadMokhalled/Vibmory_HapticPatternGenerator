#include "application/device_profiles.h"
#include "application/document_workflow.h"
#include "application/editor_session.h"
#include "application/playback_service.h"
#include "audio/pcm_renderer.h"
#include "persistence/project_codec.h"

#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>

using namespace vibmory;

namespace
{
struct Test
{
    const char* name;
    void (*function)();
};
std::vector<Test>& tests()
{
    static std::vector<Test> value;
    return value;
}
struct Register
{
    Register(const char* name, void (*function)())
    {
        tests().push_back({name, function});
    }
};
#define TEST(name)                               \
    void name();                                 \
    const Register register_##name(#name, name); \
    void name()
#define CHECK(...)                                                     \
    do                                                                 \
    {                                                                  \
        if (!(__VA_ARGS__))                                            \
            throw std::runtime_error(std::string(__FILE__) + ":" +     \
                                     std::to_string(__LINE__) + ": " + \
                                     #__VA_ARGS__);                    \
    } while (false)

PatternDocument makeDocument(GridShape shape = {2, 3})
{
    auto result = PatternDocument::create(shape);
    CHECK(result);
    return std::move(result.value());
}

PatternDocument distinctDocument()
{
    auto document = makeDocument();
    CHECK(document.addFrame());
    for (std::size_t frame = 0; frame < 2; ++frame)
        for (std::uint32_t row = 0; row < 2; ++row)
            for (std::uint32_t column = 0; column < 3; ++column)
                CHECK(document.setCell(
                    frame, {row, column},
                    {100 + static_cast<std::uint32_t>(frame * 100) + row * 10 +
                         column,
                     2000 + static_cast<std::uint32_t>(frame * 100) + row * 10 +
                         column}));
    return document;
}

std::uint32_t read32(const std::vector<std::byte>& bytes, std::size_t offset)
{
    std::uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result |= std::to_integer<std::uint32_t>(bytes.at(offset + i))
                  << (i * 8);
    return result;
}

TEST(domain_rectangular_indexing)
{
    const auto document = distinctDocument();
    CHECK(document.shape() == GridShape{2, 3});
    CHECK(document.cell(1, {1, 2}).value() == CellStimulus{212, 2112});
    CHECK(document.frame(1)[5] == document.cell(1, {1, 2}).value());
}

TEST(domain_invalid_shapes_indexes_and_values)
{
    CHECK(!PatternDocument::create({0, 3}));
    CHECK(!PatternDocument::create({3, 0}));
    CHECK(!PatternDocument::create({65535, 65535}));
    auto document = makeDocument();
    const auto original = document;
    CHECK(!document.cell(99, {0, 0}));
    CHECK(!document.setCell(0, {2, 0}, {100, 100}));
    CHECK(!document.removeFrame(0));
    CHECK(!document.duplicateFrame(std::numeric_limits<std::size_t>::max()));
    CHECK(!document.setCell(0, {0, 0}, {22001, 0}));
    CHECK(!document.setCell(0, {0, 0}, {100, 32768}));
    CHECK(!document.setSettings({0, 1000, 48000}));
    CHECK(!document.setSettings({10, 0, 48000}));
    CHECK(!document.setSettings({10, 1000, 0}));
    CHECK(document == original);
    CHECK(document.setCell(0, {0, 0}, {5000, 100}));
    CHECK(!document.setSettings({10, 1000, 8000}));
}

TEST(domain_frame_operations_preserve_order)
{
    auto document = distinctDocument();
    CHECK(document.duplicateFrame(0));
    CHECK(document.cell(1, {1, 2}).value().frequencyHz == 112);
    CHECK(document.cell(2, {1, 2}).value().frequencyHz == 212);
    CHECK(document.setCell(1, {1, 2}, {333, 100}));
    CHECK(document.cell(0, {1, 2}).value().frequencyHz == 112);
    CHECK(document.moveFrame(2, 0));
    CHECK(document.cell(0, {1, 2}).value().frequencyHz == 212);
    CHECK(document.removeFrame(2));
    CHECK(document.cell(1, {1, 2}).value().frequencyHz == 112);
}

TEST(domain_channel_map_and_budget_validation)
{
    auto document = makeDocument({16, 16});
    CHECK(!document.setSettings({10, 600000, 192000}));
    auto routing = document.routing();
    routing.cells[1] = routing.cells[0];
    CHECK(!document.setRouting(routing));
    routing = document.routing();
    routing.gains[0] = std::numeric_limits<double>::quiet_NaN();
    CHECK(!document.setRouting(routing));
    routing.gains[0] = 1.1;
    CHECK(!document.setRouting(routing));
}

TEST(domain_frame_identities_follow_frames)
{
    auto document = makeDocument();
    const auto first = document.frameId(0);
    CHECK(document.duplicateFrame(0));
    const auto second = document.frameId(1);
    CHECK(first != second);
    CHECK(document.frame(0) == document.frame(1));
    CHECK(document.moveFrame(0, 1));
    CHECK(document.frameId(0) == second && document.frameId(1) == first);
    CHECK(document.removeFrame(0));
    CHECK(document.addFrame());
    CHECK(document.frameId(0) == first);
    CHECK(document.frameId(1) > second);
}

TEST(domain_frame_identity_validation_is_atomic)
{
    auto document = distinctDocument();
    const auto before = document;
    for (const auto& ids :
         {std::vector<std::uint64_t>{}, {1}, {0, 2}, {1, 1}, {1, 3}})
    {
        CHECK(!document.restoreFrameIds(ids, 3));
        CHECK(document == before);
    }
    CHECK(!document.restoreFrameIds({std::vector<std::uint64_t>{1, 2}}, 0));
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    CHECK(
        document.restoreFrameIds({std::vector<std::uint64_t>{9, 7}}, maximum));
    const auto exhausted = document;
    CHECK(!document.addFrame());
    CHECK(!document.duplicateFrame(0));
    CHECK(document == exhausted);
}

TEST(codec_complete_round_trip)
{
    auto document = distinctDocument();
    CHECK(document.setSettings({7, 1234, 44100, PhasePolicy::LegacyAbsolute}));
    auto routing = document.routing();
    std::swap(routing.cells[0], routing.cells[5]);
    routing.gains[2] = 0.123456789012345;
    CHECK(document.setRouting(routing));
    auto decoded = decodeProject(encodeProject(document));
    CHECK(decoded);
    CHECK(decoded.value() == document);
}

TEST(codec_rejects_corruption_and_trailing_data)
{
    const auto text = encodeProject(distinctDocument());
    CHECK(!decodeProject(""));
    CHECK(!decodeProject("VIBMORY 999\n"));
    CHECK(!decodeProject(text + "extra"));
    CHECK(!decodeProject(text.substr(0, text.size() / 2)));
    auto negative = text;
    negative.replace(negative.find("GRID 2"), 6, "GRID -2");
    CHECK(!decodeProject(negative));
    CHECK(!decodeProject(std::string(maxProjectBytes + 1, 'x')));
}

TEST(codec_frame_identity_round_trip_and_version_one_migration)
{
    auto document = distinctDocument();
    CHECK(document.duplicateFrame(0));
    CHECK(document.removeFrame(1));
    CHECK(document.moveFrame(0, 1));
    const auto decoded = decodeProject(encodeProject(document));
    CHECK(decoded && decoded.value() == document);
    auto restored = decoded.value();
    CHECK(restored.addFrame());
    CHECK(restored.frameId(2) == document.nextFrameId());

    const auto versionOne = decodeProject(
        "VIBMORY 1\nGRID 1 1 2\nPLAYBACK 10 1000 48000 continuous\n"
        "ROUTING 1\n0 1\nFRAMES\n100 2000\n200 3000\nEND\n");
    CHECK(versionOne);
    CHECK(versionOne.value().frameId(0) == 1);
    CHECK(versionOne.value().frameId(1) == 2);
    CHECK(versionOne.value().nextFrameId() == 3);
    CHECK(decodeProject(encodeProject(versionOne.value())).value() ==
          versionOne.value());
}

TEST(codec_rejects_invalid_frame_identities)
{
    const auto text = encodeProject(distinctDocument());
    for (const auto& replacement :
         {"FRAME 0", "FRAME 1", "FRAME -1", "FRAME 18446744073709551616"})
    {
        auto corrupted = text;
        corrupted.replace(corrupted.find("FRAME 2"), 7, replacement);
        CHECK(!decodeProject(corrupted));
    }
    for (const auto& replacement :
         {"NEXT_FRAME_ID 0", "NEXT_FRAME_ID 2", "NEXT_FRAME_ID -1"})
    {
        auto corrupted = text;
        corrupted.replace(corrupted.find("NEXT_FRAME_ID 3"), 15, replacement);
        CHECK(!decodeProject(corrupted));
    }
}

TEST(codec_legacy_orientation_and_channel_compatibility)
{
    const std::string csv =
        "1, 2, 3\r\n100, 1,200,2,300,3,\r\n400,4,500,5,600,6,\r\n";
    const auto decoded =
        decodeLegacyCsv(csv, LegacyOrientation::RowsThenColumns);
    CHECK(decoded);
    CHECK(decoded.value().shape() == GridShape{2, 3});
    CHECK(decoded.value().cell(0, {1, 2}).value() == CellStimulus{600, 6});
    CHECK(decoded.value().routing().cells ==
          std::vector<std::size_t>({0, 3, 1, 4, 2, 5}));
    CHECK(decoded.value().settings().phase == PhasePolicy::LegacyAbsolute);
    CHECK(!decodeLegacyCsv(csv, LegacyOrientation::ColumnsThenRows));
}

TEST(codec_legacy_bad_numbers_never_become_zero)
{
    for (const std::string token : {"abc", "-1", "4294967296", "1x", ""})
        CHECK(!decodeLegacyCsv("1,1,1\n" + token + ",100,\n",
                               LegacyOrientation::RowsThenColumns));
    CHECK(!decodeLegacyCsv("0,1,1\n", LegacyOrientation::RowsThenColumns));
    CHECK(!decodeLegacyCsv("1,1,1\n100,2,,\n",
                           LegacyOrientation::RowsThenColumns));
}

TEST(codec_original_fixture_values)
{
    std::ifstream file(std::string(VIBMORY_TEST_RESOURCES) + "/valid.csv");
    CHECK(file);
    std::ostringstream text;
    text << file.rdbuf();
    const auto result =
        decodeLegacyCsv(text.str(), LegacyOrientation::RowsThenColumns);
    CHECK(result);
    CHECK(result.value().cell(0, {0, 0}).value() == CellStimulus{9778, 6805});
    CHECK(result.value().cell(0, {1, 1}).value() == CellStimulus{201, 4537});
}

TEST(audio_known_samples_and_little_endian)
{
    auto document = makeDocument({1, 2});
    CHECK(document.setSettings({10, 1, 8000}));
    CHECK(document.setCell(0, {0, 0}, {2000, 1000}));
    CHECK(document.setCell(0, {0, 1}, {2000, 2000}));
    auto renderer = PcmRenderer::create(document).value();
    std::vector<std::int16_t> samples(16);
    CHECK(renderer.render(samples).value() == 8);
    CHECK(samples[0] == 0 && samples[1] == 0);
    CHECK(samples[2] == 1000 && samples[3] == 2000);
    CHECK(samples[6] == -1000 && samples[7] == -2000);
    const auto bytes = littleEndianPcm(std::span(samples).subspan(6, 1));
    CHECK(std::to_integer<unsigned>(bytes[0]) == 0x18);
    CHECK(std::to_integer<unsigned>(bytes[1]) == 0xFC);
    CHECK(renderer.render(samples).value() == 0);
}

TEST(audio_rational_timing_and_phase_policy)
{
    for (const auto phase :
         {PhasePolicy::Continuous, PhasePolicy::LegacyAbsolute})
    {
        auto document = makeDocument({1, 1});
        CHECK(document.setSettings({7, 300, 8000, phase}));
        CHECK(document.addFrame());
        CHECK(document.setCell(1, {0, 0}, {2000, 1000}));
        auto renderer = PcmRenderer::create(document).value();
        std::vector<std::int16_t> samples(2400);
        CHECK(renderer.render(samples));
        CHECK(samples[1142] == 0);
        CHECK(samples[1143] == (phase == PhasePolicy::Continuous ? 0 : -1000));
        CHECK(samples[2286] == 0);
    }
    auto document = makeDocument({1, 1});
    CHECK(document.setSettings({10, 1, 8000, PhasePolicy::LegacyAbsolute}));
    CHECK(document.setCell(0, {0, 0}, {1000, 1001}));
    auto renderer = PcmRenderer::create(document).value();
    std::vector<std::int16_t> samples(8);
    CHECK(renderer.render(samples));
    CHECK(samples[1] ==
          707); // Legacy quantization truncates rather than rounds.
}

TEST(audio_reordered_and_saved_sequence_uses_document_order)
{
    auto document = makeDocument({1, 1});
    CHECK(document.setSettings({10, 200, 8000}));
    CHECK(document.setCell(0, {0, 0}, {2000, 1000}));
    CHECK(document.addFrame());
    CHECK(document.setCell(1, {0, 0}, {1000, 2000}));
    CHECK(document.moveFrame(1, 0));
    const auto loaded = decodeProject(encodeProject(document));
    CHECK(loaded);
    for (const auto& source : {document, loaded.value()})
    {
        auto renderer = PcmRenderer::create(source).value();
        std::vector<std::int16_t> samples(1600);
        CHECK(renderer.render(samples).value() == samples.size());
        CHECK(samples[1] == 1414);
        CHECK(samples[801] == 1000);
    }
}

TEST(audio_channel_mapping_and_calibration)
{
    auto document = makeDocument({1, 2});
    CHECK(document.setSettings({10, 1, 8000}));
    CHECK(document.setCell(0, {0, 0}, {2000, 1000}));
    CHECK(document.setCell(0, {0, 1}, {2000, 2000}));
    CHECK(document.setRouting({{1, 0}, {0.5, 0.25}}));
    auto renderer = PcmRenderer::create(document).value();
    std::vector<std::int16_t> samples(4);
    CHECK(renderer.render(samples));
    CHECK(samples[2] == 1000 && samples[3] == 250);
}

TEST(audio_block_size_does_not_change_output)
{
    auto document = distinctDocument();
    CHECK(document.setSettings({7, 100, 48000}));
    auto all = PcmRenderer::create(document).value();
    auto blocks = PcmRenderer::create(document).value();
    std::vector<std::int16_t> expected(
        static_cast<std::size_t>(all.totalFrames()) * all.channelCount());
    CHECK(all.render(expected));
    std::vector<std::int16_t> actual;
    std::vector<std::int16_t> block(13 * all.channelCount());
    while (blocks.position() < blocks.totalFrames())
    {
        const auto count = blocks.render(block).value() * all.channelCount();
        actual.insert(actual.end(), block.begin(),
                      block.begin() + static_cast<std::ptrdiff_t>(count));
    }
    CHECK(actual == expected);
    CHECK(!blocks.render(std::span(block).first(1)));
}

TEST(audio_wave_header_sizes_errors_and_cancellation)
{
    for (const auto shape : {GridShape{1, 1}, GridShape{1, 2}, GridShape{2, 3}})
    {
        auto document = makeDocument(shape);
        CHECK(document.setSettings({7, 1, 8000}));
        std::vector<std::byte> file;
        CHECK(renderWave(document, [&file](auto bytes) -> Result<void> {
            file.insert(file.end(), bytes.begin(), bytes.end());
            return {};
        }));
        const auto headerSize = shape.cellCount() > 2 ? 68U : 44U;
        CHECK(file.size() == headerSize + 8 * shape.cellCount() * 2);
        CHECK(read32(file, 4) == file.size() - 8);
        CHECK(read32(file, headerSize - 4) == 8 * shape.cellCount() * 2);
        if (shape.cellCount() > 2)
            CHECK(read32(file, 16) == 40);
    }
    const auto failed = renderWave(makeDocument(), [](auto) -> Result<void> {
        return Error{ErrorCode::Io, "disk full"};
    });
    CHECK(!failed && failed.error().code == ErrorCode::Io);
    const auto cancelled = renderWave(
        makeDocument(), [](auto) -> Result<void> { return {}; },
        [](auto, auto) { return false; });
    CHECK(!cancelled && cancelled.error().code == ErrorCode::Cancelled);
}

TEST(session_selection_does_not_edit)
{
    EditorSession session(distinctDocument());
    CHECK(session.selectFrame(1));
    CHECK(session.selectCell(CellPosition{1, 2}));
    CHECK(!session.dirty() && session.revision() == 0);
    CHECK(!session.selectFrame(99));
    CHECK(!session.selectCell(CellPosition{99, 0}));
    CHECK(session.selectedFrame() == 1);
}

TEST(session_frame_transactions_and_undo)
{
    EditorSession session(distinctDocument());
    CHECK(session.selectFrame(0));
    CHECK(session.duplicateFrame());
    CHECK(session.selectedFrame() == 1);
    CHECK(session.document().frameCount() == 3);
    CHECK(session.moveFrame(2, 0));
    CHECK(session.selectedFrame() == 2);
    CHECK(session.removeFrame());
    CHECK(session.document().cell(0, {0, 0}).value().frequencyHz == 200);
    session.undo();
    CHECK(session.document().frameCount() == 3);
    session.redo();
    CHECK(session.document().frameCount() == 2);
}

TEST(session_reordering_identical_frames_is_a_real_undoable_edit)
{
    auto document = makeDocument();
    CHECK(document.addFrame());
    EditorSession session(document);
    const auto selectedId = session.document().frameId(0);
    CHECK(session.selectCell(CellPosition{1, 2}));
    CHECK(session.moveFrame(0, 1));
    CHECK(session.dirty());
    CHECK(session.revision() == 1);
    CHECK(session.selectedFrame() == 1);
    CHECK(session.document().frameId(1) == selectedId);
    CHECK(session.selectedCell() == CellPosition{1, 2});
    session.undo();
    CHECK(session.selectedFrame() == 0);
    CHECK(session.document() == document);
    CHECK(!session.dirty());
    session.redo();
    CHECK(session.selectedFrame() == 1);
    CHECK(session.document().frameId(1) == selectedId);
}

TEST(session_deletes_first_middle_and_last_by_identity)
{
    for (std::size_t removed = 0; removed < 3; ++removed)
    {
        auto document = distinctDocument();
        CHECK(document.addFrame());
        CHECK(document.setCell(2, {0, 0}, {300, 3000}));
        EditorSession session(document);
        CHECK(session.selectFrame(removed));
        CHECK(session.removeFrame());
        CHECK(session.selectedFrame() < session.document().frameCount());
        std::size_t actual = 0;
        for (std::size_t old = 0; old < 3; ++old)
            if (old != removed)
                CHECK(session.document().frame(actual++) ==
                      document.frame(old));
    }
}

TEST(session_saved_revision_and_edit_groups)
{
    EditorSession session(makeDocument());
    CHECK(session.selectCell(CellPosition{0, 0}));
    session.beginEditGroup();
    CHECK(session.editCell({100, 10}));
    CHECK(session.editCell({200, 20}));
    session.endEditGroup();
    session.markSaved("test.vibmory");
    CHECK(!session.dirty());
    session.undo();
    CHECK(session.document().cell(0, {0, 0}).value().amplitude == 0);
    CHECK(session.dirty());
    session.redo();
    CHECK(!session.dirty());
    const auto revision = session.revision();
    CHECK(session.editCell({200, 20}));
    CHECK(session.revision() == revision);
}

TEST(session_recovery_is_unsaved_and_replacement_has_a_distinct_generation)
{
    EditorSession session(makeDocument());
    CHECK(session.selectFrame(0));
    CHECK(session.selectCell(CellPosition{0, 0}));
    CHECK(session.editCell({100, 500}));
    const auto generation = session.documentGeneration();
    const auto revision = session.revision();
    session.restore(distinctDocument(), "original.vibmory");
    CHECK(session.documentGeneration() == generation + 1);
    CHECK(session.revision() > revision);
    CHECK(session.dirty() && !session.canUndo() && !session.canRedo());
    CHECK(session.path() == "original.vibmory");
    CHECK(session.selectedFrame() == 0 && !session.selectedCell());
    session.markSaved("original.vibmory");
    CHECK(!session.dirty());
    CHECK(session.documentGeneration() == generation + 1);
    session.replace(makeDocument());
    CHECK(session.documentGeneration() == generation + 2);
    CHECK(!session.dirty() && session.path().empty());
}

class MemoryStore final : public DocumentStore
{
public:
    PatternDocument stored = makeDocument();
    bool failRead = false;
    bool failWrite = false;
    bool throwRead = false;
    std::function<void()> onSave;
    mutable std::optional<PatternDocument> saved;
    Result<PatternDocument> load(const std::filesystem::path&) const override
    {
        if (throwRead)
            throw std::runtime_error("test store exception");
        if (failRead)
            return Error{ErrorCode::Io, "read failed"};
        return stored;
    }
    Result<void> save(const std::filesystem::path&,
                      const PatternDocument& document) const override
    {
        if (failWrite)
            return Error{ErrorCode::Io, "write failed"};
        saved = document;
        if (onSave)
            onSave();
        return {};
    }
};

TEST(session_document_workflow_publishes_only_successful_loads)
{
    EditorSession session(distinctDocument());
    MemoryStore store;
    DocumentWorkflow workflow(session, store);
    const auto before = session.document();
    store.failRead = true;
    CHECK(!workflow.open("missing.vibmory"));
    CHECK(session.document() == before);
    store.failRead = false;
    store.throwRead = true;
    const auto exception = workflow.open("throw.vibmory");
    CHECK(!exception && exception.error().code == ErrorCode::Io);
    CHECK(session.document() == before);
    store.throwRead = false;
    CHECK(workflow.open("loaded.vibmory"));
    CHECK(session.document() == store.stored);
    CHECK(session.path() == "loaded.vibmory" && !session.dirty());
}

TEST(session_document_workflow_keeps_failed_or_stale_saves_dirty)
{
    EditorSession session(makeDocument());
    CHECK(session.selectCell(CellPosition{0, 0}));
    CHECK(session.editCell({100, 1000}));
    MemoryStore store;
    DocumentWorkflow workflow(session, store);
    store.failWrite = true;
    CHECK(!workflow.save("failed.vibmory"));
    CHECK(session.dirty() && session.path().empty());
    store.failWrite = false;
    store.onSave = [&session] { CHECK(session.editCell({200, 2000})); };
    const auto stale = workflow.save("snapshot.vibmory");
    CHECK(stale && !stale.value().savedCurrentRevision);
    CHECK(session.dirty());
    CHECK(store.saved->cell(0, {0, 0}).value().amplitude == 1000);
    store.onSave = {};
    const auto current = workflow.save("saved.vibmory");
    CHECK(current && current.value().savedCurrentRevision);
    CHECK(!session.dirty() && session.path() == "saved.vibmory");
}

class FakeBackend final : public PlaybackBackend
{
public:
    bool failStart = false;
    int starts = 0;
    int stops = 0;
    bool failSelection = false;
    int selections = 0;
    std::string selected = "original";
    std::vector<AudioDeviceDescription> choices{
        {"original", "Original"}, {"profile-device", "Profile device"}};
    std::optional<std::uint64_t> position;
    std::vector<AudioDeviceDescription> devices() const override
    {
        return choices;
    }
    std::string selectedDeviceId() const override
    {
        return selected;
    }
    Result<void> selectDeviceId(const std::string& id) override
    {
        ++selections;
        if (failSelection)
            return Error{ErrorCode::Device, "selection failed"};
        selected = id;
        return {};
    }
    Result<void> start(const DocumentSnapshot&) override
    {
        ++starts;
        if (failStart)
            return Error{ErrorCode::Device, "unsupported channels"};
        return {};
    }
    void stop() noexcept override
    {
        ++stops;
    }
    std::optional<std::uint64_t> processedSampleFrames() const override
    {
        return position;
    }
};
class FakeClock final : public MonotonicClock
{
public:
    std::uint64_t now = 0;
    std::uint64_t microseconds() const override
    {
        return now;
    }
};

TEST(
    session_device_profile_routing_is_undoable_and_output_selection_is_explicit)
{
    EditorSession session(distinctDocument());
    FakeBackend backend;
    const auto before = session.document();
    auto routing = before.routing();
    std::swap(routing.cells[0], routing.cells[5]);
    routing.gains[2] = 0.25;
    DeviceProfile profile{"Six channel calibration", before.shape(), routing,
                          "profile-device"};
    CHECK(applyDeviceProfile(session, backend, profile, false));
    CHECK(session.document().routing() == routing);
    CHECK(session.document().frame(0) == before.frame(0));
    CHECK(session.document().settings() == before.settings());
    CHECK(session.dirty());
    CHECK(backend.selected == "original" && backend.selections == 0);
    session.undo();
    CHECK(session.document() == before && !session.dirty());
    CHECK(applyDeviceProfile(session, backend, profile, true));
    CHECK(backend.selected == "profile-device" && backend.selections == 1);
    session.undo();
    CHECK(session.document() == before);
    CHECK(backend.selected == "profile-device");
}

TEST(session_invalid_or_unavailable_device_profile_never_changes_the_document)
{
    EditorSession session(distinctDocument());
    FakeBackend backend;
    const auto before = session.document();
    DeviceProfile profile{"Calibration", before.shape(), before.routing(),
                          "profile-device"};
    profile.routing.gains[0] = 0.5;
    backend.failSelection = true;
    CHECK(!applyDeviceProfile(session, backend, profile, true));
    CHECK(session.document() == before && !session.dirty());
    CHECK(backend.selected == "original");
    profile.outputDeviceId = "missing";
    CHECK(!applyDeviceProfile(session, backend, profile, true));
    CHECK(backend.selections == 1);
    profile.shape = {3, 2};
    CHECK(!applyDeviceProfile(session, backend, profile, false));
    profile.shape = before.shape();
    profile.routing.cells[1] = profile.routing.cells[0];
    CHECK(!applyDeviceProfile(session, backend, profile, false));
    CHECK(session.document() == before && session.revision() == 0);
}

TEST(domain_device_profile_rejects_invalid_metadata)
{
    const auto document = makeDocument();
    DeviceProfile profile{
        "Calibration", document.shape(), document.routing(), {}};
    CHECK(profile.validate());
    for (const auto& name : {std::string{}, std::string("  "),
                             std::string("Tab\tname"), std::string(129, 'x')})
    {
        profile.name = name;
        CHECK(!profile.validate());
    }
    profile.name = "Calibration";
    profile.outputDeviceId = std::string(1025, 'x');
    CHECK(!profile.validate());
    profile.outputDeviceId = std::string("a\0b", 3);
    CHECK(!profile.validate());
}

TEST(playback_delayed_ticks_and_exact_completion)
{
    FakeBackend backend;
    FakeClock clock;
    PlaybackService player(backend, clock);
    auto document = distinctDocument();
    CHECK(document.setSettings({7, 1000, 48000}));
    CHECK(player.start({document, 1}));
    CHECK(!player.start({document, 1}));
    clock.now = 450000;
    player.tick();
    CHECK(player.visualFrame() == 1);
    clock.now = 1000000;
    player.tick();
    CHECK(player.state() == PlaybackState::Idle && backend.stops == 1);
    player.stop();
    CHECK(backend.stops == 1);
}

TEST(playback_device_position_and_errors)
{
    FakeBackend backend;
    backend.position = 0;
    FakeClock clock;
    PlaybackService player(backend, clock);
    CHECK(player.start({distinctDocument(), 99}));
    clock.now = 10000000;
    backend.position = 4800;
    player.tick();
    CHECK(player.state() == PlaybackState::Playing &&
          player.visualFrame() == 1);
    CHECK(player.snapshot()->revision == 99);
    player.fail({ErrorCode::Device, "disconnected"});
    CHECK(player.state() == PlaybackState::Error && !player.snapshot());
}

TEST(playback_start_failure_and_snapshot_isolation)
{
    FakeBackend backend;
    FakeClock clock;
    PlaybackService player(backend, clock);
    backend.failStart = true;
    CHECK(!player.start({makeDocument(), 1}));
    CHECK(player.state() == PlaybackState::Error);
    backend.failStart = false;
    EditorSession session(makeDocument());
    CHECK(player.start(session.snapshot()));
    CHECK(session.selectCell(CellPosition{0, 0}));
    CHECK(session.editCell({100, 100}));
    CHECK(player.snapshot()->document.cell(0, {0, 0}).value().amplitude == 0);
}
} // namespace

int main(int argc, char** argv)
{
    const std::string prefix = argc > 1 ? std::string(argv[1]) + "_" : "";
    int failures = 0;
    int count = 0;
    for (const auto& test : tests())
    {
        if (!std::string_view(test.name).starts_with(prefix))
            continue;
        ++count;
        try
        {
            test.function();
            std::cout << "PASS " << test.name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
        }
    }
    std::cout << count << " tests, " << failures << " failures\n";
    return failures == 0 && count > 0 ? 0 : 1;
}
