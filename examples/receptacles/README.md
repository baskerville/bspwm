The scripts present in this directory can be used to store and recreate layouts.

Both scripts take a JSON state (output of `bspc wm -d`) as input or argument.

- `extract_canvas [state.json]` outputs a new JSON state where each leaf window is replaced by a receptacle.

- `induce_rules [state.json]` outputs a list of commands that, if executed, will create rules to place each window in the corresponding receptacle.
