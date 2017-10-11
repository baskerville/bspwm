#! /bin/sh

if xdo id -a "$PANEL_WM_NAME" > /dev/null ; then
	printf "%s\n" "The panel is already running." >&2
	exit 1
fi

trap 'trap - TERM; kill 0' INT TERM QUIT EXIT

[ -e "$PANEL_FIFO" ] && rm "$PANEL_FIFO"
mkfifo "$PANEL_FIFO"

xtitle -sf 'T%s\n' > "$PANEL_FIFO" &
clock -sf 'S%a %H:%M' > "$PANEL_FIFO" &
bspc subscribe report > "$PANEL_FIFO" &

. panel_colors

panel_bar < "$PANEL_FIFO" | lemonbar -a 32 -u 2 -n "$PANEL_WM_NAME" -g x$PANEL_HEIGHT -f "$PANEL_FONT" -F "$COLOR_DEFAULT_FG" -B "$COLOR_DEFAULT_BG" | sh &

wid=$(xdo id -m -a "$PANEL_WM_NAME")
xdo above -t "$(xdo id -N Bspwm -n root | sort | head -n 1)" "$wid"

wait
