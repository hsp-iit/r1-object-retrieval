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

#ifndef NEXT_LOC_PLANNER_H
#define NEXT_LOC_PLANNER_H

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/dev/INavigation2D.h>
#include <vector>
#include <map>
#include <algorithm>

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::dev::Nav2D;
using namespace std;


class NextLocPlanner : public RFModule
{

private:
    double            m_period;
    string            m_area;
    string            m_map_name;

    //Devices
    PolyDriver        m_nav2DPoly;
    INavigation2D*    m_iNav2D{nullptr};

    //Ports
    RpcServer         m_rpc_server_port;

    //Locations
    vector<string>    m_all_locations;
    vector<string>    m_locations_unchecked;
    vector<string>    m_locations_checking;
    vector<string>    m_locations_checked;

    mutex             m_mutex;

public:
    NextLocPlanner();
    ~NextLocPlanner() = default;
    virtual bool configure(ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();
    bool respond(const Bottle &cmd, Bottle &reply);

    /**
     * @brief Set a location status to the specified value
     * @param location_name The name of the location
     * @param location_status The status of the location
     * @return true if the location status was set successfully, false otherwise
     */
    bool setLocationStatus(const string location_name, const string& location_status);

    /**
     * @brief Get the name of the location currently under checking
     * @param location_name The name of the location
     * @return true if the location name was retrieved successfully, false otherwise
     */
    bool getCurrentCheckingLocation(string& location_name);

    /**
     * @brief Get the list of unchecked locations
     * @param location_list The list of unchecked locations
     * @return true if the list of unchecked locations was retrieved successfully, false otherwise
     */
    bool getUncheckedLocations(vector<string>& location_list);

    /**
     * @brief Get the list of checked locations
     * @param location_list The list of checked locations
     * @return true if the list of checked locations was retrieved successfully, false otherwise
     */
    bool getCheckedLocations(vector<string>& location_list);

    /**
     * @brief Sorts the unchecked locations based on the distance from the robot
     */
    void sortUncheckedLocations();

    /**
     * @brief Removes the specified location
     * @param loc The name of the location to be removed
     * @return true if the location was removed successfully, false otherwise
     */
    bool removeLocation(string& loc);

    /**
     * @brief Adds a location to the list of locations
     * @param loc The name of the location to be added
     * @return true if the location was added successfully, false otherwise
     */
    bool addLocation(string& loc); //add a previously defined location

    /**
     * @brief Adds a new location to the list of locations
     * @param locName The name of the location to be added
     * @param loc The location to be added
     * @return true if the location was added successfully, false otherwise
     */
    bool addLocation(string locName, Map2DLocation loc); //add a new location

private:

    /**
     * @brief Get the distance between the robot and the specified location
     * @param location_name The name of the location
     * @return The distance between the robot and the specified location
     */
    double distRobotLocation(const string& location_name);

    template <typename A, typename B>
    /**
     * @brief Zip two vectors
     * @param a The first vector
     * @param b The second vector
     * @param zipped The zipped vector
     */
    void zip(const vector<A> &a, const vector<B> &b,  vector<pair<A,B>> &zipped)
    {
        for(size_t i=0; i<a.size(); ++i)
        {
            zipped.push_back(make_pair(a[i], b[i]));
        }
    }

    template <typename A, typename B>
    /**
     * @brief Unzip a zipped vector
     * @param zipped The zipped vector
     * @param a The first vector
     * @param b The second vector
     */
    void unzip(const vector<pair<A, B>> &zipped, vector<A> &a, vector<B> &b)
    {
        for(size_t i=0; i<a.size(); i++)
        {
            a[i] = zipped[i].first;
            b[i] = zipped[i].second;
        }
    }


};

#endif
