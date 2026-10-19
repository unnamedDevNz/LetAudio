#include "../include/LetAudio.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <utility>
#include <expected>
#include <memory>
#include <string_view>
#include <print>

namespace LetAudio {

struct Engine::Impl {
    static std::expected<std::unique_ptr<Impl>, std::string_view> create(bool logging);
    Impl() = default;
    ~Impl() noexcept;

    ALCdevice* device = nullptr;
    ALCcontext* context = nullptr;

    bool logging = false;
    bool check_al_error(std::string_view operation);
    bool check_alc_error(ALCdevice* device, std::string_view operation);
};

Engine::Engine() = default;
Engine::~Engine() noexcept = default;

std::expected<std::unique_ptr<Engine>, std::string_view> Engine::create(bool logging) {
    std::unique_ptr<Engine> engine(new Engine());

    auto impl = Impl::create(logging);
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

std::expected<std::unique_ptr<Engine::Impl>, std::string_view> Engine::Impl::create(bool logging) {
    std::unique_ptr<Impl> impl(new Impl());
    impl->logging = logging;

    impl->device = alcOpenDevice(nullptr);
    if (impl->device == nullptr || impl->check_alc_error(impl->device, "alcOpenDevice")) 
        return std::unexpected("Failed to open ALC device");
    
    if (impl->logging) {
        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE) {
            std::println("<LetAudio>: Opened ALCDevice name: {}", alcGetString(impl->device, ALC_ALL_DEVICES_SPECIFIER));
        }
    }

    impl->context = alcCreateContext(impl->device, nullptr);
    if (impl->device == nullptr || impl->check_alc_error(impl->device, "alcCreateContext")) {
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
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

}