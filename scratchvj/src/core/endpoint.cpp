#include "core/endpoint.h"

#include <cctype>

namespace svj {
namespace {

std::string lowered(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

}  // namespace

EndpointChoice choose_endpoint(const std::vector<AudioEndpoint>& endpoints,
                               const std::string& fragment, unsigned first_channel) {
    const std::string want = lowered(fragment);
    const unsigned needed = first_channel + 2;

    EndpointChoice choice;
    int widest = -1;
    for (std::size_t i = 0; i < endpoints.size(); ++i) {
        const AudioEndpoint& endpoint = endpoints[i];
        if (lowered(endpoint.name).find(want) == std::string::npos) continue;

        if (endpoint.channels >= needed) {
            choice.verdict = EndpointVerdict::Chosen;
            choice.index = static_cast<int>(i);
            choice.detail = endpoint.name;
            return choice;
        }
        if (widest < 0 || endpoint.channels > endpoints[static_cast<std::size_t>(widest)].channels) {
            widest = static_cast<int>(i);
        }
    }

    if (widest >= 0) {
        const AudioEndpoint& endpoint = endpoints[static_cast<std::size_t>(widest)];
        choice.verdict = EndpointVerdict::TooFewChannels;
        choice.detail = endpoint.name + " : " + std::to_string(endpoint.channels) +
                        " canaux, il en faut " + std::to_string(needed);
    }
    return choice;
}

}  // namespace svj
