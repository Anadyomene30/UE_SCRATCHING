#include "audio_in.h"

#if defined(_WIN32)

#define NOMINMAX
#include <windows.h>

#include <initguid.h>

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <functiondiscoverykeys_devpkey.h>

#include <algorithm>
#include <atomic>
#include <cctype>

namespace svj::ui {
namespace {

std::string narrow(const wchar_t* text) {
    if (text == nullptr) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(size > 0 ? size - 1 : 0), '\0');
    if (size > 1) {
        WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), size, nullptr, nullptr);
    }
    return out;
}

std::string device_name(IMMDevice* device) {
    IPropertyStore* store = nullptr;
    if (FAILED(device->OpenPropertyStore(STGM_READ, &store))) return {};
    PROPVARIANT value;
    PropVariantInit(&value);
    std::string name;
    if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &value)) &&
        value.vt == VT_LPWSTR) {
        name = narrow(value.pwszVal);
    }
    PropVariantClear(&value);
    store->Release();
    return name;
}

std::string lowered(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

}  // namespace

struct AudioInput::Impl {
    IMMDevice* device = nullptr;
    IAudioClient* client = nullptr;
    IAudioCaptureClient* capture = nullptr;
    WAVEFORMATEX* format = nullptr;
    bool is_float = false;

    ~Impl() {
        if (capture != nullptr) capture->Release();
        if (client != nullptr) client->Release();
        if (format != nullptr) CoTaskMemFree(format);
        if (device != nullptr) device->Release();
    }
};

AudioInput::~AudioInput() { close(); }

bool AudioInput::open(const std::string& endpoint, unsigned first_channel) {
    close();

    // COM per thread; the capture thread initialises its own.
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool owns_com = SUCCEEDED(com);

    IMMDeviceEnumerator* enumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&enumerator)))) {
        if (owns_com) CoUninitialize();
        return false;
    }
    IMMDeviceCollection* collection = nullptr;
    enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
    UINT count = 0;
    if (collection != nullptr) collection->GetCount(&count);

    auto* impl = new Impl();
    const std::string want = lowered(endpoint);
    for (UINT i = 0; i < count && impl->device == nullptr; ++i) {
        IMMDevice* candidate = nullptr;
        collection->Item(i, &candidate);
        const std::string name = device_name(candidate);
        if (lowered(name).find(want) != std::string::npos) {
            impl->device = candidate;
            endpoint_ = name;
        } else {
            candidate->Release();
        }
    }
    if (collection != nullptr) collection->Release();
    enumerator->Release();

    if (impl->device == nullptr) {
        delete impl;
        if (owns_com) CoUninitialize();
        return false;
    }

    if (FAILED(impl->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                      reinterpret_cast<void**>(&impl->client))) ||
        FAILED(impl->client->GetMixFormat(&impl->format)) || impl->format == nullptr) {
        delete impl;
        if (owns_com) CoUninitialize();
        return false;
    }

    // SHARED, and only shared. See the header: exclusive would take the
    // interface away from Serato.
    if (FAILED(impl->client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 1000000, 0,
                                        impl->format, nullptr)) ||
        FAILED(impl->client->GetService(__uuidof(IAudioCaptureClient),
                                        reinterpret_cast<void**>(&impl->capture)))) {
        delete impl;
        if (owns_com) CoUninitialize();
        return false;
    }

    channels_ = impl->format->nChannels;
    if (first_channel + 1 >= channels_) {
        delete impl;
        if (owns_com) CoUninitialize();
        return false;
    }
    sample_rate_ = impl->format->nSamplesPerSec;
    first_channel_ = first_channel;
    impl->is_float =
        impl->format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (impl->format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         reinterpret_cast<WAVEFORMATEXTENSIBLE*>(impl->format)->SubFormat ==
             KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    impl_ = impl;
    stop_ = false;
    running_ = true;
    thread_ = std::thread([this] { run(); });
    return true;
}

void AudioInput::close() {
    if (thread_.joinable()) {
        stop_ = true;
        thread_.join();
    }
    delete impl_;
    impl_ = nullptr;
    running_ = false;
    {
        std::lock_guard<std::mutex> guard(lock_);
        pending_.clear();
    }
    endpoint_.clear();
    sample_rate_ = 0.0;
    channels_ = 0;
}

void AudioInput::run() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    impl_->client->Start();

    std::vector<float> local;
    while (!stop_) {
        UINT32 available = 0;
        impl_->capture->GetNextPacketSize(&available);
        while (available > 0 && !stop_) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(impl_->capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) {
                break;
            }
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr;
            const unsigned stride = channels_;
            for (UINT32 f = 0; f < frames; ++f) {
                float left = 0.0f, right = 0.0f;
                if (!silent) {
                    if (impl_->is_float) {
                        const auto* samples = reinterpret_cast<const float*>(data);
                        left = samples[f * stride + first_channel_];
                        right = samples[f * stride + first_channel_ + 1];
                    } else if (impl_->format->wBitsPerSample == 16) {
                        const auto* samples = reinterpret_cast<const std::int16_t*>(data);
                        left = samples[f * stride + first_channel_] / 32768.0f;
                        right = samples[f * stride + first_channel_ + 1] / 32768.0f;
                    }
                }
                // Silent packets still take their place in time: dropping them
                // would splice the carrier and read as a jump.
                local.push_back(left);
                local.push_back(right);
            }
            impl_->capture->ReleaseBuffer(frames);
            impl_->capture->GetNextPacketSize(&available);
        }
        if (!local.empty()) {
            std::lock_guard<std::mutex> guard(lock_);
            pending_.insert(pending_.end(), local.begin(), local.end());
            captured_ += local.size() / 2;
            local.clear();
        }
        Sleep(2);
    }

    impl_->client->Stop();
    CoUninitialize();
}

void AudioInput::drain(std::vector<float>& out) {
    std::lock_guard<std::mutex> guard(lock_);
    out.swap(pending_);
    pending_.clear();
}

std::uint64_t AudioInput::frames_captured() const {
    std::lock_guard<std::mutex> guard(const_cast<std::mutex&>(lock_));
    return captured_;
}

}  // namespace svj::ui

#else

namespace svj::ui {
struct AudioInput::Impl {};
AudioInput::~AudioInput() { close(); }
bool AudioInput::open(const std::string&, unsigned) { return false; }
void AudioInput::close() {}
void AudioInput::run() {}
void AudioInput::drain(std::vector<float>& out) { out.clear(); }
std::uint64_t AudioInput::frames_captured() const { return 0; }
}  // namespace svj::ui

#endif
