#include "core/framewindow.h"

#include <algorithm>

namespace svj {

void FrameWindow::configure(std::uint32_t frame_count, std::uint64_t frame_bytes,
                            WindowConfig config) {
    config_ = config;
    frame_count_ = frame_count;
    frame_bytes_ = frame_bytes;

    if (frame_bytes == 0 || frame_count == 0) {
        capacity_ = 0;
    } else {
        const std::uint64_t fits = config.budget_bytes / frame_bytes;
        // At least one frame, so a clip whose single frame exceeds the budget is
        // still playable rather than silently blank.
        capacity_ = static_cast<std::uint32_t>(
            std::clamp<std::uint64_t>(fits, 1u, frame_count));
    }
    reset();
}

void FrameWindow::reset() {
    placed_ = false;
    resident_ = FrameRange{};
    pending_ = FrameRange{};
    reloaded_all_ = false;
}

FrameRange FrameWindow::desired(std::uint32_t playhead, float velocity) const {
    if (capacity_ == 0 || frame_count_ == 0) return FrameRange{};
    if (capacity_ >= frame_count_) return FrameRange{0, frame_count_};

    // Bias the window the way the platter is going. A standstill sits centred, so
    // the next move has room in either direction.
    float bias = 0.5f;
    if (velocity > 0.0f) bias = config_.lead_bias;
    else if (velocity < 0.0f) bias = 1.0f - config_.lead_bias;

    const auto behind = static_cast<std::uint32_t>(
        static_cast<float>(capacity_) * (1.0f - bias));

    std::uint32_t first = playhead > behind ? playhead - behind : 0;
    const std::uint32_t max_first = frame_count_ - capacity_;
    first = std::min(first, max_first);
    return FrameRange{first, capacity_};
}

bool FrameWindow::update(std::uint32_t playhead, float velocity) {
    pending_ = FrameRange{};
    reloaded_all_ = false;

    if (capacity_ == 0 || frame_count_ == 0) return false;
    playhead = std::min(playhead, frame_count_ - 1);

    if (!placed_) {
        resident_ = desired(playhead, velocity);
        placed_ = true;
        pending_ = resident_;
        reloaded_all_ = true;
        return true;
    }

    // The whole clip is resident: nothing can ever need moving.
    if (capacity_ >= frame_count_) return false;

    // Recentre only near an edge. Working back and forth mid-window -- which is
    // what scratching is -- must not trigger a refill.
    const bool near_start =
        playhead < resident_.first + config_.edge_margin && resident_.first > 0;
    const std::uint32_t max_first = frame_count_ - capacity_;
    const bool near_end = playhead + config_.edge_margin > resident_.last() &&
                          resident_.first < max_first;
    const bool outside = !resident_.contains(playhead);

    if (!near_start && !near_end && !outside) return false;

    const FrameRange target = desired(playhead, velocity);
    if (target.first == resident_.first && target.count == resident_.count) return false;

    // A slide shares frames with the old range and only the new end is fetched;
    // a jump shares nothing and the whole window is refilled.
    const bool overlaps = target.first <= resident_.last() &&
                          resident_.first <= target.last();

    if (!overlaps) {
        pending_ = target;
        reloaded_all_ = true;
    } else if (target.first > resident_.first) {
        pending_ = FrameRange{resident_.last() + 1, target.last() - resident_.last()};
    } else {
        pending_ = FrameRange{target.first, resident_.first - target.first};
    }

    resident_ = target;
    return true;
}

double FrameWindow::window_seconds(double frame_duration_s) const {
    return static_cast<double>(resident_.count) * frame_duration_s;
}

}  // namespace svj
