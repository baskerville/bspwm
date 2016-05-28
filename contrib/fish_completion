function __fish_bspc_needs_command
  set cmd (commandline -opc)
  [ (count $cmd) -eq 1 -a $cmd[1] = 'bspc' ]; and return 0
  return 1
end

function __fish_bspc_using_command
  set cmd (commandline -opc)
  [ (count $cmd) -gt 1 ]; and [ $argv[1] = $cmd[2] ]; and return 0
  return 1
end

complete -f -c bspc -n '__fish_bspc_needs_command' -a 'node desktop monitor query rule restore wm subscribe config quit'
complete -f -c bspc -n '__fish_bspc_using_command config' -a 'external_rules_command status_prefix normal_border_color active_border_color focused_border_color presel_feedback_color border_width window_gap top_padding right_padding bottom_padding left_padding split_ratio initial_polarity borderless_monocle gapless_monocle single_monocle pointer_motion_interval pointer_modifier pointer_action1 pointer_action2 pointer_action3 click_to_focus focus_follows_pointer pointer_follows_focus pointer_follows_monitor ignore_ewmh_focus center_pseudo_tiled honor_size_hints remove_disabled_monitors remove_unplugged_monitors merge_overlapping_monitors'
