#include "virtual_mic.h"

#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace audio {

namespace {

struct VirtualMicState {
  int module_null = -1;
  int module_remap = -1;
  int module_loopback = -1;
  std::string sink_name = "csiga2_sink";
  std::string source_name = "csiga2_mic";
  std::string physical_source;
  bool active = false;
};

VirtualMicState g_state;

std::string ExecCommand(const std::string &cmd) {
  std::string output;
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    return output;
  }

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    output += buffer;
  }

  pclose(pipe);
  return output;
}

int LoadModule(const std::string &args) {
  std::string cmd = "pactl load-module " + args;
  std::string out = ExecCommand(cmd);
  if (out.empty()) {
    return -1;
  }
  while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) {
    out.pop_back();
  }
  if (out.empty()) {
    return -1;
  }
  for (char c : out) {
    if (c < '0' || c > '9') {
      return -1;
    }
  }
  int id = std::atoi(out.c_str());
  return id > 0 ? id : -1;
}

void UnloadModule(int module_id) {
  if (module_id <= 0) {
    return;
  }
  std::string cmd = "pactl unload-module " + std::to_string(module_id);
  ExecCommand(cmd);
}

bool IsMonitorSource(const std::string &name) {
  const std::string suffix = ".monitor";
  if (name.size() < suffix.size()) {
    return false;
  }
  return name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool IsPactlAvailable() {
  std::string out = ExecCommand("pactl info");
  if (out.empty()) {
    return false;
  }
  if (out.find("Server Name") == std::string::npos &&
      out.find("Server String") == std::string::npos) {
    return false;
  }
  return true;
}

}  // namespace

std::vector<MicDevice> ListPhysicalMics() {
  std::vector<MicDevice> devices;
  std::string out = ExecCommand("pactl list short sources");
  std::istringstream lines(out);
  std::string line;

  while (std::getline(lines, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream cols(line);
    std::string index;
    std::string name;
    if (!(cols >> index >> name)) {
      continue;
    }
    if (IsMonitorSource(name)) {
      continue;
    }
    if (name == g_state.source_name) {
      continue;
    }
    devices.push_back(MicDevice{name});
  }

  return devices;
}

bool InitVirtualMic(const std::string &preferred_source) {
  if (g_state.active) {
    return true;
  }

  if (!IsPactlAvailable()) {
    SDL_Log("[CSIGA2] pactl/pipewire-pulse not available; virtual mic disabled");
    return false;
  }

  static bool atexit_registered = false;
  if (!atexit_registered) {
    std::atexit(ShutdownVirtualMic);
    atexit_registered = true;
  }

  g_state.module_null = LoadModule("module-null-sink sink_name=" + g_state.sink_name +
                                   " sink_properties=device.description=CSIGA2");
  if (g_state.module_null < 0) {
    SDL_Log("[CSIGA2] Failed to create null sink");
    ShutdownVirtualMic();
    return false;
  }

  g_state.module_remap = LoadModule("module-remap-source master=" + g_state.sink_name +
                                    ".monitor source_name=" + g_state.source_name +
                                    " source_properties=device.description=CSIGA2");
  if (g_state.module_remap < 0) {
    SDL_Log("[CSIGA2] Failed to create remap source");
    ShutdownVirtualMic();
    return false;
  }

  g_state.physical_source = preferred_source;
  if (!g_state.physical_source.empty()) {
    g_state.module_loopback = LoadModule("module-loopback source=" + g_state.physical_source +
                                         " sink=" + g_state.sink_name +
                                         " latency_msec=1");
    if (g_state.module_loopback < 0) {
      SDL_Log("[CSIGA2] Failed to create mic loopback");
    }
  }

  g_state.active = true;
  return true;
}

bool SetPhysicalMic(const std::string &source_name) {
  if (!g_state.active) {
    return InitVirtualMic(source_name);
  }

  if (!IsPactlAvailable()) {
    SDL_Log("[CSIGA2] pactl/pipewire-pulse not available; cannot switch mic");
    return false;
  }

  if (source_name.empty() || source_name == g_state.physical_source) {
    return true;
  }

  UnloadModule(g_state.module_loopback);
  g_state.module_loopback = LoadModule("module-loopback source=" + source_name +
                                       " sink=" + g_state.sink_name +
                                       " latency_msec=1");
  if (g_state.module_loopback < 0) {
    SDL_Log("[CSIGA2] Failed to update mic loopback");
    return false;
  }

  g_state.physical_source = source_name;
  return true;
}

void ShutdownVirtualMic() {
  if (!IsPactlAvailable()) {
    g_state.module_loopback = -1;
    g_state.module_remap = -1;
    g_state.module_null = -1;
    g_state.active = false;
    g_state.physical_source.clear();
    return;
  }

  UnloadModule(g_state.module_loopback);
  UnloadModule(g_state.module_remap);
  UnloadModule(g_state.module_null);

  g_state.module_loopback = -1;
  g_state.module_remap = -1;
  g_state.module_null = -1;
  g_state.active = false;
  g_state.physical_source.clear();
}

bool IsVirtualMicActive() {
  return g_state.active;
}

std::string GetVirtualMicSourceName() {
  return g_state.source_name;
}

SDL_AudioStream *OpenVirtualMicStream(const SDL_AudioSpec &spec) {
  if (!g_state.active) {
    return nullptr;
  }

  int count = 0;
  SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&count);
  if (!devices || count <= 0) {
    return nullptr;
  }

  SDL_AudioDeviceID match = 0;
  for (int i = 0; i < count; ++i) {
    const char *name = SDL_GetAudioDeviceName(devices[i]);
    if (!name) {
      continue;
    }
    if (g_state.sink_name == name || std::string(name) == "CSIGA2") {
      match = devices[i];
      break;
    }
  }

  SDL_free(devices);

  if (match == 0) {
    SDL_Log("[CSIGA2] Virtual sink not found for audio mirroring");
    return nullptr;
  }

  SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(match, &spec, nullptr, nullptr);
  if (!stream) {
    SDL_Log("[CSIGA2] Virtual sink stream failed: %s", SDL_GetError());
    return nullptr;
  }

  SDL_ClearAudioStream(stream);
  SDL_ResumeAudioStreamDevice(stream);
  return stream;
}

}  // namespace audio
