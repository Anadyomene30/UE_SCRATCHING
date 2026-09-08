#include "core/matrix.h"

#include <utility>

namespace svj {

PadBank::PadBank() : PadBank("Banque 1") {}

PadBank::PadBank(std::string bank_name) : name(std::move(bank_name)) {
    a.fill(kNoClip);
    b.fill(kNoClip);
    overlay.fill(kNoClip);
}

std::array<ClipId, kPadCount>& PadBank::cells(DeckTarget target) {
    switch (target) {
        case DeckTarget::B: return b;
        case DeckTarget::Overlay: return overlay;
        case DeckTarget::A:
        case DeckTarget::None:
        default: return a;
    }
}

const std::array<ClipId, kPadCount>& PadBank::cells(DeckTarget target) const {
    switch (target) {
        case DeckTarget::B: return b;
        case DeckTarget::Overlay: return overlay;
        case DeckTarget::A:
        case DeckTarget::None:
        default: return a;
    }
}

Matrix::Matrix() { banks_.emplace_back("Banque 1"); }

const PadBank& Matrix::bank(int index) const { return banks_.at(static_cast<std::size_t>(index)); }

PadBank& Matrix::mutable_bank(int index) { return banks_.at(static_cast<std::size_t>(index)); }

void Matrix::select(int index) {
    if (index >= 0 && index < bank_count()) current_ = index;
}

int Matrix::add_bank(std::string name) {
    if (name.empty()) name = "Banque " + std::to_string(banks_.size() + 1);
    banks_.emplace_back(std::move(name));
    return bank_count() - 1;
}

bool Matrix::remove_bank(int index) {
    if (index < 0 || index >= bank_count() || banks_.size() == 1) return false;
    banks_.erase(banks_.begin() + index);
    if (current_ >= bank_count()) current_ = bank_count() - 1;
    return true;
}

void Matrix::clear_bank(int index) {
    if (index < 0 || index >= bank_count()) return;
    PadBank& bank = banks_[static_cast<std::size_t>(index)];
    bank.a.fill(kNoClip);
    bank.b.fill(kNoClip);
    bank.overlay.fill(kNoClip);
}

bool Matrix::set(int bank, DeckTarget target, int pad, ClipId clip) {
    if (bank < 0 || bank >= bank_count() || pad < 0 || pad >= kPadCount) return false;
    if (target == DeckTarget::None) return false;
    banks_[static_cast<std::size_t>(bank)].cells(target)[static_cast<std::size_t>(pad)] = clip;
    return true;
}

ClipId Matrix::at(int bank, DeckTarget target, int pad) const {
    if (bank < 0 || bank >= bank_count() || pad < 0 || pad >= kPadCount) return kNoClip;
    if (target == DeckTarget::None) return kNoClip;
    return banks_[static_cast<std::size_t>(bank)].cells(target)[static_cast<std::size_t>(pad)];
}

ClipId Matrix::clip_at(DeckTarget target, int pad) const { return at(current_, target, pad); }

int Matrix::first_free(DeckTarget target) const {
    if (target == DeckTarget::None) return -1;
    const auto& cells = banks_[static_cast<std::size_t>(current_)].cells(target);
    for (int i = 0; i < kPadCount; ++i) {
        if (cells[static_cast<std::size_t>(i)] == kNoClip) return i;
    }
    return -1;
}

void Matrix::forget(ClipId clip) {
    for (PadBank& bank : banks_) {
        for (auto* cells : {&bank.a, &bank.b, &bank.overlay}) {
            for (ClipId& cell : *cells) {
                if (cell == clip) cell = kNoClip;
            }
        }
    }
}

}  // namespace svj
