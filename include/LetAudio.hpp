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

    /// Current master volume.
    /// @return Float value from 0.0 (silence) to 2.0 (2x over normal).
    float master_volume() const;

    /// Creates and registers new audio source.
    /// @return Pointer, owned by engine. Do not free manually;
    ///         you can call Source::~Source() for destructing source
    ///         and unregistering in engine OR call Engine::delete_all_sources()
    ///         for doing same with every created by engine source.
    Source* create_source();

    /// Pauses all registered sources.
    void pause_all_sources();

    /// Stops all registered sources.
    void stop_all_sources();

    /// Unloads file from all registered sources.
    void unload_all_sources();

    /// Destroys and unregistering all registered sources.
    void delete_all_sources();

private:
    friend class Source;
    struct Impl;
    std::unique_ptr<Impl> impl;
};


/// Audio source: owns loaded data, owned by Engine.
///
/// Creates via Engine::create_source().
class LETAUDIO_EXPORT Source {
public:
    // Destroys source and unregistering in engine.
    ~Source() noexcept;

    Source() = delete;
    Source(const Source&) = delete;
    Source(Source&&) = delete;
    Source& operator=(const Source&) = delete;
    Source& operator=(Source&&) = delete;

    /// Describes audio positioning
    enum class Type {
        Sound2D,
        // Sound3D,
    };

    /// Describes playback status
    enum class PlaybackStatus {
        Idle, Playing, Paused,
    };

    /// Sets the type of audio that source will be play.
    /// @param type Type enum.
    void set_source_type(Type type);

    /// Toogles loop mode.
    void set_source_loop(bool loop);

    /// Sets volume.
    /// @param volume Float value from 0.0 (silence) to 2.0 (2x over normal).
    ///        Out of range values are clipped.
    void set_source_volume(float volume);

    /// Current type of audio.
    /// @return Type enum.
    Type source_type() const;

    /// Current loop mode.
    /// @return Boolean value (enabled/disabled).
    bool source_loop() const;

    /// Current volume.
    /// @return Float value from 0.0 (silence) to 2.0 (2x over normal).
    float source_volume() const;

    /// Loads audio data from a file. Existing data is unloaded (if any). Also saves the file path (owned string).
    /// @param path string view for the path to the file being opened.
    std::expected<void, std::string_view> load_file(std::string_view path);

    /// Unloads current audio data. Does nothing if no data.
    void unload_file();

    /// Path of current loaded file.
    /// @return String view of path to current loaded file (string data is owned by Source).
    std::string_view current_file_path() const;

    /// Duration of current loaded file.
    /// @return Double value of file duration (in seconds).
    double file_duration() const;

    /// Starts playback from current position (0 if just loaded).
    void play();

    /// Pauses playback (stops playing, not resets position).
    void pause();

    /// Stops playback (stops playing, resets position).
    void stop();

    /// Moves the playback position. Can be used while playing.
    /// @param seconds Double value of position (in seconds) to jump to.
    void seek(double seconds);
    
    /// Current status of playback.
    /// @return PlaybackStatus enum.
    PlaybackStatus playback_status() const;

    /// Current position of playback.
    /// @return Double value of playback position.
    double playback_position() const;

private:
    friend class Engine;
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}