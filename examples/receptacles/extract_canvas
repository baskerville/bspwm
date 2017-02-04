#! /usr/bin/env python3

import sys
import json

source = open(sys.argv[1]) if len(sys.argv) > 1 else sys.stdin
state = json.load(source)

def nullify_clients(node):
    if node is None:
        return
    elif node['client'] is None:
        nullify_clients(node['firstChild'])
        nullify_clients(node['secondChild'])
    else:
        node['client'] = None

state['clientsCount'] = 0
state['focusHistory'] = []
state['stackingList'] = []

for monitor in state['monitors']:
    for desktop in monitor['desktops']:
        desktop['focusedNodeId'] = 0
        nullify_clients(desktop['root'])

print(json.dumps(state))
