#pragma once

#include <SDL3/SDL.h>

void SetInteractive(SDL_Window *window, bool enabled);
void SetOverrideFullscreen(SDL_Window *window);

#ifdef __linux__
struct HotkeyState {
	void *display = nullptr;
	unsigned long root = 0;
	int keycode = 0;
	bool active = false;
};

HotkeyState SetupGlobalHotkey();
void TeardownGlobalHotkey(HotkeyState &state);
bool ConsumeGlobalHotkey(HotkeyState &state);
#endif
