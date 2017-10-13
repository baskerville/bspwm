#compdef bspc

_bspc_selector() {
	[[ ${@[(r)--]} = '--' ]] && shift ${@[(i)--]}
	local -a completions=() completions_display=()
	local index=0 name id sel_type="$1"
	shift 1
	case $sel_type in
		(node) compset -P '*[#.:/@]' ;;
		(desktop) compset -P '*[#.:]' ;;
		(monitor) compset -P '*[#.]' ;;
		(*) return 1 ;;
	esac
	case "$sel_type $IPREFIX" in
		(desktop*:)
			local ipfx=${${IPREFIX##*@}%:}
			while do
				if completions=("${(@f)$(bspc query --names -D -m ${ipfx} 2> /dev/null)}") ;then
					until ((++index > $#completions)) do
						completions[index]="^${index}:${completions[index]}"
					done
					completions+='focused'
					_describe "${sel_type} selector" completions $@ -S '' -J ${sel_type}
					break
				else
					completions=()
					[ -n "$ipfx[(r)#]" ] &&
						ipfx="${ipfx#*#}" ||
						break
				fi
			done
			;|
		(node*[^@/:.])
			bspc query -N -n .window 2> /dev/null |
				while read id ;do
					id=${id//:/\\:}
					if which xdotool &> /dev/null ;then
						name=$(xdotool getwindowname $id 2> /dev/null)
					elif which xprop &> /dev/null ;then
						name=$(xprop -id $id -notype WM_NAME 2> /dev/null) &&
							[[ "$name" = 'WM_NAME ='* ]] &&
							name="${${name#*\"}%\"*}" ||
							name=""
					else
						name="install xdotool or xprop to see window titles here"
					fi
					completions+="$id:$name"
				done
			;|
		((desktop|monitor)*[^:.])
			local max_name_len=0 max_index_len i
			local -a snames names ids
			bspc query -${(U)sel_type[1]} 2> /dev/null |
				while read id ;do
					((index++))
					name=$(bspc query --names -${(U)sel_type[1]} -${sel_type[1]} $id 2> /dev/null)
					[[ "$name" == *[:.!]* ]] &&
						sel_name="%${name//:/\\:}" ||
						sel_name="$name"
					((max_name_len = $#sel_name > max_name_len ? $#sel_name : max_name_len))
					ids+="${id}"
					snames+="${sel_name}"
					names+="${name}"
				done
				max_index_len=$(($#index + 1))
				((max_name_len >= max_name_len)) && ((max_name_len=max_name_len + 1))
				for ((i = 1 ; i <= $#ids ; i++)) ;do
					(($#ids[i] <= max_name_len)) &&
						completions_display+="${(r($max_name_len+1))ids[i]}:${names[i]}" ||
						completions_display+="${ids[i]}:${names[i]}"
					completions+="${ids[i]}:${names[i]}"
				done
				for ((i = 1 ; i <= $#ids ; i++)) ;do
					completions+="${snames[i]}:${names[i]}"
					completions_display+="${(r($max_name_len))snames[i]}:${names[i]}"
				done
				for ((i = 1 ; i <= $#ids ; i++)) ;do
					completions+="^${i}:${names[i]}"
					completions_display+="^${i}:${names[i]}"
				done
			;|
		(node*('@'(*':'|*'/'|)))
			_describe 'node path' jump -S '/' -r "#. ${quote}" -J nodes
			;|
		(node*'/')
			;;
		(*'.')
			_bspc_prefix '!' "${sel_type} modifiers" ${sel_type}_mod $@ -J ${sel_type}_mod
			;|
		((desktop|monitor)*@)
			;&
		(*[^:.@/])
			if (( $#completions_display)) ;then
			_describe "${sel_type} selector" ${sel_type}_desc $@ -S '.' -r ". \n:#${quote}\-" -J ${sel_type} \
				-- completions_display completions $@ -S '.' -r ". \n:#${quote}\-" -J ${sel_type}
			else
			_describe "${sel_type} selector" ${sel_type}_desc $@ -S '.' -r ". \n:#${quote}\-" -J ${sel_type} \
				-- completions $@ -S '.' -r ". \n:#${quote}\-" -J ${sel_type}
			fi
			;|
		(node*@*)
			_bspc_selector desktop -S ':' -qr ".#\-\n ${quote}"
			;;
		(desktop*)
			_bspc_selector monitor -S ':' -r ".#\-\n ${quote}"
			;;
	esac
}

_bspc_prefix(){
	[[ ${@[(r)--]} = '--' ]] && shift ${@[(i)--]}
	[[ "$PREFIX[1]" == "$1" ]] &&
		local a="-n" b ||
		local b="-n" a
	_describe $@[2,-1] $a -- $@[3,-1] $b -p "$1"
}

_bspc_query_names() {
	[[ ${@[(r)--]} = '--' ]] && shift ${@[(i)--]}
	local -a items=("${(@f)$(bspc query $2 --names 2> /dev/null)}") ||
		return
	local c
	for c in '\' ':' '[' '(' '*'
		items=("${(@)items//$c/\\$c}")
	_values -w  "$1" "${items[@]}"
}

_bspc() {
	local -a commands \
		commands=(node desktop monitor query rule wm subscribe config quit) \
		node_state=(tiled pseudo_tiled floating fullscreen) \
		flag=(hidden sticky private locked urgent) \
		layer=(below normal above) \
		dir=(north west south east) \
		cycle_dir=(next prev)
	local -a jump=($dir first second brother parent 1 2) \
		node_desc=($dir $cycle_dir last older newer focused pointed biggest) \
		node_mod=($node_state $flag $layer focused automatic local \
		active leaf window same_class descendant_of ancestor_of) \
		desktop_desc=($cycle_dir last older newer focused) \
		desktop_mod=(focused occupied local urgent) \
		monitor_desc=($dir $cycle_dir primary last older newer focused pointed) \
		monitor_mod=(focused occupied) \
		presel_dir=($dir cancel)
	local quote="${compstate[quote]}" context state state_descr line
	typeset -A opt_args

	compset -n 2
	compset -S "${quote}"

	if ((CURRENT==1)) ;then
		_describe 'command or domain' commands
		return
	fi

	case $words[1] in
		(node)
			((CURRENT==2)) && _bspc_selector node
			((CURRENT>2)) && [[ "$words[2]" != '-'* ]] && compset -n 2
			while do
				_arguments -C \
					'*'{-a,--activate}'[Activate the selected or given node]::node selector:_bspc_selector -- node'\
					'*'{-B,--balance}'[Adjust the split ratios of the tree rooted at the selected node so that all windows occupy the same area]'\
					'*'{-C,--circulate}'[Circulate the windows of the tree rooted at the selected node]:direction:(forward backward)'\
					'*'{-c,--close}'[Close the windows rooted at the selected node]'\
					'*'{-d,--to-desktop}'[Send the selected node to the given desktop]:desktop selector:_bspc_selector -- desktop'\
					'*'{-E,--equalize}'[Reset the split ratios of the tree rooted at the selected node to their default value]'\
					'*'{-F,--flip}'[Flip the the tree rooted at selected node]: :(horizontal vertical)'\
					'*'{-f,--focus}'[Focus the selected or given node]::node selector:_bspc_selector -- node'\
					'*'{-g,--flag}'[Set or toggle the given flag for the selected node]: :->flag'\
					'*'{-i,--insert-receptacle}'[Insert a receptacle node at the selected node]'\
					'*'{-k,--kill}'[Kill the windows rooted at the selected node]'\
					'*'{-l,--layer}"[Set the stacking layer of the selected window]:stacking layer:($layer)"\
					'*'{-m,--to-monitor}'[Send the selected node to the given monitor]:monitor selector:_bspc_selector -- monitor'\
					'*'{-n,--to-node}'[Transplant the selected node to the given node]:node selector:_bspc_selector -- node'\
					'*'{-o,--presel-ratio}'[Set the splitting ratio of the preselection area]:preselect ratio: ( )'\
					'*'{-p,--presel-dir}'[Preselect the splitting area of the selected node or cancel the preselection]: :_bspc_prefix -- "~" preselect presel_dir'\
					'*'{-r,--ratio}'[Set the splitting ratio of the selected node (0 < ratio < 1)]: :( )'\
					'*'{-R,--rotate}'[Rotate the tree rooted at the selected node]:angle:(90 270 180)'\
					'*'{-s,--swap}'[Swap the selected node with the given node]:node selector:_bspc_selector -- node'\
					'*'{-t,--state}'[Set the state of the selected window]: :_bspc_prefix -- "~" "node state" node_state '\
					'*'{-v,--move}'[Move the selected window by dx pixels horizontally and dy pixels vertically]:dx dy:'\
					'*'{-z,--resize}'[Resize the selected window by moving the given handle by dx pixels horizontally and dy pixels vertically]:*::: :->resize'
				[ "$state" = flag ] && _values 'flag' "${flag[@]:#urgent}::set flag:(on off)"
				[ "$state" = resize ] || break
				((CURRENT == 1)) &&
					_values 'handle' resize_handle {top,bottom}{,_left,_right} left right
				((CURRENT <= 3)) && break
				compset -n 3
			done
			;;
		(desktop)
			((CURRENT==2)) && _bspc_selector desktop
			((CURRENT>2)) && [[ "$words[2]" != '-'* ]] && compset -n 2
			_arguments \
				'*'{-a,--activate}'[Activate the selected or given desktop]:: :_bspc_selector -- desktop'\
				'*'{-b,--bubble}"[Bubble the selected desktop in the given direction]:direction:($cycle_dir)"\
				'*'{-f,--focus}'[Focus the selected or given desktop]:: :_bspc_selector -- desktop'\
				'*'{-l,--layout}"[Set or cycle the layout of the selected desktop]:desktop layout:($cycle_dir monocle tiled)"\
				'*'{-m,--to-monitor}'[Send the selected desktop to the given monitor]: :_bspc_selector -- monitor'\
				'*'{-n,--rename}'[Rename the selected desktop]:desktop name:( )'\
				'*'{-r,--remove}'[Remove the selected desktop]'\
				'*'{-s,--swap}'[Swap the selected desktop with the given desktop]: :_bspc_selector -- desktop'
			;;
		(monitor)
			((CURRENT==2)) && _bspc_selector monitor
			((CURRENT>2)) && [[ "$words[2]" != '-'* ]] && compset -n 2
			_arguments \
				'*'{-a,--add-desktops}'[Create desktops with the given names in the selected monitor]:*:add desktops:( )'\
				'*'{-d,--reset-desktops}'[Rename, add or remove desktops]:*: :->desktops'\
				'*'{-f,--focus}'[Focus the selected or given monitor]:: :_bspc_selector -- monitor'\
				'*'{-g,--rectangle}'[Set the rectangle of the selected monitor]:WxH+X+Y:( )'\
				'*'{-n,--rename}'[Rename the selected monitor]: :( )'\
				'*'{-o,--reorder-desktops}'[Reorder the desktops of the selected monitor]:*:reorder desktops:_bspc_query_names -- desktops -D'\
				'*'{-r,--remove}'[Remove the selected monitor]'\
				'*'{-s,--swap}'[Swap the selected monitor with the given monitor]: :_bspc_selector -- monitor'
			;;
		(query)
			local -a cmds_no_names=('-T' '--tree' '-N' '--nodes')
			local -a cmds=($cmds_no_names '-D' '--desktops' '-M' '--monitors')
			_arguments \
				'*'{-d,--desktop}'[Constrain matches to the selected desktop]: :_bspc_selector -- desktop'\
				'*'{-m,--monitor}'[Constrain matches to the selected monitor]: :_bspc_selector -- monitor'\
				'*'{-n,--node}'[Constrain matches to the selected node]: :_bspc_selector -- node'\
				"($cmds_no_names --names)--names[Print names instead of IDs. Can only be used with -M and -D]"\
				"($cmds --names)"{-N,--nodes}'[List the IDs of the matching nodes]'\
				"($cmds --names)"{-T,--tree}'[Print a JSON representation of the matching item]'\
				"($cmds)"{-D,--desktops}'[List the IDs (or names) of the matching desktops]'\
				"($cmds)"{-M,--monitors}'[List the IDs (or names) of the matching monitors]'
			;;
		(wm)
			_arguments \
				'*'{-d,--dump-state}'[Dump the current world state on standard output]'\
				'*'{-l,--load-state}'[Load a world state from the given file]:load state from file:_files'\
				'*'{-a,--add-monitor}'[Add a monitor for the given name and rectangle]:add monitor:( )'\
				'*'{-O,--reorder-monitors}'[Reorder the list of monitors to match the given order]:*: :_bspc_query_names -- monitors -M'\
				'*'{-o,--adopt-orphans}'[Manage all the unmanaged windows remaining from a previous session]'\
				'*'{-h,--record-history}'[Enable or disable the recording of node focus history]:history:(on off)'\
				'*'{-g,--get-status}'[Print the current status information]'
			;;
		(subscribe)
			if [[ "$words[CURRENT-1]" != (-c|--count) ]] ;then
				_values -w "options" \
					'(-f --fifo)'{-f,--fifo}'[Print a path to a FIFO from which events can be read and return]'\
					'(-c --count)'{-c,--count}'[Stop the corresponding bspc process after having received specified count of events]'
				_values -w -S "_" events all report pointer_action \
					"monitor:: :(add rename remove swap focus geometry)"\
					"desktop:: :(add rename remove swap transfer focus activate layout)"\
					"node:: :(add remove swap transfer focus activate presel stack geometry state flag layer)"
			fi
			;;
		(rule)
			local -a completions by_index
			local index=0 target settings class id instance json
			_arguments -C \
				{-a,--add}'[Create a new rule]:*: :->add'\
				{-r,--remove}'[Remove the given rules]:*: :->remove'\
				'(-l --list)'{-l,--list}'[List the rules]'
			compset -N "-([ar]|-add|-remove)"
			case $state$CURRENT in
				(add1)
					compset -P '*:'
					bspc query -N -n '.window' 2> /dev/null |
						while read id ; do
							json=$(bspc query -T -n $id 2>/dev/null) || continue
							[[ "$json[1]" = '{' ]] || continue
							class=${${json##*\"className\":\"}%%\",\"*}
							instance=${${json##*\"instanceName\":\"}%%\",\"*}
							[[ "$class[1]" != '{' && "$instance[1]" != '{'  ]] || continue
							if [ -n "$IPREFIX" ] ;then
								[[ "$IPREFIX" == ("${class}:"|('\'|)'*:') ]] &&
									completions[(r)$instance]=$instance
							else
								class=${class%%:/\\:}
								completions[(r)$class]="$class"
							fi
						done
					;;
				(add*)
					_values -w 'add rule' {border,focus,follow,manage,center}': :(on off)'\
						'(--one-shot)-o'\
						'(-o)--one-shot'\
						'monitor: :_bspc_selector -- monitor'\
						'desktop: :_bspc_selector -- desktop'\
						'node: :_bspc_selector -- node'\
						'rectangle: :( )'\
						'split_ratio:split ratio:( )'\
						"split_dir:split direction:(${dir})"\
						"state:state:(${node_state})"\
						"${flag[@]:#urgent}:set flag:(on off)"\
						"layer:layer:(${layer})"
					return
					;;
				(remove*)
					compset -P '*:'
					bspc rule -l 2> /dev/null |
						while IFS=" " read target settings ;do
							by_index+="^$((index++)):${target} ${settings}"
							if [ -n "$IPREFIX" ] ;then
								completions+="${target#*:}"
							else
								completions+="${target%:*}"
							fi
						done
					[[ -z "$IPREFIX" ]] &&
						_describe 'remove rule by position' '(head tail)' -J by_index -- by_index -J by_index
					;;
				(*)
					return
					;;
			esac
			completions[(r)\*]=*
			[ -n "$IPREFIX" ] &&
				_describe 'match window instance' completions ||
				_describe 'match window class' completions -q -S ':'
			;;
		(config)
			local -a look behaviour input
			# Boolean settings are identified by index!
			look=(borderless_monocle gapless_monocle {normal,active,focused}_border_color {top,right,bottom,left}_padding presel_feedback_color border_width window_gap)
			behaviour=(single_monocle ignore_ewmh_focus center_pseudo_tiled honor_size_hints remove_disabled_monitors remove_unplugged_monitors merge_overlapping_monitors status_prefix external_rules_command split_ratio initial_polarity directional_focus_tightness status_prefix)
			input=({swallow_first_click,focus_follows_pointer,pointer_follows_{focus,monitor}} click_to_focus pointer_motion_interval pointer_modifier pointer_action{1,2,3} )
			if [[ "$CURRENT" == (2|3) ]];then
				_arguments \
					'-d[Set settings for the selected desktop]: :_bspc_selector -- desktop'\
					'-m[Set settings for the selected monitor]: :_bspc_selector -- monitor'\
					'-n[Set settings for the selected node]: :_bspc_selector -- node'
			fi
			if [[ "${words[2]}" == -* ]] ;then
				(( CURRENT == 3 )) && return
				if (( CURRENT > 3 )) ;then
					((CURRENT-=$#words))
					words=('config' "${(@)words[4,-1]}")
					((CURRENT+=$#words))
				fi
			fi
			if ((CURRENT==2)) ;then
				_describe 'look' look -J look
				_describe 'input' input -J input
				_describe 'behaviour' behaviour -J behaviour
			elif ((CURRENT==3)) ;then
				setting=$words[2]
				case $setting in
					(initial_polarity)
						_values "set $setting" first_child second_child
						;;
					(pointer_action(1|2|3))
						_values "set $setting" move resize_side resize_corner focus none
						;;
					(pointer_modifier)
						_values "set $setting" shift control lock mod1 mod2 mod3 mod4 mod5
						;;
					(directional_focus_tightness)
						_values "set $setting" low high
						;;
					(click_to_focus)
						_values "set $setting" any button1 button2 button3 none
						;;
					(*)
						((look[(i)$setting] <= 2)) ||
							((behaviour[(i)$setting] <= 7)) ||
							((input[(i)$setting] <= 4)) &&
							_values "set $setting" true false
						;;
				esac
			fi
			;;
	esac
}

_bspc "$@"
