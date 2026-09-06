#include "core/livering.h"

#include <algorithm>
#include <cmath>

namespace svj {

void LiveRing::configure(std::size_t capacity) {
    capacity_ = capacity < 1 ? 1 : capacity;
    times_.assign(capacity_, 0.0);
    head_ = 0;
    size_ = 0;
}

void LiveRing::reset() {
    head_ = 0;
    size_ = 0;
}

std::size_t LiveRing::slot_of(std::size_t age_index) const {
    // age_index 0 is the oldest frame held. When the ring has wrapped the oldest
    // sits just after the head; before that it is at zero.
    const std::size_t first = size_ == capacity_ ? head_ : 0;
    return (first + age_index) % capacity_;
}

std::size_t LiveRing::push(double capture_s) {
    if (capacity_ == 0) configure(1);

    // A time that does not advance means the source restarted, its clock reset,
    // or two frames carry the same stamp. Searching a ring whose times are not
    // increasing silently returns nonsense, so the ring starts again instead.
    if (size_ > 0 && capture_s <= newest_s()) {
        head_ = 0;
        size_ = 0;
    }

    const std::size_t slot = head_;
    times_[slot] = capture_s;
    head_ = (head_ + 1) % capacity_;
    if (size_ < capacity_) ++size_;
    return slot;
}

double LiveRing::newest_s() const {
    if (size_ == 0) return 0.0;
    return times_[(head_ + capacity_ - 1) % capacity_];
}

double LiveRing::oldest_s() const {
    if (size_ == 0) return 0.0;
    return times_[slot_of(0)];
}

double LiveRing::span_s() const {
    if (size_ < 2) return 0.0;
    return newest_s() - oldest_s();
}

LivePick LiveRing::pick(double capture_s) const {
    LivePick result;
    if (size_ == 0) return result;
    result.valid = true;

    if (capture_s <= oldest_s()) {
        result.slot = slot_of(0);
        result.capture_s = oldest_s();
        // Equality is not a clamp: asking for exactly the oldest frame held is a
        // request the ring can answer.
        result.clamped = capture_s < oldest_s();
        return result;
    }
    if (capture_s >= newest_s()) {
        result.slot = slot_of(size_ - 1);
        result.capture_s = newest_s();
        result.clamped = capture_s > newest_s();
        return result;
    }

    // Binary search over the ring in age order. The times increase by
    // construction -- push() enforces it -- so this is well defined even though
    // the storage wraps.
    std::size_t low = 0;
    std::size_t high = size_ - 1;
    while (high - low > 1) {
        const std::size_t mid = low + (high - low) / 2;
        if (times_[slot_of(mid)] <= capture_s) {
            low = mid;
        } else {
            high = mid;
        }
    }

    // Nearest of the two straddling frames rather than the earlier one. A live
    // source delivers unevenly, so rounding always downwards would bias the
    // picture late by up to a whole frame interval whenever the source stutters.
    const double before = times_[slot_of(low)];
    const double after = times_[slot_of(high)];
    const bool take_after = (capture_s - before) > (after - capture_s);
    result.slot = slot_of(take_after ? high : low);
    result.capture_s = take_after ? after : before;
    return result;
}

}  // namespace svj
