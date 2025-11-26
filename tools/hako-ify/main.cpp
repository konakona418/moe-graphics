#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

constexpr static char TOOL_NAME[] = "hako-ify";
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

namespace details {
    namespace colors {
        const std::string RESET = "\033[0m";
        const std::string RED = "\033[31m";
        const std::string GREEN = "\033[32m";
        const std::string YELLOW = "\033[33m";
        const std::string BLUE = "\033[34m";
        const std::string MAGENTA = "\033[35m";
        const std::string CYAN = "\033[36m";
        const std::string WHITE = "\033[37m";
    }// namespace colors

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

    static inline size_t visible_length(const std::string& s) {
        size_t count = 0;
        bool in_escape = false;
        for (size_t i = 0; i < s.size(); ++i) {
            if (!in_escape) {
                if (s[i] == '\x1b') {
                    in_escape = true;
                } else {
                    count++;
                }
            } else {
                if (s[i] == 'm') {
                    in_escape = false;
                }
            }
        }
        return count;
    }

    static inline void append_rainbow_char(std::ostringstream& o, char c, float offset) {
        float r = std::sin(offset + 0.0f) * 127 + 128;
        float g = std::sin(offset + 2.0f) * 127 + 128;
        float b = std::sin(offset + 4.0f) * 127 + 128;

        o << "\x1b[38;2;" << (int) r << ";" << (int) g << ";" << (int) b << "m" << c << "\x1b[0m";
    }

    void progress_bar(float progress, const std::string& info) {
        static size_t last_visible = 0;

        int width = 50;
        int pos = progress * width;

        std::ostringstream oss;
        oss << "\r[";

        const float freq = 0.10f;
        for (int i = 0; i < width; ++i) {
            float offset = freq * i;
            char c = (i < pos ? '=' : (i == pos ? '>' : ' '));
            append_rainbow_char(oss, c, offset);
        }

        oss << "] ";

        oss << std::fixed << std::setprecision(1) << (progress * 100.0f) << "% ";
        oss << info;

        std::string out = oss.str();

        size_t vis = visible_length(out);
        if (vis < last_visible) {
            out.append(last_visible - vis, ' ');
        }

        last_visible = vis;

        std::cout << out << std::flush;
    }


    void print_rainbow(const std::string& text) {
        const float freq = 0.3f;
        for (size_t i = 0; i < text.size(); ++i) {
            float r = std::sin(freq * i + 0.0f) * 127 + 128;
            float g = std::sin(freq * i + 2.0f) * 127 + 128;
            float b = std::sin(freq * i + 4.0f) * 127 + 128;

            std::cout << "\x1b[38;2;"
                      << (int) r << ";" << (int) g << ";" << (int) b << "m"
                      << text[i];
        }
        std::cout << "\x1b[0m";
    }
}// namespace details

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
            std::cerr << details::colors::RED << "Failed to enumerate resources in directory: "
                      << inputDir << details::colors::RESET << std::endl;
            return false;
        }

        size_t currentOffset = 0;
        std::vector<HakoifierMetadataEntry> metadataEntries;

        std::ofstream outFile(outputFile, std::ios::binary);
        std::ofstream outMetaData(outputFile + ".meta", std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << details::colors::RED << "Failed to open output file: "
                      << outputFile << details::colors::RESET << std::endl;
            return false;
        }

        if (!outMetaData.is_open()) {
            std::cerr << details::colors::RED << "Failed to open output metadata file: " << outputFile
                      << ".meta" << details::colors::RESET << std::endl;
            return false;
        }

        int count = 0;
        int total = static_cast<int>(resources.size());
        std::cout << details::colors::CYAN << "Packaging " << total << " resources..."
                  << details::colors::RESET << std::endl;

        for (const auto& resourcePath: resources) {
            std::ifstream inFile(resourcePath, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << details::colors::RED << "Failed to open resource file: " << resourcePath
                          << details::colors::RESET << std::endl;
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

            details::progress_bar(
                    static_cast<float>(++count) / total,
                    details::colors::YELLOW + "Packing: " + path +
                            details::colors::RESET +
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
                  << details::colors::CYAN
                  << "Packaging complete. Writing metadata..."
                  << details::colors::RESET << std::endl;

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

    std::cout << "📦 ";
    details::print_rainbow(
            std::string(TOOL_NAME) + " v" + std::string(TOOL_VERSION) +
            std::string(" - A resource packaging tool for moe-graphics\n"));

    std::cout << details::colors::CYAN
              << "Packaging resources from '" << inputDir
              << "' into '" << outputFile
              << "'...\n"
              << details::colors::RESET;

    if (!Hakoifier::runPackaging(inputDir, outputFile)) {
        std::cerr << details::colors::RED << "Packaging failed.\n"
                  << details::colors::RESET;
        return 1;
    }

    std::cout << details::colors::GREEN << "All tasks completed successfully.\n"
              << details::colors::RESET;
    return 0;
}