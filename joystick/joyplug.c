// This is a joystick hotplug monitor which detects when joystick
// devices are added or removed.
// Any applications which use joysticks may want to use some of
// this code if they want to allow joysticks to be attached or
// swapped out while the application is running, otherwise the
// joystick has to be attached already when the application is started.

/*
BSD Zero Clause License

Copyright (c) 2022 Rupert Carmichael

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

// cc sdl2hp.c -std=c99 `sdl2-config --cflags --libs` -o sdl2hp

#include <SDL.h>
#include <stdio.h>

#define MAXPORTS 16

static SDL_Joystick *joystick[MAXPORTS];
static SDL_Haptic *haptic[MAXPORTS];
static int jsports[MAXPORTS];
static int jsiid[MAXPORTS];

int main(int argc, char **argv) {
  SDL_Event event;
  int running = 1;

  SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

  while (running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT: {
          running = 0;
          break;
        }
        case SDL_JOYDEVICEADDED: {
          int port = 0;

          // Choose next unplugged port
          for (int i = 0; i < MAXPORTS; ++i) {
            if (!jsports[i]) {
              joystick[i] = SDL_JoystickOpen(event.jdevice.which);
              SDL_JoystickSetPlayerIndex(joystick[i], i);
              jsports[i] = 1;
              jsiid[i] = SDL_JoystickInstanceID(joystick[i]);
              port = i;
              break;
            }
          }

          fprintf(stderr, 
                  "Joystick Connected: %s\n" "\tInstance ID: %d, Player: %d\n" "\tPort /dev/input/js%0d\n",
                  SDL_JoystickName(joystick[port]),
                  jsiid[port], SDL_JoystickGetPlayerIndex(joystick[port]),
                  port);

          if (SDL_JoystickIsHaptic(joystick[port])) {
            haptic[port] = SDL_HapticOpenFromJoystick(joystick[port]);
            SDL_HapticRumbleInit(haptic[port]) < 0
                ? fprintf(stderr, "Force Feedback Enable Failed: %s\n", SDL_GetError())
                : fprintf(stderr, "Force Feedback Enabled\n");
          } else {
            fprintf(stderr, "Device is not haptic\n");
          }
          fprintf(stderr, "Currently connected joystick devices: %s%s%s%s\n\n",
                  jsports[0]?"/dev/input/js0 ":"",
                  jsports[1]?"/dev/input/js1 ":"",
                  jsports[2]?"/dev/input/js2 ":"",
                  jsports[3]?"/dev/input/js3 ":"");
          break;
        }
        case SDL_JOYDEVICEREMOVED: {
          int id = event.jdevice.which;
          fprintf(stderr, "Instance ID: %d\n", id);
          for (int i = 0; i < MAXPORTS; ++i) {
            // If it's the one that got disconnected...
            if (jsiid[i] == id) {
              if (SDL_JoystickIsHaptic(joystick[i])) SDL_HapticClose(haptic[i]);
              jsports[i] = 0;  // Notify that this is unplugged
              fprintf(stderr, "Joystick /dev/input/js%0d Disconnected\n", i);
              SDL_JoystickClose(joystick[i]);
              break;
            }
          }
          fprintf(stderr, "Currently connected joystick devices: %s%s%s%s\n\n",
                  jsports[0]?"/dev/input/js0 ":"",
                  jsports[1]?"/dev/input/js1 ":"",
                  jsports[2]?"/dev/input/js2 ":"",
                  jsports[3]?"/dev/input/js3 ":"");
          break;
        }
      }
    }
  }

  return 0;
}
