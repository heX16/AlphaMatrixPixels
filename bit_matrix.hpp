#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "matrix_types.hpp"
#include "rect.hpp"

namespace amp {

using ::size_t;
using ::uint8_t;

// Header-only bit matrix where each pixel is represented by a single bit (boolean).
class csBitMatrix {
public:
    // Construct matrix with given size, all bits cleared.
    csBitMatrix(tMatrixPixelsSize width, tMatrixPixelsSize height, bool defaultOutOfBoundsValue = false)
        : outOfBoundsValue{defaultOutOfBoundsValue}, width_{width}, height_{height}, bytes_(allocate(width, height)) {}

    // Copy constructor: makes deep copy of bit buffer.
    csBitMatrix(const csBitMatrix& other)
        : outOfBoundsValue{other.outOfBoundsValue}, width_{other.width_}, height_{other.height_}, bytes_(allocate(width_, height_)) {
        copyBytes(bytes_, other.bytes_, byteCount());
    }

    // Move constructor: transfers ownership of buffer, leaving source empty.
    csBitMatrix(csBitMatrix&& other) noexcept
        : outOfBoundsValue{other.outOfBoundsValue}, width_{other.width_}, height_{other.height_}, bytes_{other.bytes_} {
        other.bytes_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.outOfBoundsValue = false;
    }

    // Copy assignment: deep copy when assigning existing object.
    csBitMatrix& operator=(const csBitMatrix& other) {
        if (this != &other) {
            resize(other.width_, other.height_);
            copyBytes(bytes_, other.bytes_, byteCount());
            outOfBoundsValue = other.outOfBoundsValue;
        }
        return *this;
    }

    // Move assignment: steal buffer from other, reset other to empty.
    csBitMatrix& operator=(csBitMatrix&& other) noexcept {
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

    ~csBitMatrix() { delete[] bytes_; }

    [[nodiscard]] tMatrixPixelsSize width() const noexcept { return width_; }
    [[nodiscard]] tMatrixPixelsSize height() const noexcept { return height_; }

    // Return full matrix bounds as a rectangle: (0, 0, width, height).
    [[nodiscard]] inline csRect getRect() const noexcept { return csRect{0, 0, width(), height()}; }

    // Default value returned when accessing out-of-bounds coordinates.
    bool outOfBoundsValue{false};

    // Get bit value by linear index k. Returns outOfBoundsValue when out of bounds.
    [[nodiscard]] inline bool get(uint16_t k) const noexcept {
        if (k >= bitCount()) {
            return outOfBoundsValue;
        }
        return (bytes_[k / 8] & (1U << (k % 8))) != 0;
    }

    // Set bit to 1 by linear index k. Out-of-bounds writes are silently ignored.
    inline void setbit(uint16_t k) noexcept {
        if (k < bitCount()) {
            bytes_[k / 8] |= (1U << (k % 8));
        }
    }

    // Clear bit (set to 0) by linear index k. Out-of-bounds writes are silently ignored.
    inline void clrbit(uint16_t k) noexcept {
        if (k < bitCount()) {
            bytes_[k / 8] &= ~(1U << (k % 8));
        }
    }

    // Get bit value by coordinates (x, y). Returns outOfBoundsValue when out of bounds.
    [[nodiscard]] inline bool getPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        if (!inside(x, y)) {
            return outOfBoundsValue;
        }
        const uint16_t k = static_cast<uint16_t>(y) * width_ + static_cast<uint16_t>(x);
        return get(k);
    }

    // Set bit value by coordinates (x, y). Out-of-bounds writes are silently ignored.
    inline void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, bool value) noexcept {
        if (!inside(x, y)) {
            return;
        }
        const uint16_t k = static_cast<uint16_t>(y) * width_ + static_cast<uint16_t>(x);
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

