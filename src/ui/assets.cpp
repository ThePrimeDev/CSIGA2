#include "assets.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>

#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Font.h"

namespace {

GLuint CreateTextureFromMemory(const unsigned char *data,
                               int size,
                               int *out_width,
                               int *out_height,
                               bool force_white) {
  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char *decoded =
      stbi_load_from_memory(data, size, &width, &height, &channels, 4);
  if (!decoded) {
    return 0;
  }

  if (force_white) {
    const int pixel_count = width * height;
    for (int i = 0; i < pixel_count; ++i) {
      unsigned char *px = decoded + (i * 4);
      px[0] = 255;
      px[1] = 255;
      px[2] = 255;
    }
  }

  GLuint texture_id = 0;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, decoded);
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(decoded);

  if (out_width) {
    *out_width = width;
  }
  if (out_height) {
    *out_height = height;
  }

  return texture_id;
}

GLuint CreateTextureFromFile(const char *path,
                             int *out_width,
                             int *out_height,
                             bool force_white) {
  size_t size = 0;
  void *buffer = SDL_LoadFile(path, &size);
  if (!buffer || size == 0) {
    return 0;
  }

  GLuint texture_id =
      CreateTextureFromMemory(static_cast<unsigned char *>(buffer),
                              static_cast<int>(size), out_width, out_height,
                              force_white);
  SDL_free(buffer);
  return texture_id;
}

}  // namespace

UiAssets &GetUiAssets() {
  static UiAssets assets;
  return assets;
}

void LoadUiAssets() {
  UiAssets &assets = GetUiAssets();
  ImGuiIO &io = ImGui::GetIO();

  assets.inter_s = io.Fonts->AddFontFromMemoryTTF(
      Inter_SemmiBold, sizeof(Inter_SemmiBold), 17.0f);
  assets.inter_s1 = io.Fonts->AddFontFromMemoryTTF(
      Inter_SemmiBold, sizeof(Inter_SemmiBold), 18.0f);
  assets.inter_s2 = io.Fonts->AddFontFromMemoryTTF(
      Inter_SemmiBold, sizeof(Inter_SemmiBold), 23.0f);
  assets.inter_s3 = io.Fonts->AddFontFromMemoryTTF(
      Inter_SemmiBold, sizeof(Inter_SemmiBold), 15.0f);
  assets.inter_b = io.Fonts->AddFontFromMemoryTTF(
      Inter_Bold, sizeof(Inter_Bold), 34.0f);
  assets.icon = io.Fonts->AddFontFromMemoryTTF(
      Icon_Pack, sizeof(Icon_Pack), 26.0f);
  assets.icon_arrow = io.Fonts->AddFontFromMemoryTTF(
      Arrow, sizeof(Arrow), 6.0f);

  const char *backgrounds[] = {"assets/background1.png",
                               "assets/background2.png"};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 1);
  const char *background_path = backgrounds[dist(gen)];

  GLuint texture_id = CreateTextureFromFile(background_path,
                                            &assets.background_width,
                                            &assets.background_height,
                                            false);
  if (texture_id != 0) {
    assets.background = static_cast<ImTextureID>(texture_id);
  }

  int icon_w = 0;
  int icon_h = 0;
  GLuint user_tex = CreateTextureFromFile("assets/icon_user.png", &icon_w, &icon_h,
                                          true);
  if (user_tex != 0) {
    assets.icon_user = static_cast<ImTextureID>(user_tex);
  }
  GLuint fps_tex = CreateTextureFromFile("assets/icon_fps.png", &icon_w, &icon_h,
                                         true);
  if (fps_tex != 0) {
    assets.icon_fps = static_cast<ImTextureID>(fps_tex);
  }
  GLuint ms_tex = CreateTextureFromFile("assets/icon_ms.png", &icon_w, &icon_h,
                                        true);
  if (ms_tex != 0) {
    assets.icon_ms = static_cast<ImTextureID>(ms_tex);
  }
  GLuint logo_tex = CreateTextureFromFile("assets/icon_logo.png", &icon_w, &icon_h,
                                          false);
  if (logo_tex != 0) {
    assets.icon_logo = static_cast<ImTextureID>(logo_tex);
  }
}

void UnloadUiAssets() {
  UiAssets &assets = GetUiAssets();
  if (assets.background) {
    GLuint texture_id = static_cast<GLuint>(assets.background);
    glDeleteTextures(1, &texture_id);
    assets.background = 0;
  }

  ImTextureID icons[] = {assets.icon_user, assets.icon_fps, assets.icon_ms,
                         assets.icon_logo};
  for (ImTextureID tex : icons) {
    if (!tex) {
      continue;
    }
    GLuint texture_id = static_cast<GLuint>(tex);
    glDeleteTextures(1, &texture_id);
  }
  assets.icon_user = 0;
  assets.icon_fps = 0;
  assets.icon_ms = 0;
  assets.icon_logo = 0;
}
