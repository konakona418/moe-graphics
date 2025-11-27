#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../common/fancy.hpp"

constexpr static char TOOL_NAME[] = "hako-ify";
constexpr static char TOOL_VERSION[] = "0.1.0";
constexpr static char USAGE_MESSAGE[] =
        "Usage: hako-ify <input_directory> <output_file>\n"
        "\n"
        "Arguments:\n"
        "  <input_directory>   The directory containing resources to package.\n"
        "  <output_file>       The output file to create.\n"
        "\n"
        "Example:\n"
        "  hako-ify ./assets resources.pkg\n";

std::string replaceAllReversedSlashes(const std::string& input) {
    std::string output = input;
    size_t pos = 0;
    while ((pos = output.find('\\', pos)) != std::string::npos) {
        output.replace(pos, 1, "/");
        pos += 1;
    }
    return output;
}

std::string prettifyFileSize(size_t sizeInBytes) {
    constexpr size_t KB = 1024;
    constexpr size_t MB = 1024 * KB;
    constexpr size_t GB = 1024 * MB;

    char buffer[32];
    if (sizeInBytes >= GB) {
        std::snprintf(buffer, sizeof(buffer), "%.2f GB", static_cast<float>(sizeInBytes) / GB);
    } else if (sizeInBytes >= MB) {
        std::snprintf(buffer, sizeof(buffer), "%.2f MB", static_cast<float>(sizeInBytes) / MB);
    } else if (sizeInBytes >= KB) {
        std::snprintf(buffer, sizeof(buffer), "%.2f KB", static_cast<float>(sizeInBytes) / KB);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%zu bytes", sizeInBytes);
    }
    return std::string(buffer);
}

struct HakoifierMetadataEntry {
    uint16_t pathLength;
    std::string resourcePathAligned;
    uint32_t offset;
    uint32_t size;

    static HakoifierMetadataEntry from(
            const std::string& path,
            uint32_t offset, uint32_t size,
            bool* err = nullptr) {
        HakoifierMetadataEntry entry;
        if (path.size() > std::numeric_limits<uint16_t>::max()) {
            if (err) *err = true;
            return entry;
        }

        size_t pathSize = path.size();
        size_t padded = ((pathSize + 3) / 4) * 4;
        entry.resourcePathAligned.assign(padded, '\0');
        std::memcpy(&entry.resourcePathAligned[0], path.data(), pathSize);

        entry.pathLength = static_cast<uint16_t>(padded);
        entry.offset = offset;
        entry.size = size;
        return entry;
    }

    std::vector<uint8_t> toBytes() const {
        // format: [u16 pathLen little-endian][padded path bytes][u32 offset LE][u32 size LE]
        std::vector<uint8_t> bytes;
        bytes.reserve(2 + resourcePathAligned.size() + 4 + 4);

        uint16_t pl = pathLength;
        bytes.push_back(static_cast<uint8_t>(pl & 0xFF));
        bytes.push_back(static_cast<uint8_t>((pl >> 8) & 0xFF));

        bytes.insert(bytes.end(), resourcePathAligned.begin(), resourcePathAligned.end());

        uint32_t off = offset;
        for (size_t i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>((off >> (i * 8)) & 0xFF));

        uint32_t sz = size;
        for (size_t i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>((sz >> (i * 8)) & 0xFF));

        return bytes;
    }

    static HakoifierMetadataEntry fromBytes(
            const uint8_t* data,
            size_t dataSize, size_t& outConsumed,
            bool* err = nullptr) {
        outConsumed = 0;
        if (dataSize < 2) {
            if (err) *err = true;
            return {};
        }

        uint16_t pl = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
        outConsumed = 2;

        size_t padded = ((static_cast<size_t>(pl) + 3) / 4) * 4;
        if (dataSize < outConsumed + padded + 4 + 4) {
            if (err) *err = true;
            return {};
        };

        HakoifierMetadataEntry entry;
        entry.pathLength = pl;
        entry.resourcePathAligned.assign(reinterpret_cast<const char*>(data + outConsumed), padded);
        outConsumed += padded;

        uint32_t off = 0;
        for (size_t i = 0; i < 4; ++i) off |= static_cast<uint32_t>(data[outConsumed + i]) << (i * 8);
        outConsumed += 4;

        uint32_t sz = 0;
        for (size_t i = 0; i < 4; ++i) sz |= static_cast<uint32_t>(data[outConsumed + i]) << (i * 8);
        outConsumed += 4;
        entry.offset = off;
        entry.size = sz;
        return entry;
    }
};


// Metadata format:
// [u16 numEntries LE][entries...]
// Metadata entry format:
// [u16 pathLen LE][padded path bytes][u32 offset LE][u32 size LE]
struct Hakoifier {
public:
    static bool runPackaging(const std::string& inputDir, const std::string& outputFile) {
        bool err;
        auto resources = enumerateResources(inputDir, &err);
        if (err) {
            std::cerr << fancy::colors::RED << "Failed to enumerate resources in directory: "
                      << inputDir << fancy::colors::RESET << std::endl;
            return false;
        }

        if (resources.empty()) {
            std::cerr << fancy::colors::YELLOW << "No resources found in directory: "
                      << inputDir << fancy::colors::RESET << std::endl;
            return true;
        }

        if (resources.size() > std::numeric_limits<uint16_t>::max()) {
            std::cerr << fancy::colors::RED << "Too many resources to package (max "
                      << std::numeric_limits<uint16_t>::max() << "): "
                      << resources.size() << fancy::colors::RESET << std::endl;
            return false;
        }

        size_t currentOffset = 0;
        std::vector<HakoifierMetadataEntry> metadataEntries;

        std::ofstream outFile(outputFile, std::ios::binary);
        std::ofstream outMetaData(outputFile + ".meta", std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << fancy::colors::RED << "Failed to open output file: "
                      << outputFile << fancy::colors::RESET << std::endl;
            return false;
        }

        if (!outMetaData.is_open()) {
            std::cerr << fancy::colors::RED << "Failed to open output metadata file: " << outputFile
                      << ".meta" << fancy::colors::RESET << std::endl;
            return false;
        }

        int count = 0;
        int total = static_cast<int>(resources.size());
        std::cout << fancy::colors::CYAN << "Packaging " << total << " resources..."
                  << fancy::colors::RESET << std::endl;

        for (const auto& resourcePath: resources) {
            std::ifstream inFile(resourcePath, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << fancy::colors::RED << "Failed to open resource file: " << resourcePath
                          << fancy::colors::RESET << std::endl;
                return false;
            }

            inFile.seekg(0, std::ios::end);
            size_t fileSize = static_cast<size_t>(inFile.tellg());
            inFile.seekg(0, std::ios::beg);

            std::vector<char> buffer(fileSize);
            inFile.read(buffer.data(), fileSize);
            inFile.close();

            auto path = replaceAllReversedSlashes(
                    std::filesystem::relative(
                            resourcePath,
                            inputDir)
                            .string());

            fancy::progressBar(
                    static_cast<float>(++count) / total,
                    fancy::colors::YELLOW + "Packing: " + path +
                            fancy::colors::RESET +
                            " (" + prettifyFileSize(fileSize) + ")");

            outFile.write(buffer.data(), fileSize);

            HakoifierMetadataEntry entry = HakoifierMetadataEntry::from(
                    path,
                    currentOffset,
                    fileSize);
            metadataEntries.push_back(entry);

            currentOffset += fileSize;
        }
        std::cout << std::endl
                  << fancy::colors::CYAN
                  << "Packaging complete. Writing metadata..."
                  << fancy::colors::RESET << std::endl;

        writeU16(outMetaData, metadataEntries.size());
        for (const auto& entry: metadataEntries) {
            auto bytes = entry.toBytes();
            outMetaData.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }

        outFile.close();
        outMetaData.close();

        return true;
    }

private:
    static void writeU16(std::ofstream& outFile, uint16_t value) {
        outFile.put(static_cast<char>(value & 0xFF));
        outFile.put(static_cast<char>((value >> 8) & 0xFF));
    }

    static std::vector<std::string> enumerateResources(
            const std::string& inputDir, bool* err = nullptr) {
        std::filesystem::path inputPath{inputDir};
        std::vector<std::string> resources;

        if (!std::filesystem::exists(inputPath)) {
            if (err) *err = true;
            return resources;
        }

        for (const auto& entry: std::filesystem::recursive_directory_iterator(inputPath)) {
            if (entry.is_regular_file()) {
                resources.push_back(entry.path().string());
            }
        }
        return resources;
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "📦 ";
        fancy::printRainbow(
                std::string(TOOL_NAME) + " v" + std::string(TOOL_VERSION) +
                std::string(" - A resource packaging tool for moe-graphics\n"));
        std::cout << fancy::colors::CYAN << USAGE_MESSAGE << fancy::colors::RESET;
        return 1;
    }

    std::string inputDir = argv[1];
    std::string outputFile = argv[2];

    std::cout << "📦 ";
    fancy::printRainbow(
            std::string(TOOL_NAME) + " v" + std::string(TOOL_VERSION) +
            std::string(" - A resource packaging tool for moe-graphics\n"));

    std::cout << fancy::colors::MAGENTA
              << "Packaging resources from '" << inputDir
              << "' into '" << outputFile
              << "'...\n"
              << fancy::colors::RESET;

    if (!Hakoifier::runPackaging(inputDir, outputFile)) {
        std::cerr << fancy::colors::RED << "Packaging failed.\n"
                  << fancy::colors::RESET;
        return 1;
    }

    std::cout << fancy::colors::GREEN << "All tasks completed successfully.\n"
              << fancy::colors::RESET;
    return 0;
}