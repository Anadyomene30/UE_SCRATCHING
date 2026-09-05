#include "core/library.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace svj {
namespace {

std::string lowered(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

}  // namespace

ClipId Library::add(ClipEntry entry) {
    clips_.push_back(std::move(entry));
    return static_cast<ClipId>(clips_.size() - 1);
}

const ClipEntry& Library::at(ClipId id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= clips_.size()) {
        throw std::out_of_range("svj::Library::at: clip id out of range");
    }
    return clips_[static_cast<std::size_t>(id)];
}

ClipEntry* Library::mutable_at(ClipId id) {
    if (id < 0 || static_cast<std::size_t>(id) >= clips_.size()) return nullptr;
    return &clips_[static_cast<std::size_t>(id)];
}

ClipId Library::find_by_path(const std::string& path) const {
    for (std::size_t i = 0; i < clips_.size(); ++i) {
        if (clips_[i].path == path) return static_cast<ClipId>(i);
    }
    return kNoClip;
}

std::vector<ClipId> Library::search(const std::string& text) const {
    std::vector<ClipId> found;
    const std::string needle = lowered(text);
    for (std::size_t i = 0; i < clips_.size(); ++i) {
        if (needle.empty() || lowered(clips_[i].name).find(needle) != std::string::npos) {
            found.push_back(static_cast<ClipId>(i));
        }
    }
    return found;
}

int Library::create_crate(std::string name) {
    crates_.push_back(Crate{std::move(name), {}});
    return static_cast<int>(crates_.size() - 1);
}

const std::string& Library::crate_name(int crate) const {
    static const std::string empty;
    if (crate < 0 || static_cast<std::size_t>(crate) >= crates_.size()) return empty;
    return crates_[static_cast<std::size_t>(crate)].name;
}

bool Library::add_to_crate(int crate, ClipId id) {
    if (crate < 0 || static_cast<std::size_t>(crate) >= crates_.size()) return false;
    if (id < 0 || static_cast<std::size_t>(id) >= clips_.size()) return false;
    auto& clips = crates_[static_cast<std::size_t>(crate)].clips;
    // A crate is a set: the same clip twice in one crate is a mistake, not an
    // intention. The queue is where repetition belongs.
    if (std::find(clips.begin(), clips.end(), id) != clips.end()) return false;
    clips.push_back(id);
    return true;
}

bool Library::remove_from_crate(int crate, ClipId id) {
    if (crate < 0 || static_cast<std::size_t>(crate) >= crates_.size()) return false;
    auto& clips = crates_[static_cast<std::size_t>(crate)].clips;
    const auto it = std::find(clips.begin(), clips.end(), id);
    if (it == clips.end()) return false;
    clips.erase(it);
    return true;
}

const std::vector<ClipId>& Library::crate_clips(int crate) const {
    static const std::vector<ClipId> empty;
    if (crate < 0 || static_cast<std::size_t>(crate) >= crates_.size()) return empty;
    return crates_[static_cast<std::size_t>(crate)].clips;
}

void Library::set_state(ClipId id, AnalysisState state, float progress) {
    ClipEntry* entry = mutable_at(id);
    if (entry == nullptr) return;
    entry->state = state;
    entry->progress = std::clamp(progress, 0.0f, 1.0f);
    if (state == AnalysisState::Ready) entry->progress = 1.0f;
}

std::vector<ClipId> Library::pending_analysis() const {
    std::vector<ClipId> pending;
    for (std::size_t i = 0; i < clips_.size(); ++i) {
        const AnalysisState state = clips_[i].state;
        if (state == AnalysisState::Unanalysed || state == AnalysisState::Queued) {
            pending.push_back(static_cast<ClipId>(i));
        }
    }
    return pending;
}

// ----------------------------------------------------------------- queue ----

void Queue::push(ClipId clip, DeckTarget target) {
    items_.push_back(QueueItem{clip, target});
}

bool Queue::insert(std::size_t position, ClipId clip, DeckTarget target) {
    if (position > items_.size()) return false;
    items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(position),
                  QueueItem{clip, target});
    return true;
}

bool Queue::remove(std::size_t position) {
    if (position >= items_.size()) return false;
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(position));
    return true;
}

bool Queue::move(std::size_t from, std::size_t to) {
    if (from >= items_.size() || to >= items_.size()) return false;
    if (from == to) return true;
    const QueueItem item = items_[from];
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(from));
    items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(to), item);
    return true;
}

void Queue::clear() { items_.clear(); }

const QueueItem& Queue::at(std::size_t position) const {
    static const QueueItem empty;
    if (position >= items_.size()) return empty;
    return items_[position];
}

bool Queue::set_target(std::size_t position, DeckTarget target) {
    if (position >= items_.size()) return false;
    items_[position].target = target;
    return true;
}

QueueItem Queue::take_next() {
    if (items_.empty()) return QueueItem{};
    const QueueItem item = items_.front();
    items_.erase(items_.begin());
    return item;
}

ClipId Queue::peek_for(DeckTarget target) const {
    for (const QueueItem& item : items_) {
        if (item.target == target) return item.clip;
    }
    return kNoClip;
}

}  // namespace svj
