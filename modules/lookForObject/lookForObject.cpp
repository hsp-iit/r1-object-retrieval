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


#include "lookForObject.h"

YARP_LOG_COMPONENT(LOOK_FOR_OBJECT, "r1_obr.lookForObject")

LookForObject::LookForObject() :
    m_period(1.0)
{  
}

bool LookForObject::configure(yarp::os::ResourceFinder &rf) 
{   

    if(rf.check("period")){m_period = rf.find("period").asFloat32();}  

    std::string portName = "/lookForObject/object:i";
    if(rf.check("input_object_port")){portName = rf.find("input_object_port").asString();}
    bool ret = m_inputPort.open(portName);
    if (!ret)
    {
        yCError(LOOK_FOR_OBJECT, "Unable to open input port");
        return false;
    }
   
    // --------- Thread initialization --------- //
    m_innerThread = new LookForObjectThread(rf);
    bool threadOk = m_innerThread->start();
    if (!threadOk){
        return false;
    }
    
    m_inputPort.useCallback(*m_innerThread);

    return true;
}

bool LookForObject::close()
{
    m_innerThread->stop();
    delete m_innerThread;
    m_innerThread = NULL;
    m_inputPort.close();

    return true;
}

double LookForObject::getPeriod()
{
    return m_period;
}

bool LookForObject::updateModule()
{
    if (isStopping())
    {
        if (m_innerThread) m_innerThread->stop();
        return false;
    }

    return true;
}
