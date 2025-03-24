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

#include "lookForObjectThread.h"

YARP_LOG_COMPONENT(LOOK_FOR_OBJECT_THREAD, "r1_obr.lookForObject.LookForObjectThread")


/****************************************************************/
LookForObjectThread::LookForObjectThread(yarp::os::ResourceFinder &rf):
    TypedReaderCallback(),
    m_rf(rf),
    m_ext_stop(false),
    m_status(LfO_IDLE)
{
    //Defaults
    m_outPortName = "/lookForObject/out:o";
    m_findObjectPortName = "/lookForObject/findObject:rpc";
    m_gazeTargetOutPortName = "/lookForObject/gazeControllerTarget:o";
    m_objectCoordsPortName = "/lookForObject/objectCoordinates:i";
    m_wait_for_search = 4.0;
    m_object = "";
}

/****************************************************************/
bool LookForObjectThread::threadInit()
{
    // --------- Generic config --------- //
    if (m_rf.check("out_port")) {m_outPortName = m_rf.find("out_port").asString();}
    if (m_rf.check("find_object_port_rpc")) {m_findObjectPortName = m_rf.find("find_object_port_rpc").asString();}
    if (m_rf.check("gaze_target_point_port")) {m_gazeTargetOutPortName = m_rf.find("gaze_target_point_port").asString();}
    if (m_rf.check("object_coords_port")) {m_objectCoordsPortName = m_rf.find("object_coords_port").asString();}

    // --------- Next Head Orient initialization --------- //
    m_robotOrient = new RobotOrient(m_rf);
    bool headOrientOk = m_robotOrient->configure();
    if (!headOrientOk){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Error configuring head orientation";
        return false;
    }

    if (m_rf.check("wait_for_search")) {m_wait_for_search = m_rf.find("wait_for_search").asFloat32();}

    // --------- Navigation2DClient config --------- //
    yarp::os::Property nav2DProp;
    //Defaults
    nav2DProp.put("device", "navigation2D_nwc_yarp");
    nav2DProp.put("local", "/lookForObject/navClient");
    nav2DProp.put("navigation_server", "/navigation2D_nws_yarp");
    nav2DProp.put("map_locations_server", "/map2D_nws_yarp");
    nav2DProp.put("localization_server", "/localization2D_nws_yarp");
    if(!m_rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(LOOK_FOR_OBJECT_THREAD,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
    }
    else
    {
        yarp::os::Searchable& nav_config = m_rf.findGroup("NAVIGATION_CLIENT");
        if(nav_config.check("device")) {nav2DProp.put("device", nav_config.find("device").asString());}
        if(nav_config.check("local")) {nav2DProp.put("local", nav_config.find("local").asString());}
        if(nav_config.check("navigation_server")) {nav2DProp.put("navigation_server", nav_config.find("navigation_server").asString());}
        if(nav_config.check("map_locations_server")) {nav2DProp.put("map_locations_server", nav_config.find("map_locations_server").asString());}
        if(nav_config.check("localization_server")) {nav2DProp.put("localization_server", nav_config.find("localization_server").asString());}
        }

    m_nav2DPoly.open(nav2DProp);
    if(!m_nav2DPoly.isValid())
    {
        yCWarning(LOOK_FOR_OBJECT_THREAD,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(LOOK_FOR_OBJECT_THREAD,"Error opening INavigation2D interface. Device not available");
        return false;
    }

    // --------- open ports --------- //
    if(!m_outPort.open(m_outPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open Out port with name" << m_outPortName;
        return false;
    }

    if(!m_findObjectPort.open(m_findObjectPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open findObject RPC port with name" << m_findObjectPortName;
        return false;
    }

    if(!m_gazeTargetOutPort.open(m_gazeTargetOutPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open port with name" << m_gazeTargetOutPortName;
        return false;
    }

    if(!m_objectCoordsPort.open(m_objectCoordsPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open port with name" << m_objectCoordsPortName;
        return false;
    }


    return true;
}

/****************************************************************/
void LookForObjectThread::threadRelease()
{
    if(!m_outPort.isClosed())
        m_outPort.close();

    if (m_findObjectPort.asPort().isOpen())
        m_findObjectPort.close();

    if(!m_gazeTargetOutPort.isClosed()){
        m_gazeTargetOutPort.close();
    }

    if(!m_objectCoordsPort.isClosed()){
        m_objectCoordsPort.close();
    }

    m_robotOrient->close();

    yCInfo(LOOK_FOR_OBJECT_THREAD, "Thread released");

    return;
}

/****************************************************************/
void LookForObjectThread::run()
{
    while (true)
    {

        if (m_status == LfO_SEARCHING)
        {
            lookAround(m_object);
        }

        else if (m_status == LfO_TURNING)
        {
            turn();
        }

        else if (m_status == LfO_OBJECT_FOUND)
        {
            writeResult(true);

        }
        else if (m_status == LfO_OBJECT_NOT_FOUND)
        {
            writeResult(false);
        }

        else if (m_status == LfO_IDLE)
        {
            Time::delay(0.8);
        }

        else if (m_status == LfO_STOP)
        {
            break;
        }

        Time::delay(0.2);

    }
}

/****************************************************************/
void LookForObjectThread::onRead(yarp::os::Bottle &b)
{
    yCInfo(LOOK_FOR_OBJECT_THREAD,"Received: %s",b.toString().c_str());

    if(b.size() == 0)
    {
        yCError(LOOK_FOR_OBJECT_THREAD,"The input bottle is empty");
    }
    else
    {
        std::string obj {b.get(0).asString()};

        if(b.size() > 1)
        {
            if (obj=="label")
            {
                m_findObjectPort.write(b);
                return;
            }
            else
                yCWarning(LOOK_FOR_OBJECT_THREAD,"The input bottle has more than one elements. Using only the first one.");
        }

        if (obj=="stop")
        {
            externalStop();
        }
        else if (obj=="detect")
        {
            m_findObjectPort.write(b);
        }
        else
        {
            if (m_status == LfO_SEARCHING)
            {
                m_ext_stop = true;
                yarp::os::Time::delay(m_wait_for_search + 0.25);
            }
            m_ext_stop = false;
            m_robotOrient->resetTurns();
            m_object = obj;
            m_status = LfO_SEARCHING;
        }

    }
}

/****************************************************************/
void LookForObjectThread::onStop()
{
    externalStop();
    m_status =LfO_STOP;
}


/****************************************************************/
bool LookForObjectThread::lookAround(std::string& ob)
{
    m_robotOrient->resetOrients();

    bool objectFound {false};
    int idx {1};
    while (!m_ext_stop)
    {
        //retrieve next head orientation
        Bottle replyOrient;
        if (m_robotOrient->next(replyOrient))
        {
            yCInfo(LOOK_FOR_OBJECT_THREAD) << "Checking head orientation: pos" + (std::string)(idx<10?"0":"") + std::to_string(idx);
            //gaze target output
            yarp::os::Bottle&  toSend1 = m_gazeTargetOutPort.prepare();
            toSend1.clear();
            yarp::os::Bottle& targetTypeList = toSend1.addList();
            targetTypeList.addString("target-type");
            targetTypeList.addString("angular");
            yarp::os::Bottle& targetLocationList = toSend1.addList();
            targetLocationList.addString("target-location");
            yarp::os::Bottle& targetList1 = targetLocationList.addList();
            yarp::os::Bottle* tmpBottle = replyOrient.get(0).asList();
            targetList1.addFloat32(tmpBottle->get(0).asFloat32());
            targetList1.addFloat32(tmpBottle->get(1).asFloat32());
            m_gazeTargetOutPort.write(); //sending output command to gaze-controller

            yarp::os::Time::delay(m_wait_for_search);  //waiting for the robot tilting its head

            //search for object
            Bottle request, reply;
            request.addString("where");
            request.addString(ob);
            yCDebug(LOOK_FOR_OBJECT_THREAD, "Request to object finder: %s", request.toString().c_str());
            if (m_findObjectPort.write(request,reply))
            {
                if (reply.get(0).asString()!="not found")
                {
                    objectFound = true;
                    break;
                }
            }
            else
            {
                yCError(LOOK_FOR_OBJECT_THREAD,"Unable to communicate with findObject");
            }

            idx++;

            yarp::os::Time::delay(0.2);
        }
        else
        {
            m_robotOrient->home();
            break;
        }
    }



    if (objectFound)
        m_status = LfO_OBJECT_FOUND;
    else if (!m_ext_stop)
        m_status = LfO_TURNING;

    return true;
}


/****************************************************************/
bool LookForObjectThread::turn()
{
    yarp::os::Bottle reply;
    m_robotOrient->turn(reply);
    if (reply.get(0).asString()=="noTurn")
        m_status = LfO_OBJECT_NOT_FOUND;
    else
    {
        double theta = reply.get(0).asFloat32();
        yCInfo(LOOK_FOR_OBJECT_THREAD) << "Turning" << theta << "degrees";
        yarp::dev::Nav2D::Map2DLocation loc;
        m_iNav2D->getCurrentPosition(loc);
        loc.theta += theta; // <===
        m_iNav2D->gotoTargetByAbsoluteLocation(loc);
        yarp::dev::Nav2D::NavigationStatusEnum currentStatus;
        m_iNav2D->getNavigationStatus(currentStatus);
        while (currentStatus != yarp::dev::Nav2D::navigation_status_goal_reached  && !m_ext_stop  )
        {
            yarp::os::Time::delay(0.2);
            m_iNav2D->getNavigationStatus(currentStatus);
        }

        if (!m_ext_stop)
            m_status = LfO_SEARCHING;
    }

    return true;

}

/****************************************************************/
bool LookForObjectThread::getObjCoordinates(Bottle* btl, Bottle& out)
{
    double max_conf = 0.0;
    double x = -1.0, y;
    for (int i=0; i<btl->size(); i++)
    {
        Bottle* b = btl->get(i).asList();
        if (b->get(0).asString() != m_object) //skip objects with another label
            continue;

        if(b->get(1).asFloat32() > max_conf) //get the object with the max confidence
        {
            max_conf = b->get(1).asFloat32();
            x = b->get(2).asFloat32();
            y = b->get(3).asFloat32();
        }
    }

    if (x<0)
        return false;

    out.addFloat32(x);
    out.addFloat32(y);
    return true;
}

/****************************************************************/
bool LookForObjectThread::writeResult(bool objFound)
{
    yarp::os::Bottle&  toSendOut = m_outPort.prepare();
    toSendOut.clear();
    if (objFound)
    {
        toSendOut.addString(m_object);
        Bottle& coordList = toSendOut.addList();

        Bottle* finderResult= m_objectCoordsPort.read(false);
        if(finderResult != nullptr)
        {
            if (!getObjCoordinates(finderResult, coordList))
            {
                toSendOut.clear();
                toSendOut.addString("object not found");
                yCWarning(LOOK_FOR_OBJECT_THREAD, "The Object Finder is not seeing any %s", m_object.c_str());
            }
        }
    }
    else
        toSendOut.addString("object not found");
    m_outPort.write();

    m_object = "";
    m_status = LfO_IDLE;

    return true;
}

/****************************************************************/
void LookForObjectThread::externalStop()
{

    yCWarning(LOOK_FOR_OBJECT_THREAD, "External stop command received");
    m_ext_stop = true;
    m_status = LfO_IDLE;

    yarp::dev::Nav2D::NavigationStatusEnum currentStatus;
    m_iNav2D->getNavigationStatus(currentStatus);
    if (currentStatus == yarp::dev::Nav2D::navigation_status_moving)
    {
        m_iNav2D->stopNavigation();
        yCInfo(LOOK_FOR_OBJECT_THREAD, "Navigation stopped");
    }

}
