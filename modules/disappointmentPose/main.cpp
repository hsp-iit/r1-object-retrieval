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

#ifndef DISAPPOINTMENT_POSE_CPP
#define DISAPPOINTMENT_POSE_CPP

#include <yarp/os/Network.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/RFModule.h>
#include <yarp/dev/ControlBoardInterfaces.h>

YARP_LOG_COMPONENT(DISAPPOINTMENT_POSE, "r1_obr.disappointmentPose")

#define RIGHT_ARM 0
#define LEFT_ARM 1
#define HEAD 2

using namespace yarp::os;
using namespace yarp::dev;
using namespace std;

class DisappointmentPose : public RFModule, public TypedReaderCallback<Bottle>
{
private:
    //Polydriver
    PolyDriver           m_drivers[3];
    IControlMode*        m_ictrlmode[3]{nullptr, nullptr, nullptr};
    IPositionControl*    m_iposctrl[3]{nullptr, nullptr, nullptr};

    //Port
    BufferedPort<Bottle> m_input_port;
    string               m_input_port_name;

    Bottle               m_right_arm_pos;
    Bottle               m_left_arm_pos;
    Bottle               m_head_pos;

    double               m_period;
    bool                 m_parts_on[3]{true,true,true};

public:
    //Constructor/Distructor
    DisappointmentPose();
    ~DisappointmentPose() = default;

    //Internal methods
    virtual bool configure(ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();

    using TypedReaderCallback<Bottle>::onRead;
    void onRead(Bottle& btl) override;

    void setPosition();
    void setPosCtrlMode(const int part);
};

DisappointmentPose::DisappointmentPose()
{
    //Default is the navigation position
    m_right_arm_pos.fromString("-9.0 9.0 -10.0 50.0 0.0 0.0 0.0 0.0");
    m_left_arm_pos.fromString("-9.0 9.0 -10.0 50.0 0.0 0.0 0.0 0.0");
    m_head_pos.fromString("0.0 0.0");
}


bool DisappointmentPose::configure(ResourceFinder &rf)
{

    //Generic config
    if(rf.check("period")) {m_period = rf.find("period").asFloat32();}
    string robot=rf.check("robot",Value("cer")).asString();


    //Open input port
    if(rf.check("input_port"))
        m_input_port_name = rf.find("input_port").asString();
    m_input_port.useCallback(*this);
    if(!m_input_port.open(m_input_port_name))
        yCError(DISAPPOINTMENT_POSE) << "Cannot open port" << m_input_port_name;
    else
        yCInfo(DISAPPOINTMENT_POSE) << "opened port" << m_input_port_name;


    // Check enable/disable arms options
    if(rf.check("right_arm_on"))
        m_parts_on[RIGHT_ARM] = rf.find("right_arm_on").asInt16() == 1;
    if(rf.check("left_arm_on"))
        m_parts_on[LEFT_ARM] = rf.find("left_arm_on").asInt16() == 1;
    if(rf.check("head_on"))
        m_parts_on[HEAD] = rf.find("left_arm_on").asInt16() == 1;

    //Config arms and head pose
    bool okConfig = rf.check("ACTIVE");
    if(!okConfig)
    {
        yCWarning(DISAPPOINTMENT_POSE,"There is no ACTIVE section in ini file. Using the default values");
    }
    else
    {
        Searchable& config = rf.findGroup("ACTIVE");
        if(config.check("right_arm_pos"))
            m_right_arm_pos.fromString(config.find("right_arm_pos").asString());
        if(config.check("left_arm_pos"))
            m_left_arm_pos.fromString(config.find("left_arm_pos").asString());
        if(config.check("head_pos"))
            m_head_pos.fromString(config.find("head_pos").asString());
    }


    // Polydriver config
    Property prop;

    if(m_parts_on[RIGHT_ARM])
    {
        prop.put("device","remote_controlboard");
        prop.put("local","/disappointmentPose/right_arm");
        prop.put("remote","/"+robot+"/right_arm");
        if (!m_drivers[RIGHT_ARM].open(prop))
        {
            yCError(DISAPPOINTMENT_POSE,"Unable to connect to %s",("/"+robot+"/right_arm").c_str());
            close();
            return false;
        }
        m_drivers[RIGHT_ARM].view(m_iposctrl[RIGHT_ARM]);
        m_drivers[RIGHT_ARM].view(m_ictrlmode[RIGHT_ARM]);
    }
    if(m_parts_on[LEFT_ARM])
    {
        prop.clear();
        prop.put("device","remote_controlboard");
        prop.put("local","/disappointmentPose/left_arm");
        prop.put("remote","/"+robot+"/left_arm");
        if (!m_drivers[LEFT_ARM].open(prop))
        {
            yCError(DISAPPOINTMENT_POSE,"Unable to connect to %s",("/"+robot+"/left_arm").c_str());
            close();
            return false;
        }
        m_drivers[LEFT_ARM].view(m_iposctrl[LEFT_ARM]);
        m_drivers[LEFT_ARM].view(m_ictrlmode[LEFT_ARM]);
    }
    if(m_parts_on[HEAD])
    {
        prop.clear();
        prop.put("device","remote_controlboard");
        prop.put("local","/disappointmentPose/head");
        prop.put("remote","/"+robot+"/head");
        if (!m_drivers[HEAD].open(prop))
        {
            yCError(DISAPPOINTMENT_POSE,"Unable to connect to %s",("/"+robot+"/head").c_str());
            close();
            return false;
        }
        m_drivers[HEAD].view(m_iposctrl[HEAD]);
        m_drivers[HEAD].view(m_ictrlmode[HEAD]);
    }

    if((!m_iposctrl[RIGHT_ARM] && m_parts_on[RIGHT_ARM]) || (!m_iposctrl[LEFT_ARM] && m_parts_on[LEFT_ARM]) || (!m_iposctrl[HEAD] && m_parts_on[HEAD]) )
    {
        yCError(DISAPPOINTMENT_POSE,"Error opening iPositionControl interfaces. Devices not available");
        return false;
    }

    if((!m_ictrlmode[RIGHT_ARM] && m_parts_on[RIGHT_ARM]) || (!m_ictrlmode[LEFT_ARM] && m_parts_on[LEFT_ARM]) || (!m_ictrlmode[HEAD] && m_parts_on[HEAD]))
    {
        yCError(DISAPPOINTMENT_POSE,"Error opening iControlMode interfaces. Devices not available");
        return false;
    }

    return true;
}

bool DisappointmentPose::updateModule()
{
    return true;
}

double DisappointmentPose::getPeriod()
{
    return m_period;
}

void DisappointmentPose::onRead(Bottle& b)
{

    yCInfo(DISAPPOINTMENT_POSE,"Received something. Not a good thing probably. Therefore I am setting a disappointment pose");
    setPosition();
}

void DisappointmentPose::setPosCtrlMode(const int part)
{
    int NUMBER_OF_JOINTS;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++){ m_ictrlmode[part]->setControlMode(i_joint, VOCAB_CM_POSITION); }
}


void DisappointmentPose::setPosition()
{
    for (int i = 0 ; i<=2 ; i++)
    {
        if(!m_parts_on[i])
            continue;
        setPosCtrlMode(i);
    }

    //right arm
    if(m_parts_on[RIGHT_ARM])
    {
        m_iposctrl[RIGHT_ARM]->positionMove(0, m_right_arm_pos.get(0).asFloat32());
        m_iposctrl[RIGHT_ARM]->positionMove(1, m_right_arm_pos.get(1).asFloat32());
        m_iposctrl[RIGHT_ARM]->positionMove(2, m_right_arm_pos.get(2).asFloat32());
        m_iposctrl[RIGHT_ARM]->positionMove(3, m_right_arm_pos.get(3).asFloat32());
        m_iposctrl[RIGHT_ARM]->positionMove(4, m_right_arm_pos.get(4).asFloat32());
        m_iposctrl[RIGHT_ARM]->positionMove(5, m_right_arm_pos.get(5).asFloat32());
        m_iposctrl[RIGHT_ARM]->positionMove(6, m_right_arm_pos.get(6).asFloat32());
        m_iposctrl[RIGHT_ARM]->positionMove(7, m_right_arm_pos.get(7).asFloat32());
    }

    //left arm
    if(m_parts_on[LEFT_ARM])
    {
        m_iposctrl[LEFT_ARM]->positionMove(0, m_left_arm_pos.get(0).asFloat32());
        m_iposctrl[LEFT_ARM]->positionMove(1, m_left_arm_pos.get(1).asFloat32());
        m_iposctrl[LEFT_ARM]->positionMove(2, m_left_arm_pos.get(2).asFloat32());
        m_iposctrl[LEFT_ARM]->positionMove(3, m_left_arm_pos.get(3).asFloat32());
        m_iposctrl[LEFT_ARM]->positionMove(4, m_left_arm_pos.get(4).asFloat32());
        m_iposctrl[LEFT_ARM]->positionMove(5, m_left_arm_pos.get(5).asFloat32());
        m_iposctrl[LEFT_ARM]->positionMove(6, m_left_arm_pos.get(6).asFloat32());
        m_iposctrl[LEFT_ARM]->positionMove(7, m_left_arm_pos.get(7).asFloat32());
    }
    //head
    if(!m_parts_on[HEAD])
        return;
    m_iposctrl[HEAD]->positionMove(0, m_head_pos.get(0).asFloat32());
    m_iposctrl[HEAD]->positionMove(1, m_head_pos.get(1).asFloat32());
}


bool DisappointmentPose::close()
{
    if(m_drivers[RIGHT_ARM].isValid())
    {
        m_drivers[RIGHT_ARM].close();
    }

    if(m_drivers[LEFT_ARM].isValid())
    {
        m_drivers[LEFT_ARM].close();
    }

    if(m_drivers[HEAD].isValid())
    {
        m_drivers[HEAD].close();
    }

    if (!m_input_port.isClosed())
        m_input_port.close();

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
    rf.setDefaultConfigFile("disappointmentPose.ini");          //overridden by --from parameter
    rf.setDefaultContext("disappointmentPose");                 //overridden by --context parameter
    rf.configure(argc,argv);
    DisappointmentPose mod;

    return mod.runModule(rf);
}


#endif