#include "core/matrix.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("matrix: starts with one empty bank, because a pad needs somewhere to look") {
    Matrix matrix;
    CHECK_EQ(matrix.bank_count(), 1);
    CHECK_EQ(matrix.current(), 0);
    for (int pad = 0; pad < kPadCount; ++pad) {
        CHECK(matrix.clip_at(DeckTarget::A, pad) == kNoClip);
        CHECK(matrix.clip_at(DeckTarget::B, pad) == kNoClip);
        CHECK(matrix.clip_at(DeckTarget::Overlay, pad) == kNoClip);
    }
}

SVJ_TEST("matrix: a pad reads the CURRENT bank, and the layers do not share cells") {
    Matrix matrix;
    const int two = matrix.add_bank("Banque 2");
    CHECK(matrix.set(0, DeckTarget::A, 3, 7));
    CHECK(matrix.set(two, DeckTarget::A, 3, 9));
    CHECK(matrix.set(0, DeckTarget::B, 3, 11));

    CHECK_EQ(matrix.clip_at(DeckTarget::A, 3), 7);
    CHECK_EQ(matrix.clip_at(DeckTarget::B, 3), 11);
    CHECK(matrix.clip_at(DeckTarget::Overlay, 3) == kNoClip);
    matrix.select(two);
    CHECK_EQ(matrix.clip_at(DeckTarget::A, 3), 9);
    CHECK(matrix.clip_at(DeckTarget::B, 3) == kNoClip);
}

SVJ_TEST("matrix: an out-of-range pad or bank is refused, never wrapped") {
    Matrix matrix;
    CHECK(!matrix.set(0, DeckTarget::A, 8, 1));
    CHECK(!matrix.set(0, DeckTarget::A, -1, 1));
    CHECK(!matrix.set(1, DeckTarget::A, 0, 1));
    CHECK(!matrix.set(0, DeckTarget::None, 0, 1));
    CHECK(matrix.clip_at(DeckTarget::A, 8) == kNoClip);
    matrix.select(5);
    CHECK_EQ(matrix.current(), 0);
}

SVJ_TEST("matrix: the last bank cannot be removed, and the selection follows a removal") {
    Matrix matrix;
    CHECK(!matrix.remove_bank(0));
    matrix.add_bank("");
    matrix.add_bank("");
    CHECK_EQ(matrix.bank(2).name, std::string("Banque 3"));
    matrix.select(2);
    CHECK(matrix.remove_bank(2));
    CHECK_EQ(matrix.current(), 1);
}

SVJ_TEST("matrix: first_free walks the current bank's layer") {
    Matrix matrix;
    CHECK_EQ(matrix.first_free(DeckTarget::A), 0);
    matrix.set(0, DeckTarget::A, 0, 1);
    matrix.set(0, DeckTarget::A, 1, 2);
    CHECK_EQ(matrix.first_free(DeckTarget::A), 2);
    for (int pad = 0; pad < kPadCount; ++pad) matrix.set(0, DeckTarget::B, pad, 5);
    CHECK_EQ(matrix.first_free(DeckTarget::B), -1);
    CHECK_EQ(matrix.first_free(DeckTarget::None), -1);
}

SVJ_TEST("matrix: a forgotten clip is cleared from every cell that named it") {
    Matrix matrix;
    matrix.add_bank("Banque 2");
    matrix.set(0, DeckTarget::A, 0, 4);
    matrix.set(0, DeckTarget::Overlay, 5, 4);
    matrix.set(1, DeckTarget::B, 2, 4);
    matrix.set(1, DeckTarget::B, 3, 6);
    matrix.forget(4);
    CHECK(matrix.at(0, DeckTarget::A, 0) == kNoClip);
    CHECK(matrix.at(0, DeckTarget::Overlay, 5) == kNoClip);
    CHECK(matrix.at(1, DeckTarget::B, 2) == kNoClip);
    CHECK_EQ(matrix.at(1, DeckTarget::B, 3), 6);
}
