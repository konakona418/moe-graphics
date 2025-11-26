#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

constexpr static char TOOL_NAME[] = "📦 hako-ify";
constexpr static char TOOL_VERSION[] = "0.1.0";
constexpr static char USAGE_MESSAGE[] =
        "📦 hako-ify - A resource packaging tool for moe-graphics\n"
        "\n"
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
    uint64_t offset;
    uint64_t size;

    static HakoifierMetadataEntry from(
            const std::string& path,
            uint64_t offset, uint64_t size,
            bool* err = nullptr) {
        HakoifierMetadataEntry entry;
        if (path.size() > std::numeric_limits<uint16_t>::max()) {
            if (err) *err = true;
            return entry;
        }
        entry.pathLength = static_cast<uint16_t>(path.size());

        size_t padded = ((entry.pathLength + 3) / 4) * 4;
        entry.resourcePathAligned.assign(padded, '\0');
        std::memcpy(&entry.resourcePathAligned[0], path.data(), entry.pathLength);
        entry.offset = offset;
        entry.size = size;
        return entry;
    }

    std::vector<uint8_t> toBytes() const {
        // format: [u16 pathLen little-endian][padded path bytes][u64 offset LE][u64 size LE]
        std::vector<uint8_t> bytes;
        bytes.reserve(2 + resourcePathAligned.size() + 8 + 8);

        uint16_t pl = pathLength;
        bytes.push_back(static_cast<uint8_t>(pl & 0xFF));
        bytes.push_back(static_cast<uint8_t>((pl >> 8) & 0xFF));

        bytes.insert(bytes.end(), resourcePathAligned.begin(), resourcePathAligned.end());

        uint64_t off = offset;
        for (size_t i = 0; i < 8; ++i) bytes.push_back(static_cast<uint8_t>((off >> (i * 8)) & 0xFF));

        uint64_t sz = size;
        for (size_t i = 0; i < 8; ++i) bytes.push_back(static_cast<uint8_t>((sz >> (i * 8)) & 0xFF));

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
        if (dataSize < outConsumed + padded + 8 + 8) {
            if (err) *err = true;
            return {};
        };

        HakoifierMetadataEntry entry;
        entry.pathLength = pl;
        entry.resourcePathAligned.assign(reinterpret_cast<const char*>(data + outConsumed), padded);
        outConsumed += padded;

        uint64_t off = 0;
        for (size_t i = 0; i < 8; ++i) off |= static_cast<uint64_t>(data[outConsumed + i]) << (i * 8);
        outConsumed += 8;

        uint64_t sz = 0;
        for (size_t i = 0; i < 8; ++i) sz |= static_cast<uint64_t>(data[outConsumed + i]) << (i * 8);
        outConsumed += 8;

        entry.offset = off;
        entry.size = sz;
        return entry;
    }
};

struct Hakoifier {
public:
    static bool runPackaging(const std::string& inputDir, const std::string& outputFile) {
        bool err;
        auto resources = enumerateResources(inputDir, &err);
        if (err) {
            std::cerr << "Failed to enumerate resources in directory: " << inputDir << std::endl;
            return false;
        }

        size_t currentOffset = 0;
        std::vector<HakoifierMetadataEntry> metadataEntries;

        std::ofstream outFile(outputFile, std::ios::binary);
        std::ofstream outMetaData(outputFile + ".meta", std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outputFile << std::endl;
            return false;
        }

        if (!outMetaData.is_open()) {
            std::cerr << "Failed to open output metadata file: " << outputFile
                      << ".meta" << std::endl;
            return false;
        }

        int count = 0;
        int total = static_cast<int>(resources.size());
        std::cout << "Packaging " << total << " resources...\n";

        for (const auto& resourcePath: resources) {
            std::ifstream inFile(resourcePath, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << "Failed to open resource file: " << resourcePath << std::endl;
                return false;
            }

            inFile.seekg(0, std::ios::end);
            size_t fileSize = static_cast<size_t>(inFile.tellg());
            inFile.seekg(0, std::ios::beg);

            std::vector<char> buffer(fileSize);
            inFile.read(buffer.data(), fileSize);
            inFile.close();

            std::cout << "  - Adding resource: "
                      << replaceAllReversedSlashes(resourcePath)
                      << " (" << prettifyFileSize(fileSize) << "), "
                      << (++count) << " of " << total << "\n";

            outFile.write(buffer.data(), fileSize);

            HakoifierMetadataEntry entry = HakoifierMetadataEntry::from(
                    replaceAllReversedSlashes(
                            std::filesystem::relative(
                                    resourcePath,
                                    inputDir)
                                    .string()),
                    currentOffset,
                    fileSize);
            metadataEntries.push_back(entry);

            currentOffset += fileSize;
        }

        for (const auto& entry: metadataEntries) {
            auto bytes = entry.toBytes();
            outMetaData.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }

        outFile.close();
        outMetaData.close();

        return true;
    }

private:
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
        std::cout << USAGE_MESSAGE;
        return 1;
    }

    std::string inputDir = argv[1];
    std::string outputFile = argv[2];

    std::cout << TOOL_NAME << " v" << TOOL_VERSION << '\n';
    std::cout << "Packaging resources from '" << inputDir
              << "' into '" << outputFile << "'...\n";

    if (!Hakoifier::runPackaging(inputDir, outputFile)) {
        std::cerr << "Packaging failed.\n";
        return 1;
    }

    std::cout << "Packaging completed successfully.\n";
    return 0;
}