% Bspwm User's Guide
% Bastien Dejean
% June 23, 2013

![logo](https://github.com/baskerville/bspwm/raw/master/logo/bspwm-logo.png)

# Synopsis

**bspwm** [**-h**|**-v**|**-s** *PANEL\_FIFO*|**-p** *PANEL\_PREFIX*]

**bspc** *MESSAGE* [*ARGUMENTS*] [*OPTIONS*]

# Description

**bspwm** is a tiling window manager that represents windows as the leaves of a full binary tree.

It is controlled and configured via **bspc**.

# Configuration

**bspwm** have only two sources of informations: the X events it receives and the messages it reads on a dedicated socket.

Its configuration file is *$XDG_CONFIG_HOME/bspwm/autostart*.

Keyboard and pointer bindings are defined with [sxhkd](https://github.com/baskerville/sxhkd).

Example configuration files can be found in the **examples** directory.

# Splitting Modes

There is only two splitting modes: *automatic* and *manual*.

The default mode is *automatic*. The *manual* mode is entered by sending a **presel** message.

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

Same departure, but the mode is *manual*, and a **presel** *up* message
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

# Containers

Each monitor contains at least one desktop.

Each desktop contains at most one tree.

# Messages

**get** *SETTING*
:    Return the value of the given setting.

**set** *SETTING* *VALUE*
:    Set the value of the given setting.

**list** [*DESKTOP\_NAME*]
:    Output the internal representation of the window tree.

**list\_desktops** [**--quiet**]
:    Perform a dump of each desktop for the current monitor.

**list\_monitors** [**--quiet**]
:    Perform a dump of each monitor.

**list\_history**
:    Return the node focus history of each desktop.

**list\_windows**
:    Return the list of managed windows (i.e. their identifiers).

**list\_rules**
:    Return the list of rules.

**presel** *left*|*right*|*up*|*down* [*SPLIT\_RATIO*]
:    Switch to manual mode and select the splitting direction.

**cancel**
:    Switch to automatic mode.

**ratio** *VALUE*
:    Set the splitting ratio of the focused window.

**pad** *MONITOR\_NAME* [*TOP\_PADDING* [*RIGHT\_PADDING* [*BOTTOM\_PADDING* [*LEFT\_PADDING*]]]]
:    Set the padding of the given monitor.

**focus** *left*|*right*|*up*|*down*
:    Focus the neighbor window situated in the given direction.

**shift** *left*|*right*|*up*|*down*
:    Exchange the current window with the given neighbor.

**swap** [**--keep-focus**]
:    Swap the focused window with the last focused window.

**push** *left*|*right*|*up*|*down*
:    Push the fence located in the given direction.

**pull** *left*|*right*|*up*|*down*
:    Pull the fence located in the given direction.

**fence\_ratio** *left*|*right*|*up*|*down*
:    Set the splitting ratio of the fence located in the given direction.

**cycle** *next*|*prev* [**--skip-floating**|**--skip-tiled**|**--skip-class-equal**|**--skip-class-differ**]
:    Focus the next or previous window matching the given constraints.

**nearest** *older*|*newer* [**--skip-floating**|**--skip-tiled**|**--skip-class-equal**|**--skip-class-differ**]
:    Focus the nearest window matching the given constraints.

**biggest**
:    Return the ID of the biggest tiled window.

**circulate** *forward*|*backward*
:    Circulate the leaves in the given direction.

**grab\_pointer** *focus*|*move*|*resize\_side*|*resize\_corner*
:    Begin the specified pointer action.

**track\_pointer** *ROOT\_X* *ROOT\_Y*
:    Pass the pointer root coordinates for the current pointer action.

**ungrab\_pointer**
:    End the current pointer action.

**toggle\_fullscreen**
:    Toggle the fullscreen state of the current window.

**toggle\_floating**
:    Toggle the floating state of the current window.

**toggle\_locked**
:    Toggle the locked state of the current window (locked windows will not respond to the **close** message).

**toggle\_visibility**
:    Toggle the visibility of all the managed windows.

**close**
:    Close the focused window.

**kill**
:    Kill the focused window.

**send\_to** *DESKTOP\_NAME* [**--follow**]
:    Send the focused window to the given desktop.

**drop\_to** *next*|*prev* [**--follow**]
:    Send the focused window to the next or previous desktop.

**send\_to\_monitor** *MONITOR\_NAME* [**--follow**]
:    Send the focused window to the given monitor.

**drop\_to\_monitor** *next*|*prev* [**--follow**]
:    Send the focused window to the next or previous monitor.

**use** *DESKTOP\_NAME*
:    Select the given desktop.

**use\_monitor** *MONITOR\_NAME*
:    Select the given monitor.

**alternate**
:    Alternate between the current and the last focused window.

**alternate\_desktop**
:    Alternate between the current and the last focused desktop.

**alternate\_monitor**
:    Alternate between the current and the last focused monitor.

**add** *DESKTOP\_NAME* ...
:    Make new desktops with the given names.

**add\_in** *MONITOR\_NAME* *DESKTOP\_NAME* ...
:    Make new desktops with the given names in the given monitor.

**rename\_monitor** *CURRENT\_NAME* *NEW\_NAME*
:    Rename the monitor named *CURRENT\_NAME* to *NEW\_NAME*.

**rename** *CURRENT\_NAME* *NEW\_NAME*
:    Rename the desktop named *CURRENT\_NAME* to *NEW\_NAME*.

**remove\_desktop** *DESKTOP\_NAME* ...
:    Remove the given desktops.

**send\_desktop\_to** *MONITOR\_NAME* [**--follow**]
:    Send the current desktop to the given monitor.

**cycle\_monitor** *next*|*prev*
:    Select the next or previous monitor.

**cycle\_desktop** *next*|*prev* [**--skip-free**|**--skip-occupied**]
:    Select the next or previous desktop.

**layout** *monocle*|*tiled* [*DESKTOP\_NAME* ...]
:    Set the layout of the given desktops (current if none given).

**cycle\_layout**
:    Cycle the layout of the current desktop.

**rotate** *clockwise*|*counter\_clockwise*|*full\_cycle*
:    Rotate the window tree.

**flip** *horizontal*|*vertical*
:    Flip the window tree.

**balance**
:    Adjust the split ratios so that all windows occupy the same area.

**rule** *PATTERN* [*DESKTOP\_NAME*] [*floating*] [*follow*]
:    Create a new rule (*PATTERN* must match the class or instance name).

**remove\_rule** *UID* ...
:    Remove the rules with the given *UID*s.

**put\_status**
:    Output the current state to the panel fifo.

**adopt\_orphans**
:    Manage all the unmanaged windows remaining from a previous session.

**restore\_layout** *FILE\_PATH*
:    Restore the layout of each desktop from the content of *FILE\_PATH*.

**restore\_history** *FILE\_PATH*
:    Restore the history of each desktop from the content of *FILE\_PATH*.

**quit** [*EXIT\_STATUS*]
:    Quit.

# Settings

Colors are either [X color names](http://en.wikipedia.org/wiki/X11\_color\_names) or *#RRGGBB*, booleans are *true* or *false*.

All the boolean settings are *false* by default.

*focused\_border\_color*
:    Color of the border of a focused window of a focused monitor.

*active\_border\_color*
:    Color of the border of a focused window of an unfocused monitor.

*normal\_border\_color*
:    Color of the border of an unfocused window.

*presel\_border\_color*
:    Color of the **presel** message feedback.

*focused\_locked\_border\_color*
:    Color of the border of a focused locked window of a focused monitor.

*active\_locked\_border\_color*
:    Color of the border of a focused locked window of an unfocused monitor.

*normal\_locked\_border\_color*
:    Color of the border of an unfocused locked window.

*urgent\_border\_color*
:    Color of the border of an urgent window.

*border\_width*
:    Window border width.

*window\_gap*
:    Value of the gap that separates windows.

*split\_ratio*
:    Default split ratio.

*top\_padding*, *right\_padding*, *bottom\_padding*, *left\_padding*
:    Padding space added at the sides of the current monitor.

*wm\_name*
:    The value that shall be used for the *\_NET\_WM\_NAME* property of the root window.

*borderless\_monocle*
:    Remove borders for tiled windows in monocle mode.

*gapless\_monocle*
:    Remove gaps for tiled windows in monocle mode.

*focus\_follows\_pointer*
:    Focus the window under the pointer.

*pointer\_follows\_monitor*
:    When focusing a monitor, put the pointer at its center.

*adaptative\_raise*
:    Prevent floating windows from being raised when they might cover other floating windows.

*apply\_shadow\_property*
:    Enable shadows for floating windows via the *\_COMPTON\_SHADOW* property.

*auto\_alternate*
:    Interpret two consecutive identical **use** messages as an **alternate** message.

*focus\_by\_distance*
:    Use window or leaf distance for focus movement.

*history\_aware\_focus*
:    Give priority to the focus history when focusing nodes.

# Environment Variables

*BSPWM_SOCKET*
:    The path of the socket used for the communication between **bspc** and **bspwm**.

# Panels

- Any EWMH compliant panel (e.g.: *tint2*, *bmpanel2*, etc.).
- A custom panel if the *-s* flag is used (have a look at the files in *examples/panel*).

# Key Features

- Configured and controlled through messages.
- Multiple monitors support (via *RandR*).
- EWMH support (**tint2** works).
- Automatic and manual modes.

# Mailing List

bspwm at librelist.com
