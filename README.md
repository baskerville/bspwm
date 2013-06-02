![logo](https://github.com/baskerville/bspwm/raw/master/logo/bspwm-logo.png)

## Synopsis

    bspwm [-h|-v|-s PANEL_FIFO|-p PANEL_PREFIX]

    bspc MESSAGE [ARGUMENTS] [OPTIONS]

## Description

`bspwm` is a tiling window manager that represents windows as the leaves of a full binary tree.

It is controlled and configured via `bspc`.

## Configuration

`bspwm` have only two sources of informations: the X events it receives and the messages it reads on a dedicated socket.

Its configuration file is `$XDG_CONFIG_HOME/bspwm/autostart`.

Keyboard and pointer bindings are defined with [sxhkd](https://github.com/baskerville/sxhkd).

Example configuration files can be found in the `examples` directory.

## Splitting Modes

There is only two splitting modes: *automatic* and *manual*.

The default mode is *automatic*. The *manual* mode is entered by sending a *presel* message.

Example: insertion of a new node (number 4) into the given tree in
*automatic* mode:

                     b                                   c
                    / \                                 / \
                   3   a              -->              4   b
                   ^  / \                              ^  / \
                     2   1                               3   a
                                                            / \
                                                           2   1
        +-------------------------+         +-------------------------+
        |            |            |         |            |            |
        |            |     2      |         |            |     3      |
        |            |            |         |            |            |
        |     3      |------------|   -->   |     4      |------------|
        |     ^      |            |         |     ^      |     |      |
        |            |     1      |         |            |  1  |  2   |
        |            |            |         |            |     |      |
        +-------------------------+         +-------------------------+

Same departure, but the mode is *manual*, and a `presel up` message
was sent beforehand:

                     b                                   b
                    / \                                 / \
                   3   a              -->              c   a
                   ^  / \                             / \ / \
                     2   1                           4  3 2  1
                                                     ^
        +-------------------------+         +-------------------------+
        |            |            |         |            |            |
        |            |     2      |         |     4      |     2      |
        |            |            |         |     ^      |            |
        |     3      |------------|   -->   |------------|------------|
        |     ^      |            |         |            |            |
        |            |     1      |         |     3      |     1      |
        |            |            |         |            |            |
        +-------------------------+         +-------------------------+

## Containers

Each monitor contains at least one desktop.

Each desktop contains at most one tree.

## Messages

The syntax for the client is `bspc MESSAGE [ARGUMENTS ...]`.

The following messages are handled:

- `get SETTING` — Return the value of the given setting.

- `set SETTING VALUE` — Set the value of the given setting.

- `list [DESKTOP_NAME]` — Output the internal representation of the window tree.

- `list_desktops [--quiet]` — Perform a dump of each desktop for the current monitor.

- `list_monitors [--quiet]` — Perform a dump of each monitor.

- `list_history` — Return the node focus history of each desktop.

- `list_windows` — Return the list of managed windows (i.e. their identifiers).

- `list_rules` — Return the list of rules.

- `presel left|right|up|down [SPLIT_RATIO]` — Switch to manual mode and select the splitting direction.

- `cancel` — Switch to automatic mode.

- `ratio VALUE` — Set the splitting ratio of the focused window.

- `pad MONITOR_NAME [TOP_PADDING [RIGHT_PADDING [BOTTOM_PADDING [LEFT_PADDING]]]]` — Set the padding of the given monitor.

- `focus left|right|up|down` — Focus the neighbor window situated in the given direction.

- `shift left|right|up|down` — Exchange the current window with the given neighbor.

- `swap [--swap-focus|--keep-focus]` — Swap the focused window with the last focused window.

- `push left|right|up|down` — Push the fence located in the given direction.

- `pull left|right|up|down` — Pull the fence located in the given direction.

- `cycle next|prev [--skip-floating|--skip-tiled|--skip-class-equal|--skip-class-differ]` — Focus the next or previous window matching the given constraints.

- `nearest older|newer [--skip-floating|--skip-tiled|--skip-class-equal|--skip-class-differ]` — Focus the nearest window matching the given constraints.

- `biggest` — Return the ID of the biggest tiled window.

- `circulate forward|backward` — Circulate the leaves in the given direction.

- `grab_pointer focus|move|resize_side|resize_corner` — Begin the specified pointer action.

- `track_pointer ROOT_X ROOT_Y` — Pass the pointer root coordinates for the current pointer action.

- `ungrab_pointer` — End the current pointer action.

- `toggle_fullscreen` — Toggle the fullscreen state of the current window.

- `toggle_floating` — Toggle the floating state of the current window.

- `toggle_locked` — Toggle the locked state of the current window (locked windows will not respond to the `close` message).

- `toggle_visibility` — Toggle the visibility of all the managed windows.

- `close` — Close the focused window.

- `kill` — Kill the focused window.

- `send_to DESKTOP_NAME [--follow]` — Send the focused window to the given desktop.

- `drop_to next|prev [--follow]` — Send the focused window to the next or previous desktop.

- `send_to_monitor MONITOR_NAME [--follow]` — Send the focused window to the given monitor.

- `drop_to_monitor next|prev [--follow]` — Send the focused window to the next or previous monitor.

- `use DESKTOP_NAME` — Select the given desktop.

- `use_monitor MONITOR_NAME` — Select the given monitor.

- `alternate` — Alternate between the current and the last focused window.

- `alternate_desktop` — Alternate between the current and the last focused desktop.

- `alternate_monitor` — Alternate between the current and the last focused monitor.

- `add DESKTOP_NAME ...` — Make new desktops with the given names.

- `add_in MONITOR_NAME DESKTOP_NAME ...` — Make new desktops with the given names in the given monitor.

- `rename_monitor CURRENT_NAME NEW_NAME` — Rename the monitor named `CURRENT_NAME` to `NEW_NAME`.

- `rename CURRENT_NAME NEW_NAME` — Rename the desktop named `CURRENT_NAME` to `NEW_NAME`.

- `remove_desktop DESKTOP_NAME ...` — Remove the given desktops.

- `cycle_monitor next|prev` — Select the next or previous monitor.

- `cycle_desktop next|prev [--skip-free|--skip-occupied]` — Select the next or previous desktop.

- `layout monocle|tiled [DESKTOP_NAME ...]` — Set the layout of the given desktops (current if none given).

- `cycle_layout` — Cycle the layout of the current desktop.

- `rotate clockwise|counter_clockwise|full_cycle` — Rotate the window tree.

- `flip horizontal|vertical` — Flip the window tree.

- `balance` — Adjust the split ratios so that all windows occupy the same area.

- `rule PATTERN [DESKTOP_NAME] [floating] [follow]` — Create a new rule (`PATTERN` must match the class or instance name).

- `remove_rule UID ...` — Remove the rules with the given UIDs.

- `put_status` — Output the current state to the panel fifo.

- `adopt_orphans` — Manage all the unmanaged windows remaining from a previous session.

- `restore_layout FILE_PATH` — Restore the layout of each desktop from the content of `FILE_PATH`.

- `restore_history FILE_PATH` — Restore the history of each desktop from the content of `FILE_PATH`.

- `quit [EXIT_STATUS]` — Quit.

## Settings

Colors are either [X color names](http://en.wikipedia.org/wiki/X11_color_names) or *#RRGGBB*, booleans are *true* or *false*.

- `focused_border_color` — Color of the border of a focused window of a focused monitor.

- `active_border_color` — Color of the border of a focused window of an unfocused monitor.

- `normal_border_color` — Color of the border of an unfocused window.

- `presel_border_color` — Color of the `presel` message feedback.

- `focused_locked_border_color` — Color of the border of a focused locked window of a focused monitor.

- `active_locked_border_color` — Color of the border of a focused locked window of an unfocused monitor.

- `normal_locked_border_color` — Color of the border of an unfocused locked window.

- `urgent_border_color` — Color of the border of an urgent window.

- `border_width` — Window border width.

- `window_gap` — Value of the gap that separates windows.

- `split_ratio` — Default split ratio.

- `{top,right,bottom,left}_padding` — Padding space added at the sides of the current monitor.

- `wm_name` — The value that shall be used for the `_NET_WM_NAME` property of the root window.

- `borderless_monocle` — Whether to remove borders for tiled windows in monocle mode.

- `gapless_monocle` — Whether to remove gaps for tiled windows in monocle mode.

- `focus_follows_pointer` — Whether to focus the window under the pointer.

- `adaptative_raise` — Prevent floating windows from being raised when they might cover other floating windows.

- `apply_shadow_property` — Enable shadows for floating windows via the `_COMPTON_SHADOW` property.

- `auto_alternate` — Whether to interpret two consecutive identical `use` messages as an `alternate` message.

- `focus_by_distance` — Whether to use window or leaf distance for focus movement.

## Environment Variables

- `BSPWM_SOCKET` — The path of the socket used for the communication between `bspc` and `bspwm`.

## Key Features

- Configured and controlled through messages.
- Multiple monitors support (via *RandR*).
- EWMH support (`tint2` works).
- Automatic and manual modes.

## Panels

- Any EWMH compliant panel (e.g.: `tint2`, `bmpanel2`, etc.).
- A custom panel if the `-s` flag is used (have a look at the files in `examples/panel`).

## Required Libraries:

- libxcb
- xcb-util
- xcb-util-wm

## Installation

    make
    make install

## Contributors

- [Ivan Kanakarakis](https://github.com/c00kiemon5ter)

- [Thomas Adam](https://github.com/ThomasAdam)

## Mailing List

bspwm *at* librelist *dot* com.

## License

BSD.
