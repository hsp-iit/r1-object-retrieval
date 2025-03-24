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

#include "getReadyToNav.h"

YARP_LOG_COMPONENT(GET_READY_TO_NAV, "r1_obr.goAndFindIt.getReadyToNav")

GetReadyToNav::GetReadyToNav() : m_time(2.0)
{
    m_right_arm_pos.fromString("-9.0 9.0 -10.0 50.0 0.0 0.0 0.0 0.0");
    m_left_arm_pos.fromString("-9.0 9.0 -10.0 50.0 0.0 0.0 0.0 0.0");
    m_head_pos.fromString("0.0 0.0");
    m_torso_pos.fromString("0.012");
}


bool GetReadyToNav::configure(yarp::os::ResourceFinder &rf)
{
    std::string robot=rf.check("robot",yarp::os::Value("cer")).asString();

    if(rf.check("set_nav_pos_time"))  
        m_time = rf.find("set_nav_pos_time").asFloat32()-0.5;

    bool okConfig = rf.check("SET_NAVIGATION_POSITION");
    if(!okConfig)
    {
        yCWarning(GET_READY_TO_NAV,"SET_NAVIGATION_POSITION section missing in ini file. Using the default values");
    }
    else
    {
        yarp::os::Searchable& config = rf.findGroup("SET_NAVIGATION_POSITION");
        if(config.check("right_arm_pos")) 
            m_right_arm_pos.fromString(config.find("right_arm_pos").asString());
        if(config.check("left_arm_pos")) 
            m_left_arm_pos.fromString(config.find("left_arm_pos").asString());
        if(config.check("head_pos")) 
            m_head_pos.fromString(config.find("head_pos").asString());
        if(config.check("torso_pos")) 
            m_torso_pos.fromString(config.find("torso_pos").asString());
    }

    
    // ----------- Polydriver config ----------- //
    yarp::os::Property prop;

    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavRArm");
    prop.put("remote","/"+robot+"/right_arm");
    if (!m_drivers[0].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/right_arm").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavLArm");
    prop.put("remote","/"+robot+"/left_arm");
    if (!m_drivers[1].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/left_arm").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavHead");
    prop.put("remote","/"+robot+"/head");
    if (!m_drivers[2].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/head").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavTorso");
    prop.put("remote","/"+robot+"/torso");
    if (!m_drivers[3].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/torso").c_str());
        close();
        return false;
    }

    m_drivers[0].view(m_iposctrl[0]);
    m_drivers[1].view(m_iposctrl[1]);
    m_drivers[2].view(m_iposctrl[2]);
    m_drivers[3].view(m_iposctrl[3]);
    if(!m_iposctrl[0] || !m_iposctrl[1] || !m_iposctrl[2] || !m_iposctrl[3])
    {
        yCError(GET_READY_TO_NAV,"Error opening iPositionControl interfaces. Devices not available");
        return false;
    }

    m_drivers[0].view(m_ictrlmode[0]);
    m_drivers[1].view(m_ictrlmode[1]);
    m_drivers[2].view(m_ictrlmode[2]);
    m_drivers[3].view(m_ictrlmode[3]);
    if(!m_ictrlmode[0] || !m_ictrlmode[1] || !m_ictrlmode[2] || !m_ictrlmode[3])
    {
        yCError(GET_READY_TO_NAV,"Error opening iControlMode interfaces. Devices not available");
        return false;
    }

    m_drivers[0].view(m_iencoder[0]);
    m_drivers[1].view(m_iencoder[1]);
    m_drivers[2].view(m_iencoder[2]);
    m_drivers[3].view(m_iencoder[3]);
    if(!m_iencoder[0] || !m_iencoder[1] || !m_iencoder[2] || !m_iencoder[3])
    {
        yCError(GET_READY_TO_NAV,"Error opening IEncoders interfaces. Devices not available");
        return false;
    }

    return true;
}


bool GetReadyToNav::setPosCtrlMode(const int part)
{
    int NUMBER_OF_JOINTS;
    std::vector<int> joints;
    std::vector<int> modes;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    { 
        joints.push_back(i_joint);
        modes.push_back(VOCAB_CM_POSITION);
    } 

    //need to stop the arm controllers so that the control mode of the arms can be set to Position
    std::string right_arm_controller_port{"/cer_reaching-controller/right/rpc"};
    std::string left_arm_controller_port{"/cer_reaching-controller/left/rpc"};
    yarp::os::Bottle stop_command{"stop"}; 
    if (yarp::os::Network::exists(right_arm_controller_port)) 
    {
        yarp::os::RpcClient tmpRightArmRPC;
        tmpRightArmRPC.open("/tmpRightArmRPC");
        if (yarp::os::Network::connect(tmpRightArmRPC.getName(), right_arm_controller_port)) 
        {
            tmpRightArmRPC.write(stop_command);
            yarp::os::Network::disconnect(tmpRightArmRPC.getName(), right_arm_controller_port);
        } 
        tmpRightArmRPC.close();
    }
    if (yarp::os::Network::exists(left_arm_controller_port)) 
    {
        yarp::os::RpcClient tmpLeftArmRPC;
        tmpLeftArmRPC.open("/tmpLeftArmRPC");
        if (yarp::os::Network::connect(tmpLeftArmRPC.getName(), left_arm_controller_port)) 
        {
            tmpLeftArmRPC.write(stop_command);
            yarp::os::Network::disconnect(tmpLeftArmRPC.getName(), left_arm_controller_port);
        } 
        tmpLeftArmRPC.close();
    }

    m_ictrlmode[part]->setControlModes(NUMBER_OF_JOINTS, joints.data(), modes.data());

    yarp::os::Time::delay(0.01);  // give time to update control modes value
    m_ictrlmode[part]->getControlModes(NUMBER_OF_JOINTS, joints.data(), modes.data());
    for (size_t i=0; i<NUMBER_OF_JOINTS; i++)
    {
        if(modes[i] != VOCAB_CM_POSITION)
        {
            std::string part_name="";
            if(part==0) part_name = "right_arm";
            else if(part==1) part_name = "left_arm";
            else if(part==2) part_name = "head";
            else if(part==3) part_name = "torso";
            
            yCError(GET_READY_TO_NAV) << "Joint" << i << "not in position mode for part:" << part_name;
            return false;
        }
    }

    return true;
}


bool GetReadyToNav::setJointsSpeed(const int part)
{
    int NUMBER_OF_JOINTS;
    std::vector<int>    joints;
    std::vector<double> speeds;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    { 
        double start, goal;
        m_iencoder[part]->getEncoder(i_joint,&start);

        if (part == 0)
            goal = m_right_arm_pos.get(i_joint).asFloat32();
        else if (part == 1)
            goal = m_left_arm_pos.get(i_joint).asFloat32();
        else if (part == 2)
            goal = m_head_pos.get(i_joint).asFloat32();
        else if (part == 3)
            goal = m_torso_pos.get(i_joint).asFloat32();

        double disp = start - goal;
        if(disp<0.0) disp *= -1;

        joints.push_back(i_joint);
        speeds.push_back(disp/m_time);
    } 

    return m_iposctrl[part]->setRefSpeeds(NUMBER_OF_JOINTS, joints.data(), speeds.data());
}


bool GetReadyToNav::movePart(const int part)
{
    int NUMBER_OF_JOINTS;
    std::vector<int>    joints;
    std::vector<double> positions;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    { 
        joints.push_back(i_joint);

        if (part == 0)
            positions.push_back(m_right_arm_pos.get(i_joint).asFloat32());
        else if (part == 1)
            positions.push_back(m_left_arm_pos.get(i_joint).asFloat32());
        else if (part == 2)
            positions.push_back(m_head_pos.get(i_joint).asFloat32());
        else if (part == 3)
            positions.push_back(m_torso_pos.get(i_joint).asFloat32());

    } 

    return m_iposctrl[part]->positionMove(NUMBER_OF_JOINTS, joints.data(), positions.data());
}


bool GetReadyToNav::navPosition()
{    
    //set all joints in position control mode
    for (int i = 0 ; i<=3 ; i++)
    {
        if(!setPosCtrlMode(i))
        {
            yCError(GET_READY_TO_NAV, "Error while setting navigation position: setPosCtrlMode");
            return false;
        }
    }
    
    //set the joints' speeds to get to final position at the same time
    for (int i = 0 ; i<=3 ; i++)
    {
        if(!setJointsSpeed(i))
        {
            yCError(GET_READY_TO_NAV, "Error while setting navigation position: setJointsSpeed");
            return false;
        }
    }

    yCInfo(GET_READY_TO_NAV, "Setting navigation position");  
      
    for (int i = 0 ; i<=3 ; i++)
    {
        if(!movePart(i))
        {
            yCError(GET_READY_TO_NAV, "Error while setting navigation position: movePart");
            return false;
        }
    }

    return true;

}


bool GetReadyToNav::areJointsOk()
{
    int mode=0;
    for (int i = 0 ; i<4 ; i++) 
    {
        int NUMBER_OF_JOINTS;
        m_iposctrl[i]->getAxes(&NUMBER_OF_JOINTS);
        for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
        { 
            m_ictrlmode[i]->getControlMode(i_joint, &mode);
            if (mode == VOCAB_CM_HW_FAULT)
            {
                yError() << "Error: hardware fault detected";
                return false;
            }
            else if (mode == VOCAB_CM_IDLE)
            {
                yError() << "Error: idle joint detected";
                return false;
            }
        }
    }

    return true;
}

void GetReadyToNav::close()
{
    if(m_drivers[0].isValid())
    {
        m_drivers[0].close();
    }

    if(m_drivers[1].isValid())
    {
        m_drivers[1].close();
    }

    if(m_drivers[2].isValid())
    {
        m_drivers[2].close();
    }

    if(m_drivers[3].isValid())
    {
        m_drivers[3].close();
    }
}

