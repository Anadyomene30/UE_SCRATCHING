// scratchvj — the clip library and the play queue.
//
// A set is not a folder browser. What you actually do is line up what comes next
// and hit one button, so the queue is a first-class object rather than a view onto
// the library, and "load the next one" is a single call.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace svj {

enum class AnalysisState : std::uint8_t {
    Unanalysed,
    Queued,
    Analysing,
    Ready,     // a .svcache exists and is playable
    Failed,
};

struct ClipEntry {
    std::string path;
    std::string name;
    double duration_s = 0.0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    double fps = 0.0;
    bool equirect = false;
    bool has_alpha = false;
    double bpm = 0.0;

    AnalysisState state = AnalysisState::Unanalysed;
    float progress = 0.0f;

    // Only an analysed clip can be scratched; anything else would fall back to
    // real-time decoding, which is exactly what this design refuses to do.
    bool playable() const { return state == AnalysisState::Ready; }
};

using ClipId = int;
inline constexpr ClipId kNoClip = -1;

class Library {
public:
    ClipId add(ClipEntry entry);
    std::size_t size() const { return clips_.size(); }
    const ClipEntry& at(ClipId id) const;
    ClipEntry* mutable_at(ClipId id);

    ClipId find_by_path(const std::string& path) const;

    // Case-insensitive substring match on the name. Empty matches everything.
    std::vector<ClipId> search(const std::string& text) const;

    // Crates are ordered and may hold the same clip more than once is refused,
    // because a crate is a set rather than a playlist.
    int create_crate(std::string name);
    const std::string& crate_name(int crate) const;
    int crate_count() const { return static_cast<int>(crates_.size()); }
    bool add_to_crate(int crate, ClipId id);
    bool remove_from_crate(int crate, ClipId id);
    const std::vector<ClipId>& crate_clips(int crate) const;

    // Analysis bookkeeping.
    void set_state(ClipId id, AnalysisState state, float progress = 0.0f);
    std::vector<ClipId> pending_analysis() const;

private:
    struct Crate {
        std::string name;
        std::vector<ClipId> clips;
    };

    std::vector<ClipEntry> clips_;
    std::vector<Crate> crates_;
};

enum class DeckTarget : std::uint8_t { None, A, B };

struct QueueItem {
    ClipId clip = kNoClip;
    DeckTarget target = DeckTarget::None;
};

class Queue {
public:
    void push(ClipId clip, DeckTarget target = DeckTarget::None);
    bool insert(std::size_t position, ClipId clip, DeckTarget target = DeckTarget::None);
    bool remove(std::size_t position);
    bool move(std::size_t from, std::size_t to);
    void clear();

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    const QueueItem& at(std::size_t position) const;
    bool set_target(std::size_t position, DeckTarget target);

    // Takes the front of the queue. This is what the "next" button calls, and
    // what auto-advance calls at the end of a clip -- deliberately the same path,
    // so the two can never disagree.
    QueueItem take_next();

    // The next item bound for a particular deck, without removing it.
    ClipId peek_for(DeckTarget target) const;

private:
    std::vector<QueueItem> items_;
};

}  // namespace svj
