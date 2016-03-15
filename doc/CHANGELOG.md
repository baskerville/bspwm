# From 0.9 to 0.9.1

## Overview

All the commands that acted on leaves can now be applied on internal nodes (including focusing and preselection). Consequently, the *window* domain is now a *node* domain. Please note that some commands are applied to the leaves of the tree rooted at the selected node and not to the node itself.

## Changes

- All the commands that started with `window` now start with `node`.
- `-W|--windows`, `-w|--window`, `-w|--to-window` are now `-N|--nodes`, `-n|--node`, `-n|--to-node`.
- We now use cardinal directions: `west,south,north,east` instead of `left,down,up,right` (in fact the latter is just plain wrong: the `up,down` axis is perpendicular to the screen).
- The `WINDOW_SEL` becomes `NODE_SEL` and now contains a `PATH` specifier to select internal nodes.
- The `control` domain is renamed to `wm`.
- `restore -{T,H,S}` was unified into `wm -l|--load-state` and `query -{T,H,S}` into `wm -d|--dump-state`.
- `control --subscribe` becomes `subscribe`.
- `node --toggle` (previously `window --toggle`) is split into `node --state` and `node --flag`.
- The preselection direction (resp. ratio) is now set with `node --presel-dir|-p` (resp. `node --presel-ratio|-o`).
- The following desktop commands: `--rotate`, `--flip`, `--balance`, `--equalize`, `--circulate` are now node commands.
- `query -T ...` outputs JSON.
- `query -{M,D,N}`: the descriptor part of the selector is now optional (e.g.: `query -D -d .urgent`).
- Many new modifiers were added, some were renamed. The opposite of a modifier is now expressed with the `!` prefix (e.g.: `like` becomes `same_class`, `unlike` becomes `!same_class`, etc.).
- Modifiers can now be applied to any descriptor (e.g.: `query -N -n 0x80000d.floating`).
- `wm -l` (previously `restore -T`) will now destroy the existing tree and restore from scratch instead of relying on existing monitors and desktops.
- `subscribe` (previously `control --subscribe`) now accepts arguments and can receive numerous events from different domains (see the *EVENTS* section of the manual).
- `rule -a`: it is now possible to specify the class name *and* instance name (e.g.: `rule -a Foo:bar`).
- `presel_border_color` is now `presel_feedback_color`.
- `bspwm -v` yields an accurate version.
- The monitors are sorted, by default, according to the natural visual hierarchy.

## Additions

### Settings

- `single_monocle`.
- `paddingless_monocle`.

### Commands

- `{node,desktop} --activate`.
- `node --layer`.
- `desktop --bubble`.
- `wm {--add-monitor,--remove-monitor}`.
- `monitor --rectangle`.

## Removals

### Commands

- `desktop --toggle`
- `desktop --cancel-presel`
- `control --toggle-visibility`.

### Settings

- `apply_floating_atom`.
- `auto_alternate`.
- `auto_cancel`.
- `focused_locked_border_color`
- `active_locked_border_color`
- `normal_locked_border_color`
- `focused_sticky_border_color`
- `active_sticky_border_color`
- `normal_sticky_border_color`
- `focused_private_border_color`
- `active_private_border_color`
- `normal_private_border_color`
- `urgent_border_color`

## Message Translation Guide

0.9                                      | 0.9.1
-----------------------------------------|----------------------------------
`{left,down,up,right}`                   | `{west,south,north,east}`
`window -r`                              | `node -o` (`node -r` also exists)
`window -e DIR RATIO`                    | `node @DIR -r RATIO`
`window -R DIR DEG`                      | `node @DIR -R DEG`
`window -w`                              | `node -n`
`desktop DESKTOP_SEL -R DEG`             | `node @DESKTOP_SEL:/ -R DEG`
`desktop DESKTOP_SEL -E`                 | `node @DESKTOP_SEL:/ -E`
`desktop DESKTOP_SEL -B`                 | `node @DESKTOP_SEL:/ -B`
`desktop DESKTOP_SEL -C forward|backward`| `node @DESKTOP_SEL:/ -C forward|backward`
`desktop DESKTOP_SEL --cancel-presel`    | `bspc query -N -d DESKTOP_SEL | xargs -I id -n 1 bspc node id -p cancel`
`window -t floating`                     | `node -t ~floating`
`query -W -w`                            | `query -N -n .leaf`
`query -{T,H,S}`                         | `wm -d`
`restore -{T,H,S}`                       | `wm -l`
