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

#ifndef LOOK_FOR_OBJECT_THREAD_H
#define LOOK_FOR_OBJECT_THREAD_H


#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/INavigation2D.h>
#include <yarp/os/all.h>
#include <math.h>
#include "robotOrient.h"


class LookForObjectThread : public yarp::os::Thread, 
                            public yarp::os::TypedReaderCallback<yarp::os::Bottle>
{
enum LfO_status 
{
    LfO_IDLE,
    LfO_SEARCHING,
    LfO_TURNING,
    LfO_OBJECT_FOUND,
    LfO_OBJECT_NOT_FOUND,
    LfO_STOP      
};

private:
    //Devices
    yarp::dev::PolyDriver            m_nav2DPoly;
    yarp::dev::Nav2D::INavigation2D* m_iNav2D{nullptr};

    //Ports
    std::string                                 m_outPortName;
    yarp::os::BufferedPort<yarp::os::Bottle>    m_outPort;
    std::string                                 m_findObjectPortName;
    yarp::os::RpcClient                         m_findObjectPort;
    std::string                                 m_objectCoordsPortName;
    yarp::os::BufferedPort<yarp::os::Bottle>    m_objectCoordsPort;
    std::string                                 m_gazeTargetOutPortName;
    yarp::os::BufferedPort<yarp::os::Bottle>    m_gazeTargetOutPort;

    //Others
    LfO_status                  m_status;
    std::string                 m_object;
    double                      m_wait_for_search;
    bool                        m_ext_stop;
    yarp::os::ResourceFinder&   m_rf;
    
    RobotOrient*             m_robotOrient;


public:
    //Contructor and distructor
    LookForObjectThread(yarp::os::ResourceFinder &rf);
    ~LookForObjectThread() = default;

    //methods inherited from PeriodicThread
    virtual void run() override;
    virtual bool threadInit() override;
    virtual void threadRelease() override;
    virtual void onStop() override;

    //Port inherited from TypedReaderCallback
    using TypedReaderCallback<yarp::os::Bottle>::onRead;
    void onRead(yarp::os::Bottle& b) override;

    bool lookAround(std::string& ob);
    bool turn();
    bool getObjCoordinates(Bottle* btl, Bottle& out);
    bool writeResult(bool objFound);
    void externalStop();

};

#endif
