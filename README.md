# omarchy-hyprland-inertia

Scroll inertia for [Omarchy](https://omarchy.org). Swipe with two fingers
and lift them: the page keeps scrolling for up to 0.8 s and slows down, the
way it does on macOS. Optionally, a fast mouse-wheel spin coasts the same way.

It is a small Hyprland plugin. It does not replace Hyprland or hook its
functions. It listens to Hyprland's input events and, after the fingers lift,
sends the window under the pointer a few more scroll frames.

- Touchpad: coasting starts only when the touchpad reports that the fingers
  lifted, and stops when you touch the pad again, move the pointer, click,
  type or switch windows.
- Mouse wheel (off by default): a fast spin coasts once the wheel stops for
  60 ms. A single notch or a slow, careful scroll stops exactly where it is.
  Turning the wheel back, clicking, typing or switching windows stops it;
  moving the mouse does not.
- Browsers and terminals keep their own inertia (Firefox, Chromium, Brave,
  Vivaldi, LibreWolf, Zen, kitty, Alacritty, foot, Ghostty).
- Scroll speed follows `input:touchpad:scroll_factor` and `input:scroll_factor`.

## Options

| Option | Default | |
|---|---|---|
| `plugin:inertia:touchpad` | `true` | coast after a two-finger touchpad scroll |
| `plugin:inertia:mouse` | `false` | coast after a fast mouse-wheel spin |

```lua
hl.config({ plugin = { inertia = { mouse = true } } })
```

Like any plugin option, they return to these defaults on every config reload,
so set them in the config, after the plugin is loaded.

## Use in Omarchy

Omarchy installs the package and offers **Setup > Scrolling**, with
**Touchpad Inertia** and **Mouse Wheel Inertia**. They load the plugin from
`/usr/lib/omarchy-hyprland-inertia/inertia.so` through the Hyprland config and
set its options; with both off, the plugin is unloaded on the next config
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
about a minute), loads the plugin there only, and simulates touchpad and
mouse-wheel scrolling with `zwlr_virtual_pointer_v1`. A nested Hyprland receives the parent's real
touchpad scroll as a wheel, so a real touchpad cannot be tested that way.

## License

MIT. `tests/wlr-virtual-pointer-unstable-v1.xml` is the wlroots protocol
definition, MIT, copyright Josef Gajdusek.
