// scratchvj — choosing which audio input the carrier is on.
//
// WHY THIS IS NOT A LINE OF CODE IN THE WASAPI FILE. An interface is named in
// settings.json by a FRAGMENT ("MOTU"), because the full endpoint name is long,
// changes with the driver, and is different on every desk. A fragment can match
// more than one endpoint, and on the desk this project is built for it does:
// a MOTU exposes both "In 1-2 (MOTU Pro Audio)" with two channels and
// "In 1-24 (MOTU Pro Audio)" with twenty-four. The carrier is on channels 5/6,
// so exactly one of the two can carry it.
//
// The bug this module exists to have a test for: taking the FIRST name match
// and only then checking whether it is wide enough, which rejects the whole
// open when the narrow one happens to enumerate first. The channel count is not
// a validation to run afterwards -- it is part of the question being asked.
// Nothing about that rule is platform-specific, so it lives here, where it is
// tested on three operating systems without an audio device.
//
// The failure has to be NAMED, too. On Windows this runs inside a WIN32
// subsystem application whose stderr goes nowhere, so a reason that is not
// carried back to the caller is a reason nobody will ever read -- which is how
// a silent "the live platter did nothing" costs an afternoon.
#pragma once

#include <string>
#include <vector>

namespace svj {

struct AudioEndpoint {
    std::string name;
    unsigned channels = 0;
};

enum class EndpointVerdict {
    // `index` names an endpoint that matches and carries the pair.
    Chosen,
    // No endpoint's name contains the fragment.
    NoMatch,
    // The name matched, but no match has the pair. `detail` names the widest
    // one that matched, which is the one worth reporting: it tells the
    // operator the interface was found and how many channels it really has.
    TooFewChannels,
};

struct EndpointChoice {
    EndpointVerdict verdict = EndpointVerdict::NoMatch;
    int index = -1;
    std::string detail;
};

// `fragment` is matched case-insensitively as a substring of the name; the
// pair wanted is `first_channel` and the one after it, so an endpoint serves
// when it has at least `first_channel + 2` channels. Among several that serve,
// the first is taken -- enumeration order is the machine's, so a fragment that
// matches several usable endpoints is a fragment that needs to be more
// specific, and the caller reports which one it opened.
EndpointChoice choose_endpoint(const std::vector<AudioEndpoint>& endpoints,
                               const std::string& fragment, unsigned first_channel);

}  // namespace svj
