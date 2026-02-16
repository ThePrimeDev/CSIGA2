#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace audio {

struct MicDevice {
  std::string name;
};

std::vector<MicDevice> ListPhysicalMics();

bool InitVirtualMic(const std::string &preferred_source);
bool SetPhysicalMic(const std::string &source_name);
void ShutdownVirtualMic();

bool IsVirtualMicActive();
std::string GetVirtualMicSourceName();
SDL_AudioStream *OpenVirtualMicStream(const SDL_AudioSpec &spec);

}  // namespace audio
