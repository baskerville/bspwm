# bspwm

fork of baskerville/bspwm: edge snapping and magnetic edges, rule matching by property and regex, and fewer X11 round-trips.

## overview

binary tree tiling window manager. socket IPC with bspc client. uses sxhkd for keybindings, with built-in `bspc keybind` also available.

## changes from upstream

the experimental wlroots Wayland backend has been removed; this tree targets X11 only.

### window management

- edge snapping: drag a window to a screen edge or corner, or `bspc node -S|--snap ZONE`
  (`edge_snap_enabled`, `edge_snap_threshold`)
- magnetic edges: dragged floating windows stick to work-area and window edges (`magnet_threshold`)
- per-desktop tile limit (`tile_limit_enabled`, `max_tiles_per_desktop`)
- rules match on `class`, `instance`, `name` and other properties, as regex with `~=`
  and case-insensitively with `/i`: `bspc rule -a 'class~=^Fire/i' state=floating`
- `ignore_monitor_updates` / `-i` freeze the monitor layout
- the control socket and state file live in `$XDG_RUNTIME_DIR`

### fixes to upstream behaviour

- **state restore** (`bspc wm -l`, `bspwm -r`): upstream sized the copy of monitor, desktop and
  node names from the JSON token length, so a long name overflowed `char[256]`. names are now
  clamped and token indices bounds-checked. JSON nesting is capped at 1024. the file must be a
  regular file or FIFO owned by the caller, at most 1 MiB
- **control socket**: a client that connected and sent nothing stalled the WM. requests are now
  read without blocking, so a stalled client is dropped after 2 s and blocks no one. requests
  are no longer cut at 8 KB (up to 1 MiB). the socket is mode 0600 with a `SO_PEERCRED` uid
  check, and bspwm won't take over a live instance's socket
- **argument parsing**: numbers in commands and settings must be whole integers in range
  (`window_gap 10abc`, `border_width -1`, ids above 32 bits are errors instead of being
  truncated or wrapped). `parse_degree` / `parse_rectangle` are range-checked. rules reject
  over-long consequences instead of truncating them
- **allocation**: NULL checks where upstream used `calloc` results directly (node and client
  creation, ewmh, restart argv)
- **geometry**: `rect_cmp` no longer truncates an unsigned area difference to `int`, and
  `adapt_geometry` uses 64-bit intermediates

### build

`-std=c23 -O2 -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fcf-protection`,
full RELRO, warning-free under `-Wall -Wextra -Wformat=2 -Wvla -Wnull-dereference`.

### X11 pipelining

reduced X11 round-trips in rule application (6→1) and client initialization (4→1). benefit depends on X11 latency:

- **localhost**: ~10μs per round-trip, pipelining saves ~50μs (not measurable)
- **remote X11**: ~500μs per round-trip, pipelining saves ~2-4ms per window

### benchmark results (localhost, 100 iterations)

```
                      upstream    current
command latency:         4 μs       4 μs
window create+map:    1634 μs    1637 μs
```

single run against a bspwm 0.9.12 binary; it measures X11/IPC round-trips.

## benchmarking

actual X11 latency comparison (requires Xvfb):

```bash
cd benches
./compare_latency.sh [iterations]
```

measures real IPC round-trip and window creation latency. this is what matters.

CPU cycle microbenchmarks exist in `benches/microbench.c` but are UX-irrelevant. 4000 cycles = 1.3μs. one X11 round-trip = 500μs.

## build

```bash
make                  # build bspwm and bspc
make install          # installs binaries, man pages, docs and the xsessions entry
```

`bspc` links no display library. objects live under `build/x11/`.

dependencies: libxcb, xcb-util, xcb-util-keysyms, xcb-util-wm, libxkbcommon

arch package: `pkg/arch/PKGBUILD` (bspwm1)

binary size: 285KB (+34KB / +13% vs upstream 0.9.12)

## configuration

config: `$XDG_CONFIG_HOME/bspwm/bspwmrc`
keybindings: sxhkd
examples: `examples/`

## license

BSD-2-Clause
