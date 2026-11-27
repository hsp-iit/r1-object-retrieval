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

    // --------- Parts enable/disable ---------- //
    if(m_rf.check("right_arm_on"))
        m_parts_on[RIGHT_ARM] = m_rf.find("right_arm_on").asInt16() == 1;
    if(m_rf.check("left_arm_on"))
        m_parts_on[LEFT_ARM] = m_rf.find("left_arm_on").asInt16() == 1;
    if(m_rf.check("head_on"))
        m_parts_on[HEAD] = m_rf.find("head_on").asInt16() == 1;
    if(m_rf.check("torso_on"))
        m_parts_on[TORSO] = m_rf.find("torso_on").asInt16() == 1;

    // Polydriver config
    Property prop;

    prop.put("device","controlBoard_nwc_yarp");
    prop.put("local","/r1Obr-orchestrator/tinyDancer/right_arm");
    prop.put("remote","/"+m_robot+"/right_arm");


    if(m_parts_on[RIGHT_ARM])
    {
        prop.put("device","controlBoard_nwc_yarp");
        prop.put("local","/r1Obr-orchestrator/tinyDancer/right_arm");
        prop.put("remote","/"+m_robot+"/right_arm");
        if (!m_drivers[RIGHT_ARM].open(prop))
        {
            yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/right_arm").c_str());
            close();
            return false;
        }
        m_drivers[RIGHT_ARM].view(m_iposctrl[RIGHT_ARM]);
        m_drivers[RIGHT_ARM].view(m_ictrlmode[RIGHT_ARM]);
        m_drivers[RIGHT_ARM].view(m_iencoder[RIGHT_ARM]);
    }
    if(m_parts_on[LEFT_ARM])
    {
        prop.clear();
        prop.put("device","controlBoard_nwc_yarp");
        prop.put("local","/r1Obr-orchestrator/tinyDancer/left_arm");
        prop.put("remote","/"+m_robot+"/left_arm");
        if (!m_drivers[LEFT_ARM].open(prop))
        {
            yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/left_arm").c_str());
            close();
            return false;
        }
        m_drivers[LEFT_ARM].view(m_iposctrl[LEFT_ARM]);
        m_drivers[LEFT_ARM].view(m_ictrlmode[LEFT_ARM]);
        m_drivers[LEFT_ARM].view(m_iencoder[LEFT_ARM]);
    }
    if(m_parts_on[HEAD])
    {
        prop.clear();
        prop.put("device","controlBoard_nwc_yarp");
        prop.put("local","/r1Obr-orchestrator/tinyDancer/head");
        prop.put("remote","/"+m_robot+"/head");
        if (!m_drivers[HEAD].open(prop))
        {
            yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/head").c_str());
            close();
            return false;
        }
        m_drivers[HEAD].view(m_iposctrl[HEAD]);
        m_drivers[HEAD].view(m_ictrlmode[HEAD]);
        m_drivers[HEAD].view(m_iencoder[HEAD]);
    }
    if(m_parts_on[TORSO])
    {
        prop.clear();
        prop.put("device","controlBoard_nwc_yarp");
        prop.put("local","/r1Obr-orchestrator/tinyDancer/torso");
        prop.put("remote","/"+m_robot+"/torso");
        if (!m_drivers[TORSO].open(prop))
        {
            yCError(TINY_DANCER,"Unable to connect to %s",("/"+m_robot+"/torso").c_str());
            close();
            return false;
        }
        m_drivers[TORSO].view(m_iposctrl[TORSO]);
        m_drivers[TORSO].view(m_ictrlmode[TORSO]);
        m_drivers[TORSO].view(m_iencoder[TORSO]);
    }

    if((!m_iposctrl[RIGHT_ARM] && m_parts_on[RIGHT_ARM]) || (!m_iposctrl[LEFT_ARM] && m_parts_on[LEFT_ARM]) || (!m_iposctrl[HEAD] && m_parts_on[HEAD]) || (!m_iposctrl[TORSO] && m_parts_on[TORSO]))
    {
        yCError(TINY_DANCER,"Error opening iPositionControl interfaces. Devices not available");
        return false;
    }

    if((!m_ictrlmode[RIGHT_ARM] && m_parts_on[RIGHT_ARM]) || (!m_ictrlmode[LEFT_ARM] && m_parts_on[LEFT_ARM]) || (!m_ictrlmode[HEAD] && m_parts_on[HEAD]) || (!m_ictrlmode[TORSO] && m_parts_on[TORSO]))
    {
        yCError(TINY_DANCER,"Error opening iControlMode interfaces. Devices not available");
        return false;
    }

    if((!m_iencoder[RIGHT_ARM] && m_parts_on[RIGHT_ARM]) || (!m_iencoder[LEFT_ARM] && m_parts_on[LEFT_ARM]) || (!m_iencoder[HEAD] && m_parts_on[HEAD]) || (!m_iencoder[TORSO] && m_parts_on[TORSO]))
    {
        yCError(TINY_DANCER,"Error opening IEncoders interfaces. Devices not available");
        return false;
    }

    return true;
}

// --------------------------------------------------------------- //
bool TinyDancer::areJointsOk()
{
    yarp::dev::ControlModeEnum mode{yarp::dev::ControlModeEnum::VOCAB_CM_UNKNOWN};
    for (int i = 0 ; i<4 ; i++)
    {
        if(!m_parts_on[i])
        {
            yCWarning(TINY_DANCER, "Part %d is not available", i);
            continue;
        }
        size_t NUMBER_OF_JOINTS{0};
        if (yarp::dev::ReturnValue ret = m_iposctrl[i]->getAxes(NUMBER_OF_JOINTS); !ret)
            yCError(TINY_DANCER) << "Unable to get the number of joints of part" << i << ":" << ret.toString();
        for (size_t i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
        {
            if (yarp::dev::ReturnValue ret = m_ictrlmode[i]->getControlMode(i_joint, mode); !ret)
                yCError(TINY_DANCER) << "Unable to get the control mode of joint" << i_joint << "of part" << i << ":" << ret.toString();
            if (mode == yarp::dev::ControlModeEnum::VOCAB_CM_HW_FAULT)
            {
                yCError(TINY_DANCER) << "Error: hardware fault detected";
                return false;
            }
            else if (mode == yarp::dev::ControlModeEnum::VOCAB_CM_IDLE)
            {
                yCError(TINY_DANCER) << "Error: idle joint detected";
                return false;
            }
        }
    }

    return true;
}

// --------------------------------------------------------------- //
bool TinyDancer::setCtrlMode(const int part, yarp::dev::SelectableControlModeEnum ctrlMode)
{
    if(!m_parts_on[part])
    {
        yCWarning(TINY_DANCER, "Part %d is not available", part);
        return true;
    }

    size_t NUMBER_OF_JOINTS{0};
    vector<int> joints;
    vector<yarp::dev::SelectableControlModeEnum> modes;
    if (yarp::dev::ReturnValue ret = m_iposctrl[part]->getAxes(NUMBER_OF_JOINTS); !ret)
        yCError(TINY_DANCER) << "Unable to get the number of joints of part" << part << ":" << ret.toString();
    for (size_t i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    {
        joints.push_back(i_joint);
        modes.push_back(ctrlMode);
    }

    if (yarp::dev::ReturnValue ret = m_ictrlmode[part]->setControlModes(joints, modes); !ret)
        yCError(TINY_DANCER) << "Unable to set the control mode for part" << part << ":" << ret.toString();

    yarp::os::Time::delay(0.01);  // give time to update control modes value
    vector<yarp::dev::ControlModeEnum> currentModes;
    if (yarp::dev::ReturnValue ret = m_ictrlmode[part]->getControlModes(joints, currentModes); !ret)
        yCError(TINY_DANCER) << "Unable to get the control modes of part" << part << ":" << ret.toString();
    for (size_t i=0; i<NUMBER_OF_JOINTS; i++)
    {
        if(i >= currentModes.size() || static_cast<int>(currentModes[i]) != static_cast<int>(ctrlMode))
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
    if(!m_parts_on[part])
    {
        yCWarning(TINY_DANCER, "Part %d is not available", part);
        return true;
    }
    size_t NUMBER_OF_JOINTS{0};
    vector<int>    joints;
    vector<double> speeds;
    if (yarp::dev::ReturnValue ret = m_iposctrl[part]->getAxes(NUMBER_OF_JOINTS); !ret)
        yCError(TINY_DANCER) << "Unable to get the number of joints of part" << part << ":" << ret.toString();
    for (size_t i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    {
        double start{0.0}, goal{0.0};
        if (yarp::dev::ReturnValue ret = m_iencoder[part]->getEncoder(i_joint,&start); !ret)
        {
            yCError(TINY_DANCER) << "Unable to read encoder" << i_joint << "of part" << part << ":" << ret.toString();
            return false;
        }
        goal = joint_pos->get(i_joint).asFloat32();
        double disp = start - goal;
        if(disp<0.0) disp *= -1;

        joints.push_back(i_joint);
        speeds.push_back(disp/time);
    }

    yarp::dev::ReturnValue ret = m_iposctrl[part]->setTrajSpeeds(NUMBER_OF_JOINTS, joints.data(), speeds.data());
    if (!ret)
        yCError(TINY_DANCER) << "Unable to set the joint speeds of part" << part << ":" << ret.toString();
    return static_cast<bool>(ret);
}


// --------------------------------------------------------------- //
bool TinyDancer::movePart(const int part, Bottle* joint_pos)
{
    if(!m_parts_on[part])
    {
        yCWarning(TINY_DANCER, "Part %d is not available", part);
        return true;
    }
    size_t NUMBER_OF_JOINTS{0};
    std::vector<int>    joints;
    std::vector<double> positions;
    if (yarp::dev::ReturnValue ret = m_iposctrl[part]->getAxes(NUMBER_OF_JOINTS); !ret)
        yCError(TINY_DANCER) << "Unable to get the number of joints of part" << part << ":" << ret.toString();
    for (size_t i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
    {
        joints.push_back(i_joint);
        positions.push_back(joint_pos->get(i_joint).asFloat32());
    }

    yarp::dev::ReturnValue ret = m_iposctrl[part]->positionMove(NUMBER_OF_JOINTS, joints.data(), positions.data());
    if (!ret)
        yCError(TINY_DANCER) << "Unable to move part" << part << ":" << ret.toString();
    return static_cast<bool>(ret);
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
                        ok = ok && setCtrlMode(0, yarp::dev::SelectableControlModeEnum::VOCAB_CM_POSITION);
                        if (time >= 0)
                            ok = ok && setJointsSpeed(0, time, armR_joints);
                    }
                    if (armL_joints)
                    {
                        ok = ok && setCtrlMode(1, yarp::dev::SelectableControlModeEnum::VOCAB_CM_POSITION);
                        if (time >= 0)
                            ok = ok && setJointsSpeed(1, time, armL_joints);
                    }
                    if (head_joints)
                    {
                        ok = ok && setCtrlMode(2, yarp::dev::SelectableControlModeEnum::VOCAB_CM_POSITION);
                        if (time >= 0)
                            ok = ok && setJointsSpeed(2, time, head_joints);
                    }
                    if (torso_joints)
                    {
                        ok = ok && setCtrlMode(3, yarp::dev::SelectableControlModeEnum::VOCAB_CM_POSITION);
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

    if(m_drivers[TORSO].isValid())
    {
        m_drivers[TORSO].close();
    }

    yCInfo(TINY_DANCER, "Thread released");
}

