/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include "helpers.h"
#include "help.h"

void show_bspc_help(void) {
	warn("bspc usage:\n");
	warn("  node\n");
	warn("  desktop\n");
	warn("  monitor\n");
	warn("  query\n");
	warn("  wm\n");
	warn("  rule\n");
	warn("  subscribe\n");
	warn("  config\n");
	warn("  quit\n");
	exit(EXIT_SUCCESS);
}

void show_node_help(FILE *rsp) {
	fail(rsp,"node usage:\n");
	fail(rsp,"  -f,--focus [NODE_SEL]                                   Focus the selected or given node\n");
	fail(rsp,"  -a,--activate [NODE_SEL]                                Activate the selected or given node\n");
	fail(rsp,"  -d,--to-desktop DESKTOP_SEL                             Send the selected node to the given desktop\n");
	fail(rsp,"  -m,--to-monitor MONITOR_SEL                             Send the selected node to the given monitor\n");
	fail(rsp,"  -n,--to-node NODE_SEL                                   Transplant the selected node to the given node\n");
	fail(rsp,"  -s,--swap NODE_SEL                                      Swap the selected node with the given node\n");
	fail(rsp,"  -p,--presel-dir	[~]DIR|cancel                           Preselect the splitting area of the selected node (or cancel the preselection)\n");
	fail(rsp,"  -o,--presel-ratio RATIO                                 Set the splitting ratio of the preselection area\n");
	fail(rsp,"  -v,--move dx dy                                         Move the selected window by dx pixels horizontally and dy pixels vertically\n");
	fail(rsp,"  -z,--resize	top|left|bottom|right|top_left|top_right|bottom_right|bottom_left dx dy\n");
	fail(rsp,"                                                          Resize the selected window by moving the given handle by dx pixels horizontally and dy pixels vertically.\n");
	fail(rsp,"\n");
	fail(rsp,"  -r,--ratio RATIO|(+|-)PIXELS                            Set the splitting ratio of the selected node (0 < RATIO < 1).\n");
	fail(rsp,"  -R,--rotate	90|270|180                                  Rotate the tree rooted at the selected node\n");
	fail(rsp,"  -Z,--flip horizontal|vertical                           Flip the the tree rooted at selected node\n");
	fail(rsp,"  -E,--equalize                                           Reset the split ratios of the tree rooted at the selected node to their default value\n");
	fail(rsp,"  -B,--balance                                            Adjust the split ratios of the tree rooted at the selected node so that all windows occupy the same area\n");
	fail(rsp,"  -C,--circulate forward|backward                         Circulate the windows of the tree rooted at the selected node\n");
	fail(rsp,"  -t,--state [~](tiled|pseudo_tiled|floating|fullscreen)  Set the state of the selected windo\n");
	fail(rsp,"  -g,--flag hidden|sticky|private|locked[=on|off]         Set or toggle the given flag for the selected nod\n");
	fail(rsp,"  -l,--layer below|normal|above                           Set the stacking layer of the selected window\n");
	fail(rsp,"  -i,--insert-receptacle                                  Insert a receptacle node at the selected node\n");
	fail(rsp,"  -c,--close                                              Close the windows rooted at the selected node\n");
	fail(rsp,"  -k,--kill                                               Kill the windows rooted at the selected nodes\n");
	fail(rsp,"  -h,--help                                               Help\n");
}

void show_desktop_help(FILE *rsp) {
	fail(rsp,"desktop usage:\n");
	fail(rsp,"  -f, --focus [DESKTOP_SEL]             Focus the selected or given desktop\n");
	fail(rsp,"  -a, --activate [DESKTOP_SEL]          Activate the selected or given desktop\n");
	fail(rsp,"  -m, --to-monitor MONITOR_SEL          Send the selected desktop to the given monitor\n");
	fail(rsp,"  -s, --swap DESKTOP_SEL                Swap the selected desktop with the given desktop\n");
	fail(rsp,"  -b, --bubble CYCLE_DIR                Bubble the selected desktop in the given direction\n");
	fail(rsp,"  -l, --layout CYCLE_DIR|monocle|tiled  Set or cycle the layout of the selected desktop\n");
	fail(rsp,"  -n, --rename <new_name>               Rename the selected desktop\n");
	fail(rsp,"  -r, --remove                          Remove the selected desktop\n");
	fail(rsp,"  -h,--help                             Help\n");
}

void show_monitor_help(FILE *rsp) {
	fail(rsp,"monitor usage:\n");
	fail(rsp,"  -f, --focus [MONITOR_SEL]      Focus the selected or given monitor\n");
	fail(rsp,"  -s, --swap MONITOR_SEL         Swap the selected monitor with the given monitor\n");
	fail(rsp,"  -d, --reset-desktop            Rename, add or remove desktops depending on whether the number of given names is equal\n");
	fail(rsp,"  -a, --add-desktops <name>      Create desktops with the given names in the selected monito\n");
	fail(rsp,"  -o, --reorder-desktops <name>  Reorder the desktops of the selected monitor to match the given order\n");
	fail(rsp,"  -g, --rectangle WxH+X+Y        Set the rectangle of the selected monitor\n");
	fail(rsp,"  -n, --rename <new_name>        Rename the selected monitor\n");
	fail(rsp,"  -r, --remove                   Remove the selected monitor\n");
	fail(rsp,"  -h,--help                      Help\n");
}

void show_query_help(FILE *rsp) {
	fail(rsp,"query usage:\n");
	fail(rsp,"  -N, --nodes [NODE_SEL]        List the IDs of the matching nodes\n");
	fail(rsp,"  -D, --desktops [DESKTOP_SEL]  List the IDs of the matching desktops\n");
	fail(rsp,"  -M, --monitors [MONITOR_SEL]  List the IDs of the matching monitors\n");
	fail(rsp,"  -T, --tree                    Print a JSON representation of the matching item\n");
	fail(rsp,"\n");
	fail(rsp,"  Options:\n");
	fail(rsp,"    -m,--monitor [MONITOR_SEL]\n");
	fail(rsp,"    -d,--desktop [DESKTOP_SEL]\n");
	fail(rsp,"    -n, --node [NODE_SEL]\n");
	fail(rsp,"\n");
	fail(rsp,"  -h,--help                     Help\n");
}

void show_wm_help(FILE *rsp) {
	fail(rsp,"wm usage:                           \n");
	fail(rsp,"  -d, --dump-state                  Dump the current world state on standard output\n");
	fail(rsp,"  -l, --load-state <file_path>      Load a world state from the given file\n");
	fail(rsp,"  -a, --add-monitor <name> WxH+X+Y  Add a monitor for the given name and rectangle\n");
	fail(rsp,"  -o, --adopt-orphans               Manage all the unmanaged windows remaining from a previous session\n");
	fail(rsp,"  -h, --record-history on|off       Enable or disable the recording of node focus history\n");
	fail(rsp,"  -O, --reorder-monitors            Reorder the list of monitors to match the given order\n");
	fail(rsp,"  -g, --get-status                  Print the current status information\n");
	fail(rsp,"  -e,--help                         Help\n");
}


void show_rule_help(FILE *rsp) {
	fail(rsp,"rule usage:\n");
	fail(rsp,"  -a,--add (<class_name>|*)[:(<instance_name>|*)] [-o|--one-shot]\n");
	fail(rsp,"           [monitor=MONITOR_SEL|desktop=DESKTOP_SEL|node=NODE_SEL] [state=STATE] [layer=LAYER] [split_dir=DIR] \n");
	fail(rsp,"           [split_ratio=RATIO] [(hidden|sticky|private|locked|center|follow|manage|focus|border)=(on|off)]\n");
	fail(rsp,"                                                                          Create a new rule\n");
	fail(rsp,"\n");
	fail(rsp,"  -r,--remove ^<n>|head|tail|(<class_name>|*)[:(<instance_name>|*)]...    Remove the given rules\n");
	fail(rsp,"  -l,--list                                                               List the rules\n");
	fail(rsp,"  -h,--help                                                               Help\n");
}

void show_subscribe_help(FILE *rsp) {
	fail(rsp,"subscribe usage:\n");
	fail(rsp,"  (all|report|monitor|desktop|node|…​)*\n");
	fail(rsp,"\n");
	fail(rsp,"  Options:\n");
	fail(rsp,"    -f, --fifo\n");
	fail(rsp,"    -c, --count COUNT\n");
	fail(rsp,"  -h,--help                                                               Help\n");
}

void show_config_help(FILE *rsp) {
	fail(rsp,"config usage:\n");
	fail(rsp,"  [-m MONITOR_SEL] <setting> [<value>]\n");
	fail(rsp,"  [-d DESKTOP_SEL] <setting> [<value>]\n");
	fail(rsp,"  [-n NODE_SEL]    <setting> [<value>]\n");
	fail(rsp,"\n");
	fail(rsp,"  Options:\n");
	fail(rsp,"    -f, --fifo         Print a path to a FIFO from which events can be read and return\n");
	fail(rsp,"    -c, --count COUNT  Stop the corresponding bspc process after having received COUNT events\n");
	fail(rsp,"  -h,--help            Help\n");
}
