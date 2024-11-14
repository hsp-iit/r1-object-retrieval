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

#include "tinyDancer.h"

YARP_LOG_COMPONENT(TINY_DANCER, "r1_obr.orchestrator.tinyDancer")


TinyDancer::TinyDancer(ResourceFinder &_rf) : m_rf(_rf)
{
    m_robot = "cer";
};


// --------------------------------------------------------------- //
bool TinyDancer::configure()
{
    if(m_rf.check("robot")) {m_robot = m_rf.find("robot").asString();}

    // Polydriver config
    Property prop;

    prop.put("device","remote_controlboard");
    prop.put("local","/r1Obr-orchestrator/tinyDancer/right_arm");
    prop.put("remote","/"+m_robot+"/right_arm");
    if (!m_drivers[0].open(prop))
    {
        yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/right_arm").c_str());
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/r1Obr-orchestrator/tinyDancer/left_arm");
    prop.put("remote","/"+m_robot+"/left_arm");
    if (!m_drivers[1].open(prop))
    {
        yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/left_arm").c_str());
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/r1Obr-orchestrator/tinyDancer/head");
    prop.put("remote","/"+m_robot+"/head");
    if (!m_drivers[2].open(prop))
    {
        yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/head").c_str());
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/r1Obr-orchestrator/tinyDancer/torso");
    prop.put("remote","/"+m_robot+"/torso");
    if (!m_drivers[3].open(prop))
    {
        yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/torso").c_str());
        return false;
    }

    m_drivers[0].view(m_iposctrl[0]);
    m_drivers[1].view(m_iposctrl[1]);
    m_drivers[2].view(m_iposctrl[2]);
    m_drivers[3].view(m_iposctrl[3]);
    if(!m_iposctrl[0] || !m_iposctrl[1] || !m_iposctrl[2] || !m_iposctrl[3] )
    {
        yCError(TINY_DANCER,"Error opening iPositionControl interfaces. Devices not available");
        return false;
    }

    m_drivers[0].view(m_ictrlmode[0]);
    m_drivers[1].view(m_ictrlmode[1]);
    m_drivers[2].view(m_ictrlmode[2]);
    m_drivers[3].view(m_ictrlmode[3]);
    if(!m_ictrlmode[0] || !m_ictrlmode[1] || !m_ictrlmode[2] || !m_ictrlmode[3])
    {
        yCError(TINY_DANCER,"Error opening iControlMode interfaces. Devices not available");
        return false;
    }

    m_drivers[0].view(m_iencoder[0]);
    m_drivers[1].view(m_iencoder[1]);
    m_drivers[2].view(m_iencoder[2]);
    m_drivers[3].view(m_iencoder[3]);
    if(!m_iencoder[0] || !m_iencoder[1] || !m_iencoder[2] || !m_iencoder[3])
    {
        yCError(TINY_DANCER,"Error opening IEncoders interfaces. Devices not available");
        return false;
    }

    return true;
}

// --------------------------------------------------------------- //
bool TinyDancer::areJointsOk()
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
                yCError(TINY_DANCER) << "Error: hardware fault detected";
                return false;
            }
            else if (mode == VOCAB_CM_IDLE)
            {
                yCError(TINY_DANCER) << "Error: idle joint detected";
                return false;
            }
        }
    }

    return true;
}

// --------------------------------------------------------------- //
bool TinyDancer::setCtrlMode(const int part, int ctrlMode)
{
    int NUMBER_OF_JOINTS;
    vector<int> joints;
    vector<int> modes;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    {
        joints.push_back(i_joint);
        modes.push_back(ctrlMode);
    }

    m_ictrlmode[part]->setControlModes(NUMBER_OF_JOINTS, joints.data(), modes.data());

    yarp::os::Time::delay(0.01);  // give time to update control modes value
    m_ictrlmode[part]->getControlModes(NUMBER_OF_JOINTS, joints.data(), modes.data());
    for (size_t i=0; i<NUMBER_OF_JOINTS; i++)
    {
        if(modes[i] != ctrlMode)
        {
            yCError(TINY_DANCER) << "Joint " << i << " not in correct control mode for part" << part;
            return false;
        }
    }

    return true;
}

// --------------------------------------------------------------- //
bool TinyDancer::setJointsSpeed(const int part, const double time, Bottle* joint_pos)
{
    int NUMBER_OF_JOINTS;
    vector<int>    joints;
    vector<double> speeds;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    {
        double start, goal;
        m_iencoder[part]->getEncoder(i_joint,&start);
        goal = joint_pos->get(i_joint).asFloat32();
        double disp = start - goal;
        if(disp<0.0) disp *= -1;

        joints.push_back(i_joint);
        speeds.push_back(disp/time);
    }

    return m_iposctrl[part]->setRefSpeeds(NUMBER_OF_JOINTS, joints.data(), speeds.data());
}


// --------------------------------------------------------------- //
bool TinyDancer::movePart(const int part, Bottle* joint_pos)
{
    int NUMBER_OF_JOINTS;
    std::vector<int>    joints;
    std::vector<double> positions;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    {
        joints.push_back(i_joint);
        positions.push_back(joint_pos->get(i_joint).asFloat32());
    }

    return m_iposctrl[part]->positionMove(NUMBER_OF_JOINTS, joints.data(), positions.data());
}


// --------------------------------------------------------------- //
bool TinyDancer::doDance(string& dance_name)
{
    if (!areJointsOk())
    {
        yCError(TINY_DANCER) << "An error occurred occured while checking joints";
        return false;
    }


    string dance_file = "dances/" + (dance_name.find(".ini") == string::npos ? dance_name + ".ini" : dance_name);

    string path_to_file = m_rf.findFileByName(dance_file);
    ifstream file;
    file.open(path_to_file);

    if (!file)
    {
        yCError(TINY_DANCER) << dance_name.c_str() << "could not be opened";
        return false;
    }


    for (string line; getline(file, line);)
    {
        if (!line.empty() && line[0] != ';' && line[0] != '#') // if not empty and not
        {
            /* must be a key[=: ]value pair */
            size_t endKey = line.find_first_of("=: ");
            if (endKey != string::npos)
            {
                string line_cmd = line.substr(endKey + 1);
                Bottle cmd;
                cmd.fromString(line_cmd);
                if (cmd.get(0).asString() == "pause")
                {
                    double time = cmd.get(1).asFloat32();
                    Time::delay(time);
                }
                else if (cmd.get(0).asString() == "move")
                {
                    Bottle* motion = cmd.get(1).asList();

                    double time = -1.0;
                    if (motion->check("time"))
                        time = motion->find("time").asFloat32();

                    Bottle* armR_joints=motion->find("right_arm").asList();
                    Bottle* armL_joints=motion->find("left_arm").asList();
                    Bottle* head_joints=motion->find("head").asList();
                    Bottle* torso_joints=motion->find("torso").asList();

                    //Setting control mode e joint speed before moving
                    bool ok = true;
                    if (armR_joints)
                    {
                        ok = ok && setCtrlMode(0, VOCAB_CM_POSITION);
                        if (time >= 0)
                            ok = ok && setJointsSpeed(0, time, armR_joints);
                    }
                    if (armL_joints)
                    {
                        ok = ok && setCtrlMode(1, VOCAB_CM_POSITION);
                        if (time >= 0)
                            ok = ok && setJointsSpeed(1, time, armL_joints);
                    }
                    if (head_joints)
                    {
                        ok = ok && setCtrlMode(2, VOCAB_CM_POSITION);
                        if (time >= 0)
                            ok = ok && setJointsSpeed(2, time, head_joints);
                    }
                    if (torso_joints)
                    {
                        ok = ok && setCtrlMode(3, VOCAB_CM_POSITION);
                        if (time >= 0)
                            ok = ok && setJointsSpeed(3, time, torso_joints);
                    }

                    //Check that no error occurred
                    if (!ok || !areJointsOk())
                    {
                        yCError(TINY_DANCER) << "An error occurred occured while preparing motion";
                        return false;
                    }

                    //Move each part
                    if (armR_joints)
                        ok = ok && movePart(0, armR_joints);
                    if (armL_joints)
                        ok = ok && movePart(1, armL_joints);
                    if (head_joints)
                        ok = ok && movePart(2, head_joints);
                    if (torso_joints)
                        ok = ok && movePart(3, torso_joints);

                    //Check again that no error occurred
                    if (!ok || !areJointsOk())
                    {
                        yCError(TINY_DANCER) << "An error occurred occured during motion";
                        return false;
                    }

                }
            }
        }
    }


    return true;
}


// --------------------------------------------------------------- //
void TinyDancer::close()
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

    yCInfo(TINY_DANCER, "Thread released");
}

