#pragma once

#include <imgui.h>

#include <atomic>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;
struct SwrContext;
struct AVFrame;
struct AVPacket;
struct SDL_AudioStream;

namespace video {

class VideoPlayer {
 public:
  VideoPlayer();
  ~VideoPlayer();

  bool PlayRandomFromDir(const std::string &dir);
  bool PlayFile(const std::string &path);
  void Stop();
  bool IsPlaying() const;

  void Render(const ImVec2 &pos, const ImVec2 &size, const ImVec2 &display_size);

 private:
  void DecodeLoop();
  void Cleanup();
  bool EnsureTexture(int width, int height);
  void ClearTexture();

  std::string current_path_;
  std::atomic<bool> playing_{false};
  std::atomic<bool> stop_{false};
  std::thread decode_thread_;

  std::mutex frame_mutex_;
  std::vector<unsigned char> frame_data_;
  int frame_width_ = 0;
  int frame_height_ = 0;
  double frame_pts_ = 0.0;
  bool frame_ready_ = false;

  ::AVFormatContext *fmt_ctx_ = nullptr;
  ::AVCodecContext *video_ctx_ = nullptr;
  ::AVCodecContext *audio_ctx_ = nullptr;
  ::SwsContext *sws_ctx_ = nullptr;
  ::SwrContext *swr_ctx_ = nullptr;
  ::AVFrame *frame_ = nullptr;
  ::AVFrame *frame_rgb_ = nullptr;
  ::AVPacket *packet_ = nullptr;
  int video_stream_index_ = -1;
  int audio_stream_index_ = -1;
  ::SDL_AudioStream *audio_stream_ = nullptr;
  ::SDL_AudioStream *virtual_audio_stream_ = nullptr;

  unsigned int texture_id_ = 0;
  std::mt19937 rng_;
};

}  // namespace video
