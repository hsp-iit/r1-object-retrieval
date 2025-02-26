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

#ifndef TINY_DANCER_H
#define TINY_DANCER_H

#include <yarp/os/all.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <vector>
#include <fstream>

#ifndef PARTS_NAMES
#define PARTS_NAME 255
#define RIGHT_ARM 0
#define LEFT_ARM 1
#define HEAD 2
#define TORSO 3
#endif


using namespace yarp::os;
using namespace yarp::dev;
using namespace std;

class TinyDancer
{

private:
    PolyDriver          m_drivers[4];
    IControlMode*       m_ictrlmode[4];     //to set the Position control mode
    IPositionControl*   m_iposctrl[4];      //to retrieve the number of joints of each part
    IEncoders*          m_iencoder[4];      //to retrieve joint position

    string              m_robot;
    ResourceFinder&     m_rf;
    bool                m_parts_on[4]{true, true, true, true};

public:
    TinyDancer(ResourceFinder &_rf);
    ~TinyDancer() = default;

    bool configure();
    void close();

    bool areJointsOk();
    bool setCtrlMode(const int part, int ctrlMode);
    bool setJointsSpeed(const int part, const double time, Bottle* joint_pos);
    bool movePart(const int part, Bottle* joint_pos);
    bool doDance(string& dance_name);

};

#endif

