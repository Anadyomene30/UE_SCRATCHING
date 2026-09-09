#include "core/profile.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace svj {
namespace {

std::string lowered(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

}  // namespace

std::vector<LearnTarget> profile_targets(const DeviceProfile& profile) {
    std::vector<LearnTarget> targets;
    targets.reserve(profile.controls.size());
    for (const ProfileControl& c : profile.controls) {
        targets.push_back(LearnTarget{c.id, c.kind});
    }
    return targets;
}

void declare_profile(const DeviceProfile& profile, Surface& surface) {
    for (const ProfileControl& c : profile.controls) surface.declare(c.id, c.kind, c.encoder);
}

void bind_profile(const DeviceProfile& profile, std::uint8_t device_index, Surface& surface) {
    for (const ProfileBinding& b : profile.bindings) {
        const ProfileControl* control = profile_control(profile, b.id);
        if (control == nullptr) continue;
        const ControlIndex index = surface.declare(control->id, control->kind, control->encoder);
        MidiAddress address = b.address;
        address.device = device_index;
        surface.bind(address, index);
    }
}

const ProfileControl* profile_control(const DeviceProfile& profile, std::string_view id) {
    for (const ProfileControl& c : profile.controls) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

bool profile_has_geometry(const DeviceProfile& profile) {
    if (profile.panel_w <= 0.0f || profile.panel_h <= 0.0f) return false;
    for (const ProfileGroup& g : profile.layout) {
        if (!g.front_panel && !g.placed()) return false;
    }
    return true;
}

std::vector<std::string> validate_profile(const DeviceProfile& profile) {
    std::vector<std::string> faults;
    if (profile.name.empty()) faults.push_back("le profil n'a pas de nom");
    std::set<std::string> ids;
    for (const ProfileControl& c : profile.controls) {
        if (c.id.empty()) {
            faults.push_back("un contrôle sans id");
            continue;
        }
        if (!ids.insert(c.id).second) faults.push_back("id en double : " + c.id);
        if (c.kind != ControlKind::Encoder && c.encoder != EncoderMode::Absolute) {
            faults.push_back("mode relatif sur un contrôle qui n'est pas un encodeur : " + c.id);
        }
    }
    for (const ProfileGroup& group : profile.layout) {
        for (const std::string& id : group.controls) {
            if (ids.count(id) == 0) {
                faults.push_back("le groupe « " + group.title + " » dessine un contrôle inconnu : " + id);
            }
        }
        if (group.placed() && profile.panel_w > 0.0f &&
            (group.x < 0.0f || group.y < 0.0f || group.x + group.w > profile.panel_w + 0.01f ||
             group.y + group.h > profile.panel_h + 0.01f)) {
            faults.push_back("le groupe « " + group.title + " » sort du panneau");
        }
    }
    std::set<std::uint32_t> addresses;
    for (const ProfileBinding& b : profile.bindings) {
        if (ids.count(b.id) == 0) {
            faults.push_back("liaison vers un contrôle inconnu : " + b.id);
        }
        if (!addresses.insert(b.address.key()).second) {
            faults.push_back("deux liaisons sur la même adresse MIDI, dont : " + b.id);
        }
    }
    return faults;
}

int match_port(std::string_view fragment, int ordinal, const std::vector<std::string>& ports) {
    if (fragment.empty()) return -1;
    const std::string want = lowered(fragment);
    int seen = 0;
    for (std::size_t i = 0; i < ports.size(); ++i) {
        if (lowered(ports[i]).find(want) == std::string::npos) continue;
        if (seen == ordinal) return static_cast<int>(i);
        ++seen;
    }
    return -1;
}

}  // namespace svj
