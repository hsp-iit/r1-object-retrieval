/*
 * Copyright (C) 2026 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ACTIONS_PLAYER_CLIENT_H
#define ACTIONS_PLAYER_CLIENT_H

#include <yarp/os/Property.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/Searchable.h>

#include <mutex>
#include <string>

#include "yarpActionsPlayer_IDL.h"

/**
 * Plays the actions loaded by a running yarpActionsPlayer.
 *
 * The connection to the player is made on each play() call if missing, so the
 * player can be started after (or restarted independently of) the client.
 *
 * yarpActionsPlayer reaches the first frame of an action with a position move
 * lasting "initial move time" seconds, a value that it cannot read from the
 * action files. The initial move time of each action is therefore read from a
 * timing file (generated together with the actions) and sent before playing.
 *
 * Configuration parameters (all optional):
 *   local_rpc_port   name of the local RPC port (default: the one passed to configure())
 *   remote_rpc_port  RPC port of yarpActionsPlayer (default: /yarpActionsPlayer/rpc)
 *   timing_context   context of the timing file (default: r1Obr-actions)
 *   timing_file      name of the timing file (default: actions_timing.ini)
 */
class ActionsPlayerClient
{
public:
    bool configure(const yarp::os::Searchable& config, const std::string& defaultLocalPort);

    /**
     * Plays an action and waits for its end.
     * @return false if the player is not reachable or the action does not exist
     */
    bool play(const std::string& action);

    void close();

private:
    bool connect();

    yarp::os::RpcClient     m_port;
    yarpActionsPlayer_IDL   m_player;
    std::string             m_remotePort{"/yarpActionsPlayer/rpc"};
    yarp::os::Property      m_initialMoveTimes;
    double                  m_defaultInitialMoveTime{4.0};  // yarpActionsPlayer default
    std::mutex              m_mutex;
};

#endif // ACTIONS_PLAYER_CLIENT_H
