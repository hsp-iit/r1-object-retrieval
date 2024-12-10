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

#include "continuousSearch.h"

YARP_LOG_COMPONENT(CONTINUOUS_SEARCH, "r1_obr.orchestrator.continousSearch")

ContinuousSearch::ContinuousSearch()
{
    m_object_finder_result_port_name= "/r1Obr-orchestrator/continousSearch/object_finder_result:i";
}


/****************************************************************/
bool ContinuousSearch::configure(ResourceFinder& rf)
{
    if(!rf.check("CONTINUOUS_SEARCH"))
    {
        yCWarning(CONTINUOUS_SEARCH,"CONTINUOUS_SEARCH section missing in ini file. Using the default values");
    }
    Searchable& cs_config = rf.findGroup("CONTINUOUS_SEARCH");
    if(cs_config.check("object_finder_result_port")) {m_object_finder_result_port_name = cs_config.find("object_finder_result_port").asString();}
    m_active = cs_config.check("active")  ? !(cs_config.find("active").asString() == "false") : true;

    if(!m_object_finder_result_port.open(m_object_finder_result_port_name))
    {
        yCError(CONTINUOUS_SEARCH) << "Cannot open port" << m_object_finder_result_port_name;
        return false;
    }
    else
        yCInfo(CONTINUOUS_SEARCH) << "opened port" << m_object_finder_result_port_name;



    return true;
}


/****************************************************************/
void ContinuousSearch::close()
{
    if (!m_object_finder_result_port.isClosed())
        m_object_finder_result_port.close();

    yCInfo(CONTINUOUS_SEARCH, "Continuous search thread released");
}


/****************************************************************/
bool ContinuousSearch::seeObject(string& obj)
{
    if (!m_active)
    {
        yCError(CONTINUOUS_SEARCH,"Continuous search is not active");
        return false;
    }

    Bottle* finderResult = m_object_finder_result_port.read(false);
    if(finderResult != nullptr)
    {
        if(finderResult->check(obj))
        {
            return true;
        }
    }

    yCWarning(CONTINUOUS_SEARCH, "The Object Finder is not seeing any %s", obj.c_str());
    return false;
}


/****************************************************************/
bool ContinuousSearch::whereObject(string& obj, Bottle& coords)
{
    if (!m_active)
    {
        yCError(CONTINUOUS_SEARCH,"Continuous search is not active");
        return false;
    }

    // we repeat the same as seeObject for two reasons:
    // - to be sure that the robot has really seen the object while navigating
    // - the first time the robot hasn't stopped the navigation

    Bottle* finderResult = m_object_finder_result_port.read(false);
    if(finderResult != nullptr)
    {
        if (getObjCoordinates(finderResult, obj, coords))
        {
            return true;
        }
    }

    yCWarning(CONTINUOUS_SEARCH, "The Object Finder is not seeing any %s", obj.c_str());
    return false;
}

/****************************************************************/
bool ContinuousSearch::getObjCoordinates(Bottle* inputBtl, string& object, Bottle& outputBtl)
{
    double max_conf = 0.0;
    double x = -1.0, y;
    for (int i=0; i<inputBtl->size(); i++)
    {
        Bottle* b = inputBtl->get(i).asList();
        if (b->get(0).asString() != object) //skip objects with another label
            continue;

        if(b->get(1).asFloat32() > max_conf) //get the object with the max confidence
        {
            max_conf = b->get(1).asFloat32();
            x = b->get(2).asFloat32();
            y = b->get(3).asFloat32();
        }
    }
    if (x<0)
    {
        yCError(CONTINUOUS_SEARCH, "Object %s not found", object.c_str());
        return false;
    }

    outputBtl.addFloat32(x);
    outputBtl.addFloat32(y);

    return true;
}




