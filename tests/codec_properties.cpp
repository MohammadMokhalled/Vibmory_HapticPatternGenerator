#include "persistence/project_codec.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>

namespace
{
void verify(std::string_view input)
{
    auto decoded = vibmory::decodeProject(input);
    if(decoded)
    {
        const auto roundTrip =
            vibmory::decodeProject(vibmory::encodeProject(decoded.value()));
        if(!roundTrip || roundTrip.value() != decoded.value())
            throw std::runtime_error(
                "Accepted project failed canonical round trip.");
    }
    for(const auto orientation : {vibmory::LegacyOrientation::RowsThenColumns,
                                  vibmory::LegacyOrientation::ColumnsThenRows})
    {
        auto legacy = vibmory::decodeLegacyCsv(input, orientation);
        if(legacy && !legacy.value().validateOutput())
            throw std::runtime_error(
                "Accepted CSV violates document output invariants.");
    }
}
} // namespace

int main()
{
    std::mt19937 random(0x564942U);
    const auto draw = [&random] {
        return static_cast<std::uint32_t>(random());
    };
    for(unsigned iteration = 0; iteration < 500; ++iteration)
    {
        const vibmory::GridShape shape{1 + draw() % 5, 1 + draw() % 7};
        auto created = vibmory::PatternDocument::create(shape);
        if(!created)
            return 1;
        auto document = std::move(created.value());
        for(unsigned index = 0; index < iteration % 5; ++index)
            if(!document.addFrame())
                return 1;
        for(std::size_t frame = 0; frame < document.frameCount(); ++frame)
            for(std::uint32_t row = 0; row < shape.rows; ++row)
                for(std::uint32_t column = 0; column < shape.columns; ++column)
                    if(!document.setCell(frame, {row, column},
                                         {draw() % 22001, draw() % 32768}))
                        return 1;
        auto routing = document.routing();
        std::shuffle(routing.cells.begin(), routing.cells.end(), random);
        for(auto &gain : routing.gains)
            gain = static_cast<double>(random() % 10001) / 10000;
        if(!document.setRouting(routing))
            return 1;
        const auto original = vibmory::encodeProject(document);
        auto parsed = vibmory::decodeProject(original);
        if(!parsed || parsed.value() != document)
            return 1;
        verify(original);
        for(unsigned mutation = 0; mutation < 20; ++mutation)
        {
            auto corrupted = original;
            const auto position = random() % corrupted.size();
            switch(mutation % 4)
            {
            case 0:
                corrupted[position] = static_cast<char>(random() % 256);
                break;
            case 1:
                corrupted.resize(position);
                break;
            case 2:
                corrupted.insert(position, 1,
                                 static_cast<char>(random() % 256));
                break;
            default:
                corrupted.erase(position, 1);
                break;
            }
            verify(corrupted);
        }
    }
    std::cout << "500 generated complete round trips and 10000 parser "
                 "mutations passed.\n";
    return 0;
}
