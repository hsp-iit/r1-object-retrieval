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

#include <math.h>
#include "nextLocPlanner.h"

YARP_LOG_COMPONENT(NEXT_LOC_PLANNER, "r1_obr.nextLocPlanner")

NextLocPlanner::NextLocPlanner() :
    m_period(1.0),
    m_area("")
{
}

/****************************************************************/
bool NextLocPlanner::configure(ResourceFinder &rf)
{
    m_period = rf.check("period")  ? rf.find("period").asFloat32() : 1.0;
    m_area   = rf.check("area")    ? rf.find("area").asString()    : "";

    //Open RPC Server Port
    string rpcPortName = rf.check("rpcPort") ? rf.find("rpcPort").asString() : "/nextLocPlanner/request/rpc";
    if (!m_rpc_server_port.open(rpcPortName))
    {
        yCError(NEXT_LOC_PLANNER, "open() error could not open rpc port %s, check network", rpcPortName.c_str());
        return false;
    }
    if (!attach(m_rpc_server_port))
    {
        yCError(NEXT_LOC_PLANNER, "attach() error with rpc port %s", rpcPortName.c_str());
        return false;
    }

    //Navigation2DClient config
    Property nav2DProp;
        //Defaults
    nav2DProp.put("device", "navigation2D_nwc_yarp");
    nav2DProp.put("local", "/nextLocPlanner/navClient");
    nav2DProp.put("navigation_server", "/navigation2D_nws_yarp");
    nav2DProp.put("map_locations_server", "/map2D_nws_yarp");
    nav2DProp.put("localization_server", "/localization2D_nws_yarp");
    if(!rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(NEXT_LOC_PLANNER,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
    }
    else
    {
        Searchable& nav_config = rf.findGroup("NAVIGATION_CLIENT");
        if(nav_config.check("device")) {nav2DProp.put("device", nav_config.find("device").asString());}
        if(nav_config.check("local")) {nav2DProp.put("local", nav_config.find("local").asString());}
        if(nav_config.check("navigation_server")) {nav2DProp.put("navigation_server", nav_config.find("navigation_server").asString());}
        if(nav_config.check("map_locations_server")) {nav2DProp.put("map_locations_server", nav_config.find("map_locations_server").asString());}
        if(nav_config.check("localization_server")) {nav2DProp.put("localization_server", nav_config.find("localization_server").asString());}
        }

    m_nav2DPoly.open(nav2DProp);
    if(!m_nav2DPoly.isValid())
    {
        yCWarning(NEXT_LOC_PLANNER,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(NEXT_LOC_PLANNER,"Error opening INavigation2D interface. Device not available");
        return false;
    }

    MapGrid2D  map;
    if(!m_iNav2D->getCurrentNavigationMap(NavigationMapTypeEnum::global_map, map))
    {
        yCError(NEXT_LOC_PLANNER, "Error retrieving current global map");
    }
    m_map_name = map.getMapName();

    //Load all the locations in m_all_locations
    vector<string> all_locations;
    if (!m_iNav2D->getLocationsList(all_locations))
    {
        yCError(NEXT_LOC_PLANNER,"Error getting locations list from map server");
        return false;
    }


    if(!all_locations.empty())
    {
        for (string & loc_name : all_locations)
        {
            Map2DLocation loc;
            m_iNav2D->getLocation(loc_name, loc);
            if(loc.map_id == m_map_name)
                m_all_locations.push_back(loc_name);
        }
        if(m_all_locations.empty())
        {
            yCWarning(NEXT_LOC_PLANNER,"Error: no locations from map server for the area specified");
            return false;
        }

        if (m_area != "")
        {
            //remove elements not belonging to the area defined in .ini file (i.e. whose name starts with the name of the area)
            auto new_end = remove_if(m_all_locations.begin(), m_all_locations.end(), [this](string& str){return (str.find(m_area) == string::npos);} );
            m_all_locations.erase(new_end, m_all_locations.end());

            if(m_all_locations.empty())
            {
                yCWarning(NEXT_LOC_PLANNER,"Warning: no locations from map server for the area specified");
                return false;
            }
        }

        m_locations_unchecked.insert(m_locations_unchecked.end(), m_all_locations.begin(), m_all_locations.end());

    }
    else
    {
        yCWarning(NEXT_LOC_PLANNER,"Warning: no locations from map server");
    }

    return true;
}

/****************************************************************/
bool NextLocPlanner::close()
{

    if (m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();

    if(m_nav2DPoly.isValid())
        m_nav2DPoly.close();

    return true;
}

/****************************************************************/
double NextLocPlanner::getPeriod()
{
    return m_period;
}


/****************************************************************/
bool NextLocPlanner::setLocationStatus(const string location_name, const string& location_status)
{
    bool uncheckedOk = (location_status == "unchecked" || location_status == "Unchecked" || location_status == "UNCHECKED" );
    bool checkingOk  = (location_status == "checking" || location_status == "Checking"  || location_status == "CHECKING"   );
    bool checkedOk   = (location_status == "checked"  || location_status == "Checked"   || location_status == "CHECKED"    );
    if ( !(uncheckedOk || checkedOk || checkingOk)  )
    {
        yCError(NEXT_LOC_PLANNER,"Error: wrong location status specified. You should use: unchecked, checking or checked.");
        return false;
    }

    vector<string>::iterator findUnchecked {find(m_locations_unchecked.begin(), m_locations_unchecked.end(), location_name)};
    vector<string>::iterator findChecking {find(m_locations_checking.begin(), m_locations_checking.end(), location_name)};
    vector<string>::iterator findChecked {find(m_locations_checked.begin(), m_locations_checked.end(), location_name)};
    bool locFound { findUnchecked != m_locations_unchecked.end() || findChecking != m_locations_checking.end() || findChecked != m_locations_checked.end()};

    if (locFound)
    {
        if (findUnchecked != m_locations_unchecked.end())   { m_locations_unchecked.erase(findUnchecked);}
        else if(findChecking != m_locations_checking.end()) { m_locations_checking.erase(findChecking);  }
        else if(findChecked != m_locations_checked.end())   { m_locations_checked.erase(findChecked);    }


        if (uncheckedOk)
            {m_locations_unchecked.push_back(location_name); }
        else if (checkingOk)
            {m_locations_checking.push_back(location_name); }
        else if (checkedOk)
            {m_locations_checked.push_back(location_name); }
    }
    else if (location_name=="all")
    {

        if (uncheckedOk)
        {
            m_locations_unchecked.insert(m_locations_unchecked.end(), m_locations_checked.begin(), m_locations_checked.end());
            m_locations_unchecked.insert(m_locations_unchecked.end(), m_locations_checking.begin(), m_locations_checking.end());
            m_locations_checking.clear();
            m_locations_checked.clear();
        }
        else if (checkingOk)
        {
            m_locations_checking.insert(m_locations_checking.end(), m_locations_checked.begin(), m_locations_checked.end());
            m_locations_checking.insert(m_locations_checking.end(), m_locations_unchecked.begin(), m_locations_unchecked.end());
            m_locations_unchecked.clear();
            m_locations_checked.clear();
        }
        else if (checkedOk)
        {
            m_locations_checked.insert(m_locations_checked.end(), m_locations_checking.begin(), m_locations_checking.end());
            m_locations_checked.insert(m_locations_checked.end(), m_locations_unchecked.begin(), m_locations_unchecked.end());
            m_locations_unchecked.clear();
            m_locations_checking.clear();
        }

    }
    else
    {
        yCError(NEXT_LOC_PLANNER,"Error: specified location name not found.");
        return false;
    }

    sortUncheckedLocations();

    return true;
}


/****************************************************************/
bool NextLocPlanner::respond(const Bottle &cmd, Bottle &reply)
{
    lock_guard<mutex>  lock(m_mutex);
    reply.clear();
    string cmd_0=cmd.get(0).asString();
    if (cmd.size()==1)
    {
        if (cmd_0=="next")
        {
            // If "next" arrived while the robot is still checking a location, the location is set as "unchecked"
            if (m_locations_checking.size()>0)
            {
                vector<string>::iterator it;
                for(it = m_locations_checking.begin(); it != m_locations_checking.end(); it++)
                {
                    //setting all the "checking" locations as "unchecked"
                    setLocationStatus(*it, "unchecked");
                }
            }

            if (m_locations_unchecked.size()>0)
            {
                //reading the first unchecked location
                reply.addString(m_locations_unchecked[0]);
                //setting that location as "checking"
                setLocationStatus(m_locations_unchecked[0], "checking");
            }
            else
            {
                reply.addString("noLocation");
            }
        }
        else if (cmd_0=="help")
        {
            reply.addVocab32("many");
            reply.addString("next : returns the next unchecked location or noLocation");
            reply.addString("set <locationName> <status> : sets the status of a location to unchecked, checking or checked");
            reply.addString("set all <status> : sets the status of all locations");
            reply.addString("find <locationName> : checks if a location is in the list of the available ones");
            reply.addString("remove <locationName> : removes the defined location by any list");
            reply.addString("add <locationName> : adds a previously defined location in the unchecked list");
            reply.addString("add <locationName> <x,y,th coordinates>: adds a new location in the unchecked list");
            reply.addString("list : lists all the locations and their status");
            reply.addString("list2 : lists all the locations divided by their status");
            reply.addString("close : closes the nextLocationPlanner module");
            reply.addString("help : gets this list");
        }
        else if (cmd_0=="close")
        {
            close();
        }
        else if (cmd_0=="list")
        {
            reply.addVocab32("many");

            if (m_all_locations.size()!=0)
            {
                Bottle& tempList1 = reply.addList();
                vector<string>::iterator it;
                for(it = m_all_locations.begin(); it != m_all_locations.end(); it++)
                {
                    Bottle& tempList = tempList1.addList();
                    tempList.addString(*it);

                    if (find(m_locations_unchecked.begin(), m_locations_unchecked.end(), *it) != m_locations_unchecked.end())
                        tempList.addString("Unchecked");
                    else if (find(m_locations_checking.begin(), m_locations_checking.end(), *it) != m_locations_checking.end())
                        tempList.addString("Checking");
                    else if (find(m_locations_checked.begin(), m_locations_checked.end(), *it) != m_locations_checked.end())
                        tempList.addString("Checked");
                    else
                        tempList.addString("NotValid or Removed");
                }
            }
        }
        else if (cmd_0=="list2")
        {
            reply.addVocab32("many");
            Bottle& tempList = reply.addList();

            tempList.addString("Unchecked: ");
            if (m_locations_unchecked.size()!=0)
            {
                vector<string>::iterator it;
                for(it = m_locations_unchecked.begin(); it != m_locations_unchecked.end(); it++)
                {
                    tempList.addString(*it);
                }
            }
            tempList.addString(" ");
            tempList.addString("Checking: ");
            if (m_locations_checking.size()!=0)
            {
                vector<string>::iterator it;
                for(it = m_locations_checking.begin(); it != m_locations_checking.end(); it++)
                {
                    tempList.addString(*it);
                }
            }
            tempList.addString(" ");
            tempList.addString("Checked: ");
            if (m_locations_checked.size()!=0)
            {
                vector<string>::iterator it;
                for(it = m_locations_checked.begin(); it != m_locations_checked.end(); it++)
                {
                    tempList.addString(*it);
                }
            }
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(NEXT_LOC_PLANNER,"Error: wrong RPC command. Type 'help'");
        }
    }
    else if (cmd.size()==2)    //expected 'find <location>', 'add <location>' or 'remove <location>'
    {
        string loc=cmd.get(1).asString();

        if (cmd_0=="find")
        {
            if (find(m_locations_unchecked.begin(), m_locations_unchecked.end(), loc) != m_locations_unchecked.end())
                reply.fromString("ok unchecked");
            else if(find(m_locations_checking.begin(), m_locations_checking.end(), loc) != m_locations_checking.end())
                reply.fromString("ok checking");
            else if(find(m_locations_checked.begin(), m_locations_checked.end(), loc) != m_locations_checked.end())
                reply.fromString("ok checked");
            else
                reply.addString("notValid");

        }
        else if (cmd_0=="add")
        {
            if(addLocation(loc))
                reply.addString(loc + " added as unchecked location ");
            else
            {
                reply.addString(loc + " NOT added");
                yCWarning(NEXT_LOC_PLANNER,"Cannot add %s. It is not listed as a valid location for this map", loc.c_str());
            }
        }
        else if (cmd_0=="remove")
        {
            if(removeLocation(loc))
                reply.addString(loc + " removed");
            else
            {
                reply.addString(loc + " NOT removed");
                yCWarning(NEXT_LOC_PLANNER,"Cannot remove %s", loc.c_str());
            }
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(NEXT_LOC_PLANNER,"Error: wrong RPC command. Type 'help'");
        }
    }
    else if (cmd.size()==3)    //expected 'set <location> <status>'  or 'set all <status>'
    {
        if (cmd_0=="set")
        {
            string cmd_1=cmd.get(1).asString();
            string cmd_2=cmd.get(2).asString();

            if(!setLocationStatus(cmd_1, cmd_2))
            {
                reply.addVocab32(Vocab32::encode("nack"));
            }
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(NEXT_LOC_PLANNER,"Error: wrong RPC command. Type 'help'");
        }
    }
    else if (cmd.size()==5)    //expected 'add <location> <x> <y> <th>'
    {

        string locName = cmd.get(1).asString();
        double x = cmd.get(2).asFloat32();
        double y = cmd.get(3).asFloat32();
        double th = cmd.get(4).asFloat32();

        Map2DLocation loc;
        loc.map_id=m_map_name;
        loc.x=x;
        loc.y=y;
        loc.theta=th;
        loc.description=locName;

        if(addLocation(locName, loc))
            reply.addString(locName + " added");
        else
        {
            reply.addString(locName + " NOT added");
            yCWarning(NEXT_LOC_PLANNER,"Cannot add %s", locName.c_str());
        }

    }
    else
    {
        reply.addVocab32(Vocab32::encode("nack"));
        yCWarning(NEXT_LOC_PLANNER,"Error: input RPC command bottle has a wrong number of elements.");
    }

    if (reply.size()==0)
        reply.addVocab32(Vocab32::encode("ack"));

    return true;
}


/****************************************************************/
bool NextLocPlanner::getCurrentCheckingLocation(string& location_name)
{
    if (m_locations_checking.size()==0)
    {
        location_name = "<noLocation>";
    }
    else if (m_locations_checking.size()==1)
    {
        location_name = m_locations_checking[0];
    }
    else
    {
        yCWarning(NEXT_LOC_PLANNER,"Warning: more than one location set as Checking");
        location_name = m_locations_checking.back();
    }
    return true;
}


/****************************************************************/
bool NextLocPlanner::getUncheckedLocations(vector<string>& location_list)
{
    if (m_locations_unchecked.size()==0)
    {
        location_list.push_back("<noLocation>");
    }
    else
    {
        location_list = m_locations_unchecked;
    }
    return true;
}


/****************************************************************/
bool NextLocPlanner::getCheckedLocations(vector<string>& location_list)
{
    if (m_locations_checked.size()==0)
    {
        location_list.push_back("<noLocation>");
    }
    else
    {
        location_list = m_locations_checked;
    }
    return true;
}


/****************************************************************/
double NextLocPlanner::distRobotLocation(const string& location_name)
{
    Map2DLocation robotLoc;
    Map2DLocation loc;
    m_iNav2D->getCurrentPosition(robotLoc);
    m_iNav2D->getLocation(location_name, loc);

    return sqrt(pow((robotLoc.x - loc.x), 2) + pow((robotLoc.y - loc.y), 2));
}


/****************************************************************/
bool NextLocPlanner::updateModule()
{

    lock_guard<mutex> lock(m_mutex);

    sortUncheckedLocations();

    return true;
}


/****************************************************************/
void NextLocPlanner::sortUncheckedLocations()
{
    vector<double> m_unchecked_dist;
    for(size_t i=0; i<m_locations_unchecked.size(); ++i)
    {
        m_unchecked_dist.push_back(distRobotLocation(m_locations_unchecked[i]));
    }

    // Zip the vectors together
    vector<pair<string,double>> zipped;
    zip(m_locations_unchecked, m_unchecked_dist, zipped);

    // Sort the vector of pairs
    sort(begin(zipped), end(zipped),
        [&](const auto& a, const auto& b)
        {
            return a.second < b.second;
        });

    // Write the sorted pairs back to the original vectors
    unzip(zipped, m_locations_unchecked, m_unchecked_dist);
}


/****************************************************************/
bool NextLocPlanner::removeLocation(string& location_name)
{
    vector<string>::iterator findUnchecked {find(m_locations_unchecked.begin(), m_locations_unchecked.end(), location_name)};
    vector<string>::iterator findChecking {find(m_locations_checking.begin(), m_locations_checking.end(), location_name)};
    vector<string>::iterator findChecked {find(m_locations_checked.begin(), m_locations_checked.end(), location_name)};

    if (findUnchecked != m_locations_unchecked.end())   { m_locations_unchecked.erase(findUnchecked);}
    else if(findChecking != m_locations_checking.end()) { m_locations_checking.erase(findChecking);  }
    else if(findChecked != m_locations_checked.end())   { m_locations_checked.erase(findChecked);    }
    else
        return false;

    return true;
}


/****************************************************************/
bool NextLocPlanner::addLocation(string& location_name)
{
    vector<string>::iterator findLoc {find(m_all_locations.begin(), m_all_locations.end(), location_name)};

    if (findLoc != m_all_locations.end())
        m_locations_unchecked.push_back(location_name);
    else
        return false;

    return true;
}

/****************************************************************/
bool NextLocPlanner::addLocation(string locName, Map2DLocation loc)
{
    m_iNav2D->storeLocation(locName, loc);
    m_all_locations.push_back(locName);
    m_locations_unchecked.push_back(locName);

    return true;
}