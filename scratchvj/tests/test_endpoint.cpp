#include <vector>

#include "core/endpoint.h"
#include "harness.h"

using namespace svj;

namespace {

// What this desk actually enumerates, measured with audio_probe: a MOTU
// exposes its first pair and its whole 24-channel front separately, and both
// names contain "MOTU".
std::vector<AudioEndpoint> desk() {
    return {
        {"Headset Microphone (Oculus Virtual Audio Device)", 2},
        {"In 1-2 (MOTU Pro Audio)", 2},
        {"Microphone (HD Pro Webcam C920)", 1},
        {"In 1-24 (MOTU Pro Audio)", 24},
    };
}

}  // namespace

SVJ_TEST("endpoint: a fragment matching two inputs takes the one that has the pair") {
    // The bug this test exists for: taking the first name match and only THEN
    // checking the channel count rejects the open outright, because the
    // two-channel "In 1-2" enumerates before the twenty-four-channel "In 1-24".
    // Measured on the desk: the live platter opened nothing, and said so only
    // on a stderr that a WIN32 application does not have.
    const EndpointChoice choice = choose_endpoint(desk(), "MOTU", 4);

    CHECK(choice.verdict == EndpointVerdict::Chosen);
    CHECK_EQ(choice.index, 3);
    CHECK_EQ(choice.detail, std::string("In 1-24 (MOTU Pro Audio)"));
}

SVJ_TEST("endpoint: the narrow input is right when the pair asked for fits on it") {
    // And the same fragment must NOT always skip to the widest: channels 1/2
    // are on the first one, which is what an operator naming "MOTU" for a
    // stereo pair means.
    const EndpointChoice choice = choose_endpoint(desk(), "MOTU", 0);

    CHECK(choice.verdict == EndpointVerdict::Chosen);
    CHECK_EQ(choice.index, 1);
}

SVJ_TEST("endpoint: the fragment is matched without regard to case") {
    CHECK(choose_endpoint(desk(), "motu pro", 4).verdict == EndpointVerdict::Chosen);
    CHECK(choose_endpoint(desk(), "IN 1-24", 4).verdict == EndpointVerdict::Chosen);
}

SVJ_TEST("endpoint: a mono input cannot carry a pair, however well it is named") {
    // Written after this test file got it wrong: the webcam microphone matches
    // its fragment and is one channel, and a quadrature pair needs two legs.
    // "Matches the name" is not "can carry the signal", which is the whole
    // point of the module.
    const EndpointChoice choice = choose_endpoint(desk(), "Webcam", 0);
    CHECK(choice.verdict == EndpointVerdict::TooFewChannels);
}

SVJ_TEST("endpoint: an interface found but too narrow is named, not just refused") {
    // Two failures that look alike from the outside -- "the interface is not
    // there" and "the interface is there and has four channels, not six" --
    // and only one of them is fixed by checking a cable.
    const EndpointChoice choice = choose_endpoint(desk(), "MOTU", 30);

    CHECK(choice.verdict == EndpointVerdict::TooFewChannels);
    CHECK_EQ(choice.index, -1);
    // The widest match is the informative one: it says how many the interface
    // really has.
    CHECK(choice.detail.find("In 1-24") != std::string::npos);
    CHECK(choice.detail.find("24") != std::string::npos);
    CHECK(choice.detail.find("32") != std::string::npos);
}

SVJ_TEST("endpoint: a fragment that names nothing is a different failure") {
    const EndpointChoice choice = choose_endpoint(desk(), "Scarlett", 0);

    CHECK(choice.verdict == EndpointVerdict::NoMatch);
    CHECK_EQ(choice.index, -1);
    CHECK(choice.detail.empty());
}

SVJ_TEST("endpoint: the pair must fit whole, not just start inside the device") {
    // first_channel is the LEFT leg, so the last usable one on a 24-channel
    // interface is 22 (channels 23 and 24). Off by one here would open a
    // device and read a leg that is not there.
    const std::vector<AudioEndpoint> one = {{"In 1-24 (MOTU Pro Audio)", 24}};

    CHECK(choose_endpoint(one, "MOTU", 22).verdict == EndpointVerdict::Chosen);
    CHECK(choose_endpoint(one, "MOTU", 23).verdict == EndpointVerdict::TooFewChannels);
}

SVJ_TEST("endpoint: nothing enumerated is no match rather than a crash") {
    const EndpointChoice choice = choose_endpoint({}, "MOTU", 4);
    CHECK(choice.verdict == EndpointVerdict::NoMatch);
    CHECK_EQ(choice.index, -1);
}
