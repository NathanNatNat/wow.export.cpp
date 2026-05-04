/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#ifdef _WIN32
#define NOMINMAX
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#undef MINIAUDIO_IMPLEMENTATION

#include "audio-helper.h"
#include "../core.h"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

AudioType detectFileType(const BufferWrapper& data) {
	const auto& raw = data.raw();

	if (raw.size() >= 4 && std::memcmp(raw.data(), "OggS", 4) == 0)
		return AudioType::OGG;

	if (raw.size() >= 3 && std::memcmp(raw.data(), "ID3", 3) == 0)
		return AudioType::MP3;

	if (raw.size() >= 5 && raw[3] == 0xFF && raw[4] == 0xFB)
		return AudioType::MP3;

	if (raw.size() >= 7 && raw[5] == 0xFF && raw[6] == 0xF3)
		return AudioType::MP3;

	if (raw.size() >= 9 && raw[7] == 0xFF && raw[8] == 0xF2)
		return AudioType::MP3;

	return AudioType::Unknown;
}

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

std::span<const uint8_t> AudioPlayer::load(std::span<const uint8_t> data) {
	stop();

	audio_data.assign(data.begin(), data.end());
	duration_cache = 0.0;

	ma_decoder_config decoderConfig = ma_decoder_config_init_default();
	ma_decoder tempDecoder;
	if (ma_decoder_init_memory(audio_data.data(), audio_data.size(), &decoderConfig, &tempDecoder) != MA_SUCCESS)
		throw std::runtime_error("Failed to decode audio data");

	ma_uint64 frameCount = 0;
	ma_decoder_get_length_in_pcm_frames(&tempDecoder, &frameCount);
	if (frameCount > 0 && tempDecoder.outputSampleRate > 0)
		duration_cache = static_cast<double>(frameCount) / static_cast<double>(tempDecoder.outputSampleRate);
	ma_decoder_uninit(&tempDecoder);

	return std::span<const uint8_t>(audio_data);
}

void AudioPlayer::unload() {
	stop();
	audio_data.clear();
	start_offset = 0;
	duration_cache = 0.0;
}

void AudioPlayer::play(double from_offset) {
	if (audio_data.empty())
		return;

	if (!engine)
		throw std::runtime_error("AudioPlayer not initialized");

	stop_source();

	if (from_offset >= 0.0)
		start_offset = std::max(0.0, std::min(from_offset, duration_cache));

	decoder = new ma_decoder();
	ma_decoder_config decoderConfig = ma_decoder_config_init_default();
	if (ma_decoder_init_memory(audio_data.data(), audio_data.size(), &decoderConfig, decoder) != MA_SUCCESS) {
		delete decoder;
		decoder = nullptr;
		return;
	}

	if (start_offset > 0.0 && decoder->outputSampleRate > 0) {
		ma_uint64 offsetFrames = static_cast<ma_uint64>(start_offset * decoder->outputSampleRate);
		ma_decoder_seek_to_pcm_frame(decoder, offsetFrames);
	}

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

	ma_sound_set_end_callback(sound, [](void* user_data, ma_sound* /*pSound*/) {
		auto* self = static_cast<AudioPlayer*>(user_data);
		if (!self)
			return;

		// only handle natural completion (not stopped programmatically)
		if (self->is_playing && !self->loop) {
			self->is_playing = false;
			self->start_offset = 0.0;

			core::postToMainThread([self]() {
				self->stop_source();

				if (self->on_ended)
					self->on_ended();
			});
		}
	}, this);

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
		ma_sound_set_end_callback(sound, nullptr, nullptr);
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
		double position = static_cast<double>(cursor);

		if (loop && duration_cache > 0.0)
			return std::fmod(position, duration_cache);

		return std::min(position, duration_cache);
	}

	return start_offset;
}

double AudioPlayer::get_duration() {
	return duration_cache;
}

bool AudioPlayer::is_loaded() const {
	return !audio_data.empty();
}

void AudioPlayer::set_volume(float value) {
	if (!engine)
		return;

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
