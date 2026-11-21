#include "data/PngExporter.h"

#include <zlib.h>

#include <array>
#include <fstream>
#include <vector>

namespace argmap {
namespace data {
namespace {

std::array<unsigned int, 256> makeCrcTable() {
    std::array<unsigned int, 256> table{};
    for (unsigned int n = 0; n < 256; ++n) {
        unsigned int c = n;
        for (int k = 0; k < 8; ++k) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        table[n] = c;
    }
    return table;
}

void appendBigEndian(std::vector<unsigned char>& out, unsigned int value) {
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    out.push_back(static_cast<unsigned char>(value & 0xFF));
}

void appendChunk(std::vector<unsigned char>& out, const char* name,
                 const std::vector<unsigned char>& data) {
    static const std::array<unsigned int, 256> crcTable = makeCrcTable();
    appendBigEndian(out, static_cast<unsigned int>(data.size()));
    const std::size_t nameStart = out.size();
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<unsigned char>(name[i]));
    }
    out.insert(out.end(), data.begin(), data.end());

    unsigned int crc = 0xFFFFFFFFu;
    for (std::size_t i = nameStart; i < out.size(); ++i) {
        crc = crcTable[(crc ^ out[i]) & 0xFF] ^ (crc >> 8);
    }
    crc ^= 0xFFFFFFFFu;
    appendBigEndian(out, crc);
}

} // namespace

bool PngExporter::writePng(const std::string& path, const unsigned char* pixels,
                           int width, int height, int channels, std::string* error) {
    if (width <= 0 || height <= 0 || !pixels) {
        if (error) *error = "Cannot export an empty image.";
        return false;
    }
    if (channels != 3 && channels != 4) {
        if (error) *error = "PNG export supports RGB and RGBA images only.";
        return false;
    }

    // One filter byte per scanline, then all pixel rows.
    const std::size_t stride = static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    std::vector<unsigned char> raw;
    raw.reserve((stride + 1) * static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0); // filter type: none
        const unsigned char* row = pixels + static_cast<std::size_t>(y) * stride;
        raw.insert(raw.end(), row, row + stride);
    }

    uLongf compressedSize = compressBound(static_cast<uLong>(raw.size()));
    std::vector<unsigned char> compressed(compressedSize);
    const int result = compress2(compressed.data(), &compressedSize, raw.data(),
                                 static_cast<uLong>(raw.size()), Z_BEST_COMPRESSION);
    if (result != Z_OK) {
        if (error) *error = "Could not compress PNG image data.";
        return false;
    }
    compressed.resize(compressedSize);

    std::vector<unsigned char> png;
    static const unsigned char signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    png.insert(png.end(), signature, signature + sizeof(signature));

    std::vector<unsigned char> ihdr;
    appendBigEndian(ihdr, static_cast<unsigned int>(width));
    appendBigEndian(ihdr, static_cast<unsigned int>(height));
    ihdr.push_back(8);                    // bit depth
    ihdr.push_back(channels == 4 ? 6 : 2); // colour type
    ihdr.push_back(0);                    // compression
    ihdr.push_back(0);                    // filter
    ihdr.push_back(0);                    // interlace
    appendChunk(png, "IHDR", ihdr);
    appendChunk(png, "IDAT", compressed);
    appendChunk(png, "IEND", {});

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        if (error) *error = "Could not open '" + path + "' for image export.";
        return false;
    }
    file.write(reinterpret_cast<const char*>(png.data()),
               static_cast<std::streamsize>(png.size()));
    if (!file) {
        if (error) *error = "Failed while writing '" + path + "'.";
        return false;
    }
    return true;
}

} // namespace data
} // namespace argmap
