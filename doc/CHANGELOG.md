# From 0.9.9 to 0.9.10

## Additions

- New node descriptor: `first_ancestor`.
- New node modifiers: `horizontal`, `vertical`.

## Changes

- The node descriptors `next` and `prev` might now return any node. The previous behavior can be emulated by appending `.!hidden.window`.
- The node descriptors `pointed`, `biggest` and `smallest` now return leaves (in particular `pointed` will now return the *id* of a pointed receptacle). The previous behavior can be emulated by appending `.window`.
- The *query* command now handles all the possible descriptor-free constraints (for example, `query -N -d .active` now works as expected).
- The rules can now match against the window's names (`WM_NAME`).
- The configuration script now receives an argument to indicate whether is was executed after a restart or not.
- The *intermediate consequences* passed to the external rules command are now in resolved form to avoid unwanted code execution.

# From 0.9.8 to 0.9.9

- Fix a memory allocation bug in the implementation of `wm --restart`.
- Honor `single_monocle` when the `hidden` flag is toggled.

# From 0.9.7 to 0.9.8

- Fix a potential infinite loop.
- Fix two bugs having to do with `single_monocle`.
- Honor `removal_adjustment` for the spiral automatic insertion scheme.

# From 0.9.6 to 0.9.7

This release fixes a bug in the behavior of `single_monocle`.

# From 0.9.4 to 0.9.6

## Additions

- New *wm* command: `--restart`. It was already possible to restart `bspwm` without loosing the current state through `--{dump,load}-state`, but this command will also keep the existing subscribers intact.
- New settings: `automatic_scheme`, `removal_adjustment`. The automatic insertion mode now provides three ways of inserting a new node: `spiral`, `longest_side` (the default) and `alternate`. Those schemes are described in the README.
- New settings: `ignore_ewmh_struts`, `presel_feedback`, `{top,right,bottom,left}_monocle_padding`.
- New node descriptor: `smallest`.
- New desktop modifier: `active`.

## Changes

- The `focused` and `active` modifiers now mean the same thing across every object.
- Fullscreen windows are no longer sent to the `above` layer. Within the same layer, fullscreen windows are now above floating windows. If you want a floating window to be above a fullscreen window, you'll need to rely on layers.
- Pseudo-tiled windows now shrink automatically.

## Removals

- The `paddingless_monocle` setting was removed (and subsumed). The effect of `paddingless_monocle` can now be achieved with:
```shell
for side in top right bottom left; do
	bspc config ${side}_monocle_padding -$(bspc config ${side}_padding)
done
```

# From 0.9.3 to 0.9.4

## Changes

- The following events: `node_{manage,unmanage}` are now `node_{add,remove}`.

## Additions

- New monitor/desktop/node descriptors: `any`, `newest`.
- New node flag: `marked`.
- New monitor descriptor: `pointed`.
- New *wm* command: `--reorder-monitors`.
- Receptacles are now described in the manual.
- New `--follow` option added to `node -{m,d,n,s}` and `desktop -{m,s}`.
- The *subscribe* command now has the following options: `--fifo`, `--count`.
- New settings: `ignore_ewmh_fullscreen`, `mapping_events_count`.

# From 0.9.2 to 0.9.3

## Changes

- *click_to_focus* is now a button name. Specifying a boolean is deprecated but will still work (`true` is equivalent to `button1`).

## Additions

- `node -r` now accepts a relative fraction argument.
- An option was added to `query -{M,D,N}` in order to output names instead of IDs: `--names`.
- New rule consequence: `rectangle=WxH+X+Y`.
- New settings: `swallow_first_click` and `directional_focus_tightness`.

# From 0.9.1 to 0.9.2

## Changes

- Monitors, desktops and nodes have unique IDs, `bspc query -{N,D,M}` returns IDs and events reference objects by ID instead of name.
- `bspc` fails verbosely and only returns a single non-zero exit code.
- The `DIR` descriptor is based on [right-window](https://github.com/ntrrgc/right-window).
- The `CYCLE_DIR` descriptor isn't limited to the current desktop/monitor anymore. (You can emulate the previous behavior by appending a `.local` modifier to the selector.)
- `bspc query -{N,D,M}` accepts an optional reference argument used by certain descriptors/modifiers.
- Monitors are ordered visually by default.
- The following settings: `border_width`, `window_gap` and `*_padding` behave as expected.
- External rules also receives the monitor, desktop and node selectors computed from the built-in rules stage as subsequent arguments.
- The `focus_follows_pointer` setting is implemented via enter notify events.

## Additions

- Nodes can be hidden/shown via the new `hidden` flag.
- Node receptacles can be inserted with `node -i`. An example is given in `git show e8aa679`.
- Non-tiled nodes can be moved/resized via `node -{v,z}`.
- The reference of a selector can be set via the `{NODE,DESKTOP,MONITOR}_SEL#` prefix, example: `bspc node 0x0080000c#south -c` will close the node at the south of `0x0080000c`.
- Node descriptors: `<node_id>`, `pointed`.
- Node modifiers: `hidden`, `descendant_of`, `ancestor_of`, `window`, `active`. Example: `bspc query -N 0x00400006 -n .descendant_of` returns the descendants of `0x00400006`.
- Desktop descriptor: `<desktop_id>`.
- Monitor descriptor: `<monitor_id>`.
- Settings: `pointer_motion_interval`, `pointer_modifier`, `pointer_action{1,2,3}`, `click_to_focus`, `honor_size_hints`.
- Event: `pointer_action`.
- ICCCM/EWMH atoms: `WM_STATE`, `_NET_WM_STRUT_PARTIAL`.
- `bspc` shell completions for `fish`.

## Removals

- The `pointer` domain. Pointer actions are handled internally. You need to remove any binding that uses this domain from your `sxhkdrc`.
- Settings: `history_aware_focus`, `focus_by_distance`. Both settings are merged into the new `DIR` implementation.
- `monitor -r|--remove-desktops`: use `desktop -r|--remove` instead.
- `wm -r|--remove-monitor`: use `monitor -r|--remove` instead.

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
