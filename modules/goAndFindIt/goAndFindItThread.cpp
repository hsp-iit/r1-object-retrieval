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

#include "goAndFindItThread.h"


YARP_LOG_COMPONENT(GO_AND_FIND_IT_THREAD, "r1_obr.goAndFindIt.goAndFindItThread")

/****************************************************************/
GoAndFindItThread::GoAndFindItThread(yarp::os::ResourceFinder &rf) :
    TypedReaderCallback(),
    m_rf(rf),
    m_what(""),
    m_where(""),
    m_where_specified(false),
    m_nowhere_else(false),
    m_status(GaFI_IDLE),
    m_in_nav_position(false)
{
    m_nextLoc_rpc_port_name = "/goAndFindIt/nextLocPlanner:rpc";
    m_lookObject_port_name  = "/goAndFindIt/lookForObject/object:o";
    m_objectFound_port_name = "/goAndFindIt/lookForObject/result:i";
    m_output_port_name      = "/goAndFindIt/output:o";
    m_max_nav_time          = 300.0;
    m_max_search_time       = 120.0;
    m_setNavPos_time        = 3.0;
}

/****************************************************************/
bool GoAndFindItThread::threadInit()
{
    // --------- Generic config --------- //
    if(m_rf.check("max_nav_time"))
        m_max_nav_time = m_rf.find("max_nav_time").asFloat32();
    if(m_rf.check("max_search_time"))
        m_max_search_time = m_rf.find("max_search_time").asFloat32();
    if(m_rf.check("set_nav_pos_time"))
        m_setNavPos_time = m_rf.find("set_nav_pos_time").asFloat32();

    //Open ports
    if(m_rf.check("nextLoc_rpc_port"))
        m_nextLoc_rpc_port_name = m_rf.find("nextLoc_rpc_port").asString();
    if(!m_nextLoc_rpc_port.open(m_nextLoc_rpc_port_name))
        yCError(GO_AND_FIND_IT_THREAD) << "Cannot open port" << m_nextLoc_rpc_port_name;
    else
        yCInfo(GO_AND_FIND_IT_THREAD) << "opened port" << m_nextLoc_rpc_port_name;


    if(m_rf.check("lookObject_port"))
        m_lookObject_port_name = m_rf.find("lookObject_port").asString();
    if(!m_lookObject_port.open(m_lookObject_port_name))
        yCError(GO_AND_FIND_IT_THREAD) << "Cannot open port" << m_lookObject_port_name;
    else
        yCInfo(GO_AND_FIND_IT_THREAD) << "opened port" << m_lookObject_port_name;


    if(m_rf.check("objectFound_port"))
        m_objectFound_port_name = m_rf.find("objectFound_port").asString();
    if(!m_objectFound_port.open(m_objectFound_port_name))
        yCError(GO_AND_FIND_IT_THREAD) << "Cannot open port" << m_objectFound_port_name;
    else
        yCInfo(GO_AND_FIND_IT_THREAD) << "opened port" << m_objectFound_port_name;

    m_objectFound_port.useCallback(*this);


    if(m_rf.check("output_port"))
        m_output_port_name = m_rf.find("output_port").asString();
    if(!m_output_port.open(m_output_port_name))
        yCError(GO_AND_FIND_IT_THREAD) << "Cannot open port" << m_output_port_name;
    else
        yCInfo(GO_AND_FIND_IT_THREAD) << "opened port" << m_output_port_name;

    //Navigation2DClient config
    yarp::os::Property nav2DProp;
    //Defaults
    nav2DProp.put("device", "navigation2D_nwc_yarp");
    nav2DProp.put("local", "/goAndFindIt/navClient");
    nav2DProp.put("navigation_server", "/navigation2D_nws_yarp");
    nav2DProp.put("map_locations_server", "/map2D_nws_yarp");
    nav2DProp.put("localization_server", "/localization2D_nws_yarp");
    if(!m_rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(GO_AND_FIND_IT_THREAD,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
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
        yCWarning(GO_AND_FIND_IT_THREAD,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(GO_AND_FIND_IT_THREAD,"Error opening INavigation2D interface. Device not available");
        return false;
    }

    // --------- getReadyToNav config --------- //
    m_getReadyToNav = new GetReadyToNav();
    bool ok{m_getReadyToNav->configure(m_rf)};
    if(!ok)
    {
        yCError(GO_AND_FIND_IT_THREAD,"GetReadyToNav configuration failed");
        return false;
    }

    return true;
}

/****************************************************************/
void GoAndFindItThread::threadRelease()
{

    if (m_nextLoc_rpc_port.asPort().isOpen())
        m_nextLoc_rpc_port.close();

    if (!m_lookObject_port.isClosed())
        m_lookObject_port.close();

    if (!m_objectFound_port.isClosed())
        m_objectFound_port.close();

    if (!m_output_port.isClosed())
        m_output_port.close();

    if(m_nav2DPoly.isValid())
        m_nav2DPoly.close();

    m_getReadyToNav->close();

    yCInfo(GO_AND_FIND_IT_THREAD, "Thread released");

    return;
}

/****************************************************************/
void GoAndFindItThread::run()
{

    while (true)
    {

        if (m_status == GaFI_NEW_SEARCH)
        {
            lock_guard<mutex> lock(m_mutex);
            if(m_what != "")
                nextWhere();
            else
            {
                yCError(GO_AND_FIND_IT_THREAD,"Invalid object to search");
                m_status = GaFI_IDLE;
            }
        }

        else if (m_status == GaFI_NAVIGATING)
        {
            lock_guard<mutex> lock(m_mutex);
            if(m_what != "")
            {
                if(!goThere())
                {
                    m_status = GaFI_IDLE;
                    yCError(GO_AND_FIND_IT_THREAD,"Error trying to navigate to %s. Look at previous errors to find the cause",m_where.c_str());
                    continue;
                }
            }
            else
            {
                yCError(GO_AND_FIND_IT_THREAD,"Invalid object to search");
                m_status = GaFI_IDLE;
            }
        }

        else if (m_status == GaFI_ARRIVED)
        {
            lock_guard<mutex> lock(m_mutex);
            search();
        }

        else if (m_status == GaFI_SEARCHING && Time::now() - m_searching_time > m_max_search_time) //two minutes to find "m_what"
        {
            yCError(GO_AND_FIND_IT_THREAD,"Too much time has passed waiting for '%s' to be found.",m_what.c_str());
            m_status = GaFI_OBJECT_NOT_FOUND;
        }

        else if (m_status == GaFI_OBJECT_FOUND)
        {
            lock_guard<mutex> lock(m_mutex);
            objFound();
        }

        else if (m_status == GaFI_OBJECT_NOT_FOUND)
        {
            lock_guard<mutex> lock(m_mutex);
            objNotFound();
        }

        else if (m_status == GaFI_IDLE)
        {
            Time::delay(0.8);
        }

        else if (m_status == GaFI_STOP)
        {
            break;
        }

        Time::delay(0.2);

    }

}

/****************************************************************/
void GoAndFindItThread::setWhat(string& what)
{

    if (what == m_what && m_status != GaFI_IDLE && m_where_specified == false)
    {
        yCWarning(GO_AND_FIND_IT_THREAD, "Already looking for %s. If you want to perform a new search, please send reset command.", m_what.c_str());
    }
    else
    {
        if (m_status == GaFI_NAVIGATING)
        {
            resetSearch();
            Time::delay(1.0);
        }
        else
            resetSearch();

        m_where_specified = false;
        m_where = "";

        m_status = GaFI_NEW_SEARCH;
        m_what = what;

        Time::delay(0.1);
        Bottle&  l = m_lookObject_port.prepare();
        l.clear();
        l.addString("label"); l.addString(what);
        m_lookObject_port.write();
    }
}

/****************************************************************/
void GoAndFindItThread::setWhatWhere(string& what, string& where)
{
    if (what == m_what && where == m_where && m_status != GaFI_IDLE && m_where_specified == true)
    {
        yCWarning(GO_AND_FIND_IT_THREAD, "Already looking for %s in location %s. If you want to perform a new search, please send reset command.", m_what.c_str(), m_where.c_str());
    }
    else
    {
        if (m_status == GaFI_NAVIGATING)
        {
            resetSearch();
            Time::delay(1.0);
        }
        else
            resetSearch();

        m_where_specified = true;

        m_status = GaFI_NAVIGATING;
        m_where = where;
        m_what = what;

        Time::delay(0.1);
        Bottle&  l = m_lookObject_port.prepare();
        l.clear();
        l.addString("label"); l.addString(what);
        m_lookObject_port.write();

        Bottle request,_rep_;
        request.fromString("set " + m_where + " checking");
        m_nextLoc_rpc_port.write(request,_rep_);
    }
}

/****************************************************************/
void GoAndFindItThread::nextWhere()
{
    Bottle request,reply;
    request.addString("next");
    if(m_nextLoc_rpc_port.write(request,reply))
    {
        string loc = reply.get(0).asString();
        if (loc != "noLocation")
        {
            m_where = loc;
            m_status = GaFI_NAVIGATING;
        }
        else
        {
            yCWarning(GO_AND_FIND_IT_THREAD,"Nowhere else to go");
            m_nowhere_else = true;
            m_status = GaFI_OBJECT_NOT_FOUND;
        }
    }
    else
    {
        yCError(GO_AND_FIND_IT_THREAD,"Error trying to contact nextLocPlanner");
        m_status = GaFI_IDLE;
    }
}


/****************************************************************/
bool GoAndFindItThread::setNavigationPosition()
{
    if(m_getReadyToNav->areJointsOk())
    {
        if(!m_getReadyToNav->navPosition())
            return false;

        Time::delay(m_setNavPos_time);
        m_in_nav_position = true;
        return true;
    }
    else
    {
        yCError(GO_AND_FIND_IT_THREAD,"Cannot set robot navigation position. Robot's joints present some issues");
        m_status = GaFI_IDLE;
        return false;
    }
}

/****************************************************************/
bool GoAndFindItThread::goThere()
{
    //check if "m_where" is a valid location
    Bottle request,reply;
    request.fromString("find " + m_where);
    if(m_nextLoc_rpc_port.write(request,reply))
    {
        if (reply.get(0).asString() == "ok" && reply.get(1).asString() == "checked")
        {
            yCWarning(GO_AND_FIND_IT_THREAD,"Location has already been checked");
            m_status = GaFI_NEW_SEARCH;
            return false;
        }
        else if (reply.get(0).asString() != "ok")
        {
            yCError(GO_AND_FIND_IT_THREAD,"Location specified is not valid. Terminating search.");
            m_status = GaFI_IDLE;
            return false;
        }
    }

    //setting navigation position, if needed
    if(!m_in_nav_position)
    {
        if(!setNavigationPosition())
            return false;
    }

    if (m_status != GaFI_NAVIGATING)
    {
        yCError(GO_AND_FIND_IT_THREAD) << "The system is not in the correct status (should be GaFI_NAVIGATING instead of" << m_status <<"). Cannot navigate to " << m_where;
        return false; //possible external stop during setNavigationPosition
    }

    //navigating to "m_where"
    m_iNav2D->gotoTargetByLocationName(m_where);
    yCInfo(GO_AND_FIND_IT_THREAD, "Going to location %s", m_where.c_str());

    Nav2D::NavigationStatusEnum currentStatus;
    m_iNav2D->getNavigationStatus(currentStatus);
    double toomuchtime = Time::now() + m_max_nav_time; //five minutes to reach "m_where"

    while (currentStatus != Nav2D::navigation_status_goal_reached)
    {
        if (currentStatus == Nav2D::navigation_status_aborted || m_status != GaFI_NAVIGATING)
        {
            yCWarning(GO_AND_FIND_IT_THREAD,"Navigation has been interrupted. Location not reached.");
            m_status = GaFI_IDLE;
            return false;
        }

        if (Time::now() > toomuchtime)
        {
            yCError(GO_AND_FIND_IT_THREAD,"Too much time has passed to navigate to %s.",m_where.c_str());
            stopSearch();
        }
        Time::delay(0.2);
        m_iNav2D->getNavigationStatus(currentStatus);

    }
    m_status = GaFI_ARRIVED;
    yCInfo(GO_AND_FIND_IT_THREAD, "Arrived at location %s." , m_where.c_str() ) ;

    return true;
}

/****************************************************************/
bool GoAndFindItThread::search()
{
    //looking for "m_what"
    Bottle&  ask = m_lookObject_port.prepare();
    ask.clear();
    ask.addString(m_what);
    m_lookObject_port.write();

    yCInfo(GO_AND_FIND_IT_THREAD, "Started looking for %s at location %s", m_what.c_str(), m_where.c_str());

    m_searching_time = Time::now();
    m_status = GaFI_SEARCHING;

    return true;
}

/****************************************************************/
void GoAndFindItThread::onRead(Bottle& b)
{
    yCInfo(GO_AND_FIND_IT_THREAD,"Search at location %s finished",m_where.c_str());

    string result = b.get(0).asString();
    if (m_status == GaFI_SEARCHING)
    {
        if (result == "object not found")
            m_status = GaFI_OBJECT_NOT_FOUND;
        else
        {
            m_coords = b.get(1).asList();
            m_status = GaFI_OBJECT_FOUND;
        }
    }
}

/****************************************************************/
bool GoAndFindItThread::objFound()
{
    yCInfo(GO_AND_FIND_IT_THREAD,"%s found at %s!", m_what.c_str(), m_where.c_str());

    Bottle&  toSend = m_output_port.prepare();     //Search successfull
    toSend.clear();
    toSend.addString(m_what);
    Bottle&  coords = toSend.addList();
    coords = *m_coords;
    m_output_port.write();
    m_status = GaFI_IDLE;


    Bottle request,_rep_;
    request.fromString("set " + m_where + " checked");
    m_nextLoc_rpc_port.write(request,_rep_);

    m_in_nav_position = false;

    return true;
}

/****************************************************************/
bool GoAndFindItThread::objNotFound()
{
    if(m_where_specified || m_nowhere_else)
    {
        if (m_nowhere_else)
            yCInfo(GO_AND_FIND_IT_THREAD,"%s not found", m_what.c_str());
        else
            yCInfo(GO_AND_FIND_IT_THREAD,"%s not found at %s", m_what.c_str(), m_where.c_str());

        Bottle&  toSend = m_output_port.prepare();
        toSend.clear();
        toSend.addString("not found");      //Search failed
        m_output_port.write();

        m_status = GaFI_IDLE;
    }
    else
    {
        yCInfo(GO_AND_FIND_IT_THREAD,"%s not found at %s. Continuing search", m_what.c_str(), m_where.c_str());

        m_status = GaFI_NEW_SEARCH;

    }

    Bottle request,_rep_;
    request.fromString("set " + m_where + " checked");
    m_nextLoc_rpc_port.write(request,_rep_);

    m_in_nav_position = false;

    return true;
}

/****************************************************************/
bool GoAndFindItThread::stopSearch()
{
    if (m_status == GaFI_NAVIGATING)
    {
        Nav2D::NavigationStatusEnum currentStatus;
        m_iNav2D->getNavigationStatus(currentStatus);
        if (currentStatus == Nav2D::navigation_status_moving)
            m_iNav2D->stopNavigation();
        yCInfo(GO_AND_FIND_IT_THREAD, "Navigation and search stopped");
    }
    else if (m_status == GaFI_SEARCHING)
    {
        Bottle&  ask = m_lookObject_port.prepare();
        ask.clear();
        ask.addString("stop");
        m_lookObject_port.write();
        yCInfo(GO_AND_FIND_IT_THREAD, "Search stopped");
    }

    m_status = GaFI_IDLE;

    return true;
}

/****************************************************************/
bool GoAndFindItThread::resumeSearch()
{
    yCInfo(GO_AND_FIND_IT_THREAD, "Resuming search");

    if (m_status != GaFI_IDLE)
    {
        yCError(GO_AND_FIND_IT_THREAD) << "Resume is only allowed when the status is GaFI_IDLE. Current status is " << m_status;
        return false;
    }

    if (m_where != "")
    {
        Bottle request,reply,btl;
        request.fromString("find " + m_where);
        m_nextLoc_rpc_port.write(request,reply);

        btl.fromString("ok checking") ;
        if (reply == btl)
        {
            request.fromString("set " + m_where + " unchecked");
            m_nextLoc_rpc_port.write(request,reply);
        }
    }

    m_status = GaFI_NEW_SEARCH;

    return true;
}

/****************************************************************/
bool GoAndFindItThread::resetSearch()
{
    stopSearch();

    m_in_nav_position = false;
    m_where_specified = false;
    m_nowhere_else = false;
    m_what = "";
    m_where = "";

    Bottle request,reply;
    request.fromString("set all unchecked");
    m_nextLoc_rpc_port.write(request,reply);

    Bottle&  ask = m_lookObject_port.prepare();
    ask.clear();
    ask.addString("detect");
    m_lookObject_port.write();

    return true;
}


/****************************************************************/
string GoAndFindItThread::getWhat()
{
    return m_what;
}

/****************************************************************/
string GoAndFindItThread::getWhere()
{
    return m_where;
}

/****************************************************************/
string GoAndFindItThread::getStatus()
{
    string str;
    switch (m_status)
    {
    case GaFI_IDLE:
        str = "idle";
        break;
    case GaFI_NEW_SEARCH:
        str = "new_search";
        break;
    case GaFI_NAVIGATING:
        str = "navigating";
        break;
    case GaFI_ARRIVED:
        str = "arrived";
        break;
    case GaFI_SEARCHING:
        str = "searching";
        break;
    case GaFI_OBJECT_FOUND:
        str = "object_found";
        break;
    case GaFI_OBJECT_NOT_FOUND:
        str = "object_not_found";
        break;
    case GaFI_STOP:
        str = "stop";
        break;
    default:
        break;
    };

    return str;
}

/****************************************************************/
void GoAndFindItThread::onStop()
{
    stopSearch();
    m_status = GaFI_STOP;
}