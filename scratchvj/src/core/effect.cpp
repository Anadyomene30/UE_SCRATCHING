#include "core/effect.h"

#include <algorithm>

namespace svj {
namespace {

// The correspondence table. Every "Identical" row is a claim that the two sides
// are the same operation in a different domain -- not a resemblance.
const std::vector<EffectDescriptor> kCatalogue = {
    {EffectType::LowPass, "lowpass",
     "passe-bas : retire les hautes fréquences temporelles",
     "flou gaussien : retire les hautes fréquences spatiales", Correspondence::Identical},
    {EffectType::HighPass, "highpass",
     "passe-haut : retire les basses fréquences",
     "rehaussement de contours, gris médian", Correspondence::Identical},
    {EffectType::DjFilter, "djfilter",
     "filtre DJ : passe-bas ← neutre → passe-haut",
     "flou ← neutre → netteté", Correspondence::Identical},
    {EffectType::Bitcrusher, "bitcrush",
     "réduction de profondeur de bits",
     "postérisation et réduction de palette", Correspondence::Identical},
    {EffectType::Decimate, "decimate",
     "baisse de fréquence d'échantillonnage",
     "gel et décimation de frames", Correspondence::Identical},
    {EffectType::Delay, "delay",
     "delay à contre-réaction",
     "traînées d'images à contre-réaction, même équation", Correspondence::Identical},
    {EffectType::Distortion, "distortion",
     "écrêtage d'amplitude",
     "écrêtage des couleurs, saturation dure", Correspondence::Identical},
    {EffectType::RingMod, "ringmod",
     "multiplication par une porteuse",
     "multiplication par un motif spatial", Correspondence::Identical},
    {EffectType::Gate, "gate",
     "modulation d'amplitude",
     "strobe et gate de luminosité, au même taux", Correspondence::Identical},
    {EffectType::Reverse, "reverse",
     "lecture inversée", "frames à l'envers", Correspondence::Identical},
    {EffectType::BeatRepeat, "beatrepeat",
     "boucle de N temps",
     "boucle de frames de même durée", Correspondence::Identical},
    {EffectType::Noise, "noise",
     "bruit additif", "grain et neige", Correspondence::Identical},
    {EffectType::Reverb, "reverb",
     "diffusion et décroissance",
     "smear diffus, bloom persistant", Correspondence::Analogue},
    {EffectType::Flanger, "flanger",
     "filtre en peigne balayé",
     "déplacement UV ondulant, moiré chroma", Correspondence::Analogue},
    {EffectType::PitchShift, "pitch",
     "transposition", "zoom, échelle spatiale", Correspondence::Analogue},
    {EffectType::Pan, "pan",
     "position stéréo",
     "translation horizontale — et en 360, le yaw", Correspondence::Analogue},

    {EffectType::SlitScan, "slitscan", nullptr,
     "chaque colonne lit une frame différente de la fenêtre", Correspondence::VideoOnly},
    {EffectType::Datamosh, "datamosh", nullptr,
     "déplacement par le mouvement entre frames voisines", Correspondence::VideoOnly},
    {EffectType::Feedback, "feedback", nullptr,
     "rebouclage avec zoom et rotation", Correspondence::VideoOnly},
    {EffectType::Kaleidoscope, "kaleidoscope", nullptr,
     "segments miroir en rotation", Correspondence::VideoOnly},
    {EffectType::Mirror, "mirror", nullptr,
     "miroir horizontal, vertical ou quadruple", Correspondence::VideoOnly},
    {EffectType::Displace, "displace", nullptr,
     "déformation par un bruit ou une texture", Correspondence::VideoOnly},
    {EffectType::ChromaKey, "chromakey", nullptr,
     "incrustation par couleur", Correspondence::VideoOnly},
    {EffectType::LumaKey, "lumakey", nullptr,
     "incrustation par luminance", Correspondence::VideoOnly},
    {EffectType::PixelSort, "pixelsort", nullptr,
     "tri de pixels par seuil", Correspondence::VideoOnly},
    {EffectType::Tile, "tile", nullptr,
     "répétition en damier", Correspondence::VideoOnly},
    {EffectType::Polar, "polar", nullptr,
     "coordonnées polaires, dont la petite planète", Correspondence::VideoOnly},
    {EffectType::Halftone, "halftone", nullptr,
     "trame d'imprimerie, angle et pas", Correspondence::VideoOnly},
    {EffectType::Vhs, "vhs", nullptr,
     "scanlines, tracking, décalage chroma", Correspondence::VideoOnly},
    {EffectType::Bloom, "bloom", nullptr,
     "diffusion des hautes lumières", Correspondence::VideoOnly},
    {EffectType::HueIsolate, "hueisolate", nullptr,
     "garder une teinte, désaturer le reste", Correspondence::VideoOnly},
    {EffectType::Invert, "invert", nullptr,
     "négatif", Correspondence::VideoOnly},
};

}  // namespace

const std::vector<EffectDescriptor>& effect_catalogue() { return kCatalogue; }

const EffectDescriptor* describe(EffectType type) {
    for (const EffectDescriptor& d : kCatalogue) {
        if (d.type == type) return &d;
    }
    return nullptr;
}

double Sync::seconds(double beat_duration_s, float time_param, double free_max_s) const {
    if (tempo) return beats * beat_duration_s;
    return static_cast<double>(time_param) * free_max_s;
}

void EffectUnit::unlink() {
    if (!link) return;
    // Both sides keep exactly what they had, so nothing lurches at the moment the
    // chain is broken. Divergence should be something the user creates, never
    // something that happens to them.
    audio_override = shared;
    video_override = shared;
    link = false;
}

void EffectUnit::relink() { link = true; }

bool EffectUnit::audio_active() const {
    if (!enabled) return false;
    const EffectDescriptor* d = describe(type);
    if (d == nullptr || d->audio == nullptr) return false;  // video-only effect
    return audio_params().mix > 0.0f;
}

bool EffectUnit::video_active() const {
    return enabled && video_params().mix > 0.0f;
}

EffectRack::EffectRack(std::size_t slots) : units_(slots) {}

void EffectRack::load(std::size_t index, EffectType type) {
    if (index >= units_.size()) return;
    units_[index] = EffectUnit{};
    units_[index].type = type;
    units_[index].enabled = true;
}

void EffectRack::clear(std::size_t index) {
    if (index >= units_.size()) return;
    units_[index] = EffectUnit{};
}

void EffectRack::set_all_links(bool linked) {
    for (EffectUnit& unit : units_) {
        if (linked) unit.relink();
        else unit.unlink();
    }
}

std::size_t EffectRack::active_count() const {
    return static_cast<std::size_t>(std::count_if(
        units_.begin(), units_.end(),
        [](const EffectUnit& u) { return u.audio_active() || u.video_active(); }));
}

}  // namespace svj
