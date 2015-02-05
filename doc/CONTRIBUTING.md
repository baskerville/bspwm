## Requirements

You must be comfortable with [C][1], [XCB][2] and [Git][3].

## Coding Style

I follow the [Linux Coding Style][4].

## Browsing the Code

If you use `vim`:
- Hitting *K* will lead you to the manual page of the function under the cursor (works with most `xcb_*` functions), sometimes you'll have to explicitly specify the section of the manual you're interested in with *3K* (e.g.: `open`).
- Install `ctags` and run `ctags *.{c,h}` in the directory holding the source. Then, hitting *Ctrl-]* will lead you to the definition of the function/variable/structure under the cursor (to go back: *Ctrl-T*).
- You can run `make` from `vim` with `:make` and then navigate to the next and the previous error with `:cn` and `:cp`.

## Debugging

To install debug executables:
```
make clean debug && make install
```

You can attach to a running *bspwm* process with:
```
gdb bspwm $(pgrep -x bspwm)
c
```

Or if you just want to generate a backtrace (saved in `gdb.txt`):
```
ulimit -c unlimited
startx
sudo systemd-coredumpctl gdb bspwm
set logging on
bt full
q
```

[1]: http://cm.bell-labs.com/cm/cs/cbook/
[2]: http://www.x.org/releases/X11R7.5/doc/libxcb/tutorial/
[3]: http://git-scm.com/documentation
[4]: http://www.kernel.org/doc/Documentation/CodingStyle
