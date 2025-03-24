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

#include "goAndFindIt.h"


YARP_LOG_COMPONENT(GO_AND_FIND_IT, "r1_obr.goAndFindIt")


/****************************************************************/
GoAndFindIt::GoAndFindIt() : m_period(1.0)
{
    m_input_port_name       = "/goAndFindIt/input:i";
    m_rpc_server_port_name  = "/goAndFindIt/rpc";
}


/****************************************************************/
bool GoAndFindIt::configure(ResourceFinder &rf)
{
    //Generic config
    if(rf.check("period"))
        m_period = rf.find("period").asFloat32();

    //Open input port
    if(rf.check("input_port"))
        m_input_port_name = rf.find("input_port").asString();
    m_input_port.useCallback(*this);
    if(!m_input_port.open(m_input_port_name))
        yCError(GO_AND_FIND_IT) << "Cannot open port" << m_input_port_name;
    else
        yCInfo(GO_AND_FIND_IT) << "opened port" << m_input_port_name;

    //Open RPC Server Port
    if(rf.check("rpc_port"))
        m_rpc_server_port_name = rf.find("rpc_port").asString();
    if (!m_rpc_server_port.open(m_rpc_server_port_name))
    {
        yCError(GO_AND_FIND_IT, "open() error could not open rpc port %s, check network", m_rpc_server_port_name.c_str());
        return false;
    }
    if (!attach(m_rpc_server_port))
    {
        yCError(GO_AND_FIND_IT, "attach() error with rpc port %s", m_rpc_server_port_name.c_str());
        return false;
    }

    //Initialize Thread
    m_thread = new GoAndFindItThread(rf);
    bool threadOk = m_thread->start();
    if (!threadOk){
        yCError(GO_AND_FIND_IT) << "Could not correctly initialize the inner thread";
        return false;
    }

    return true;
}


/****************************************************************/
bool GoAndFindIt::close()
{
    if (!m_input_port.isClosed())
        m_input_port.close();

    if (m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();

    m_thread->stop();
    delete m_thread;

    return true;
}

/****************************************************************/
double GoAndFindIt::getPeriod()
{
    return m_period;
}

/****************************************************************/
bool GoAndFindIt::updateModule()
{
    return true;
}

/****************************************************************/
void GoAndFindIt::onRead(Bottle& b)
{

    yCInfo(GO_AND_FIND_IT,"Received:  %s",b.toString().c_str());

    if(b.size() == 2)           //expected "<object> <location>"
    {
        string what = b.get(0).asString();
        string where = b.get(1).asString();

        m_thread->setWhatWhere(what,where);

    }
    else if(b.size() == 1)      //expected "<object>" or "stop"/"reset"/"resume"
    {
        string what = b.get(0).asString();

        if (what == "stop")
        {
            m_thread->stopSearch();
        }
        else if (what == "resume")
        {
            if(!m_thread->resumeSearch())
            {
                yCError(GO_AND_FIND_IT,"Cannot resume search. Please send a stop command first");
                return;
            }
        }
        else if (what == "reset")
        {
            m_thread->resetSearch();
        }
        else if (what == "navpos")
        {
            if (m_thread->getStatus() != "navigating" && m_thread->getStatus() != "searching")
            {
                m_thread->setNavigationPosition();
            }
            else
            {
                yCError(GO_AND_FIND_IT,"Doing something else now, cannot set navigation position. Please send a stop command first");
            }
        }
        else
        {
           m_thread->setWhat(what);
        }

    }
    else
    {
        yCError(GO_AND_FIND_IT,"Error: wrong bottle format received");
        return;
    }

}

/****************************************************************/
bool GoAndFindIt::respond(const Bottle &cmd, Bottle &reply)
{
    reply.clear();
    string cmd_0=cmd.get(0).asString();
    if (cmd.size()==1)
    {
        if (cmd_0=="help")
        {
            reply.addVocab32("many");
            reply.addString("search <what> : starts to search for 'what'");
            reply.addString("search <what> <where>: starts searching for 'what' at location 'where'");
            reply.addString("status : returns the current status of the search");
            reply.addString("what   : returns the object of the current search");
            reply.addString("where  : returns the location of the current search");
            reply.addString("stop   : stops search");
            reply.addString("resume : resumes stopped search");
            reply.addString("reset  : resets search");
            reply.addString("navpos : sets the robot in navigation position");
            reply.addString("help   : gets this list");
        }
        else if (cmd_0=="what")
        {
            reply.addString(m_thread->getWhat());
        }
        else if (cmd_0=="where")
        {
            reply.addString(m_thread->getWhere());
        }
        else if (cmd_0=="status")
        {
            reply.addString(m_thread->getStatus());
        }
        else if (cmd_0=="stop")
        {
            m_thread->stopSearch();
            reply.addString("search stopped");
        }
        else if (cmd_0=="resume")
        {
            if(!m_thread->resumeSearch())
            {
                yCError(GO_AND_FIND_IT,"Cannot resume search. Please send a stop command first");
                reply.addString("search not resumed");
                return true;
            }
            reply.addString("search resumed");
        }
        else if (cmd_0=="reset")
        {
            m_thread->resetSearch();
            reply.addString("search reset");
        }
        else if (cmd_0=="navpos")
        {
            if (m_thread->getStatus() != "navigating" && m_thread->getStatus() != "searching")
            {
                if(m_thread->setNavigationPosition())
                    reply.addString("robot set in navigation position");
                else
                    reply.addString("cannot set the robot in navigation position");
            }
            else
            {
                yCError(GO_AND_FIND_IT,"Doing something else now, cannot set navigation position. Please send a stop command first");
                reply.addVocab32(Vocab32::encode("nack"));
            }

        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(GO_AND_FIND_IT,"Error: wrong RPC command. Type 'help'");
        }
    }
    else if (cmd.size()==2)
    {
        if (cmd_0=="search")
        {
            string what=cmd.get(1).asString();
            m_thread->setWhat(what);
            reply.addString("searching for '" + what + "'");
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(GO_AND_FIND_IT,"Error: wrong RPC command. Type 'help'");
        }
    }
    else if (cmd.size()==3)
    {
        if (cmd_0=="search")
        {
            string what=cmd.get(1).asString();
            string where=cmd.get(2).asString();
            m_thread->setWhatWhere(what,where);
            reply.addString("searching for '" + what + "' at '" + where + "'");
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(GO_AND_FIND_IT,"Error: wrong RPC command. Type 'help'");
        }
    }
    else
    {
        reply.addVocab32(yarp::os::Vocab32::encode("nack"));
        yCWarning(GO_AND_FIND_IT,"Error: input RPC command bottle has a wrong number of elements.");
    }

    if (reply.size()==0)
        reply.addVocab32(yarp::os::Vocab32::encode("ack"));

    return true;
}
