// scratchvj — the pad banks: which clip each pad loads.
//
// The hardware's main interface is pads: eight on each side of the Elite,
// twenty-four on each RP-8000. A pad in "clips" mode loads a clip onto its
// deck, which is live editing: the rushes of a scene laid on the pads and
// triggered in time. This is the table behind that -- one bank is eight
// cells per layer (deck A, deck B, the overlay), and a set keeps several.
//
// Clips are named by ClipId, the library's row index; library.json stores the
// cache path instead (config/library_io), because ids are not stable across
// a rescan and paths are the one key every entry has.
#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/library.h"

namespace svj {

inline constexpr int kPadCount = 8;

struct PadBank {
    std::string name;
    std::array<ClipId, kPadCount> a{};
    std::array<ClipId, kPadCount> b{};
    std::array<ClipId, kPadCount> overlay{};

    PadBank();
    explicit PadBank(std::string name);

    std::array<ClipId, kPadCount>& cells(DeckTarget target);
    const std::array<ClipId, kPadCount>& cells(DeckTarget target) const;
};

class Matrix {
public:
    // Starts with one empty bank: a matrix with no bank has no pad to press.
    Matrix();

    int bank_count() const { return static_cast<int>(banks_.size()); }
    const PadBank& bank(int index) const;
    PadBank& mutable_bank(int index);

    // The bank the pads read from. Selecting out of range is ignored.
    int current() const { return current_; }
    void select(int index);

    int add_bank(std::string name);
    // Refuses to remove the last bank. The selection follows.
    bool remove_bank(int index);
    void clear_bank(int index);

    // pad is 0..7. An out-of-range bank or pad is refused, never wrapped.
    bool set(int bank, DeckTarget target, int pad, ClipId clip);
    ClipId at(int bank, DeckTarget target, int pad) const;

    // What a pad press in "clips" mode loads, from the current bank.
    ClipId clip_at(DeckTarget target, int pad) const;
    // The first empty cell of the current bank for that layer, or -1.
    int first_free(DeckTarget target) const;

    // A clip that left the library is cleared from every cell that named it,
    // so a pad never loads a row that no longer exists.
    void forget(ClipId clip);

private:
    std::vector<PadBank> banks_;
    int current_ = 0;
};

}  // namespace svj
