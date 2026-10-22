#include "audio/pcm_renderer.h"

#include <chrono>
#include <iostream>

int main()
{
    auto created = vibmory::PatternDocument::create({4, 4});
    if (!created)
        return 1;
    auto document = std::move(created.value());
    if (!document.setSettings({25, 10000, 48000}))
        return 1;
    for (std::uint32_t row = 0; row < 4; ++row)
        for (std::uint32_t column = 0; column < 4; ++column)
            if (!document.setCell(0, {row, column},
                                  {100 + row * 10 + column, 16000}))
                return 1;
    auto result = vibmory::PcmRenderer::create(document);
    if (!result)
        return 1;
    auto renderer = std::move(result.value());
    std::vector<std::int16_t> block(4096 * 16);
    std::uint64_t checksum = 0;
    const auto started = std::chrono::steady_clock::now();
    while (renderer.position() < renderer.totalFrames())
    {
        const auto rendered = renderer.render(block);
        if (!rendered)
            return 1;
        checksum += static_cast<std::uint16_t>(block[0]);
    }
    const auto elapsed = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - started)
                             .count();
    std::cout << "16 channels, 48000 Hz, 10 seconds: " << elapsed
              << " wall seconds; " << (10 / elapsed) << "x realtime; checksum "
              << checksum << '\n';
    // Informational only: hardware-dependent speed is not a pass/fail gate.
    return 0;
}
