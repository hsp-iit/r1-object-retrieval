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

#ifndef R1OBR_ORCHESTRATOR_H
#define R1OBR_ORCHESTRATOR_H

#include <yarp/os/all.h>
#include <yarp/sig/AudioPlayerStatus.h>

#include "orchestratorThread.h"
#include "speechSynthesizer.h"
#include "storyTeller.h"
#include <r1OrchestratorRPC.h>

using namespace yarp::os;
using namespace yarp::dev;
using namespace std;

class Orchestrator : public RFModule, public TypedReaderCallback<Bottle>, public r1OrchestratorRPC
{
private:

    //Callback thread
    OrchestratorThread*         m_inner_thread;

    //RPC Server
    RpcServer                   m_rpc_server_port;
    string                      m_rpc_server_port_name;

    //Input command port
    BufferedPort<Bottle>        m_input_port;
    string                      m_input_port_name;

    //Feedback port from positive outcome of the search
    BufferedPort<Bottle>        m_positive_feedback_port;
    string                      m_positive_feedback_port_name;

    //Status port
    BufferedPort<Bottle>        m_status_port;
    string                      m_status_port_name;

    //additional Speech Synthesizer
    SpeechSynthesizer*          m_additional_speaker;

    //storyTeller
    StoryTeller*                m_story_teller;

    //Thrift Server
    string                      m_orchestrator_thrift_port_name;
    Port                        m_orchestrator_thrift_port;

    //Others
    double                      m_period;
    RpcClient                   m_audiorecorderRPCPort;
    BufferedPort<Bottle>        m_audioPlayPort;

public:
    //Constructor/Destructor
    Orchestrator();

    //RFModule
    virtual bool configure(ResourceFinder &rf) override;
    virtual bool close() override;
    virtual double getPeriod() override;
    virtual bool updateModule() override;

    //inherited from TypedReaderCallback
    using TypedReaderCallback<Bottle>::onRead;
    void onRead(Bottle& b) override;

    bool respond(const Bottle &cmd, Bottle &reply) override;

    //inherited from r1OrchestratorRPC
    bool searchObject(const string& what)  override;
    bool searchObjectLocation(const string& what, const string& where)  override;
    bool stopSearch()  override;
    bool resetSearch()  override;
    bool resetHome()  override;
    bool resume()  override;
    string status()  override;
    string what()  override;
    string where()  override;
    bool navpos()  override;
    bool go(const string& location)  override;
    bool say(const string& toSay)  override;
    bool tell(const string& key)  override;
    bool dance(const string& motion)  override;
};

#endif
