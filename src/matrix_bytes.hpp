#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "matrix_base.hpp"
#include "matrix_types.hpp"
#include "rect.hpp"

namespace amp {

using ::size_t;
using ::uint8_t;

// Header-only byte matrix where each pixel is represented by a single uint8_t.
class csMatrixBytes : public csMatrixBase {
public:
    // Construct matrix with given size, all bytes cleared to 0.
    csMatrixBytes(tMatrixPixelsSize width, tMatrixPixelsSize height, uint8_t defaultOutOfBoundsValue = 0)
        : outOfBoundsValue{defaultOutOfBoundsValue}, width_{width}, height_{height}, bytes_(allocate(width, height)) {}

    // Copy constructor: makes deep copy of byte buffer.
    csMatrixBytes(const csMatrixBytes& other)
        : outOfBoundsValue{other.outOfBoundsValue}, width_{other.width_}, height_{other.height_}, bytes_(allocate(width_, height_)) {
        copyBytes(bytes_, other.bytes_, count());
    }

    // Move constructor: transfers ownership of buffer, leaving source empty.
    csMatrixBytes(csMatrixBytes&& other) noexcept
        : outOfBoundsValue{other.outOfBoundsValue}, width_{other.width_}, height_{other.height_}, bytes_{other.bytes_} {
        other.bytes_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.outOfBoundsValue = 0;
    }

    // Copy assignment: deep copy when assigning existing object.
    csMatrixBytes& operator=(const csMatrixBytes& other) {
        if (this != &other) {
            resize(other.width_, other.height_);
            copyBytes(bytes_, other.bytes_, count());
            outOfBoundsValue = other.outOfBoundsValue;
        }
        return *this;
    }

    // Move assignment: steal buffer from other, reset other to empty.
    csMatrixBytes& operator=(csMatrixBytes&& other) noexcept {
        if (this != &other) {
            delete[] bytes_;
            width_ = other.width_;
            height_ = other.height_;
            outOfBoundsValue = other.outOfBoundsValue;
            bytes_ = other.bytes_;
            other.bytes_ = nullptr;
            other.width_ = 0;
            other.height_ = 0;
            other.outOfBoundsValue = 0;
        }
        return *this;
    }

    ~csMatrixBytes() { delete[] bytes_; }

    [[nodiscard]] tMatrixPixelsSize width() const noexcept override { return width_; }
    [[nodiscard]] tMatrixPixelsSize height() const noexcept override { return height_; }

    // csMatrixBase (RGBA interface)
    [[nodiscard]] inline csColorRGBA getPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept override {
        const uint8_t v = getValue(x, y);
        return csColorRGBA{255, v, v, v};
    }

    inline void setPixelRewrite(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept override {
        const uint8_t rg = (color.r > color.g) ? color.r : color.g;
        const uint8_t i = (rg > color.b) ? rg : color.b;
        setValue(x, y, i);
    }

    inline void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color, uint8_t alpha) noexcept override {
        const uint8_t a = mul8(color.a, alpha);
        if (a == 0) {
            return;
        }

        const uint8_t rg = (color.r > color.g) ? color.r : color.g;
        const uint8_t src = (rg > color.b) ? rg : color.b;
        const uint8_t dst = getValue(x, y);

        const int32_t diff = static_cast<int32_t>(src) - static_cast<int32_t>(dst);
        const int32_t out32 = static_cast<int32_t>(dst) + (diff * static_cast<int32_t>(a) + 127) / 255;
        const uint8_t out = (out32 <= 0) ? 0 : (out32 >= 255) ? 255 : static_cast<uint8_t>(out32);
        setValue(x, y, out);
    }

    // Default value returned when accessing out-of-bounds coordinates.
    uint8_t outOfBoundsValue{0};

    // Get byte value by linear index k. Returns outOfBoundsValue when out of bounds.
    [[nodiscard]] inline uint8_t get(size_t k) const noexcept {
        if (k >= count()) {
            return outOfBoundsValue;
        }
        return bytes_[k];
    }

    // Set byte value by linear index k. Out-of-bounds writes are silently ignored.
    inline void set(size_t k, uint8_t value) noexcept {
        if (k < count()) {
            bytes_[k] = value;
        }
    }

    // Get byte value by coordinates (x, y). Returns outOfBoundsValue when out of bounds.
    [[nodiscard]] inline uint8_t getValue(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        if (!inside(x, y)) {
            return outOfBoundsValue;
        }
        return bytes_[index(x, y)];
    }

    // Set byte value by coordinates (x, y). Out-of-bounds writes are silently ignored.
    inline void setValue(tMatrixPixelsCoord x, tMatrixPixelsCoord y, uint8_t value) noexcept {
        if (!inside(x, y)) {
            return;
        }
        bytes_[index(x, y)] = value;
    }

    // Clear matrix: set all bytes to 0.
    inline void clear() noexcept {
        const size_t n = count();
        if (n != 0 && bytes_) {
            memset(static_cast<void*>(bytes_), 0, n);
        }
    }

    // Resize matrix to new dimensions. Existing bytes are lost (matrix is cleared).
    void resize(tMatrixPixelsSize w, tMatrixPixelsSize h) {
        if (w == width_ && h == height_) {
            return;
        }
        delete[] bytes_;
        width_ = w;
        height_ = h;
        bytes_ = allocate(width_, height_);
    }

private:
    tMatrixPixelsSize width_;
    tMatrixPixelsSize height_;
    uint8_t* bytes_{nullptr};

    [[nodiscard]] inline bool inside(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        return x >= 0 && y >= 0 && x < static_cast<tMatrixPixelsCoord>(width_) && y < static_cast<tMatrixPixelsCoord>(height_);
    }

    [[nodiscard]] inline size_t index(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        const auto xx = static_cast<size_t>(x);
        const auto yy = static_cast<size_t>(y);
        return yy * width_ + xx;
    }

    [[nodiscard]] inline size_t count() const noexcept {
        return static_cast<size_t>(width_) * static_cast<size_t>(height_);
    }

    [[nodiscard]] static uint8_t* allocate(uint16_t w, uint16_t h) {
        const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);
        uint8_t* buf = n > 0 ? new uint8_t[n] : nullptr;
        if (buf && n > 0) {
            memset(static_cast<void*>(buf), 0, n);
        }
        return buf;
    }

    static void copyBytes(uint8_t* dst, const uint8_t* src, size_t n) {
        if (n > 0 && dst && src) {
            memcpy(static_cast<void*>(dst), static_cast<const void*>(src), n);
        }
    }
};

} // namespace amp
