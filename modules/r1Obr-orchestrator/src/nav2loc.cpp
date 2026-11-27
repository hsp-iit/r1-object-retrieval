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

#include "nav2loc.h"

YARP_LOG_COMPONENT(NAV_2_LOC, "r1_obr.orchestrator.nav2loc")

bool Nav2Loc::configure(yarp::os::ResourceFinder &rf)
{

    // --------- Navigation2DClient config --------- //
   Property nav2DProp;
    if(!rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(NAV_2_LOC,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
    }
    Searchable& nav_config = rf.findGroup("NAVIGATION_CLIENT");
    nav2DProp.put("device", nav_config.check("device", Value("navigation2D_nwc_yarp")));
    nav2DProp.put("local", nav_config.check("local", Value("/r1Obr-orchestrator/nav2loc/")));
    nav2DProp.put("navigation_server", nav_config.check("navigation_server", Value("/navigation2D_nws_yarp")));
    nav2DProp.put("map_locations_server", nav_config.check("map_locations_server", Value("/map2D_nws_yarp")));
    nav2DProp.put("localization_server", nav_config.check("localization_server", Value("/localization2D_nws_yarp")));

    m_nav2DPoly.open(nav2DProp);
    if(!m_nav2DPoly.isValid())
    {
        yCWarning(NAV_2_LOC,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(NAV_2_LOC,"Error opening INavigation2D interface. Device not available");
        return false;
    }

    // --------- Home coordinates config --------- //
    Vector home_position(3, 0.0);

    if(!rf.check("NAV2LOC"))
    {
        yCWarning(NAV_2_LOC,"NAV2LOC section missing in ini file. Using the default values");
    }
    Searchable& home_config = rf.findGroup("NAV2LOC");
    if(home_config.check("home"))
    {
        Bottle home;
        home.fromString(home_config.find("home").asString());
        home_position[0]=home.get(0).asFloat32();
        home_position[1]=home.get(1).asFloat32();
        home_position[2]=home.get(2).asFloat32();
    }
    if(home_config.check("near_distance")) {m_near_distance = home_config.find("near_distance").asFloat32();}

    MapGrid2D  map;
    if(ReturnValue ret = m_iNav2D->getCurrentNavigationMap(NavigationMapTypeEnum::global_map, map); !ret)
    {
        yCError(NAV_2_LOC) << "Error retrieving current global map:" << ret.toString();
        return false;
    }

    m_home_location = Map2DLocation(map.getMapName(), home_position[0], home_position[1], home_position[2]);

    return true;
}


void Nav2Loc::close()
{
    if(m_nav2DPoly.isValid())
        m_nav2DPoly.close();
}


bool Nav2Loc::goHome()
{

    if(ReturnValue ret = m_iNav2D->gotoTargetByAbsoluteLocation(m_home_location); !ret)
    {
        yCError(NAV_2_LOC) << "Error with navigation to home location:" << ret.toString();
        return false;
    }

    m_current_target_location = "home";

    return true;
}


bool Nav2Loc::go(string loc)
{
    if (loc == "home")
        return goHome();

    // This piece of code will work with a specific locations.ini configuration
    // Forcing a specific path following if we want to go to the laboratory from outside
    if(loc == "laboratory")
    {
        bool is_inside = false;
        if(ReturnValue ret = m_iNav2D->checkInsideArea("laboratory_area", is_inside); !ret)
        {
            yCError(NAV_2_LOC) << "Error checking if robot is inside lab area:" << ret.toString();
            return false;
        }
        if(!is_inside)
        {
            if(ReturnValue ret = m_iNav2D->gotoTargetByLocationName("laboratory_path"); !ret)
            {
                yCError(NAV_2_LOC) << "Error with navigation to lab area from outside:" << ret.toString();
                return false;
            }
        }
        // If we are inside the lab area, we swap to the default behaviour
    }

    if(ReturnValue ret = m_iNav2D->gotoTargetByLocationName(loc); !ret)
    {
        yCError(NAV_2_LOC) << "Error with navigation to location" << loc << ":" << ret.toString();
        return false;
    }

    m_current_target_location = loc;

    return true;
}

bool Nav2Loc::stop()
{
    yCInfo(NAV_2_LOC, "Navigation stopped");

    if(ReturnValue ret = m_iNav2D->stopNavigation(); !ret)
    {
        yCError(NAV_2_LOC) << "Error stopping navigation:" << ret.toString();
        return false;
    }

    return true;
}

bool Nav2Loc::resumeGo()
{
    if (m_current_target_location=="home")
        goHome();
    else
        go(m_current_target_location);

    return true;
}

bool Nav2Loc::areYouArrived()
{
    NavigationStatusEnum currentStatus{navigation_status_error};
    if (ReturnValue ret = m_iNav2D->getNavigationStatus(currentStatus); !ret)
        yCErrorThrottle(NAV_2_LOC, 5.0) << "Error retrieving the navigation status:" << ret.toString();

    return currentStatus == navigation_status_goal_reached;
}


bool Nav2Loc::areYouNearToGoal()
{
    Map2DLocation robot, target;
    ReturnValue retTarget = m_iNav2D->getAbsoluteLocationOfCurrentTarget(target);
    ReturnValue retRobot = m_iNav2D->getCurrentPosition(robot);
    if (!retTarget || !retRobot)
    {
        yCErrorThrottle(NAV_2_LOC, 5.0) << "Error retrieving the target or robot location. target:" << retTarget.toString() << "robot:" << retRobot.toString();
        return false;
    }

    return sqrt(pow((robot.x-target.x), 2) + pow((robot.y-target.y), 2)) < m_near_distance;
}


bool Nav2Loc::areYouMoving()
{
    NavigationStatusEnum currentStatus{navigation_status_error};
    if (ReturnValue ret = m_iNav2D->getNavigationStatus(currentStatus); !ret)
        yCErrorThrottle(NAV_2_LOC, 5.0) << "Error retrieving the navigation status:" << ret.toString();

    return currentStatus == navigation_status_moving;
}

bool Nav2Loc::isNavigationAborted()
{
    NavigationStatusEnum currentStatus{navigation_status_error};
    if (ReturnValue ret = m_iNav2D->getNavigationStatus(currentStatus); !ret)
        yCErrorThrottle(NAV_2_LOC, 5.0) << "Error retrieving the navigation status:" << ret.toString();

    return currentStatus == navigation_status_aborted;
}

string Nav2Loc::getCurrentTarget()
{
    return m_current_target_location;
}
