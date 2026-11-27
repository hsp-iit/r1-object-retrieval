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

#include "ActionsPlayerClient.h"

#include <yarp/os/LogComponent.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Network.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Value.h>

YARP_LOG_COMPONENT(ACTIONS_PLAYER_CLIENT, "r1_obr.actionsPlayerClient")


bool ActionsPlayerClient::configure(const yarp::os::Searchable& config, const std::string& defaultLocalPort)
{
    std::string localPort = config.check("local_rpc_port", yarp::os::Value(defaultLocalPort)).asString();
    m_remotePort = config.check("remote_rpc_port", yarp::os::Value(m_remotePort)).asString();
    std::string timingContext = config.check("timing_context", yarp::os::Value("r1Obr-actions")).asString();
    std::string timingFile = config.check("timing_file", yarp::os::Value("actions_timing.ini")).asString();

    yarp::os::ResourceFinder rf;
    rf.setDefaultContext(timingContext);
    rf.configure(0, nullptr);
    std::string timingPath = rf.findFileByName(timingFile);
    if (timingPath.empty() || !m_initialMoveTimes.fromConfigFile(timingPath))
    {
        yCWarning(ACTIONS_PLAYER_CLIENT) << "Cannot read" << timingFile << "from context" << timingContext
                                         << ": every action will start with a" << m_defaultInitialMoveTime << "s initial move";
    }

    if (!m_port.open(localPort))
    {
        yCError(ACTIONS_PLAYER_CLIENT) << "Cannot open port" << localPort;
        return false;
    }
    m_player.yarp().attachAsClient(m_port);

    return true;
}


bool ActionsPlayerClient::connect()
{
    if (m_port.getOutputCount() > 0)
        return true;
    return yarp::os::Network::connect(m_port.getName(), m_remotePort, "", true);
}


bool ActionsPlayerClient::play(const std::string& action)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!connect())
    {
        yCWarning(ACTIONS_PLAYER_CLIENT) << "yarpActionsPlayer not reachable at" << m_remotePort << ": cannot play" << action;
        return false;
    }

    // play_action returns true even for unknown actions: choose_action does not
    if (!m_player.choose_action(action))
    {
        yCError(ACTIONS_PLAYER_CLIENT) << "yarpActionsPlayer cannot select action" << action;
        return false;
    }

    // The initial move time is stored per remapper by the player:
    // always send it, otherwise the value set for a previous action would be used
    double initialMoveTime = m_initialMoveTimes.check(action, yarp::os::Value(m_defaultInitialMoveTime)).asFloat64();
    if (!m_player.set_initial_move_time(initialMoveTime))
        yCWarning(ACTIONS_PLAYER_CLIENT) << "Unable to set the initial move time of action" << action;

    yCInfo(ACTIONS_PLAYER_CLIENT) << "Playing action" << action;
    if (!m_player.play_action(action))
    {
        yCError(ACTIONS_PLAYER_CLIENT) << "yarpActionsPlayer failed playing action" << action;
        return false;
    }

    return true;
}


void ActionsPlayerClient::close()
{
    m_port.interrupt();
    m_port.close();
}
