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

// Whether a clip is shown as a sphere or as a flat picture. The cache header
// carries what the analysis pass GUESSED (2:1 footage is taken for equirect);
// this is what the performer SAID, and it wins. It lives on the library entry
// rather than in the header because a preference must survive a re-analysis --
// rewriting sixty-four bytes would be cheap, but it would mix what the file is
// with what someone decided about it.
enum class ProjectionOverride : std::uint8_t {
    Auto,      // follow the header's flag
    Flat,
    Equirect,
};

// The one place the decision is made. Every deck, every badge and every render
// pass asks this rather than re-deriving "is it 360" from the aspect ratio --
// which the interface once did in five places, each a separate rule.
bool effective_equirect(bool header_flag, ProjectionOverride override);

struct ClipEntry {
    // The playable file: the .svcache. What the deck opens.
    std::string path;
    // The video it was analysed from, when known. Empty for an orphan cache
    // whose source has gone: still playable, so still listed.
    std::string source_path;
    std::string name;
    double duration_s = 0.0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    double fps = 0.0;
    bool equirect = false;  // the header's flag, as analysed
    ProjectionOverride projection = ProjectionOverride::Auto;
    bool has_alpha = false;
    double bpm = 0.0;

    // A source that is a numbered image sequence, or a single still. Both go
    // through the same analysis pass with different ffmpeg arguments; the
    // library only needs to remember which, so the pass can be re-run.
    bool is_sequence = false;
    bool is_still = false;

    bool shown_equirect() const { return effective_equirect(equirect, projection); }

    // The cache's thumbnail, BC1, read back with its header. Empty for a
    // cache written before thumbnails existed; the front end shows a blank
    // well rather than re-analysing anything.
    std::uint32_t thumb_w = 0;
    std::uint32_t thumb_h = 0;
    std::vector<std::uint8_t> thumbnail;

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
    ClipId find_by_source(const std::string& source_path) const;

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

// Where a clip can be put. The overlay is a target like the decks: a logo or
// a mask is chosen from the same library, by the same button.
enum class DeckTarget : std::uint8_t { None, A, B, Overlay };

struct QueueItem {
    ClipId clip = kNoClip;
    DeckTarget target = DeckTarget::None;
};

// The deck a queued clip goes to when nobody said. A: the queue is a running
// order, and a "next" button that refused to act because no deck was named
// would be a riddle at the wrong moment. Here rather than in the interface so
// a pad and a click cannot disagree about it.
DeckTarget default_target(const QueueItem& item);

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
