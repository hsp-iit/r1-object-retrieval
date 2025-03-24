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

#ifndef ROBOT_ORIENT_H
#define ROBOT_ORIENT_H

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IRGBDSensor.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/RFModule.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <map>
#include <vector>
#include <algorithm>
#include <math.h>

//Defaults RGBD sensor
#define RGBDClient            "RGBDSensorClient"
#define RGBDLocalImagePort    "/lookForObject/robotOrient/clientRgbPort:i"
#define RGBDLocalDepthPort    "/lookForObject/robotOrient/clientDepthPort:i"
#define RGBDLocalRpcPort      "/lookForObject/robotOrient/clientRpcPort"
#define RGBDRemoteImagePort   "/cer/depthCamera/rgbImage:o"
#define RGBDRemoteDepthPort   "/cer/depthCamera/depthImage:o"
#define RGBDRemoteRpcPort     "/cer/depthCamera/rpc:i"
#define RGBDImageCarrier      "mjpeg"
#define RGBDDepthCarrier      "fast_tcp"

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;

class RobotOrient
{
private:
    //Polydriver
    PolyDriver            m_Poly;
    IRemoteCalibrator*    m_iremcal;       
    IControlMode*         m_ictrlmode;     
    IPositionControl*     m_iposctrl;
    IControlLimits*       m_ilimctrl;      

    PolyDriver            m_rgbdPoly;
    IRGBDSensor*          m_iRgbd{nullptr};

    //head orientations
    map<string, pair<double,double>>         m_orientations;

    //turn around
    bool                  m_turning;
    double                m_turn_deg;
    int                   m_max_turns;
    int                   m_current_turn;
    int                   m_current_orient;

    //others
    double                m_period;
    double                m_overlap;
    mutex                 m_mutex;
    ResourceFinder&       m_rf;

public:
    //Constructor/Distructor
    RobotOrient(ResourceFinder &rf);
    ~RobotOrient(){}

    //Internal methods
    bool configure();
    virtual bool  close();

    bool next(Bottle& reply);
    bool turn(Bottle& reply);
    void resetOrients();
    void resetTurns();
    void home();
    void help();
};

#endif 
