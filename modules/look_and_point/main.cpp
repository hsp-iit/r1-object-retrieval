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

#include <yarp/os/Network.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/RFModule.h>
#include <yarp/dev/ControlBoardInterfaces.h>

YARP_LOG_COMPONENT(LOOK_AND_POINT, "r1_obr.look_and_point")

using namespace yarp::os;
using namespace std;

class LookAndPoint : public RFModule, public TypedReaderCallback<Bottle>
{
private:

    //Port
    BufferedPort<Bottle> m_input_port;
    BufferedPort<Bottle> m_output_port;
    double  m_period;

public:
    //Constructor/Distructor
    LookAndPoint() : m_period(0.5) {};
    ~LookAndPoint() = default;

    //Internal methods
    virtual bool configure(ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();

    using TypedReaderCallback<Bottle>::onRead;
    void onRead(Bottle& btl) override;
};

bool LookAndPoint::configure(ResourceFinder &rf)
{
    string input_port_name = "/look_and_point/in:i";
    string output_port_name = "/look_and_point/out:o";
    
    m_input_port.useCallback(*this);
    
    if(!m_input_port.open(input_port_name))
        yCError(LOOK_AND_POINT) << "Cannot open port" << input_port_name; 
    else
        yCInfo(LOOK_AND_POINT) << "opened port" << input_port_name;
    
    if(!m_output_port.open(output_port_name))
        yCError(LOOK_AND_POINT) << "Cannot open port" << output_port_name; 
    else
        yCInfo(LOOK_AND_POINT) << "opened port" << output_port_name;

    return true;
}


bool LookAndPoint::updateModule()
{
    return true;
}

double LookAndPoint::getPeriod()
{
    return m_period;
}

void LookAndPoint::onRead(Bottle& b) 
{
    yCInfo(LOOK_AND_POINT,"Received: %s", b.toString().c_str());
    
    if (b.size()==2)
    {
        Bottle* out_ptr = b.get(1).asList();
        if(out_ptr)
        {
            Bottle&  out = m_output_port.prepare();
            out.clear();
            out = *out_ptr;
            m_output_port.write();
            yCInfo(LOOK_AND_POINT,"Sending: %s", out.toString().c_str());
        }
    }
    
    yCInfo(LOOK_AND_POINT,"Sending nothing");
}


bool LookAndPoint::close()
{
    if (!m_input_port.isClosed())
        m_input_port.close();

    if (!m_output_port.isClosed())
        m_output_port.close();

    return true;
}

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;
    if (!yarp.checkNetwork())
    {
        yError("check Yarp network.\n");
        return -1;
    }

    yarp::os::ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("look_and_point");  
    rf.configure(argc,argv);
    LookAndPoint mod;
    
    return mod.runModule(rf);
}
