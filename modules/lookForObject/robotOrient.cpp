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

#include "robotOrient.h"


YARP_LOG_COMPONENT(ROBOT_ORIENT, "r1_obr.lookForObject.robotOrient")

RobotOrient::RobotOrient(yarp::os::ResourceFinder &rf) : m_rf(rf) 
{
    m_current_turn = 1;
    m_current_orient = 1;
}

// ********************************************** //
bool RobotOrient::configure()
{
    //Generic config
    m_period = m_rf.check("period")  ? m_rf.find("period").asFloat32() : 1.0;
    m_overlap = m_rf.check("fov_overlap_degrees")  ? m_rf.find("fov_overlap_degrees").asFloat32() : 5.0;
    m_turning = m_rf.check("turning")  ? !(m_rf.find("turning").asString() == "false") : true;

    bool useFov = m_rf.check("useCameraFOV") ? m_rf.find("useCameraFOV").asString()=="true" : false;

    // --------- RGBDSensor config --------- //
    Property rgbdProp;
    // Prepare default prop object
    rgbdProp.put("device", RGBDClient);
    rgbdProp.put("localImagePort", RGBDLocalImagePort);
    rgbdProp.put("localDepthPort", RGBDLocalDepthPort);
    rgbdProp.put("localRpcPort", RGBDLocalRpcPort);
    rgbdProp.put("remoteImagePort", RGBDRemoteImagePort);
    rgbdProp.put("remoteDepthPort", RGBDRemoteDepthPort);
    rgbdProp.put("remoteRpcPort", RGBDRemoteRpcPort);
    rgbdProp.put("ImageCarrier", RGBDImageCarrier);
    rgbdProp.put("DepthCarrier", RGBDDepthCarrier);
    bool okRgbdRf = m_rf.check("RGBD_SENSOR_CLIENT");
    if(!okRgbdRf)
    {
        yCWarning(ROBOT_ORIENT,"RGBD_SENSOR_CLIENT section missing in ini file. Using default values");
    }
    else
    {
        Searchable& rgbd_config = m_rf.findGroup("RGBD_SENSOR_CLIENT");
        if(rgbd_config.check("device")) {rgbdProp.put("device", rgbd_config.find("device").asString());}
        if(rgbd_config.check("localImagePort")) {rgbdProp.put("localImagePort", rgbd_config.find("localImagePort").asString());}
        if(rgbd_config.check("localDepthPort")) {rgbdProp.put("localDepthPort", rgbd_config.find("localDepthPort").asString());}
        if(rgbd_config.check("localRpcPort")) {rgbdProp.put("localRpcPort", rgbd_config.find("localRpcPort").asString());}
        if(rgbd_config.check("remoteImagePort")) {rgbdProp.put("remoteImagePort", rgbd_config.find("remoteImagePort").asString());}
        if(rgbd_config.check("remoteDepthPort")) {rgbdProp.put("remoteDepthPort", rgbd_config.find("remoteDepthPort").asString());}
        if(rgbd_config.check("remoteRpcPort")) {rgbdProp.put("remoteRpcPort", rgbd_config.find("remoteRpcPort").asString());}
        if(rgbd_config.check("ImageCarrier")) {rgbdProp.put("ImageCarrier", rgbd_config.find("ImageCarrier").asString());}
        if(rgbd_config.check("DepthCarrier")) {rgbdProp.put("DepthCarrier", rgbd_config.find("DepthCarrier").asString());}
    }

    m_rgbdPoly.open(rgbdProp);
    if(!m_rgbdPoly.isValid())
    {
        yCError(ROBOT_ORIENT,"Error opening PolyDriver check parameters");
        return false;
    }
    m_rgbdPoly.view(m_iRgbd);
    if(!m_iRgbd)
    {
        yCError(ROBOT_ORIENT,"Error opening iRGBD interface. Device not available");
        return false;
    }
    

    // ----------- Polydriver config ----------- //
    Property controllerProp;
    if(!m_rf.check("REMOTE_CONTROL_BOARD"))
    {
        yCWarning(ROBOT_ORIENT,"REMOTE_CONTROL_BOARD section missing in ini file. Using default values.");

        // defaults
        controllerProp.clear();
        controllerProp.put("device","remote_controlboard");
        controllerProp.put("local","/lookForObject/robotOrient/remote_controlboard");
        controllerProp.put("remote","/cer/head");
    }
    else
    {
        Searchable& pos_configs = m_rf.findGroup("REMOTE_CONTROL_BOARD");
        if(pos_configs.check("device")) {controllerProp.put("device", pos_configs.find("device").asString());}
        if(pos_configs.check("local")) {controllerProp.put("local", pos_configs.find("local").asString());}
        if(pos_configs.check("remote")) {controllerProp.put("remote", pos_configs.find("remote").asString());}
    }

    if (!m_Poly.open(controllerProp))
    {
        yCError(ROBOT_ORIENT,"Unable to connect to remote");
        close();
        return false;
    }
    
    m_Poly.view(m_iremcal);
    if(!m_iremcal)
    {
        yCError(ROBOT_ORIENT,"Error opening iRemoteCalibrator interface. Device not available");
        return false;
    }

    m_Poly.view(m_iposctrl);
    if(!m_iposctrl)
    {
        yCError(ROBOT_ORIENT,"Error opening iPositionControl interface. Device not available");
        return false;
    }

    m_Poly.view(m_ictrlmode);
    if(!m_ictrlmode)
    {
        yCError(ROBOT_ORIENT,"Error opening iControlMode interface. Device not available");
        return false;
    }

    m_Poly.view(m_ilimctrl);
    if(!m_ilimctrl)
    {
        yCError(ROBOT_ORIENT,"Error opening IControlLimits interface. Device not available");
        return false;
    }

    // ----------- Configure Head Positions ----------- //
    map<string, pair<double,double>>  orientations_default{
        {"pos01" , {0.0, 0.0}    },
        {"pos02" , {35.0, 0.0}   },
        {"pos03" , {-35.0, 0.0}  },
        {"pos04" , {0.0, 20.0}   },
        {"pos05" , {35.0, 20.0}  },
        {"pos06" , {-35.0, 20.0} },
        {"pos07" , {0.0, -20.0}  },
        {"pos08" , {35.0, -20.0} },
        {"pos09" , {-35.0, -20.0}}   };

    double visual_span;
    
    if (!useFov) 
    {
        if(!m_rf.check("HEAD_POSITIONS"))
        {
            yCWarning(ROBOT_ORIENT,"HEAD_POSITIONS section missing in ini file. Using default values.");

            m_orientations = orientations_default;
            visual_span = 70.0;
        }
        else
        {
            Searchable& pos_configs = m_rf.findGroup("HEAD_POSITIONS");
            double maxDeg {0.0}, minDeg {0.0}; 
            int idx {1};
            while (true) 
            {
                string pos = "pos" + (string)(idx<10?"0":"") + to_string(idx);
                if (!pos_configs.check(pos))
                    break;

                stringstream  ss(pos_configs.find(pos).asString());
                string s; 
                ss >> s; double n1 = stod(s);
                ss >> s; double n2 = stod(s);            

                m_orientations.insert({pos, {n1,n2} });
                idx++;

                maxDeg = n1>maxDeg ? n1 : maxDeg;
                minDeg = n1<minDeg ? n1 : minDeg;
            }
            visual_span = maxDeg-minDeg;
        }
    }
    else 
    {
        double verticalFov{0.0}, horizontalFov{0.0};
        double newVFov{0.0}, newHFov{0.0};
        bool fovGot = m_iRgbd->getRgbFOV(horizontalFov,verticalFov);
        if(!fovGot)
        {
            yCError(ROBOT_ORIENT,"An error occurred while retrieving the rgb camera FOV");
        }
        else
        {
            //consider the width of the fov to inspect an area of 180 degrees laterally and 90 degrees vertically
            // overlapping the views by at least <m_overlap> degrees
            newHFov = horizontalFov>60 ?  (90.0 - horizontalFov/2) : (horizontalFov-m_overlap) ;
            newVFov = verticalFov>30 ?  (45.0 - verticalFov/2) : (verticalFov-m_overlap) ;
        }
        
        double min_pos_h {0}, min_pos_v {0};
        double max_pos_h {100}, max_pos_v {100};

        bool limGotV = m_ilimctrl->getLimits(0, &min_pos_v, &max_pos_v); //pitch
        bool limGotH = m_ilimctrl->getLimits(1, &min_pos_h, &max_pos_h); //yaw
        if(!limGotV || !limGotH)
        {
            yCError(ROBOT_ORIENT,"An error occurred while retrieving the head joint limits");
        }

        double up = limGotV ? min(fovGot ? newVFov : 20.0 , max_pos_v) : (fovGot ? newVFov : 20.0);
        double down = limGotV ? max(fovGot ? -newVFov : -20.0 , min_pos_v) : (fovGot ? -newVFov : -20.0);
        double left = limGotH ? min(fovGot ? newHFov : 35.0 , max_pos_h) : (fovGot ? newHFov : 35.0);;
        double right = limGotH ? max(fovGot ? -newHFov : -35.0 , min_pos_h) : (fovGot ? -newHFov : -35.0); 

        m_orientations = {
            {"pos01", {0.0,  0.0}  },
            {"pos02", {left, 0.0}  },   //left
            {"pos03", {right,0.0}  },   //right
            {"pos04", {0.0,  up}   },   //up
            {"pos05", {left, up}   },   //up left
            {"pos06", {right,up}   },   //up right
            {"pos07", {0.0,  down} },   //down
            {"pos08", {left, down} },   //down left
            {"pos09", {right,down} }};  //down right

        visual_span = left-right;
    }

    double _v_, horizontalFov{0.0};
    if(!m_iRgbd->getRgbFOV(horizontalFov,_v_))
        yCError(ROBOT_ORIENT,"An error occurred while retrieving the rgb camera FOV");
    
    m_max_turns = ceil(360.0/(visual_span+horizontalFov));
    m_turn_deg = 360.0/m_max_turns;

    return true;
}

// ********************************************** //
bool RobotOrient::next(Bottle& reply)
{
    lock_guard<mutex> m_lock(m_mutex);
    string posname = "pos" + (string)(m_current_orient<10?"0":"") + to_string(m_current_orient);
    if (m_orientations.find(posname) != m_orientations.end())
    {
        Bottle& tempList = reply.addList();
        pair<double,double> tempPair = m_orientations.at(posname);
        tempList.addFloat32(tempPair.first);
        tempList.addFloat32(tempPair.second);
        m_current_orient++;
        return true;
    }  
    else
    {
        reply.addVocab32(Vocab32::encode("nack"));
        return false;
    }
}

// ********************************************** //
void RobotOrient::help()
{
    yCInfo(ROBOT_ORIENT,"next : responds the next unchecked orientation or, if not present, returns false");
    yCInfo(ROBOT_ORIENT,"home : let the robot head go back to its home position");
    yCInfo(ROBOT_ORIENT,"turn : returns the angle the robot should turn to inspect another area, or 'noTurn'");
    yCInfo(ROBOT_ORIENT,"help : gets this list");
}

// ********************************************** //
bool RobotOrient::turn(Bottle& reply)
{ 
    lock_guard<mutex> m_lock(m_mutex);
    if (m_turning && m_current_turn<m_max_turns)
    {
        reply.addFloat32(m_turn_deg);
        m_current_turn++;
    }
    else
        reply.addString("noTurn");
    
    
    return true;
}

// ********************************************** //
void RobotOrient::resetOrients()
{ 
    m_current_orient = 1;
}

// ********************************************** //
void RobotOrient::resetTurns()
{ 
    m_current_turn = 1;
}

// ********************************************** //
void RobotOrient::home()
{    
    // ------ set head to Position control mode ------ //
    int NUMBER_OF_JOINTS;
    m_iposctrl->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++){ m_ictrlmode->setControlMode(i_joint, VOCAB_CM_POSITION); }    
    
    // ------ homing command  ------ //
    bool ok = m_iremcal->homingWholePart();
    if (!ok)  
    {
        bool isCalib = false;
        m_iremcal->isCalibratorDevicePresent(&isCalib);
        if (!isCalib) // providing better feedback to user by verifying if the calibrator device was set or not. 
                      // (SIM_CER_ROBOT does not have a calibrator!)
        { yCError(ROBOT_ORIENT) << "Error: No calibrator device was configured to perform this action, please verify that the wrapper config file for the part has the 'Calibrator' keyword in the attach phase"; }
        else
        { yCError(ROBOT_ORIENT) << "Error: The remote calibrator reported that something went wrong during the calibration procedure"; }
    }
    
}

// ********************************************** //
bool RobotOrient::close()
{
    if(m_Poly.isValid())
        m_Poly.close();

    if(m_rgbdPoly.isValid())
        m_rgbdPoly.close();
    
    return true;
}
