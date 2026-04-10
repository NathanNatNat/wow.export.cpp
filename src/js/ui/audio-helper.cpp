/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

// NOMINMAX must be defined before miniaudio includes <windows.h> on MSVC,
// otherwise the min/max macros conflict with std::min / std::max.
#ifdef _WIN32
#define NOMINMAX
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#undef MINIAUDIO_IMPLEMENTATION

#include "audio-helper.h"

#include <algorithm>
#include <cstring>
#include <cmath>

// -----------------------------------------------------------------------
// detectFileType — detect audio format from raw bytes.
// JS equivalent: detectFileType(data)
// -----------------------------------------------------------------------

AudioType detectFileType(const uint8_t* data, size_t size) {
	// OGG: starts with "OggS"
	if (size >= 4 && std::memcmp(data, "OggS", 4) == 0)
		return AudioType::OGG;

	// MP3: starts with "ID3" or common sync bytes 0xFFFB / 0xFFF3 / 0xFFF2
	if (size >= 3 && std::memcmp(data, "ID3", 3) == 0)
		return AudioType::MP3;

	if (size >= 2) {
		if ((data[0] == 0xFF && data[1] == 0xFB) ||
		    (data[0] == 0xFF && data[1] == 0xF3) ||
		    (data[0] == 0xFF && data[1] == 0xF2))
			return AudioType::MP3;
	}

	return AudioType::Unknown;
}

// -----------------------------------------------------------------------
// AudioPlayer
// -----------------------------------------------------------------------

AudioPlayer::AudioPlayer() = default;

AudioPlayer::~AudioPlayer() {
	destroy();
}

AudioPlayer::AudioPlayer(AudioPlayer&& other) noexcept
	: engine(other.engine),
	  sound(other.sound),
	  decoder(other.decoder),
	  audio_data(std::move(other.audio_data)),
	  start_offset(other.start_offset),
	  loop(other.loop),
	  volume(other.volume),
	  duration_cache(other.duration_cache),
	  is_playing(other.is_playing),
	  on_ended(std::move(other.on_ended))
{
	other.engine = nullptr;
	other.sound = nullptr;
	other.decoder = nullptr;
	other.is_playing = false;
}

AudioPlayer& AudioPlayer::operator=(AudioPlayer&& other) noexcept {
	if (this != &other) {
		destroy();
		engine = other.engine;
		sound = other.sound;
		decoder = other.decoder;
		audio_data = std::move(other.audio_data);
		start_offset = other.start_offset;
		loop = other.loop;
		volume = other.volume;
		duration_cache = other.duration_cache;
		is_playing = other.is_playing;
		on_ended = std::move(other.on_ended);

		other.engine = nullptr;
		other.sound = nullptr;
		other.decoder = nullptr;
		other.is_playing = false;
	}
	return *this;
}

void AudioPlayer::init() {
	if (engine)
		return;

	engine = new ma_engine();
	ma_engine_config config = ma_engine_config_init();
	if (ma_engine_init(&config, engine) != MA_SUCCESS) {
		delete engine;
		engine = nullptr;
	}
}

void AudioPlayer::load(const std::vector<uint8_t>& data) {
	stop();

	// Keep a copy of the data so the decoder can read from it.
	audio_data = data;
	duration_cache = 0.0;

	// Create a decoder to determine duration.
	ma_decoder_config decoderConfig = ma_decoder_config_init_default();
	ma_decoder tempDecoder;
	if (ma_decoder_init_memory(audio_data.data(), audio_data.size(), &decoderConfig, &tempDecoder) == MA_SUCCESS) {
		ma_uint64 frameCount = 0;
		ma_decoder_get_length_in_pcm_frames(&tempDecoder, &frameCount);
		if (frameCount > 0 && tempDecoder.outputSampleRate > 0)
			duration_cache = static_cast<double>(frameCount) / static_cast<double>(tempDecoder.outputSampleRate);
		ma_decoder_uninit(&tempDecoder);
	}
}

void AudioPlayer::unload() {
	stop();
	audio_data.clear();
	start_offset = 0;
	duration_cache = 0.0;
}

void AudioPlayer::play(double from_offset) {
	if (audio_data.empty() || !engine)
		return;

	stop_source();

	if (from_offset >= 0.0)
		start_offset = std::max(0.0, std::min(from_offset, duration_cache));

	// Create decoder from memory.
	decoder = new ma_decoder();
	ma_decoder_config decoderConfig = ma_decoder_config_init_default();
	if (ma_decoder_init_memory(audio_data.data(), audio_data.size(), &decoderConfig, decoder) != MA_SUCCESS) {
		delete decoder;
		decoder = nullptr;
		return;
	}

	// Seek to offset if needed.
	if (start_offset > 0.0 && decoder->outputSampleRate > 0) {
		ma_uint64 offsetFrames = static_cast<ma_uint64>(start_offset * decoder->outputSampleRate);
		ma_decoder_seek_to_pcm_frame(decoder, offsetFrames);
	}

	// Create sound from decoder (data source).
	sound = new ma_sound();
	if (ma_sound_init_from_data_source(engine, decoder, 0, nullptr, sound) != MA_SUCCESS) {
		ma_decoder_uninit(decoder);
		delete decoder;
		decoder = nullptr;
		delete sound;
		sound = nullptr;
		return;
	}

	ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);
	ma_sound_set_volume(sound, volume);

	// miniaudio does not have a direct per-sound "onended" callback like Web Audio API.
	// The on_ended callback is fired from get_position() which polls ma_sound_at_end().
	// Consumers must call get_position() periodically for end-of-playback detection.

	ma_sound_start(sound);
	is_playing = true;
}

void AudioPlayer::pause() {
	if (!is_playing)
		return;

	start_offset = get_position();
	stop_source();
	is_playing = false;
}

void AudioPlayer::stop() {
	stop_source();
	is_playing = false;
	start_offset = 0;
}

void AudioPlayer::stop_source() {
	if (sound) {
		// ignore errors during cleanup
		ma_sound_stop(sound);
		ma_sound_uninit(sound);
		delete sound;
		sound = nullptr;
	}

	if (decoder) {
		ma_decoder_uninit(decoder);
		delete decoder;
		decoder = nullptr;
	}
}

void AudioPlayer::seek(double position) {
	if (audio_data.empty())
		return;

	const double clamped = std::max(0.0, std::min(position, duration_cache));

	if (is_playing)
		play(clamped);
	else
		start_offset = clamped;
}

double AudioPlayer::get_position() {
	if (audio_data.empty())
		return 0;

	if (is_playing && sound) {
		float cursor = 0.0f;
		ma_sound_get_cursor_in_seconds(sound, &cursor);
		double position = start_offset + static_cast<double>(cursor);

		// Check for natural end of playback.
		if (ma_sound_at_end(sound) && !loop) {
			is_playing = false;
			start_offset = 0;
			stop_source();

			if (on_ended)
				on_ended();

			return 0;
		}

		if (loop && duration_cache > 0.0)
			return std::fmod(position, duration_cache);

		return std::min(position, duration_cache);
	}

	return start_offset;
}

double AudioPlayer::get_duration() {
	return duration_cache;
}

void AudioPlayer::set_volume(float value) {
	volume = value;

	if (sound)
		ma_sound_set_volume(sound, value);
}

void AudioPlayer::set_loop(bool enabled) {
	loop = enabled;

	if (sound)
		ma_sound_set_looping(sound, enabled ? MA_TRUE : MA_FALSE);
}

void AudioPlayer::destroy() {
	unload();

	if (engine) {
		ma_engine_uninit(engine);
		delete engine;
		engine = nullptr;
	}
}
