/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
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

#ifndef GET_READY_TO_NAV_H
#define GET_READY_TO_NAV_H

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RpcClient.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <vector>

class GetReadyToNav
{
private:
    //Polydriver
    yarp::dev::PolyDriver           m_drivers[4];
    yarp::dev::IControlMode*        m_ictrlmode[4];     //to set the Position control mode
    yarp::dev::IPositionControl*    m_iposctrl[4];      //to retrieve the number of joints of each part
    yarp::dev::IEncoders*           m_iencoder[4];      //to retrieve joint position

    std::string                     m_set_nav_position_file;
    yarp::os::Bottle                m_right_arm_pos;
    yarp::os::Bottle                m_left_arm_pos;
    yarp::os::Bottle                m_head_pos;
    yarp::os::Bottle                m_torso_pos;

    double                          m_time;

public:
    //Constructor/Distructor
    GetReadyToNav();
    ~GetReadyToNav() = default;

    //Internal methods
    bool configure(yarp::os::ResourceFinder &rf);
    bool navPosition();
    bool setPosCtrlMode(const int part);
    bool setJointsSpeed(const int part);
    bool movePart(const int part);
    bool areJointsOk();

    void close();
};

#endif //GET_READY_TO_NAV_H
