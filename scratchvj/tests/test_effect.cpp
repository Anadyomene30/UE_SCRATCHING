#include <set>
#include <string>

#include "core/effect.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("catalogue: every effect is described exactly once") {
    std::set<std::string> ids;
    for (const EffectDescriptor& d : effect_catalogue()) {
        CHECK(d.id != nullptr);
        CHECK(ids.insert(d.id).second);
        CHECK(d.video != nullptr);  // everything has a visual side; that is the point
    }
    CHECK_EQ(ids.size(), effect_catalogue().size());
}

SVJ_TEST("catalogue: a paired effect has both sides, a video-only effect has one") {
    for (const EffectDescriptor& d : effect_catalogue()) {
        if (d.relation == Correspondence::VideoOnly) {
            // No audio counterpart is invented where none exists.
            CHECK(d.audio == nullptr);
        } else {
            CHECK(d.audio != nullptr);
        }
    }
}

SVJ_TEST("catalogue: the battery is the size the design says it is") {
    std::size_t paired = 0, video_only = 0, identical = 0;
    for (const EffectDescriptor& d : effect_catalogue()) {
        if (d.relation == Correspondence::VideoOnly) ++video_only;
        else ++paired;
        if (d.relation == Correspondence::Identical) ++identical;
    }
    CHECK_EQ(paired, std::size_t{16});
    CHECK_EQ(video_only, std::size_t{16});
    // Most pairings are the same operation in another domain, not a metaphor.
    CHECK(identical >= 12);
}

SVJ_TEST("catalogue: the exact correspondences really are the exact ones") {
    // If one of these is ever downgraded to an analogue, the claim in the docs
    // has changed and this should fail rather than quietly become a fib.
    for (const EffectType type : {EffectType::LowPass, EffectType::Bitcrusher,
                                  EffectType::Decimate, EffectType::Delay}) {
        const EffectDescriptor* d = describe(type);
        CHECK(d != nullptr);
        CHECK(d->relation == Correspondence::Identical);
    }
    // And the honest analogues stay marked as analogues.
    for (const EffectType type : {EffectType::Reverb, EffectType::Flanger,
                                  EffectType::PitchShift, EffectType::Pan}) {
        CHECK(describe(type)->relation == Correspondence::Analogue);
    }
}

SVJ_TEST("catalogue: an unknown type describes as nothing rather than crashing") {
    CHECK(describe(static_cast<EffectType>(240)) == nullptr);
}

SVJ_TEST("unit: linked by default, so one knob drives both sides") {
    EffectUnit unit;
    unit.type = EffectType::Delay;
    unit.shared.mix = 0.62f;
    unit.shared.feedback = 0.55f;

    CHECK(unit.link);
    CHECK_NEAR(unit.audio_params().mix, 0.62, 1e-6);
    CHECK_NEAR(unit.video_params().mix, 0.62, 1e-6);
    CHECK_NEAR(unit.video_params().feedback, 0.55, 1e-6);
}

SVJ_TEST("unit: UNLINKING DOES NOT LURCH") {
    // Divergence has to be something the user creates, never something that
    // happens to them the instant they break the chain.
    EffectUnit unit;
    unit.type = EffectType::Bitcrusher;
    unit.shared.mix = 0.77f;
    unit.shared.amount = 0.4f;

    unit.unlink();
    CHECK(!unit.link);
    CHECK_NEAR(unit.audio_params().mix, 0.77, 1e-6);
    CHECK_NEAR(unit.video_params().mix, 0.77, 1e-6);
    CHECK_NEAR(unit.video_params().amount, 0.4, 1e-6);
}

SVJ_TEST("unit: once unlinked the two sides move independently") {
    EffectUnit unit;
    unit.type = EffectType::Bitcrusher;
    unit.enabled = true;
    unit.shared.mix = 0.5f;
    unit.unlink();

    unit.audio_override.mix = 0.0f;   // bypassed
    unit.video_override.mix = 0.77f;  // still going

    CHECK(!unit.audio_active());
    CHECK(unit.video_active());
    CHECK_NEAR(unit.video_params().mix, 0.77, 1e-6);
}

SVJ_TEST("unit: relinking puts both sides back on the shared values") {
    EffectUnit unit;
    unit.shared.mix = 0.3f;
    unit.unlink();
    unit.video_override.mix = 0.9f;
    unit.relink();
    CHECK_NEAR(unit.video_params().mix, 0.3, 1e-6);
    CHECK_NEAR(unit.audio_params().mix, 0.3, 1e-6);
}

SVJ_TEST("unit: unlinking twice does not overwrite the divergence") {
    EffectUnit unit;
    unit.shared.mix = 0.5f;
    unit.unlink();
    unit.video_override.mix = 0.9f;
    unit.unlink();  // already unlinked
    CHECK_NEAR(unit.video_params().mix, 0.9, 1e-6);
}

SVJ_TEST("unit: a video-only effect is never audio-active, linked or not") {
    EffectUnit unit;
    unit.type = EffectType::SlitScan;
    unit.enabled = true;
    unit.shared.mix = 1.0f;
    CHECK(unit.video_active());
    CHECK(!unit.audio_active());

    unit.unlink();
    unit.audio_override.mix = 1.0f;
    CHECK(!unit.audio_active());  // there is no audio side to activate
}

SVJ_TEST("unit: a disabled unit is inactive on both sides") {
    EffectUnit unit;
    unit.type = EffectType::Delay;
    unit.enabled = false;
    unit.shared.mix = 1.0f;
    CHECK(!unit.audio_active());
    CHECK(!unit.video_active());
}

SVJ_TEST("sync: a tempo-synced effect follows the beat, a free one follows its knob") {
    const double beat = 0.5;  // 120 bpm

    Sync synced;
    synced.tempo = true;
    synced.beats = 0.25;  // a sixteenth
    CHECK_NEAR(synced.seconds(beat, 0.9f), 0.125, 1e-9);  // the knob is ignored

    Sync free;
    CHECK_NEAR(free.seconds(beat, 0.5f, 2.0), 1.0, 1e-9);
    CHECK_NEAR(free.seconds(beat, 0.0f, 2.0), 0.0, 1e-9);
}

SVJ_TEST("sync: the same division gives the same time to both domains") {
    // The point of sharing parameters: an eighth-note echo of the sound is an
    // eighth-note echo of the picture, without either side being told twice.
    EffectUnit unit;
    unit.type = EffectType::Delay;
    unit.sync.tempo = true;
    unit.sync.beats = 0.5;

    const double beat = 60.0 / 124.0;
    const double audio = unit.sync.seconds(beat, unit.audio_params().time);
    const double video = unit.sync.seconds(beat, unit.video_params().time);
    CHECK_NEAR(audio, video, 1e-12);
    CHECK_NEAR(audio, beat * 0.5, 1e-12);
}

SVJ_TEST("rack: loading a slot replaces what was there and enables it") {
    EffectRack rack(3);
    CHECK_EQ(rack.size(), std::size_t{3});
    CHECK_EQ(rack.active_count(), std::size_t{0});

    rack.load(0, EffectType::Delay);
    rack.at(0).shared.mix = 0.6f;
    CHECK(rack.at(0).enabled);
    CHECK_EQ(rack.active_count(), std::size_t{1});

    rack.load(0, EffectType::LowPass);
    CHECK(rack.at(0).type == EffectType::LowPass);
    CHECK_NEAR(rack.at(0).shared.mix, 0.0, 1e-6);  // a fresh unit, not the old one
}

SVJ_TEST("rack: clearing a slot silences it") {
    EffectRack rack;
    rack.load(1, EffectType::Reverb);
    rack.at(1).shared.mix = 1.0f;
    CHECK_EQ(rack.active_count(), std::size_t{1});
    rack.clear(1);
    CHECK_EQ(rack.active_count(), std::size_t{0});
}

SVJ_TEST("rack: an out-of-range slot is ignored rather than corrupting the rack") {
    EffectRack rack(2);
    rack.load(9, EffectType::Delay);
    rack.clear(9);
    CHECK_EQ(rack.size(), std::size_t{2});
    CHECK_EQ(rack.active_count(), std::size_t{0});
}

SVJ_TEST("rack: links can be broken and restored across the whole rack") {
    EffectRack rack(3);
    rack.load(0, EffectType::Delay);
    rack.load(1, EffectType::LowPass);
    rack.at(0).shared.mix = 0.5f;

    rack.set_all_links(false);
    CHECK(!rack.at(0).link);
    CHECK(!rack.at(1).link);
    CHECK_NEAR(rack.at(0).video_params().mix, 0.5, 1e-6);  // still no lurch

    rack.set_all_links(true);
    CHECK(rack.at(0).link);
}
