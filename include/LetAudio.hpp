#pragma once
#include <memory>
#include <string_view>
#include <expected>

#include <LetAudio_export.h>

namespace LetAudio {

class Source;

/// Audio engine. Lives longer than any registered source.
///
/// Creates via Engine::create().
class LETAUDIO_EXPORT Engine {
public:
    enum class SampleRate {
        Hz44100 = 44100,
        Hz48000 = 48000,
        Hz88200 = 88200,
        Hz96000 = 96000,
    };
    struct CreateSettings {
        bool logging = false;
        SampleRate sample_rate = SampleRate::Hz48000;
    };

    /// Creates engine and opens default audio output device.
    /// @return Engine or error.
    /// @param create_settings CreateSettings struct, specifies the initial settings for the engine work.
    static std::expected<std::unique_ptr<Engine>, std::string_view> create(CreateSettings create_settings);

    /// Toggles logging (by default disabled).
    /// @param enabled Boolean value (enabled/disabled).
    void toggle_logging(bool enabled);

    /// Destroys engine and all belonging sources.
    ~Engine() noexcept;
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    // /// Sets master volume (volume, that affects every source).
    // /// @param volume Float value from 0.0 (silence) to 1.0 (full).
    // ///        Out of range values are clipped.
    // void set_master_volume(float volume);

    // /// Current master volume.
    // /// @return Float value from 0.0 (silence) to 1.0 (full).
    // float master_volume() const;

    // /// Pauses all registered sources.
    // void pause_all_sources();

    // /// Stops all registered sources.
    // void stop_all_sources();

    // /// Unloads data from all registered sources.
    // void unload_all_sources();

private:
    friend class Source;
    struct Impl;
    std::unique_ptr<Impl> impl;
    Engine();
};


/// Audio source. 
///
/// Creates via Source::create().
class LETAUDIO_EXPORT Source {
public:
    // Destroys source and unregistering in engine.
    ~Source() noexcept;

    /// Describes audio positioning
    enum class PositioningType {
        Sound2D,
        // Sound3D, // unsupported
    };

    /// Describes audio decoding method
    enum class DecodingType {
        Preload,
        // Streaming, // unsupported
    };

    struct CreateSettings {
        PositioningType positioning_type = PositioningType::Sound2D;
        DecodingType decoding_type = DecodingType::Preload;
        int buffer_samples = 8192; // unsupported
        int buffers_count = 4; // unsupported
    };

    /// Creates source and registers it in Engine.
    /// @return Source or error.
    /// @param create_settings CreateSettings struct, specifies the initial settings for the source work.
    static std::expected<std::unique_ptr<Source>, std::string_view> create(Engine* engine, CreateSettings create_settings);

    Source(const Source&) = delete;
    Source(Source&&) = delete;
    Source& operator=(const Source&) = delete;
    Source& operator=(Source&&) = delete;

    /// Describes playback status
    enum class PlaybackStatus {
        Idle, Playing, Paused,
    };

    // /// Sets positioning type.
    // void set_source_positioning_type(PositioningType type);
    
    // /// Sets decoding type.
    // void set_source_decoding_type(DecodingType type);

    // /// Toogles loop mode.
    // void set_source_loop(bool loop);

    /// Sets volume.
    /// @param volume Float value from 0.0 (silence) to 1.0 (full).
    ///        Out of range values are clipped.
    void set_source_volume(float volume);

    /// Current positioning type.
    PositioningType source_positioning_type() const;

    /// Current decoding type.
    DecodingType source_decoding_type() const;

    // /// Current loop mode.
    // /// @return Boolean value (enabled/disabled).
    // bool source_loop() const;

    /// Current volume.
    /// @return Float value from 0.0 (silence) to 1.0 (full).
    float source_volume() const;

    /// Loads audio data from a file. Existing data is unloaded (if any). Also saves the file path (owned string).
    /// @param path string view for the path to the file being opened.
    std::expected<void, std::string_view> load_file(std::string_view path);

    /// Unloads current audio data.
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

    // /// Moves the playback position. Can be used while playing.
    // /// @param seconds Double value of position (in seconds) to jump to.
    // void seek(double seconds);
    
    /// Current status of playback.
    /// @return PlaybackStatus enum.
    PlaybackStatus playback_status() const;

    // /// Current position of playback.
    // /// @return Double value of playback position.
    // double playback_position() const;

private:
    friend class Engine;
    struct Impl;
    std::unique_ptr<Impl> impl;
    Source();
};

}