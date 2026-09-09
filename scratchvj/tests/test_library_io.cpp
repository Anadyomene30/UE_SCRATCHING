#include <cstdio>
#include <string>

#include "config/library_io.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("library_io: clips, projection overrides and crates survive a round trip") {
    LibraryFile file;
    LibraryFile::Clip a;
    a.source = "D:/rushes/tokyo.mp4";
    a.cache = "D:/rushes/tokyo.mp4.svcache";
    a.name = "tokyo.mp4";
    a.projection = ProjectionOverride::Flat;
    LibraryFile::Clip b;
    b.source = "D:/shots/frame_%04d.png";
    b.cache = "D:/svcache/frame_-1a2b3c4d.svcache";
    b.name = "frame_%04d.png";
    b.is_sequence = true;
    LibraryFile::Clip c;
    c.cache = "D:/old/orphan.svcache";  // no source: an orphan cache
    c.name = "orphan";
    file.clips = {a, b, c};
    file.crates = {LibraryFile::Crate{"Set 12 sept.", {a.cache, c.cache}}};

    LibraryFile back;
    std::string error;
    CHECK(library_from_json(library_to_json(file), back, error));
    CHECK_EQ(back.clips.size(), std::size_t{3});
    CHECK(back.clips[0].projection == ProjectionOverride::Flat);
    CHECK(back.clips[1].is_sequence);
    CHECK(!back.clips[1].is_still);
    CHECK(back.clips[2].source.empty());
    CHECK_EQ(back.crates.size(), std::size_t{1});
    CHECK_EQ(back.crates[0].clips.size(), std::size_t{2});
    CHECK_EQ(back.crates[0].clips[1], c.cache);
}

SVJ_TEST("library_io: a malformed file leaves the library untouched") {
    LibraryFile file;
    file.clips.push_back(LibraryFile::Clip{"", "keep.svcache", "keep", ProjectionOverride::Auto,
                                           false, false});
    std::string error;
    CHECK(!library_from_json("{not json", file, error));
    CHECK_EQ(file.clips.size(), std::size_t{1});
    CHECK(!library_from_json(R"({"clips": [{"name": "no cache"}]})", file, error));
    CHECK(error.find("cache") != std::string::npos);
    CHECK_EQ(file.clips[0].cache, std::string("keep.svcache"));
}

SVJ_TEST("library_io: an unknown projection word is named rather than defaulted") {
    // Silently reading "sphere" as auto would undo a decision the performer
    // made and never say so.
    LibraryFile file;
    std::string error;
    CHECK(!library_from_json(R"({"clips": [{"cache": "x.svcache", "projection": "sphere"}]})",
                             file, error));
    CHECK(error.find("projection") != std::string::npos);
    CHECK(error.find("sphere") != std::string::npos);
}

SVJ_TEST("library_io: a missing file is an empty library, not an error") {
    LibraryFile file;
    file.clips.push_back(LibraryFile::Clip{});
    std::string error;
    CHECK(library_load("this/does/not/exist/library.json", file, error));
    CHECK(file.clips.empty());
}

SVJ_TEST("library_io: the snapshot and the restore are inverse on what the file keeps") {
    Library library;
    ClipEntry entry;
    entry.path = "a.svcache";
    entry.source_path = "a.mp4";
    entry.name = "a.mp4";
    entry.projection = ProjectionOverride::Equirect;
    entry.state = AnalysisState::Ready;
    const ClipId a = library.add(entry);
    entry.path = "b.svcache";
    entry.source_path = "b.mov";
    entry.name = "b.mov";
    entry.projection = ProjectionOverride::Auto;
    const ClipId b = library.add(entry);
    const int crate = library.create_crate("loops");
    library.add_to_crate(crate, b);
    library.add_to_crate(crate, a);

    Library restored;
    library_restore(library_snapshot(library), restored);
    CHECK_EQ(restored.size(), std::size_t{2});
    CHECK_EQ(restored.find_by_source("a.mp4"), 0);
    CHECK(restored.at(0).projection == ProjectionOverride::Equirect);
    // Restored entries are unanalysed until a disk says the cache is there:
    // the file records decisions, not the state of a drive.
    CHECK(!restored.at(0).playable());
    CHECK_EQ(restored.crate_count(), 1);
    CHECK_EQ(restored.crate_clips(0).size(), std::size_t{2});
    CHECK_EQ(restored.crate_clips(0)[0], 1);  // b first, as it was
}

SVJ_TEST("library_io: restoring over an existing entry does not duplicate it") {
    Library library;
    ClipEntry entry;
    entry.path = "a.svcache";
    entry.name = "a";
    library.add(entry);
    LibraryFile file;
    file.clips.push_back(LibraryFile::Clip{"a.mp4", "a.svcache", "a", ProjectionOverride::Auto,
                                           false, false});
    library_restore(file, library);
    CHECK_EQ(library.size(), std::size_t{1});
}

SVJ_TEST("library_io: save then load through a real file") {
    const std::string path = "test_library_io_roundtrip.json";
    LibraryFile file;
    file.clips.push_back(LibraryFile::Clip{"s.mp4", "s.mp4.svcache", "s", ProjectionOverride::Flat,
                                           false, false});
    std::string error;
    CHECK(library_save(file, path, error));
    LibraryFile back;
    CHECK(library_load(path, back, error));
    CHECK_EQ(back.clips.size(), std::size_t{1});
    CHECK(back.clips[0].projection == ProjectionOverride::Flat);
    std::remove(path.c_str());
}

SVJ_TEST("library_io: the pad banks travel by cache path and come back by id") {
    Library library;
    ClipEntry a;
    a.path = "D:/svcache/a.svcache";
    a.name = "a";
    ClipEntry b;
    b.path = "D:/svcache/b.svcache";
    b.name = "b";
    const ClipId id_a = library.add(a);
    const ClipId id_b = library.add(b);

    Matrix matrix;
    matrix.mutable_bank(0).name = "Intro";
    matrix.add_bank("Drop");
    matrix.set(0, DeckTarget::A, 0, id_a);
    matrix.set(0, DeckTarget::B, 7, id_b);
    matrix.set(1, DeckTarget::Overlay, 3, id_b);
    matrix.select(1);

    const LibraryFile file = library_snapshot(library, matrix);
    CHECK_EQ(file.banks.size(), std::size_t{2});
    CHECK_EQ(file.banks[0].a[0], a.path);
    CHECK(file.banks[0].a[1].empty());
    CHECK_EQ(file.current_bank, 1);

    LibraryFile back;
    std::string error;
    CHECK(library_from_json(library_to_json(file), back, error));

    // Restored into a library where the ids come out in another order.
    Library other;
    other.add(b);
    other.add(a);
    Matrix restored;
    library_restore(back, other, restored);
    CHECK_EQ(restored.bank_count(), 2);
    CHECK_EQ(restored.bank(0).name, std::string("Intro"));
    CHECK_EQ(restored.current(), 1);
    CHECK_EQ(other.at(restored.at(0, DeckTarget::A, 0)).path, a.path);
    CHECK_EQ(other.at(restored.at(0, DeckTarget::B, 7)).path, b.path);
    CHECK_EQ(other.at(restored.at(1, DeckTarget::Overlay, 3)).path, b.path);
}

SVJ_TEST("library_io: a bank cell naming a path the library lost is restored empty") {
    LibraryFile file;
    LibraryFile::Clip a;
    a.cache = "D:/svcache/a.svcache";
    a.name = "a";
    file.clips = {a};
    LibraryFile::Bank bank;
    bank.name = "Banque 1";
    bank.a = {"D:/svcache/gone.svcache", a.cache};
    file.banks = {bank};

    Library library;
    Matrix matrix;
    library_restore(file, library, matrix);
    CHECK(matrix.at(0, DeckTarget::A, 0) == kNoClip);
    CHECK_EQ(library.at(matrix.at(0, DeckTarget::A, 1)).path, a.cache);
}

SVJ_TEST("library_io: a file without banks leaves the matrix as it was") {
    LibraryFile file;
    Library library;
    Matrix matrix;
    matrix.add_bank("Kept");
    library_restore(file, library, matrix);
    CHECK_EQ(matrix.bank_count(), 2);
}
