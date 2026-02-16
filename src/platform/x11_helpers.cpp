#include "x11_helpers.h"

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/shape.h>
#endif

#include <SDL3/SDL_properties.h>

void SetOverrideFullscreen(SDL_Window *window) {
#ifdef __linux__
  SDL_PropertiesID props = SDL_GetWindowProperties(window);
  Display *display = static_cast<Display *>(
      SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                             nullptr));
  Sint64 window_number =
      SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
  if (!display || window_number == 0) {
    return;
  }

  Window x11_window = static_cast<Window>(window_number);

  SDL_Log("Attempting to set X11 window overrides for window %lu", x11_window);

  // Set override_redirect to bypass WM completely
  XSetWindowAttributes attrs;
  attrs.override_redirect = True;
  XChangeWindowAttributes(display, x11_window, CWOverrideRedirect, &attrs);

  // Still set ABOVE state for compositors that might respect it
  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom above_state = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty(display, x11_window, wm_state, XA_ATOM, 32,
                  PropModeReplace, reinterpret_cast<unsigned char *>(&above_state), 1);

  XSync(display, False);
  SDL_Log("X11 window overrides set: override_redirect=True + ABOVE");
#else
  (void)window;
#endif
}

void SetInteractive(SDL_Window *window, bool enabled) {
#ifdef __linux__
  SDL_PropertiesID props = SDL_GetWindowProperties(window);
  Display *display = static_cast<Display *>(
      SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                             nullptr));
  Sint64 window_number =
      SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
  if (!display || window_number == 0) {
    return;
  }

  Window x11_window = static_cast<Window>(window_number);

  if (enabled) {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);
    XRectangle rect = {0, 0, static_cast<unsigned short>(width),
                       static_cast<unsigned short>(height)};
    XShapeCombineRectangles(display, x11_window, ShapeInput, 0, 0, &rect, 1,
                            ShapeSet, YXBanded);
  } else {
    XShapeCombineRectangles(display, x11_window, ShapeInput, 0, 0, nullptr, 0,
                            ShapeSet, YXBanded);
  }

  XSync(display, False);
#else
  (void)window;
  (void)enabled;
#endif
}

#ifdef __linux__
HotkeyState SetupGlobalHotkey() {
  HotkeyState state;
  state.display = XOpenDisplay(nullptr);
  if (!state.display) {
    return state;
  }

  Display *display = static_cast<Display *>(state.display);
  state.root = DefaultRootWindow(display);
  state.keycode = XKeysymToKeycode(display, XK_Insert);
  if (state.keycode == 0) {
    XCloseDisplay(display);
    state.display = nullptr;
    return state;
  }

  XSelectInput(display, state.root, KeyPressMask);
  XGrabKey(display, state.keycode, AnyModifier, state.root, True,
           GrabModeAsync, GrabModeAsync);
  XSync(display, False);
  state.active = true;
  return state;
}

void TeardownGlobalHotkey(HotkeyState &state) {
  if (!state.active || !state.display) {
    return;
  }

  Display *display = static_cast<Display *>(state.display);
  XUngrabKey(display, state.keycode, AnyModifier, state.root);
  XCloseDisplay(display);
  state = HotkeyState{};
}

bool ConsumeGlobalHotkey(HotkeyState &state) {
  if (!state.active || !state.display) {
    return false;
  }

  Display *display = static_cast<Display *>(state.display);
  bool pressed = false;
  while (XPending(display)) {
    XEvent event;
    XNextEvent(display, &event);
    if (event.type == KeyPress && event.xkey.keycode == state.keycode) {
      pressed = true;
    }
  }

  return pressed;
}
#endif
