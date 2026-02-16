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

// Header-only bit matrix where each pixel is represented by a single bit (boolean).
class csMatrixBoolean : public csMatrixBase {
public:
    // Construct matrix with given size, all bits cleared.
    csMatrixBoolean(tMatrixPixelsSize width, tMatrixPixelsSize height, bool defaultOutOfBoundsValue = false)
        : outOfBoundsValue{defaultOutOfBoundsValue}, width_{width}, height_{height}, bytes_(allocate(width, height)) {}

    // Copy constructor: makes deep copy of bit buffer.
    csMatrixBoolean(const csMatrixBoolean& other)
        : outOfBoundsValue{other.outOfBoundsValue}, width_{other.width_}, height_{other.height_}, bytes_(allocate(width_, height_)) {
        copyBytes(bytes_, other.bytes_, byteCount());
    }

    // Move constructor: transfers ownership of buffer, leaving source empty.
    csMatrixBoolean(csMatrixBoolean&& other) noexcept
        : outOfBoundsValue{other.outOfBoundsValue}, width_{other.width_}, height_{other.height_}, bytes_{other.bytes_} {
        other.bytes_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.outOfBoundsValue = false;
    }

    // Copy assignment: deep copy when assigning existing object.
    csMatrixBoolean& operator=(const csMatrixBoolean& other) {
        if (this != &other) {
            resize(other.width_, other.height_);
            copyBytes(bytes_, other.bytes_, byteCount());
            outOfBoundsValue = other.outOfBoundsValue;
        }
        return *this;
    }

    // Move assignment: steal buffer from other, reset other to empty.
    csMatrixBoolean& operator=(csMatrixBoolean&& other) noexcept {
        if (this != &other) {
            delete[] bytes_;
            width_ = other.width_;
            height_ = other.height_;
            outOfBoundsValue = other.outOfBoundsValue;
            bytes_ = other.bytes_;
            other.bytes_ = nullptr;
            other.width_ = 0;
            other.height_ = 0;
            other.outOfBoundsValue = false;
        }
        return *this;
    }

    ~csMatrixBoolean() { delete[] bytes_; }

    [[nodiscard]] tMatrixPixelsSize width() const noexcept override { return width_; }
    [[nodiscard]] tMatrixPixelsSize height() const noexcept override { return height_; }

    // csMatrixBase (RGBA interface)
    [[nodiscard]] inline csColorRGBA getPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept override {
        return getValue(x, y) ? csColorRGBA{255, 255, 255, 255} : csColorRGBA{0, 0, 0, 0};
    }

    inline void setPixelRewrite(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept override {
        setValue(x, y, (color.r != 0) || (color.g != 0) || (color.b != 0));
    }

    inline void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color, uint8_t alpha) noexcept override {
        const uint8_t a = mul8(color.a, alpha);
        if (a == 0) {
            return;
        }
        setValue(x, y, (color.r != 0) || (color.g != 0) || (color.b != 0));
    }

    // Default value returned when accessing out-of-bounds coordinates.
    bool outOfBoundsValue{false};

    // Get bit value by linear index k. Returns outOfBoundsValue when out of bounds.
    [[nodiscard]] inline bool get(size_t k) const noexcept {
        if (k >= bitCount()) {
            return outOfBoundsValue;
        }
        return (bytes_[k / 8] & static_cast<uint8_t>(1U << (k % 8))) != 0;
    }

    // Set bit to 1 by linear index k. Out-of-bounds writes are silently ignored.
    inline void setbit(size_t k) noexcept {
        if (k < bitCount()) {
            bytes_[k / 8] |= static_cast<uint8_t>(1U << (k % 8));
        }
    }

    // Clear bit (set to 0) by linear index k. Out-of-bounds writes are silently ignored.
    inline void clrbit(size_t k) noexcept {
        if (k < bitCount()) {
            bytes_[k / 8] &= static_cast<uint8_t>(~(1U << (k % 8)));
        }
    }

    // Get bit value by coordinates (x, y). Returns outOfBoundsValue when out of bounds.
    [[nodiscard]] inline bool getValue(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        if (!inside(x, y)) {
            return outOfBoundsValue;
        }
        const auto xx = static_cast<size_t>(x);
        const auto yy = static_cast<size_t>(y);
        const auto k = yy * width_ + xx;
        return get(k);
    }

    // Set bit value by coordinates (x, y). Out-of-bounds writes are silently ignored.
    inline void setValue(tMatrixPixelsCoord x, tMatrixPixelsCoord y, bool value) noexcept {
        if (!inside(x, y)) {
            return;
        }
        const auto xx = static_cast<size_t>(x);
        const auto yy = static_cast<size_t>(y);
        const auto k = yy * width_ + xx;
        if (value) {
            setbit(k);
        } else {
            clrbit(k);
        }
    }

    // Clear matrix: set all bits to 0.
    inline void clear() noexcept {
        const size_t bytes = byteCount();
        if (bytes != 0 && bytes_) {
            memset(static_cast<void*>(bytes_), 0, bytes);
        }
    }

private:
    tMatrixPixelsSize width_;
    tMatrixPixelsSize height_;
    uint8_t* bytes_{nullptr};

    [[nodiscard]] inline bool inside(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        return x >= 0 && y >= 0 && x < static_cast<tMatrixPixelsCoord>(width_) && y < static_cast<tMatrixPixelsCoord>(height_);
    }

    [[nodiscard]] inline size_t bitCount() const noexcept {
        return static_cast<size_t>(width_) * static_cast<size_t>(height_);
    }

    [[nodiscard]] inline size_t byteCount() const noexcept {
        return (bitCount() + 7) / 8;
    }

    [[nodiscard]] static uint8_t* allocate(uint16_t w, uint16_t h) {
        const size_t bits = static_cast<size_t>(w) * static_cast<size_t>(h);
        const size_t bytes = (bits + 7) / 8;
        uint8_t* buf = bytes > 0 ? new uint8_t[bytes] : nullptr;
        if (buf && bytes > 0) {
            memset(static_cast<void*>(buf), 0, bytes);
        }
        return buf;
    }

    static void copyBytes(uint8_t* dst, const uint8_t* src, size_t n) {
        if (n > 0 && dst && src) {
            memcpy(static_cast<void*>(dst), static_cast<const void*>(src), n);
        }
    }

    void resize(uint16_t w, uint16_t h) {
        if (w == width_ && h == height_) {
            return;
        }
        delete[] bytes_;
        width_ = w;
        height_ = h;
        bytes_ = allocate(width_, height_);
    }
};

} // namespace amp

