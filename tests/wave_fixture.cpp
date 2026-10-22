#include "audio/pcm_renderer.h"

#include <fstream>
#include <iostream>

int main(int argc, char **argv)
{
    if(argc != 3)
        return 2;
    const auto channels = std::string_view(argv[2]) == "6" ? 6U : 2U;
    auto documentResult = vibmory::PatternDocument::create({1, channels});
    if(!documentResult)
        return 1;
    auto document = std::move(documentResult.value());
    if(!document.setSettings({7, 10, 8000}))
        return 1;
    for(std::uint32_t channel = 0; channel < channels; ++channel)
        if(!document.setCell(0, {0, channel}, {1000, 1000 * (channel + 1)}))
            return 1;
    std::ofstream output(argv[1], std::ios::binary);
    if(!output)
        return 1;
    const auto result = vibmory::renderWave(
        document, [&output](auto bytes) -> vibmory::Result<void> {
            output.write(reinterpret_cast<const char *>(bytes.data()),
                         static_cast<std::streamsize>(bytes.size()));
            if(!output)
                return vibmory::Error{vibmory::ErrorCode::Io,
                                      "Fixture output write failed."};
            return {};
        });
    output.close();
    if(!result || !output)
        return 1;
    std::cout << "Wrote " << channels << "-channel WAV fixture.\n";
    return 0;
}
