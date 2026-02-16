#include "mvp_music.h"
#ifdef CSIGA2_FUN
#include "virtual_mic.h"
#endif

#include <SDL3/SDL.h>

#include <vector>

#define DR_MP3_IMPLEMENTATION
#include "third_party/dr_mp3.h"

namespace audio {

namespace {

struct AudioState {
  SDL_AudioStream *stream = nullptr;
#ifdef CSIGA2_FUN
  SDL_AudioStream *virtual_stream = nullptr;
#endif
  int freq = 0;
  int channels = 0;
  Uint64 mvp_end_ms = 0;
};

AudioState g_audio_state;

const char *MusicPathForSelection(int selection) {
  switch (selection) {
    case 1:
      return "assets/ez4ence.mp3";
    case 0:
    default:
      return "assets/latinas.mp3";
  }
}

bool LoadMp3(const char *path, std::vector<float> &pcm, int &channels, int &freq) {
  drmp3 mp3;
  if (!drmp3_init_file(&mp3, path, nullptr)) {
    return false;
  }

  channels = static_cast<int>(mp3.channels);
  freq = static_cast<int>(mp3.sampleRate);
  drmp3_uint64 frame_count = drmp3_get_pcm_frame_count(&mp3);

  pcm.resize(static_cast<size_t>(frame_count) * mp3.channels);
  drmp3_uint64 frames_read = drmp3_read_pcm_frames_f32(&mp3, frame_count, pcm.data());
  drmp3_uninit(&mp3);

  if (frames_read == 0) {
    pcm.clear();
    return false;
  }

  pcm.resize(static_cast<size_t>(frames_read) * channels);
  return true;
}

bool EnsureStream(int freq, int channels) {
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    SDL_Log("SDL audio init failed: %s", SDL_GetError());
    return false;
  }

  if (g_audio_state.stream && (g_audio_state.freq != freq || g_audio_state.channels != channels)) {
    SDL_DestroyAudioStream(g_audio_state.stream);
    g_audio_state.stream = nullptr;
#ifdef CSIGA2_FUN
    if (g_audio_state.virtual_stream) {
      SDL_DestroyAudioStream(g_audio_state.virtual_stream);
      g_audio_state.virtual_stream = nullptr;
    }
#endif
  }

  if (!g_audio_state.stream) {
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = channels;
    spec.freq = freq;

    g_audio_state.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!g_audio_state.stream) {
      SDL_Log("SDL audio stream failed: %s", SDL_GetError());
      return false;
    }

#ifdef CSIGA2_FUN
    g_audio_state.virtual_stream = audio::OpenVirtualMicStream(spec);
#endif

    g_audio_state.freq = freq;
    g_audio_state.channels = channels;
  }

  SDL_ClearAudioStream(g_audio_state.stream);
  SDL_ResumeAudioStreamDevice(g_audio_state.stream);
  return true;
}

}  // namespace

bool PlayMvpMusic(int selection) {
  const char *path = MusicPathForSelection(selection);
  int channels = 0;
  int freq = 0;
  std::vector<float> pcm;
  g_audio_state.mvp_end_ms = 0;

  if (!LoadMp3(path, pcm, channels, freq)) {
    SDL_Log("Failed to load MP3: %s", path);
    return false;
  }

  if (!EnsureStream(freq, channels)) {
    return false;
  }

  int byte_len = static_cast<int>(pcm.size() * sizeof(float));
  if (!SDL_PutAudioStreamData(g_audio_state.stream, pcm.data(), byte_len)) {
    return false;
  }
#ifdef CSIGA2_FUN
  if (g_audio_state.virtual_stream) {
    SDL_PutAudioStreamData(g_audio_state.virtual_stream, pcm.data(), byte_len);
  }
#endif
  if (freq > 0 && channels > 0) {
    double frames = static_cast<double>(pcm.size()) /
                    static_cast<double>(channels);
    double duration_ms = (frames * 1000.0) /
                         static_cast<double>(freq);
    g_audio_state.mvp_end_ms = SDL_GetTicks() +
                               static_cast<Uint64>(duration_ms + 0.5);
  } else {
    g_audio_state.mvp_end_ms = 0;
  }
  return true;
}

bool IsMvpMusicPlaying() {
  if (g_audio_state.mvp_end_ms == 0) {
    return false;
  }
  return SDL_GetTicks() < g_audio_state.mvp_end_ms;
}

void ShutdownMvpMusic() {
  if (g_audio_state.stream) {
    SDL_DestroyAudioStream(g_audio_state.stream);
    g_audio_state.stream = nullptr;
  }
#ifdef CSIGA2_FUN
  if (g_audio_state.virtual_stream) {
    SDL_DestroyAudioStream(g_audio_state.virtual_stream);
    g_audio_state.virtual_stream = nullptr;
  }
#endif
  g_audio_state.freq = 0;
  g_audio_state.channels = 0;
  g_audio_state.mvp_end_ms = 0;
}

}  // namespace audio
