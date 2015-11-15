## Description

*bspwm* is a tiling window manager that represents windows as the leaves of a full binary tree.

It only responds to X events, and the messages it receives on a dedicated socket.

*bspc* is a program that writes messages on *bspwm*'s socket.

*bspwm* doesn't handle any keyboard or pointer inputs: a third party program (e.g. *sxhkd*) is needed in order to translate keyboard and pointer events to *bspc* invocations.

The outlined architecture is the following:

```
        PROCESS          SOCKET
sxhkd  -------->  bspc  <------>  bspwm
```

## Configuration

The default configuration file is `$XDG_CONFIG_HOME/bspwm/bspwmrc`: this is simply a shell script that calls *bspc*.

Keyboard and pointer bindings are defined with [sxhkd](https://github.com/baskerville/sxhkd).

Example configuration files can be found in the `examples` directory.

## Monitors, desktops and windows

*bspwm* holds a list of monitors.

A monitor is just a rectangle that contains desktops.

A desktop is just a pointer to a tree.

Monitors only show the tree of one desktop at a time (their focused desktop).

The tree is a partition of a monitor's rectangle into smaller rectangular regions.

Each node in a tree either has zero or two children.

Each internal node is responsible for splitting a rectangle in half.

A split is defined by two parameters: the type (horizontal or vertical) and the ratio (a real number *r* such that *0 < r < 1*).

Each leaf node holds exactly one window.

## Insertion Modes

### Prelude

When *bspwm* receives a new window, it inserts it into a window tree at the specified insertion point (a leaf) using the insertion mode specified for that insertion point.

The insertion mode tells *bspwm* how it should alter the tree in order to insert new windows on a given insertion point.

By default the insertion point is the focused window and its default insertion mode is *automatic*.

### Automatic Mode

The *automatic* mode, as opposed to the *manual* mode, doesn't require any user choice: the new window will *take the space* of the insertion point.

For example, let's consider the following scenario:

```
             a                          a                          a
            / \                        / \                        / \
           1   b         --->         1   c         --->         1   d
              / \                        / \                        / \
             2   3                      4   b                      5   c
             ^                          ^  / \                     ^  / \
                                          3   2                      b   4
                                                                    / \ 
                                                                   3   2

 +-----------------------+  +-----------------------+  +-----------------------+
 |           |           |  |           |           |  |           |           |
 |           |     2     |  |           |     4     |  |           |     5     |
 |           |     ^     |  |           |     ^     |  |           |     ^     |
 |     1     |-----------|  |     1     |-----------|  |     1     |-----------|
 |           |           |  |           |     |     |  |           |  3  |     |
 |           |     3     |  |           |  3  |  2  |  |           |-----|  4  |
 |           |           |  |           |     |     |  |           |  2  |     |
 +-----------------------+  +-----------------------+  +-----------------------+

              X                         Y                          Z 
```

In state *X*, the insertion point, *2* is in automatic mode.

When we add a new window, *4*, the whole tree rooted at *b* is reattached, as the second child of a new internal node, *c*.

The splitting parameters of *b* (type: *horizontal*, ratio: *½*) are copied to *c* and *b* is rotated by 90° clockwise.

The tiling rectangle of *4* in state *Y* is equal to the tiling rectangle of *2* in state *X*.

Then the insertion of *5*, with *4* as insertion point, leads to *Z*.

The automatic mode generates window spirals that rotate clockwise (resp. anti-clockwise) if the insertion point is the first (resp. second) child of its parent.

### Manual Mode

The user can specify a region in the insertion point where the next new window should appear by sending a *window --presel DIR* message to *bspwm*.

The *DIR* argument allows to specify how the insertion point should be split (horizontally or vertically) and if the new window should be the first or the second child of the new internal node (the insertion point will become its *brother*).

After doing so the insertion point goes into *manual* mode.

For example, let's consider the following scenario:

```
            a                          a                          a
           / \                        / \                        / \
          1   b         --->         c   b         --->         c   b
          ^  / \                    / \ / \                    / \ / \
            2   3                  4  1 2  3                  d  1 2  3
                                   ^                         / \
                                                            5   4
                                                            ^

+-----------------------+  +-----------------------+  +-----------------------+
|           |           |  |           |           |  |     |     |           |
|           |     2     |  |     4     |     2     |  |  5  |  4  |     2     |
|           |           |  |     ^     |           |  |  ^  |     |           |
|     1     |-----------|  |-----------|-----------|  |-----------|-----------|
|     ^     |           |  |           |           |  |           |           |
|           |     3     |  |     1     |     3     |  |     1     |     3     |
|           |           |  |           |           |  |           |           |
+-----------------------+  +-----------------------+  +-----------------------+

            X                          Y                          Z 
```

In state *X*, the insertion point is *1*.

We send the following message to *bspwm*: *window --presel up*.

Then add a new window: *4*, this leads to state *Y*: the new internal node, *c* becomes *a*'s first child.

Finally we send another message: *window --presel left* and add window *5*.

The ratio of the preselection (that ends up being the ratio of the split of the new internal node) can be change with the *window --ratio* message.

## Supported protocols and standards

- The RandR and Xinerama protocols.
- A subset of the EWMH and ICCCM standards.
