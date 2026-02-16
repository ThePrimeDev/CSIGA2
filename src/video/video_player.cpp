#include "video_player.h"
#include "audio/virtual_mic.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <random>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace video {

VideoPlayer::VideoPlayer() : rng_(std::random_device{}()) {}

VideoPlayer::~VideoPlayer() {
  Stop();
  ClearTexture();
}

bool VideoPlayer::PlayRandomFromDir(const std::string &dir) {
  std::vector<std::string> files;
  if (std::filesystem::exists(dir)) {
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      auto ext = entry.path().extension().string();
      for (auto &c : ext) {
        if (c >= 'A' && c <= 'Z') {
          c = static_cast<char>(c - 'A' + 'a');
        }
      }
      if (ext == ".mp4") {
        files.push_back(entry.path().string());
      }
    }
  }

  if (files.empty()) {
    SDL_Log("No MP4 files found in %s", dir.c_str());
    return false;
  }

  std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
  return PlayFile(files[dist(rng_)]);
}

bool VideoPlayer::PlayFile(const std::string &path) {
  Stop();

  current_path_ = path;
  stop_ = false;
  playing_ = false;

  if (avformat_open_input(&fmt_ctx_, path.c_str(), nullptr, nullptr) < 0) {
    SDL_Log("Failed to open video: %s", path.c_str());
    Cleanup();
    return false;
  }

  if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
    SDL_Log("Failed to read stream info: %s", path.c_str());
    Cleanup();
    return false;
  }

  video_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  audio_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

  if (video_stream_index_ < 0) {
    SDL_Log("No video stream found: %s", path.c_str());
    Cleanup();
    return false;
  }

  const AVCodecParameters *video_params = fmt_ctx_->streams[video_stream_index_]->codecpar;
  const AVCodec *video_codec = avcodec_find_decoder(video_params->codec_id);
  if (!video_codec) {
    SDL_Log("Video decoder not found: %s", path.c_str());
    Cleanup();
    return false;
  }

  video_ctx_ = avcodec_alloc_context3(video_codec);
  if (!video_ctx_) {
    SDL_Log("Failed to allocate video context");
    Cleanup();
    return false;
  }
  avcodec_parameters_to_context(video_ctx_, video_params);
  if (avcodec_open2(video_ctx_, video_codec, nullptr) < 0) {
    SDL_Log("Failed to open video decoder");
    Cleanup();
    return false;
  }

  if (audio_stream_index_ >= 0) {
    const AVCodecParameters *audio_params = fmt_ctx_->streams[audio_stream_index_]->codecpar;
    const AVCodec *audio_codec = avcodec_find_decoder(audio_params->codec_id);
    if (audio_codec) {
      audio_ctx_ = avcodec_alloc_context3(audio_codec);
      if (audio_ctx_) {
        avcodec_parameters_to_context(audio_ctx_, audio_params);
        if (avcodec_open2(audio_ctx_, audio_codec, nullptr) < 0) {
          SDL_Log("Failed to open audio decoder");
          avcodec_free_context(&audio_ctx_);
        }
      }
    }
  }

  sws_ctx_ = sws_getContext(video_ctx_->width, video_ctx_->height, video_ctx_->pix_fmt,
                            video_ctx_->width, video_ctx_->height, AV_PIX_FMT_RGB24,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!sws_ctx_) {
    SDL_Log("Failed to create sws context");
    Cleanup();
    return false;
  }

  if (audio_ctx_) {
    AVChannelLayout out_layout;
    av_channel_layout_copy(&out_layout, &audio_ctx_->ch_layout);
    if (swr_alloc_set_opts2(&swr_ctx_, &out_layout, AV_SAMPLE_FMT_FLT,
                            audio_ctx_->sample_rate, &audio_ctx_->ch_layout,
                            audio_ctx_->sample_fmt, audio_ctx_->sample_rate,
                            0, nullptr) < 0) {
      SDL_Log("Failed to allocate resampler");
      Cleanup();
      return false;
    }

    if (swr_init(swr_ctx_) < 0) {
      SDL_Log("Failed to init resampler");
      Cleanup();
      return false;
    }

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
      SDL_Log("SDL audio init failed: %s", SDL_GetError());
    } else {
      SDL_AudioSpec spec{};
      spec.format = SDL_AUDIO_F32;
      spec.channels = static_cast<int>(out_layout.nb_channels);
      spec.freq = audio_ctx_->sample_rate;

      audio_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                &spec, nullptr, nullptr);
      if (!audio_stream_) {
        SDL_Log("SDL audio stream failed: %s", SDL_GetError());
      } else {
        SDL_ClearAudioStream(audio_stream_);
        SDL_ResumeAudioStreamDevice(audio_stream_);
      }

      virtual_audio_stream_ = audio::OpenVirtualMicStream(spec);
    }

    av_channel_layout_uninit(&out_layout);
  }

  frame_ = av_frame_alloc();
  frame_rgb_ = av_frame_alloc();
  packet_ = av_packet_alloc();
  if (!frame_ || !frame_rgb_ || !packet_) {
    SDL_Log("Failed to allocate FFmpeg frame/packet");
    Cleanup();
    return false;
  }

  playing_ = true;
  decode_thread_ = std::thread(&VideoPlayer::DecodeLoop, this);
  return true;
}

void VideoPlayer::Stop() {
  stop_ = true;
  if (decode_thread_.joinable()) {
    decode_thread_.join();
  }
  playing_ = false;
  Cleanup();
}

bool VideoPlayer::IsPlaying() const {
  return playing_.load();
}

void VideoPlayer::DecodeLoop() {
  std::vector<unsigned char> rgb_buffer;
  std::vector<unsigned char> rgba_buffer;
  std::vector<uint8_t> audio_buffer;

  int rgb_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
                                          video_ctx_->width,
                                          video_ctx_->height, 1);
  rgb_buffer.resize(static_cast<size_t>(rgb_size));
  rgba_buffer.resize(static_cast<size_t>(video_ctx_->width * video_ctx_->height * 4));
  av_image_fill_arrays(frame_rgb_->data, frame_rgb_->linesize, rgb_buffer.data(),
                       AV_PIX_FMT_RGB24, video_ctx_->width,
                       video_ctx_->height, 1);

  const AVRational video_time_base = fmt_ctx_->streams[video_stream_index_]->time_base;
  auto start_time = std::chrono::steady_clock::now();

  while (!stop_) {
    int ret = av_read_frame(fmt_ctx_, packet_);
    if (ret < 0) {
      break;
    }

    if (packet_->stream_index == video_stream_index_) {
      ret = avcodec_send_packet(video_ctx_, packet_);
      while (ret >= 0) {
        ret = avcodec_receive_frame(video_ctx_, frame_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        }
        if (ret < 0) {
          break;
        }

        sws_scale(sws_ctx_, frame_->data, frame_->linesize, 0, video_ctx_->height,
                  frame_rgb_->data, frame_rgb_->linesize);

        int64_t ts = frame_->best_effort_timestamp;
        double pts = 0.0;
        if (ts != AV_NOPTS_VALUE) {
          pts = static_cast<double>(ts) * av_q2d(video_time_base);
        }

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        if (pts > elapsed) {
          std::this_thread::sleep_for(std::chrono::duration<double>(pts - elapsed));
        }

        const int width = video_ctx_->width;
        const int height = video_ctx_->height;
        const size_t pixel_count = static_cast<size_t>(width * height);
        for (size_t i = 0; i < pixel_count; ++i) {
          const unsigned char r = rgb_buffer[i * 3 + 0];
          const unsigned char g = rgb_buffer[i * 3 + 1];
          const unsigned char b = rgb_buffer[i * 3 + 2];

          const float dr = static_cast<float>(r) / 255.0f;
          const float dg = static_cast<float>(g) / 255.0f - 1.0f;
          const float db = static_cast<float>(b) / 255.0f;
          const float dist = std::sqrt(dr * dr + dg * dg + db * db);

          float alpha = (dist - 0.35f) / 0.08f;
          if (alpha < 0.0f) alpha = 0.0f;
          if (alpha > 1.0f) alpha = 1.0f;

          rgba_buffer[i * 4 + 0] = r;
          rgba_buffer[i * 4 + 1] = g;
          rgba_buffer[i * 4 + 2] = b;
          rgba_buffer[i * 4 + 3] = static_cast<unsigned char>(alpha * 255.0f);
        }

        {
          std::lock_guard<std::mutex> lock(frame_mutex_);
          if (frame_data_.size() != rgba_buffer.size()) {
            frame_data_.resize(rgba_buffer.size());
          }
          std::memcpy(frame_data_.data(), rgba_buffer.data(), rgba_buffer.size());
          frame_width_ = width;
          frame_height_ = height;
          frame_pts_ = pts;
          frame_ready_ = true;
        }
      }
    } else if (audio_ctx_ && packet_->stream_index == audio_stream_index_) {
      ret = avcodec_send_packet(audio_ctx_, packet_);
      while (ret >= 0) {
        ret = avcodec_receive_frame(audio_ctx_, frame_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        }
        if (ret < 0) {
          break;
        }

        int out_samples = swr_get_out_samples(swr_ctx_, frame_->nb_samples);
        int out_channels = static_cast<int>(audio_ctx_->ch_layout.nb_channels);
        int out_bytes = av_samples_get_buffer_size(nullptr, out_channels, out_samples,
                                                   AV_SAMPLE_FMT_FLT, 0);
        if (out_bytes <= 0) {
          continue;
        }

        audio_buffer.resize(static_cast<size_t>(out_bytes));
        uint8_t *out_data[1] = {audio_buffer.data()};
        int converted = swr_convert(swr_ctx_, out_data, out_samples,
                                    const_cast<const uint8_t **>(frame_->data),
                                    frame_->nb_samples);
        if (converted > 0) {
          int byte_len = converted * out_channels * static_cast<int>(sizeof(float));
          if (audio_stream_) {
            SDL_PutAudioStreamData(audio_stream_, audio_buffer.data(), byte_len);
          }
          if (virtual_audio_stream_) {
            SDL_PutAudioStreamData(virtual_audio_stream_, audio_buffer.data(), byte_len);
          }
        }
      }
    }

    av_packet_unref(packet_);
  }

  playing_ = false;
  Cleanup();
}

void VideoPlayer::Render(const ImVec2 &pos, const ImVec2 &size, const ImVec2 &display_size) {
  if (display_size.x <= 0.0f || display_size.y <= 0.0f) {
    return;
  }

  std::vector<unsigned char> frame_copy;
  int width = 0;
  int height = 0;
  {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (!frame_ready_ || frame_data_.empty()) {
      return;
    }
    frame_copy = frame_data_;
    width = frame_width_;
    height = frame_height_;
  }

  if (!EnsureTexture(width, height)) {
    return;
  }

  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                  GL_RGBA, GL_UNSIGNED_BYTE, frame_copy.data());

  glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, display_size.x, display_size.y, 0.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(pos.x, pos.y);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f(pos.x + size.x, pos.y);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f(pos.x + size.x, pos.y + size.y);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(pos.x, pos.y + size.y);
  glEnd();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopAttrib();
}

bool VideoPlayer::EnsureTexture(int width, int height) {
  if (width <= 0 || height <= 0) {
    return false;
  }

  if (texture_id_ == 0) {
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    return true;
  }

  glBindTexture(GL_TEXTURE_2D, texture_id_);
  GLint tex_width = 0;
  GLint tex_height = 0;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_height);
  if (tex_width != width || tex_height != height) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

  return true;
}

void VideoPlayer::ClearTexture() {
  if (texture_id_ != 0) {
    glDeleteTextures(1, &texture_id_);
    texture_id_ = 0;
  }
}

void VideoPlayer::Cleanup() {
  if (audio_stream_) {
    SDL_DestroyAudioStream(audio_stream_);
    audio_stream_ = nullptr;
  }
  if (virtual_audio_stream_) {
    SDL_DestroyAudioStream(virtual_audio_stream_);
    virtual_audio_stream_ = nullptr;
  }

  if (packet_) {
    av_packet_free(&packet_);
  }
  if (frame_) {
    av_frame_free(&frame_);
  }
  if (frame_rgb_) {
    av_frame_free(&frame_rgb_);
  }
  if (sws_ctx_) {
    sws_freeContext(sws_ctx_);
    sws_ctx_ = nullptr;
  }
  if (swr_ctx_) {
    swr_free(&swr_ctx_);
  }
  if (video_ctx_) {
    avcodec_free_context(&video_ctx_);
  }
  if (audio_ctx_) {
    avcodec_free_context(&audio_ctx_);
  }
  if (fmt_ctx_) {
    avformat_close_input(&fmt_ctx_);
  }

  frame_ready_ = false;
  frame_data_.clear();
  frame_width_ = 0;
  frame_height_ = 0;
  frame_pts_ = 0.0;
  video_stream_index_ = -1;
  audio_stream_index_ = -1;
}

}  // namespace video
