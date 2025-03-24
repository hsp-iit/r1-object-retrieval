# r1Obr-orchestrator

## General description
Module to orchestrate the search for an object with the robot R1.

This has been designed as a plug-in module where the input commands can be received from various sources, and the final behaviour of the robot is defined by other modules.

While the search is performed by the goAndFindIt module, this one acts as interface with the user.
![orchestrator scheme](https://github.com/colombraf/r1-object-retrieval/assets/45776020/10a1f851-1eea-41cf-999e-471d9c42a348)

## Architecture
![orchestrator architecture](https://github.com/colombraf/r1-object-retrieval/assets/45776020/9d8566cc-ff8f-481a-8f01-e9d45f288b62)


## Usage

### Inputs
There are two ways to send commands to this module:

Sending an input bottle to the port `/r1Obr-orchestrator/input:i` (the name can be modified in the .ini file).
The commands that can be sent to these ports are:
- `search <object> <where>`: starts looking for "object" at location "where". In this case, if "object" is not found, the module will ask you if you want to continue the search in other locations. In this case you can answer `yes` or `no` using one of the input ports available.
- `search <object>`: starts looking for "object" in all the map starting from the closest location
- `stop`: stops the robot
- `resume`: resumes a stopped search
- `reset`: stops the robot without the possibility to resume it
- `reset_home`: resets the robot and the chat bot, and starts the navigation of the robot towards the home location
- `navpos`: sets the robot in a navigation position
- `go <location>` : navigates the robot to 'location' if it is valid

Sending an RPC command to the port `/r1Obr-orchestrator/rpc`:
- `search <what>` starts looking for "what" in all the map starting from the closest location
- `search <what> <where>`: starts looking for "what" at location "where". In this case, if "what" is not found, the module will ask you if you want to continue the search in other locations. In this case you can answer `yes` or `no` using one of the input ports available.
- `what`: returns the object of the current search
- `where`: returns the location of the current search
- `stop`: stops the robot
- `resume`: resumes stopped search
- `reset`: stops the robot without the possibility to resume it
- `reset_home`: resets the robot and the chat bot, and starts the navigation of the robot towards the home location
- `help`: gets this list
- `status`: returns the current status. The robot could be: idle, asking network, searching (navigating or effectively searching around), waiting for an answer, performing some kind of motion when the object is found or not found.
- `info`: get the information about what, where and status
- `navpos`: sets the robot in a navigation position
- `go <location>` : navigates the robot to 'location' if it is valid
- `say <sentence>`: the sentence is synthesized and played by the audio player
- `tell <key>`: a previously stored sentence is accessed through the corresponfing key and it is played by the audio player

A third way of sending commands to this module is via vocal commands. See the paragraph about the Chat Bot.

### Outputs
A search can have a negative or a positive outcome.
In the first case, if you have specified a location where to perform the search, you will be asked if you want to continue the search in other locations.

When a search ends without having found the object of the search, an output message is sent to the negative outcome port (default name `/r1Obr-orchestrator/negative_outcome:o`).
While if the object is found, an output message is sent to the positive outcome port (default name `/r1Obr-orchestrator/positive_outcome:o`) containing the name of the object followed by a Bottle with its pixel coordinates in the camera reference frame. 
Output format:  `<objectName> (<coordsX> <coordsY>)`
Example:        `ball (156 203)`

### Continous Search 
The continous search is an optional feature of this orchestrator. 
If it set as active, the orchestrator will check constantly, during the navigation of the robot, if the object of the search can already be found without waiting for the robot to reach a certain location.
If `<where>` has been specified in the `search` command, the continuous search will activate only in proximity of the specified location.

### Chat Bot and speech Synthesizer 
The orchestrator manages also the vocal interaction between robot and people around it. 
The trascribed text of what a person tells to the robot is read from an input port (default name is `/r1Obr-orchestrator/voice_command:i`). This text is sent to a Chat Bot device which replies to the orchestrator translating the vocal commands in RPC commands. 
These RPC commands are the same as described before.
Together with the Chat Bot device, a Speech Synthesizer device let the robot speak. The exact words that the robot says are asked to the Chat Bot.
Example:
- from voice command input port: `can you search an apple in the kitchen?`
- the orchestrator sends the above string to the Chat Bot device. Reply from Chat Bot: `(search apple kitchen) (say "Sure, I am going to the kitchen looking for an apple")`
- the command `(search apple kitchen)` is sent to the RPC input port of the orchestrator
- the sentence to say is sent to the Speech Synthesizer device


### Integration of the Sensor Network
Before starting looking around for the object, this module asks to the Sensor Network if it can find it, so that the robot can directly navigate to it.
[NOT IMPLEMENTED YET]
