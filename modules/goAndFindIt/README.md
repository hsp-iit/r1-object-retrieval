# goAndFindIt

## General description
Module to search for an object with the robot R1 in a given map with determined locations.

Given as input the name of the object to search, the robot navigates to the closest location and starts looking around searching for it.
If the search is not successful, the robot continues the search at the next location until no location is left unchecked.

If a location name is specified as input too, the search is performed just at that location.

## Usage:
There are two ways to send commands to this module:

Sending an input bottle to the port `/goAndFindIt/input:i`
- `<object> <where>` starts looking for "object" at location "where"
- `<object>` starts looking for "object" in all the map starting from the closest location
- `stop`: stops the search
- `resume`: resumes the stopped search
- `reset`: stops the search without the possibility to resume it, resetting the information about object and locations.

Sending an RPC command to the port `/goAndFindIt/rpc`:
- `search <what>` starts to search for "what"
- `search <what> <where>`: starts searching for "what" at location "where"
- `status`: returns the current status of the search
- `what`: returns the object of the current search
- `where`: returns the location of the current search
- `stop`: stops search
- `resume`: resumes stopped search
- `reset`: resets search
- `help`: gets this list

![goAndFindIt scheme](https://github.com/colombraf/r1-object-retrieval/assets/45776020/a77a7501-bb21-4282-87fe-1aaceb873133)

