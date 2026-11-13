#include "gallery/view/Placement.hpp"

#include <algorithm>

namespace gallery {

namespace {

bool positive(float v) noexcept {
    return v > 0.0f;   // false for NaN as well
}

} // namespace

Placement placeImage(Size image, Rect slot, Fit fit) noexcept {
    if (!positive(image.width) || !positive(image.height) || !positive(slot.width) ||
        !positive(slot.height)) {
        return {};
    }
    const Rect full{0.0f, 0.0f, image.width, image.height};
    switch (fit) {
    case Fit::Cover: {
        const float s = std::max(slot.width / image.width, slot.height / image.height);
        const float srcW = slot.width / s;
        const float srcH = slot.height / s;
        return {{(image.width - srcW) / 2.0f, (image.height - srcH) / 2.0f, srcW, srcH}, slot};
    }
    case Fit::Contain: {
        const float s = std::min(slot.width / image.width, slot.height / image.height);
        const float w = image.width * s;
        const float h = image.height * s;
        return {full, {slot.x + (slot.width - w) / 2.0f, slot.y + (slot.height - h) / 2.0f, w, h}};
    }
    case Fit::FitWidth: {
        const float s = slot.width / image.width;
        return {full, {slot.x, slot.y, slot.width, image.height * s}};
    }
    case Fit::FitHeight: {
        const float s = slot.height / image.height;
        return {full, {slot.x, slot.y, image.width * s, slot.height}};
    }
    }
    return {};
}

} // namespace gallery
