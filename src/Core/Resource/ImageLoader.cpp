#include "Core/Resource/ImageLoader.hpp"

#include <stb_image.h>

MOE_BEGIN_NAMESPACE

namespace Detail {
    bool loadImageFromMemory(
            const uint8_t* data,
            size_t size,
            Vector<uint8_t>& outImageData,
            int& outWidth,
            int& outHeight,
            int& outChannels,
            int desiredChannels) {
        int width, height, channels;
        uint8_t* imgData = stbi_load_from_memory(
                data,
                static_cast<int>(size),
                &width,
                &height,
                &channels,
                desiredChannels);
        if (!imgData) {
            Logger::error(
                    "Failed to load image from memory: {}",
                    stbi_failure_reason());
            return false;
        }

        outWidth = width;
        outHeight = height;
        outChannels = desiredChannels > 0 ? desiredChannels : channels;
        size_t dataSize =
                static_cast<size_t>(width) *
                static_cast<size_t>(height) *
                static_cast<size_t>(outChannels);
        outImageData.resize(dataSize);
        std::memcpy(outImageData.data(), imgData, dataSize);
        stbi_image_free(imgData);
        return true;
    }
}// namespace Detail

MOE_END_NAMESPACE