#pragma once
#include <memory>
#include <string_view>
#include <expected>

#include <LetAudio_export.h>

namespace LetAudio {

class Source;

/// Audio engine: owns all sources
///
/// Creates via Engine::create(). Lives longer than all own sources:
/// when the engine is destroyed, all Source* are invalidated.
class LETAUDIO_EXPORT Engine {
public:
    /// Creates engine and opens default audio output device.
    /// @return Engine or error.
    static std::expected<std::unique_ptr<Engine>, std::string_view> create();

    /// Destroys engine and all belonging sources.
    ~Engine() noexcept;

    Engine() = delete;
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    /// Sets master volume (volume, that affects every source).
    /// @param volume Float value from 0.0 (silence) to 2.0 (2x over normal).
    ///        Out of range values are clipped.
    void set_master_volume(float volume);

    /// Current master volume
    /// @return Float value from 0.0 (silence) to 2.0 (2x over normal).
    float master_volume() const;

    /// Creates and registers new audio source.
    /// @return Pointer, owned by engine. Do not free manually;
    ///         you can call Source::~Source() for destructing source
    ///         and unregistering in engine OR call Engine::delete_all_sources()
    ///         for doing same with every created by engine source.
    Source* create_source();

    // Pauses all registered sources
    void pause_all_sources();

    // Stops all registered sources
    void stop_all_sources();

    // Unloads file from all registered sources
    void unload_all_sources();

    // Destroys and unregistering all registered sources
    void delete_all_sources();

private:
    friend class Source;
    struct Impl;
    std::unique_ptr<Impl> impl;
};

class LETAUDIO_EXPORT Source {
public:
    // Destroys source and unregistering in engine.
    ~Source() noexcept;

    Source() = delete;
    Source(const Source&) = delete;
    Source(Source&&) = delete;
    Source& operator=(const Source&) = delete;
    Source& operator=(Source&&) = delete;

    enum class Type {
        Sound2D,
        // Sound3D,
    };
    enum class PlaybackStatus {
        Idle, Playing, Paused,
    };

    /// Sets the type of audio that source will be play
    /// @param type Type enum
    void set_source_type(Type type);

    // 
    void set_source_loop(bool loop);
    void set_source_volume(float volume);
    Type source_type() const;
    bool source_loop() const;
    float source_volume() const;

    bool load_file(std::string_view path);
    void unload_file();
    std::string_view current_file_path() const;
    double file_duration() const;

    void play(), pause(), stop(), seek(double seconds);
    
    PlaybackStatus playback_status() const;
    double playback_position() const;

private:
    friend class Engine;
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}