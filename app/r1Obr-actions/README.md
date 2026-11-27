# r1Obr-actions

Predefined actions (dances, poses) performed by R1 through
[yarpActionsPlayer](https://github.com/robotology/yarp/tree/yarp-4.0/src/commands/yarpActionsPlayer),
which ships with YARP.

| Action            | Played by                                   | Description                              |
|-------------------|---------------------------------------------|------------------------------------------|
| `hi`              | r1Obr-orchestrator (`dance hi`)             | waves the right hand                     |
| `dab`             | r1Obr-orchestrator (`dance dab`)            | dab                                      |
| `sadness`         | r1Obr-orchestrator, when the object is not found | disappointment pose                 |
| `navigation_pose` | goAndFindIt, before navigating              | arms, head and torso pose for navigation |

Only R1SN003 (R1 mk3) is configured.

## Running

Start the `R1_actionsPlayer` yarpmanager application. It runs

```
yarpActionsPlayer --filename actionsPlayer_R1SN003.ini --execute
```

with `conf/` as working directory, because yarpActionsPlayer opens the action
files relative to it. Without `--execute` the player only simulates the actions.

The modules play actions through `ActionsPlayerClient`
(`interfaces/yarpActionsPlayerRPC`), which connects to `/yarpActionsPlayer/rpc`
when needed. Actions can also be played by hand:

```
yarp rpc /yarpActionsPlayer/rpc
>> choose_action hi
>> set_initial_move_time 3.0
>> play_action hi
```

## Files

| File | Content |
|------|---------|
| `conf/actionsPlayer_R1SN003.ini` | robot parts, remappers (joints selected by name) and actions |
| `conf/keyframes/<action>.txt`    | **source** of each action: edit these                         |
| `conf/actions/<action>.txt`      | generated trajectories loaded by yarpActionsPlayer            |
| `conf/actions_timing.ini`        | generated initial move time of each action                    |
| `tools/generate_actions.py`      | generator                                                     |

## Editing or adding an action

1. Edit (or create) `conf/keyframes/<action>.txt`:
   ```
   joints <joint names, same order as the remapper of the action>
   move   <duration [s]> <one value per joint>
   pause  <duration [s]>
   ```
   The first `move` is the start pose: the player reaches it in position mode,
   in the given time. Every following `move` is interpolated with a
   minimum-jerk profile and played in position direct mode; `pause` holds the pose.
2. For a new action, add it to the `[ACTIONS]` group of `conf/actionsPlayer_R1SN003.ini`.
3. Regenerate the actions and the timing file:
   ```
   python3 tools/generate_actions.py
   ```
   The generator checks the joint names of each keyframe file against the remapper of the action.
4. Commit the keyframes together with the generated files.
