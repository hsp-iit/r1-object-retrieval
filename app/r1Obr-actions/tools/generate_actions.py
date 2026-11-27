#!/usr/bin/env python3
#
# Copyright (C) 2026 Istituto Italiano di Tecnologia (IIT)
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

"""Generate yarpActionsPlayer action files from keyframe files.

Keyframe file format (one per action, conf/keyframes/<action>.txt):

    # comment
    joints <joint name> <joint name> ...
    move   <duration [s]> <value> <value> ...
    pause  <duration [s]>

The first `move` is the start pose of the action: yarpActionsPlayer reaches it
with a position move whose duration (the "initial move time") cannot be stored
in the action file, so it is written to the timing file read by the clients.
Every following `move` is interpolated with a minimum-jerk profile and every
`pause` holds the current pose.

The action files are written with the fixed sampling period used by
yarpActionsPlayer (10 ms) and contain joint positions only.

The joint names of each keyframe file are checked against the remapper of the
action in the yarpActionsPlayer configuration file, so that columns cannot be
assigned to the wrong joints.
"""

import argparse
import pathlib
import re
import sys

PERIOD = 0.010  # yarpActionsPlayer loads every action file with this period


def parse_groups(path):
    """Return {group: [line tokens]} for an ini file with [GROUP] sections."""
    groups, current = {}, None
    for raw in path.read_text().splitlines():
        line = raw.split('#', 1)[0].strip()
        if not line:
            continue
        m = re.fullmatch(r'\[(\S+)\]', line)
        if m:
            current = groups.setdefault(m.group(1), [])
        elif current is not None:
            current.append(line)
    return groups


def action_joints(player_conf):
    """Return {action: [joint names]} from the yarpActionsPlayer configuration."""
    groups = parse_groups(player_conf)
    remappers = {}
    for line in groups.get('REMAPPERS', []):
        m = re.fullmatch(r'(\S+)\s+\(([^)]*)\)\s+\(([^)]*)\)', line)
        if not m:
            sys.exit(f'{player_conf}: invalid REMAPPERS line: {line}')
        remappers[m.group(1)] = m.group(3).split()
    actions = {}
    for line in groups.get('ACTIONS', []):
        tokens = line.split()
        if len(tokens) < 3 or tokens[1] not in remappers:
            sys.exit(f'{player_conf}: invalid ACTIONS line: {line}')
        actions[tokens[0]] = (remappers[tokens[1]], tokens[2])
    return actions


def min_jerk(q0, q1, n_steps):
    for i in range(1, n_steps + 1):
        t = i / n_steps
        s = 10 * t**3 - 15 * t**4 + 6 * t**5
        yield [a + (b - a) * s for a, b in zip(q0, q1)]


def steps(duration, where):
    n = round(duration / PERIOD)
    if n < 1:
        sys.exit(f'{where}: duration must be at least {PERIOD} s')
    return n


def generate(keyframes):
    """Return (initial move time, frames) for a keyframe file."""
    joints, frames, initial_time = None, [], None
    for num, raw in enumerate(keyframes.read_text().splitlines(), 1):
        tokens = raw.split('#', 1)[0].split()
        if not tokens:
            continue
        where = f'{keyframes}:{num}'
        cmd, args = tokens[0], tokens[1:]
        if cmd == 'joints':
            joints = args
        elif cmd == 'move':
            if joints is None:
                sys.exit(f'{where}: "joints" must come before the first "move"')
            values = [float(v) for v in args[1:]]
            if len(values) != len(joints):
                sys.exit(f'{where}: {len(values)} values for {len(joints)} joints')
            if not frames:
                initial_time = float(args[0])
                frames.append(values)
            else:
                frames.extend(min_jerk(frames[-1], values, steps(float(args[0]), where)))
        elif cmd == 'pause':
            if not frames:
                sys.exit(f'{where}: an action cannot start with "pause"')
            frames.extend([frames[-1]] * steps(float(args[0]), where))
        else:
            sys.exit(f'{where}: unknown command "{cmd}"')
    if not frames:
        sys.exit(f'{keyframes}: no "move" found')
    return joints, initial_time, frames


def main():
    app_dir = pathlib.Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--conf-dir', type=pathlib.Path, default=app_dir / 'conf')
    parser.add_argument('--player-conf', default='actionsPlayer_R1SN003.ini')
    parser.add_argument('--timing-file', default='actions_timing.ini')
    args = parser.parse_args()

    conf_dir = args.conf_dir
    actions = action_joints(conf_dir / args.player_conf)
    timing = []
    for name, (remapper_joints, action_file) in sorted(actions.items()):
        keyframes = conf_dir / 'keyframes' / f'{name}.txt'
        if not keyframes.exists():
            sys.exit(f'Missing keyframes for action {name}: {keyframes}')
        joints, initial_time, frames = generate(keyframes)
        if joints != remapper_joints:
            sys.exit(f'{keyframes}: joints differ from the remapper of action {name} in {args.player_conf}')
        out = conf_dir / action_file
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text('\n'.join(' '.join(f'{v:.4f}' for v in frame) for frame in frames) + '\n')
        timing.append((name, initial_time))
        print(f'{name}: {len(frames)} frames ({(len(frames) - 1) * PERIOD:.2f} s + {initial_time} s initial move) -> {out}')

    width = max(len(name) for name, _ in timing)
    lines = ['# Generated by tools/generate_actions.py: do not edit.',
             '# Initial move time [s] of each action, i.e. the duration of its first keyframe.',
             '# Read by ActionsPlayerClient (interfaces/yarpActionsPlayerRPC).']
    lines += [f'{name:<{width}}  {t}' for name, t in timing]
    (conf_dir / args.timing_file).write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
