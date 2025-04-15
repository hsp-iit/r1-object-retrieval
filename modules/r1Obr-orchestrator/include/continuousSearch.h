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
#ifndef CONTINUOUS_SEARCH_H
#define CONTINUOUS_SEARCH_H

#include <yarp/os/all.h>
#include <vector>

using namespace yarp::os;
using namespace std;

class ContinuousSearch 
{
private:
    bool                    m_active;

    string                  m_object_finder_result_port_name;
    BufferedPort<Bottle>    m_object_finder_result_port;
    
    
public:
    
    ContinuousSearch();
    ~ContinuousSearch() = default;

    bool configure(ResourceFinder& rf);
    void close();
    bool seeObject(string& obj);
    bool getObjCoordinates(Bottle* inputBtl, string& object, Bottle& out);
    bool whereObject(string& obj, Bottle& coords) ;
};

#endif