# omarchy-hyprland-inertia

Touchpad inertia for [Omarchy](https://omarchy.org). Swipe with two fingers
and lift them: the page keeps scrolling for up to 0.8 s and slows down, the
way it does on macOS.

It is a small Hyprland plugin. It does not replace Hyprland or hook its
functions. It listens to Hyprland's input events and, after the fingers lift,
sends the window under the pointer a few more scroll frames.

- Only touchpad (finger) scrolling. Mouse wheels are untouched.
- Coasting starts only when the touchpad reports that the fingers lifted, and
  stops when you touch the pad again, move the pointer, click, type or switch
  windows.
- Browsers and terminals keep their own inertia (Firefox, Chromium, Brave,
  Vivaldi, LibreWolf, Zen, kitty, Alacritty, foot, Ghostty).
- Scroll speed follows `input:touchpad:scroll_factor`.

## Use in Omarchy

Omarchy installs the package and offers **Setup > Touchpad > Inertia**. The
switch loads the plugin from `/usr/lib/omarchy-hyprland-inertia/inertia.so`
through the Hyprland config, and turning it off unloads it on the next config
reload.

To load it by hand:

```lua
hl.plugin.load("/usr/lib/omarchy-hyprland-inertia/inertia.so")
```

A Hyprland plugin only loads into the exact Hyprland build it was compiled
for. Omarchy's package repository rebuilds it with every Hyprland release.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Needs Hyprland's headers (they ship with the `hyprland` package), CMake and
pkgconf.

## Tests

```bash
c++ -std=c++23 -I src tests/momentum-test.cpp -o build/momentum-test && build/momentum-test
python3 -I tests/nested-test.py
```

`momentum-test` checks the coast curve. `nested-test.py` runs inside a
Hyprland session: it starts a disposable nested Hyprland (a window appears for
about a minute), loads the plugin there only, and simulates touchpad scrolling
with `zwlr_virtual_pointer_v1`. A nested Hyprland receives the parent's real
touchpad scroll as a wheel, so a real touchpad cannot be tested that way.

## License

MIT. `tests/wlr-virtual-pointer-unstable-v1.xml` is the wlroots protocol
definition, MIT, copyright Josef Gajdusek.
