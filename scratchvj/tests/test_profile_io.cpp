#include <cstdio>
#include <string>

#include "config/profile_io.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("profile_io: the Elite survives a round trip through JSON") {
    const DeviceProfile& elite = builtin_elite();
    DeviceProfile back;
    std::string error;
    CHECK(profile_from_json(profile_to_json(elite), back, error));
    CHECK_EQ(back.name, elite.name);
    CHECK_EQ(back.controls.size(), elite.controls.size());
    CHECK_EQ(back.layout.size(), elite.layout.size());
    CHECK(back.verified == elite.verified);
    const ProfileControl* browse = profile_control(back, "browse.encoder");
    CHECK(browse != nullptr);
    CHECK(browse->encoder == EncoderMode::Relative64);
    CHECK(profile_control(back, "xfader.curve")->optional);
}

SVJ_TEST("profile_io: an unknown control kind is named rather than defaulted") {
    DeviceProfile p;
    std::string error;
    CHECK(!profile_from_json(R"({"name": "x", "controls": [{"id": "k", "kind": "slider"}]})", p,
                             error));
    CHECK(error.find("slider") != std::string::npos);
}

SVJ_TEST("profile_io: a file that parses but does not validate is refused with the fault") {
    DeviceProfile p;
    std::string error;
    CHECK(!profile_from_json(
        R"({"name": "x", "controls": [{"id": "k", "kind": "knob"}], "layout": [{"title": "G", "controls": ["nope"]}]})",
        p, error));
    CHECK(error.find("nope") != std::string::npos);
}

SVJ_TEST("profile_io: a profile without a name is refused") {
    DeviceProfile p;
    std::string error;
    CHECK(!profile_from_json(R"({"controls": []})", p, error));
    CHECK(error.find("name") != std::string::npos);
}

SVJ_TEST("profile_io: shipped bindings read their MIDI address and default to unverified") {
    DeviceProfile p;
    std::string error;
    CHECK(profile_from_json(
        R"({"name": "x", "controls": [{"id": "k", "kind": "knob"}],
            "bindings": [{"id": "k", "midi": {"type": "cc", "channel": 3, "number": 48}}]})",
        p, error));
    CHECK(!p.verified);
    CHECK_EQ(p.bindings.size(), std::size_t{1});
    CHECK(p.bindings[0].address.kind == MidiKind::ControlChange);
    CHECK_EQ(static_cast<int>(p.bindings[0].address.channel), 3);
    CHECK_EQ(static_cast<int>(p.bindings[0].address.number), 48);
}

#ifdef SCRATCHVJ_PROFILE_DIR
SVJ_TEST("profile_io: every file shipped in profiles/ loads and validates") {
    // A profile that does not load is a controller that silently does not
    // exist; a profile that does not validate is a knob that draws and never
    // moves. Both are caught here, by the file, before the desk.
    std::vector<std::string> errors;
    const std::vector<DeviceProfile> loaded = profiles_in(SCRATCHVJ_PROFILE_DIR, errors);
    for (const std::string& e : errors) std::fprintf(stderr, "%s\n", e.c_str());
    CHECK(errors.empty());
    CHECK(loaded.size() >= 2);
    bool apc = false, push = false;
    for (const DeviceProfile& p : loaded) {
        CHECK(validate_profile(p).empty());
        if (p.name == "apc40_mk2") {
            apc = true;
            CHECK(!p.verified);  // written from a document, not measured
            CHECK(profile_control(p, "pad.apc40.r1.c1") != nullptr);
            CHECK(profile_control(p, "xfader") != nullptr);
            CHECK(!p.bindings.empty());
        }
        if (p.name == "push2") {
            push = true;
            CHECK(profile_control(p, "pad.push.r8.c8") != nullptr);
            CHECK(profile_control(p, "encoder.push.1")->encoder == EncoderMode::Signed7);
        }
    }
    CHECK(apc);
    CHECK(push);
    CHECK(find_profile("apc40_mk2", loaded) != nullptr);
    CHECK(find_profile("reloop_elite", loaded) == &builtin_elite());
    CHECK(find_profile("nope", loaded) == nullptr);
}
#endif
