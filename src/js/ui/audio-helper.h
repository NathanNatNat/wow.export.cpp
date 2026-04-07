/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <vector>
#include <functional>

// Forward declare miniaudio types to avoid exposing the header.
struct ma_engine;
struct ma_sound;
struct ma_decoder;

/**
 * Audio file type constants.
 * JS equivalent: Symbol('AudioTypeUnk'), Symbol('AudioTypeOgg'), Symbol('AudioTypeMP3')
 */
enum class AudioType {
	Unknown,
	OGG,
	MP3
};

/**
 * Detect the audio file type from raw data.
 * JS equivalent: detectFileType(data)
 * @param data BufferWrapper (or similar) whose startsWith() method is used in JS.
 *             Here we accept raw bytes.
 * @param size Number of bytes available.
 * @returns AudioType enum value.
 */
AudioType detectFileType(const uint8_t* data, size_t size);

/**
 * AudioPlayer — C++ equivalent of the JS AudioPlayer class.
 * Uses miniaudio for decoding and playback instead of Web Audio API.
 *
 * JS equivalent: class AudioPlayer
 */
class AudioPlayer {
public:
	AudioPlayer();
	~AudioPlayer();

	// Non-copyable, moveable.
	AudioPlayer(const AudioPlayer&) = delete;
	AudioPlayer& operator=(const AudioPlayer&) = delete;
	AudioPlayer(AudioPlayer&& other) noexcept;
	AudioPlayer& operator=(AudioPlayer&& other) noexcept;

	/**
	 * Initialize the audio engine.
	 * JS equivalent: init()
	 */
	void init();

	/**
	 * Load audio data from a raw byte buffer.
	 * JS equivalent: async load(array_buffer)
	 * @param data Raw audio bytes (OGG, MP3, etc.).
	 */
	void load(const std::vector<uint8_t>& data);

	/**
	 * Unload the current audio buffer.
	 * JS equivalent: unload()
	 */
	void unload();

	/**
	 * Start or resume playback.
	 * JS equivalent: play(from_offset)
	 * @param from_offset Optional playback position in seconds (-1 = current).
	 */
	void play(double from_offset = -1.0);

	/**
	 * Pause playback, retaining current position.
	 * JS equivalent: pause()
	 */
	void pause();

	/**
	 * Stop playback and reset position to the beginning.
	 * JS equivalent: stop()
	 */
	void stop();

	/**
	 * Seek to a position in seconds.
	 * JS equivalent: seek(position)
	 * @param position Position in seconds.
	 */
	void seek(double position);

	/**
	 * Get current playback position in seconds.
	 * JS equivalent: get_position()
	 * @returns Current position in seconds.
	 */
	double get_position();

	/**
	 * Get total duration in seconds.
	 * JS equivalent: get_duration()
	 * @returns Duration in seconds, or 0 if no buffer loaded.
	 */
	double get_duration();

	/**
	 * Set playback volume.
	 * JS equivalent: set_volume(value)
	 * @param value Volume level (0.0 – 1.0).
	 */
	void set_volume(float value);

	/**
	 * Enable or disable looping.
	 * JS equivalent: set_loop(enabled)
	 * @param enabled True to loop.
	 */
	void set_loop(bool enabled);

	/**
	 * Destroy the audio engine and release all resources.
	 * JS equivalent: destroy()
	 */
	void destroy();

	/// True if audio is currently playing.
	bool is_playing = false;

	/// Callback invoked when playback ends naturally (not stopped programmatically).
	std::function<void()> on_ended;

private:
	/**
	 * Stop the active source without resetting offset.
	 * JS equivalent: stop_source()
	 */
	void stop_source();

	ma_engine* engine = nullptr;
	ma_sound* sound = nullptr;
	ma_decoder* decoder = nullptr;

	std::vector<uint8_t> audio_data; // Owned copy of audio bytes for the decoder.

	double start_offset = 0.0;
	bool loop = false;
	float volume = 1.0f;
	double duration_cache = 0.0;
};
