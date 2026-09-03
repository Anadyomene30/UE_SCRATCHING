#include <algorithm>
#include <set>

#include "core/layout.h"
#include "core/protocol.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("layout: every control id in the default rig is unique") {
    // Duplicate ids would silently collapse two physical controls into one.
    const auto targets = default_rig_layout();
    std::set<std::string> seen;
    for (const LearnTarget& t : targets) {
        CHECK(seen.insert(t.id).second);
    }
    CHECK_EQ(seen.size(), targets.size());
}

SVJ_TEST("layout: the Elite exposes both channel strips and the crossfader") {
    const auto targets = elite_layout();
    const auto has = [&targets](const std::string& id) {
        return std::any_of(targets.begin(), targets.end(),
                           [&id](const LearnTarget& t) { return t.id == id; });
    };
    CHECK(has("ch1.eq.hi"));
    CHECK(has("ch2.filter"));
    CHECK(has("ch1.fader"));
    CHECK(has("xfader"));
    CHECK(has("fx.mix"));
    CHECK(has("browse.encoder"));
}

SVJ_TEST("layout: the Elite carries sixteen performance pads") {
    const auto targets = elite_layout();
    const auto pads = std::count_if(targets.begin(), targets.end(), [](const LearnTarget& t) {
        return t.kind == ControlKind::Pad;
    });
    CHECK_EQ(pads, 16);
}

SVJ_TEST("layout: each turntable contributes eight pads per layer") {
    const auto a = rp8000_layout('a', 1);
    CHECK_EQ(a.size(), std::size_t{8});
    CHECK_EQ(a[0].id, std::string("pad.rp8000.a.l1.1"));
    CHECK(a[0].kind == ControlKind::Pad);

    const auto b = rp8000_layout('b', 2);
    CHECK_EQ(b[7].id, std::string("pad.rp8000.b.l2.8"));
}

SVJ_TEST("layout: the crossfader and channel faders are faders, not knobs") {
    const auto targets = elite_layout();
    for (const LearnTarget& t : targets) {
        if (t.id == "xfader" || t.id == "ch1.fader" || t.id == "ch2.fader") {
            CHECK(t.kind == ControlKind::Fader);
        }
    }
}

SVJ_TEST("layout: declaring the rig fills the surface entirely with ghosts") {
    // Before anything is touched the whole table must be drawable, and every
    // control must read as unknown rather than as a plausible invented value.
    Surface surface;
    const auto targets = default_rig_layout();
    declare_layout(targets, surface);

    CHECK_EQ(surface.size(), targets.size());
    CHECK_EQ(surface.unknown_count(), targets.size());
    CHECK_EQ(surface.last_touched(), kNoControl);
}

SVJ_TEST("layout: declaring twice is idempotent") {
    Surface surface;
    const auto targets = elite_layout();
    declare_layout(targets, surface);
    declare_layout(targets, surface);
    CHECK_EQ(surface.size(), targets.size());
}

SVJ_TEST("layout: the schema hash is stable for an unchanged layout") {
    Surface first;
    Surface second;
    declare_layout(default_rig_layout(), first);
    declare_layout(default_rig_layout(), second);
    CHECK_EQ(schema_hash(schema_from(first)), schema_hash(schema_from(second)));
}

SVJ_TEST("layout: adding a control changes the schema hash") {
    // The receiver relies on this to notice that its cached schema is stale.
    Surface base;
    declare_layout(elite_layout(), base);
    const std::uint32_t before = schema_hash(schema_from(base));

    base.declare("pad.rp8000.a.l1.1", ControlKind::Pad);
    CHECK(schema_hash(schema_from(base)) != before);
}
