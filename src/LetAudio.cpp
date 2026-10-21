#include "../include/LetAudio.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <sndfile.h>

#include <utility>
#include <expected>
#include <memory>
#include <string_view>
#include <print>
#include <vector>
#include <mutex>

namespace LetAudio {

struct Engine::Impl {
    static std::expected<std::unique_ptr<Impl>, std::string_view> create(CreateSettings create_settings);
    Impl() = default;
    ~Impl() noexcept;

    ALCdevice* device = nullptr;
    ALCcontext* context = nullptr;

    bool logging = false;
    int sample_rate = 48000;

    bool check_al_error(std::string_view operation);
    bool check_alc_error(ALCdevice* device, std::string_view operation);

    std::vector<Source*> registered_sources;
    std::mutex sources_mutex;

    void register_source(Source* source);
    void unregister_source(Source* source);
};

Engine::Engine() = default;
Engine::~Engine() noexcept = default;

std::expected<std::unique_ptr<Engine>, std::string_view> Engine::create(CreateSettings create_settings) {
    std::unique_ptr<Engine> engine(new Engine());

    auto impl = Impl::create(create_settings);
    if (!impl) return std::unexpected(impl.error());

    engine->impl = std::move(impl.value());
    return engine;
}

void Engine::toggle_logging(bool enabled) {
    impl->logging = enabled;
}

bool Engine::Impl::check_al_error(std::string_view operation) {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        if (logging) std::println("<LetAudio>: OpenAL (AL) error '{}' during operation '{}'", alGetString(error), operation);
        return true;
    }
    return false;
}

bool Engine::Impl::check_alc_error(ALCdevice* device, std::string_view operation) {
    if (device == nullptr) return true;
    ALCenum error = alcGetError(device);
    if (error != ALC_NO_ERROR) {
        if (logging) std::println("<LetAudio>: OpenAL (ALC) error '{}' during operation '{}'", alcGetString(device, error), operation);
        return true;
    }
    return false;
}

std::expected<std::unique_ptr<Engine::Impl>, std::string_view> Engine::Impl::create(CreateSettings create_settings) {
    std::unique_ptr<Impl> impl(new Impl());
    impl->logging = create_settings.logging;
    impl->sample_rate = static_cast<int>(create_settings.sample_rate);

    impl->device = alcOpenDevice(nullptr);
    if (impl->device == nullptr || impl->check_alc_error(impl->device, "alcOpenDevice")) 
        return std::unexpected("Failed to open ALC device");
    
    if (impl->logging) {
        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE) {
            std::println("<LetAudio>: Opened ALCDevice name: {}", alcGetString(impl->device, ALC_ALL_DEVICES_SPECIFIER));
        }
    }

    ALCint attrs[] = {ALC_FREQUENCY, impl->sample_rate, 0};
    impl->context = alcCreateContext(impl->device, attrs);
    if (impl->context == nullptr || impl->check_alc_error(impl->device, "alcCreateContext")) {
        alcCloseDevice(impl->device);
        return std::unexpected("Failed to create ALC context");
    }

    if (!alcMakeContextCurrent(impl->context)) {
        alcDestroyContext(impl->context);
        alcCloseDevice(impl->device);
        impl->check_alc_error(impl->device, "alcMakeContextCurrent");
        return std::unexpected("Failed to make ALC context current");
    }

    if (impl->logging)
        std::println("<LetAudio>: Created engine");

    return impl;
}

Engine::Impl::~Impl() noexcept {
    {
        std::lock_guard<std::mutex> lock(sources_mutex);
        registered_sources.clear();
    }

    alcMakeContextCurrent(nullptr);
    if (context) alcDestroyContext(context);
    if (device) alcCloseDevice(device);
}

struct Source::Impl {
    Engine* engine = nullptr;
    Source* owner = nullptr;

    PositioningType positioning_type;
    DecodingType decoding_type;
    int buffer_samples, buffers_count;

    ALuint al_source = 0;
    std::vector<ALuint> buffers;

    std::vector<int16_t> decode_buffer;

    std::string current_path;
    SF_INFO sf_info{};
    SNDFILE* snd_file = nullptr;

    PlaybackStatus state = PlaybackStatus::Idle;
    std::mutex state_mutex;

    float volume = 1.0f;

    Impl() = default;
    ~Impl() noexcept;
    static std::expected<std::unique_ptr<Impl>, std::string_view> create(Engine* engine, Source* owner, CreateSettings create_settings);
};

Source::Source() = default;
Source::~Source() noexcept {
    if (impl && impl->engine)
        impl->engine->impl->unregister_source(this);
}

std::expected<std::unique_ptr<Source>, std::string_view> Source::create(Engine* engine, CreateSettings create_settings) {
    std::unique_ptr<Source> source(new Source());

    auto impl = Impl::create(engine, source.get(), create_settings);
    if (!impl) return std::unexpected(impl.error());
    source->impl = std::move(impl.value());

    engine->impl->register_source(source.get());

    return source; 
}

void Engine::Impl::register_source(Source* source) {
    std::lock_guard<std::mutex> lock(sources_mutex);
    registered_sources.push_back(source);
}

void Engine::Impl::unregister_source(Source* source) {
    std::lock_guard<std::mutex> lock(sources_mutex);
    std::erase(registered_sources, source);
}

std::expected<std::unique_ptr<Source::Impl>, std::string_view> Source::Impl::create(Engine* engine, Source* owner, CreateSettings create_settings) {
    (void)create_settings.positioning_type;

    std::unique_ptr<Impl> impl(new Impl());
    impl->engine = engine;
    impl->owner = owner;
    impl->positioning_type = create_settings.positioning_type;
    impl->decoding_type = create_settings.decoding_type;
    impl->buffer_samples = create_settings.buffer_samples;
    impl->buffers_count = create_settings.buffers_count;

    alGenSources(1, &impl->al_source);
    if (impl->engine->impl->check_al_error("alGenSources"))
        return std::unexpected("Failed to generate AL source");

    // disables HRTF (which is not currently needed, but might be needed in future for 3D sound sources)
    alSourcei(impl->al_source, AL_DIRECT_CHANNELS_SOFT, AL_REMIX_UNMATCHED_SOFT);

    impl->buffers.resize(impl->decoding_type == DecodingType::Preload ? 1 : impl->buffers_count);
    alGenBuffers(impl->buffers.size(), impl->buffers.data());
    if (impl->engine->impl->check_al_error("alGenBuffers"))
        return std::unexpected("Failed to generate AL buffers");

    // if (impl->decoding_type == DecodingType::Streaming)
    //     impl->decode_buffer.resize(impl->buffer_samples * 2);

    return impl;
}

Source::Impl::~Impl() noexcept {
    owner->unload_file();
}

std::expected<void, std::string_view> Source::load_file(std::string_view path) {
    if (impl->decoding_type != DecodingType::Preload)
        return std::unexpected("Only Preload mode is supported");

    if (impl->state != PlaybackStatus::Idle)
        stop();

    std::lock_guard<std::mutex> guard(impl->state_mutex);

    impl->current_path = std::string(path);

    impl->sf_info = {};
    impl->snd_file = sf_open(impl->current_path.c_str(), SFM_READ, &impl->sf_info);
    if (!impl->snd_file)
        return std::unexpected("libsndfile: Failed to open audio file");

    ALenum al_format = 0;
    if (impl->sf_info.channels == 1) {
        al_format = AL_FORMAT_MONO16;
    } else if (impl->sf_info.channels == 2) {
        al_format = AL_FORMAT_STEREO16;
    } else {
        sf_close(impl->snd_file);
        impl->snd_file = nullptr;
        return std::unexpected("Unsupported channel count");
    }
    
    sf_count_t total_samples = impl->sf_info.frames * impl->sf_info.channels;
    impl->decode_buffer = std::vector<int16_t>(total_samples);

    sf_count_t frames_read = sf_readf_short(impl->snd_file, impl->decode_buffer.data(), impl->sf_info.frames);
    if (frames_read < impl->sf_info.frames) {
        if (impl->engine->impl->logging)
            std::println("<LetAudio>: libsndfile: readed frames is less than file has");

        impl->decode_buffer.resize(frames_read * impl->sf_info.channels);
    }

    ALsizei data_size_bytes = impl->decode_buffer.size() * sizeof(int16_t);

    alBufferData(impl->buffers[0], al_format, impl->decode_buffer.data(), data_size_bytes, impl->sf_info.samplerate);
    if (impl->engine->impl->check_al_error("alBufferData (Preload)"))
        return std::unexpected("Failed to upload AL buffer data");

    alSourcei(impl->al_source, AL_BUFFER, static_cast<ALint>(impl->buffers[0]));
    if (impl->engine->impl->check_al_error("alSourcei (AL_BUFFER)"))
        return std::unexpected("Failed to bind AL buffer to source");

    if (impl->engine->impl->logging)
        std::println("<LetAudio>: Successfully preloaded file '{}' (Channels: {}, Sample rate: {}Hz)", 
                     impl->current_path, impl->sf_info.channels, impl->sf_info.samplerate);

    return {};
}

std::string_view Source::current_file_path() const {
    return std::string_view(impl->current_path);
}

Source::PositioningType Source::source_positioning_type() const {
    return impl->positioning_type;
}

Source::DecodingType Source::source_decoding_type() const {
    return impl->decoding_type;
}

void Source::play() {
    if (impl->decoding_type != DecodingType::Preload)
        return;
    
    std::lock_guard<std::mutex> guard(impl->state_mutex);

    if (impl->state != PlaybackStatus::Playing) {
        if (impl->state != PlaybackStatus::Paused) alSourceRewind(impl->al_source);
        alSourcePlay(impl->al_source);
        impl->engine->impl->check_al_error("alSourcePlay");

        impl->state = PlaybackStatus::Playing;
    }
}

void Source::stop() {
    if (impl->decoding_type != DecodingType::Preload)
        return;

    std::lock_guard<std::mutex> guard(impl->state_mutex);

    if (impl->state != PlaybackStatus::Idle) {
        alSourceStop(impl->al_source);
        impl->engine->impl->check_al_error("alSourceStop");

        impl->state = PlaybackStatus::Idle;
    }
}

void Source::pause() {
    if (impl->decoding_type != DecodingType::Preload)
        return;

    std::lock_guard<std::mutex> guard(impl->state_mutex);

    if (impl->state == PlaybackStatus::Playing) {
        alSourcePause(impl->al_source);
        impl->engine->impl->check_al_error("alSourcePause");

        impl->state = PlaybackStatus::Paused;
    }
}

Source::PlaybackStatus Source::playback_status() const {
    return impl->state;
}

double Source::file_duration() const {
    return impl->snd_file ? impl->sf_info.frames / impl->sf_info.samplerate : 0;
}

void Source::unload_file() {
    if (impl->decoding_type != DecodingType::Preload)
        return;

    if (impl->state != PlaybackStatus::Idle) stop();
    std::lock_guard<std::mutex> guard(impl->state_mutex);

    alDeleteSources(1, &impl->al_source);
    alDeleteBuffers(impl->buffers.size(), impl->buffers.data());

    if (impl->snd_file) sf_close(impl->snd_file);
    impl->sf_info = {};
    impl->current_path = {};
    impl->decode_buffer.clear();
}

void Source::set_source_volume(float volume) {
    impl->volume = volume;
    if (impl->decoding_type == DecodingType::Preload)
        alSourcef(impl->al_source, AL_GAIN, volume);
}

float Source::source_volume() const {
    return impl->volume;
}

}