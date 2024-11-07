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

#ifndef GO_AND_FIND_IT_THREAD_H
#define GO_AND_FIND_IT_THREAD_H


#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/INavigation2D.h>
#include <yarp/os/all.h>
#include "getReadyToNav.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;

class GoAndFindItThread : public Thread, public TypedReaderCallback<Bottle>
{
enum GaFI_status 
{
    GaFI_IDLE,
    GaFI_NEW_SEARCH,
    GaFI_NAVIGATING,
    GaFI_ARRIVED,
    GaFI_SEARCHING,
    GaFI_OBJECT_FOUND,
    GaFI_OBJECT_NOT_FOUND,
    GaFI_STOP      
};

private:
    //Ports
    RpcClient               m_nextLoc_rpc_port;     //next location planner
    BufferedPort<Bottle>    m_lookObject_port;      //object research
    BufferedPort<Bottle>    m_objectFound_port;     //object research result
    BufferedPort<Bottle>    m_output_port;
        
    string                  m_nextLoc_rpc_port_name; 
    string                  m_lookObject_port_name; 
    string                  m_objectFound_port_name;
    string                  m_output_port_name;

    //Devices
    PolyDriver              m_nav2DPoly;
    Nav2D::INavigation2D*   m_iNav2D{nullptr};

    //ResourceFinder
    ResourceFinder&         m_rf;

    //Others
    GaFI_status             m_status;
    mutex                   m_mutex;
    double                  m_max_nav_time;
    double                  m_max_search_time;
    double                  m_searching_time;
    string                  m_what;
    string                  m_where;
    Bottle*                 m_coords;
    bool                    m_where_specified;
    bool                    m_nowhere_else;

    GetReadyToNav*          m_getReadyToNav;
    bool                    m_in_nav_position;

public:
    //Contructor and distructor
    GoAndFindItThread(ResourceFinder &rf);
    ~GoAndFindItThread() = default;

    //methods inherited from Thread
    virtual void run() override;
    virtual bool threadInit() override;
    virtual void threadRelease() override;
    virtual void onStop() override;

    //Port inherited from TypedReaderCallback
    using TypedReaderCallback<Bottle>::onRead;
    void onRead(Bottle& b) override;

    //member functions
    void setWhat(string& what);
    void setWhatWhere(string& what, string& where);
    void nextWhere();
    bool setNavigationPosition();
    bool goThere();
    bool search();
    bool objFound();
    bool objNotFound();
    bool stopSearch();
    bool resumeSearch();
    bool resetSearch();

    string getWhat();
    string getWhere();
    string getStatus();

};

#endif
