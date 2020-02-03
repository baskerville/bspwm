#! /bin/sh

wid=$1
class=$2
instance=$3
consequences=$4

if [ "$instance" = fontforge ] ; then
	title=$(xtitle "$wid")
	case "$title" in
		Layers|Tools|Warning)
			echo "focus=off"
			;;
	esac
fi

case "$class" in
	Lutris|Liferea)
		eval "$consequences"
		[ "$state" ] || echo "state=pseudo_tiled"
		;;
esac
