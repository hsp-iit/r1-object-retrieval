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

#ifndef GO_AND_FIND_IT_H
#define GO_AND_FIND_IT_H

#include <yarp/os/all.h>
#include "goAndFindItThread.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;

class GoAndFindIt : public RFModule, public TypedReaderCallback<Bottle>
{
private:
    double              m_period;
    GoAndFindItThread*  m_thread;

    //Ports
    BufferedPort<Bottle>    m_input_port; 
    RpcServer               m_rpc_server_port; 
    string                  m_input_port_name; 
    string                  m_rpc_server_port_name;  
    
public:
    GoAndFindIt();
    virtual bool configure(ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();

    using TypedReaderCallback<Bottle>::onRead;
    void onRead(Bottle& btl) override;

    bool respond(const Bottle &cmd, Bottle &reply);
};

#endif 
