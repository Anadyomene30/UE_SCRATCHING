#include "core/library.h"
#include "harness.h"

using namespace svj;

namespace {

ClipEntry clip(std::string name, bool equirect = false) {
    ClipEntry entry;
    entry.path = "/clips/" + name;
    entry.name = std::move(name);
    entry.duration_s = 120.0;
    entry.width = 1920;
    entry.height = 1080;
    entry.fps = 60.0;
    entry.equirect = equirect;
    return entry;
}

}  // namespace

SVJ_TEST("library: clips are added and read back") {
    Library library;
    const ClipId a = library.add(clip("tokyo_nightdrive_360.mp4", true));
    const ClipId b = library.add(clip("grain_loop_04.mov"));

    CHECK_EQ(library.size(), std::size_t{2});
    CHECK_EQ(library.at(a).name, std::string("tokyo_nightdrive_360.mp4"));
    CHECK(library.at(a).equirect);
    CHECK(!library.at(b).equirect);
}

SVJ_TEST("library: an unanalysed clip is not playable") {
    // Playing one would mean falling back to real-time decoding, which is exactly
    // what this design refuses to do.
    Library library;
    const ClipId id = library.add(clip("fresh.mp4"));
    CHECK(!library.at(id).playable());

    library.set_state(id, AnalysisState::Analysing, 0.4f);
    CHECK(!library.at(id).playable());
    CHECK_NEAR(library.at(id).progress, 0.4, 1e-6);

    library.set_state(id, AnalysisState::Ready);
    CHECK(library.at(id).playable());
    CHECK_NEAR(library.at(id).progress, 1.0, 1e-6);
}

SVJ_TEST("library: a failed analysis is not playable either") {
    Library library;
    const ClipId id = library.add(clip("broken.mkv"));
    library.set_state(id, AnalysisState::Failed);
    CHECK(!library.at(id).playable());
}

SVJ_TEST("library: search is case-insensitive and matches anywhere in the name") {
    Library library;
    library.add(clip("Tokyo_NightDrive_360.mp4"));
    library.add(clip("grain_loop_04.mov"));
    library.add(clip("rooftop_pan_4k.mp4"));

    CHECK_EQ(library.search("tokyo").size(), std::size_t{1});
    CHECK_EQ(library.search("NIGHT").size(), std::size_t{1});
    CHECK_EQ(library.search("mp4").size(), std::size_t{2});
    CHECK_EQ(library.search("").size(), std::size_t{3});
    CHECK(library.search("nothing here").empty());
}

SVJ_TEST("library: a clip can be found by its path") {
    Library library;
    const ClipId id = library.add(clip("a.mp4"));
    CHECK_EQ(library.find_by_path("/clips/a.mp4"), id);
    CHECK_EQ(library.find_by_path("/clips/missing.mp4"), kNoClip);
}

SVJ_TEST("library: an out-of-range id throws rather than returning nonsense") {
    Library library;
    bool threw = false;
    try {
        library.at(7);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(library.mutable_at(-1) == nullptr);
}

SVJ_TEST("library: crates hold clips in order") {
    Library library;
    const ClipId a = library.add(clip("a.mp4"));
    const ClipId b = library.add(clip("b.mp4"));
    const int crate = library.create_crate("360");

    CHECK_EQ(library.crate_name(crate), std::string("360"));
    CHECK(library.add_to_crate(crate, b));
    CHECK(library.add_to_crate(crate, a));
    CHECK_EQ(library.crate_clips(crate).size(), std::size_t{2});
    CHECK_EQ(library.crate_clips(crate)[0], b);
}

SVJ_TEST("library: A CRATE IS A SET, so the same clip cannot go in twice") {
    // Repetition belongs in the queue, not in a crate; adding a clip twice there
    // is a slip rather than an intention.
    Library library;
    const ClipId a = library.add(clip("a.mp4"));
    const int crate = library.create_crate("set");
    CHECK(library.add_to_crate(crate, a));
    CHECK(!library.add_to_crate(crate, a));
    CHECK_EQ(library.crate_clips(crate).size(), std::size_t{1});
}

SVJ_TEST("library: a bad crate or clip id is refused, not stored") {
    Library library;
    const ClipId a = library.add(clip("a.mp4"));
    CHECK(!library.add_to_crate(9, a));
    const int crate = library.create_crate("x");
    CHECK(!library.add_to_crate(crate, 99));
    CHECK(library.crate_clips(crate).empty());
    CHECK(library.crate_clips(9).empty());
}

SVJ_TEST("library: removing from a crate leaves the clip in the library") {
    Library library;
    const ClipId a = library.add(clip("a.mp4"));
    const int crate = library.create_crate("x");
    library.add_to_crate(crate, a);
    CHECK(library.remove_from_crate(crate, a));
    CHECK(!library.remove_from_crate(crate, a));
    CHECK(library.crate_clips(crate).empty());
    CHECK_EQ(library.size(), std::size_t{1});
}

SVJ_TEST("library: pending analysis lists what still needs doing") {
    Library library;
    const ClipId a = library.add(clip("a.mp4"));
    const ClipId b = library.add(clip("b.mp4"));
    const ClipId c = library.add(clip("c.mp4"));
    library.set_state(b, AnalysisState::Ready);
    library.set_state(c, AnalysisState::Queued);

    const auto pending = library.pending_analysis();
    CHECK_EQ(pending.size(), std::size_t{2});
    CHECK_EQ(pending[0], a);
    CHECK_EQ(pending[1], c);
}

SVJ_TEST("queue: items come out in the order they went in") {
    Queue queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);
    CHECK_EQ(queue.size(), std::size_t{3});

    CHECK_EQ(queue.take_next().clip, 1);
    CHECK_EQ(queue.take_next().clip, 2);
    CHECK_EQ(queue.size(), std::size_t{1});
}

SVJ_TEST("queue: taking from an empty queue gives nothing, not a crash") {
    Queue queue;
    CHECK(queue.empty());
    CHECK_EQ(queue.take_next().clip, kNoClip);
}

SVJ_TEST("queue: items carry the deck they are bound for") {
    Queue queue;
    queue.push(10, DeckTarget::A);
    queue.push(11, DeckTarget::B);
    queue.push(12);

    CHECK_EQ(queue.peek_for(DeckTarget::A), 10);
    CHECK_EQ(queue.peek_for(DeckTarget::B), 11);
    CHECK(queue.at(2).target == DeckTarget::None);

    CHECK(queue.set_target(2, DeckTarget::A));
    CHECK_EQ(queue.peek_for(DeckTarget::A), 10);  // still the first one bound for A
}

SVJ_TEST("queue: peeking for a deck nothing is bound to gives nothing") {
    Queue queue;
    queue.push(1, DeckTarget::A);
    CHECK_EQ(queue.peek_for(DeckTarget::B), kNoClip);
}

SVJ_TEST("queue: items can be inserted, moved and removed") {
    Queue queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);

    CHECK(queue.insert(1, 99));
    CHECK_EQ(queue.at(1).clip, 99);

    CHECK(queue.move(1, 3));
    CHECK_EQ(queue.at(3).clip, 99);

    CHECK(queue.remove(3));
    CHECK_EQ(queue.size(), std::size_t{3});
    CHECK_EQ(queue.at(0).clip, 1);
}

SVJ_TEST("queue: inserting at the end is allowed, past it is not") {
    Queue queue;
    queue.push(1);
    CHECK(queue.insert(1, 2));   // append
    CHECK(!queue.insert(9, 3));
    CHECK_EQ(queue.size(), std::size_t{2});
}

SVJ_TEST("queue: out-of-range moves and removals are refused") {
    Queue queue;
    queue.push(1);
    CHECK(!queue.move(0, 5));
    CHECK(!queue.move(5, 0));
    CHECK(!queue.remove(5));
    CHECK(!queue.set_target(5, DeckTarget::A));
    CHECK_EQ(queue.size(), std::size_t{1});
}

SVJ_TEST("queue: moving an item onto itself changes nothing") {
    Queue queue;
    queue.push(1);
    queue.push(2);
    CHECK(queue.move(1, 1));
    CHECK_EQ(queue.at(1).clip, 2);
}

SVJ_TEST("queue: the next button and auto-advance are the same path") {
    // Deliberately one call, so the two can never disagree about what plays next.
    Queue queue;
    queue.push(7, DeckTarget::A);
    queue.push(8, DeckTarget::B);

    const QueueItem by_button = queue.take_next();
    CHECK_EQ(by_button.clip, 7);
    CHECK(by_button.target == DeckTarget::A);

    const QueueItem by_auto_advance = queue.take_next();
    CHECK_EQ(by_auto_advance.clip, 8);
}
