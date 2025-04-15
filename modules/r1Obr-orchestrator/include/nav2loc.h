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

#ifndef NAV_2_LOC_H
#define NAV_2_LOC_H

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/INavigation2D.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/RFModule.h>
#include <cmath>

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::dev::Nav2D;
using namespace yarp::sig;
using namespace std;

class Nav2Loc
{
private:
    Map2DLocation           m_home_location;
    string                  m_current_target_location;
    double                  m_near_distance;

    // Devices
    PolyDriver              m_nav2DPoly;
    Nav2D::INavigation2D*   m_iNav2D{nullptr};

public:
    Nav2Loc() : m_near_distance(3.0) {}
    ~Nav2Loc(){}

    bool configure(yarp::os::ResourceFinder &rf);
    void close();
    bool goHome();
    bool go(string loc_name);
    bool stop();
    bool resumeGo();
    bool areYouArrived();
    bool areYouNearToGoal();
    bool areYouMoving();
    bool isNavigationAborted();
    string getCurrentTarget();
};

#endif //NAV_2_LOC_H
