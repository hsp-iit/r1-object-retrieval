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


#include "r1Obr-orchestrator.h"

YARP_LOG_COMPONENT(R1OBR_ORCHESTRATOR, "r1_obr.orchestrator")


Orchestrator::Orchestrator() :
    m_period(1.0)
{
    m_rpc_server_port_name  = "/r1Obr-orchestrator/rpc";
    m_input_port_name = "/r1Obr-orchestrator/input:i";
    m_status_port_name = "/r1Obr-orchestrator/status:o";
    m_positive_feedback_port_name = "/r1Obr-orchestrator/positive_outcome_feedback:i";
    m_orchestrator_thrift_port_name = "/r1Obr-orchestrator/thrift:s";
}


/****************************************************************/
bool Orchestrator::configure(ResourceFinder &rf)
{
    if(rf.check("period")){m_period = rf.find("period").asFloat32();}

    // --------- Open RPC Server Port --------- //
    if(rf.check("input_rpc_port"))
        m_rpc_server_port_name = rf.find("input_rpc_port").asString();
    if (!m_rpc_server_port.open(m_rpc_server_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Could not open rpc port %s, check network", m_rpc_server_port_name.c_str());
        return false;
    }
    if (!attach(m_rpc_server_port))
    {
        yCError(R1OBR_ORCHESTRATOR, "attach() error with rpc port %s", m_rpc_server_port_name.c_str());
        return false;
    }


    // --------- Thrift interface server side config --------- //
    if(rf.check("thrift_server_port"))  m_orchestrator_thrift_port_name = rf.find("thrift_server_port").asString();
    if (!m_orchestrator_thrift_port.open(m_orchestrator_thrift_port_name))
    {
        yCWarning(R1OBR_ORCHESTRATOR, "Error! Cannot open the thrift port");
        return false;
    }

    if (!this->yarp().attachAsServer(m_orchestrator_thrift_port))
    {
        yCWarning(R1OBR_ORCHESTRATOR, "Error! Cannot attach port %s as a server", m_orchestrator_thrift_port_name.c_str());
        return false;
    }


    // --------- Open Input command Port --------- //
    if(rf.check("input_port"))
        m_input_port_name = rf.find("input_port").asString();
    if (!m_input_port.open(m_input_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open input port");
        return false;
    }


    // --------- Open Positive Feedback Port --------- //
    if(rf.check("positive_feedback_port"))
        m_positive_feedback_port_name = rf.find("positive_feedback_port").asString();
    if (!m_positive_feedback_port.open(m_positive_feedback_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open positive feedback port");
        return false;
    }
    m_positive_feedback_port.useCallback(*this);


    // --------- Thread initialization --------- //
    m_inner_thread = new OrchestratorThread(rf);
    bool threadOk = m_inner_thread->start();
    if (!threadOk){
        return false;
    }
    m_input_port.useCallback(*m_inner_thread);


    // --------- Status output ----------- //
    if(rf.check("status_port")){m_status_port_name = rf.find("status_port").asString();}
    if (!m_status_port.open(m_status_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open output status port");
        return false;
    }

    // --------- SpeechSynthesizer config --------- //
    m_additional_speaker = new SpeechSynthesizer();
    if(!m_additional_speaker->configure(rf, "/second"))
    {
        yCError(R1OBR_ORCHESTRATOR,"SpeechSynthesizer configuration failed");
        return false;
    }

    string audiorecorderRPCPortName = "/r1Obr-orchestrator/microphone:rpc";
    if(!m_audiorecorderRPCPort.open(audiorecorderRPCPortName))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open RPC port to audio recorder");
        return false;
    }

    string audioplayerStatusPortName = "/r1Obr-orchestrator/audioplayerStatus:i";
    if(!m_audioPlayPort.open(audioplayerStatusPortName))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open audio player status port");
        return false;
    }


    // --------- Story Teller --------- //
    m_story_teller = new StoryTeller();
    if(!m_story_teller->configure(rf))
    {
        yCError(R1OBR_ORCHESTRATOR,"StoryTeller configuration failed");
        return false;
    }

    return true;
}


/****************************************************************/
bool Orchestrator::close()
{
    if (m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();

    if (!m_input_port.isClosed())
        m_input_port.close();

    if (!m_positive_feedback_port.isClosed())
        m_positive_feedback_port.close();

    if (!m_status_port.isClosed())
        m_status_port.close();

    if (m_audiorecorderRPCPort.asPort().isOpen())
        m_audiorecorderRPCPort.close();

    if (!m_audioPlayPort.isClosed())
        m_audioPlayPort.close();

    if (m_orchestrator_thrift_port.isOpen())
        m_orchestrator_thrift_port.close();

    m_inner_thread->stop();
    delete m_inner_thread;

    m_additional_speaker->close();
    delete m_additional_speaker;

    delete m_story_teller;

    return true;
}


/****************************************************************/
double Orchestrator::getPeriod()
{
    return m_period;
}


/****************************************************************/
bool Orchestrator::updateModule()
{
    Bottle&  status = m_status_port.prepare();
    status.clear();
    m_inner_thread->info(status);
    m_status_port.write();

    if (isStopping())
    {
        if (m_inner_thread) m_inner_thread->stop();
        return false;
    }

    return true;
}


/****************************************************************/
bool Orchestrator::respond(const Bottle &request, Bottle &reply)
{
    yCInfo(R1OBR_ORCHESTRATOR,"RPC Received: %s",request.toString().c_str());

    reply.clear();
    string cmd=request.get(0).asString();
    if (request.size()==1)
    {
        if (cmd=="help")
        {
            reply.addVocab32("many");
            reply.addString("help   : gets this list");
            reply.addString("search <what> : starts to search for 'what'");
            reply.addString("search <what> <where>: starts searching for 'what' at location 'where'");
            reply.addString("stop   : stops search");
            reply.addString("reset  : resets search");
            reply.addString("reset_home: resets search and navigates the robot home");
            reply.addString("resume : resumes stopped search");
            reply.addString("status : returns the current status of the search");
            reply.addString("what   : returns the object of the current search");
            reply.addString("where  : returns the location of the current search");
            reply.addString("info   : returns the status, object and location of the current search");
            reply.addString("navpos : sets the robot in navigation position");
            reply.addString("go <location> : navigates the robot to 'location' if it is valid");
            reply.addString("say <sentence>: the sentence is synthesized and played by the audio player");
            reply.addString("tell <key>    : a previously stored sentence is accessed through the corresponfing key and it is played by the audio player");
            reply.addString("dance <motion>: make the robot move as defined in the corresponding ini file containing the choreography");

        }
        else if (cmd=="status")
        {
            reply.addString(m_inner_thread->getStatus());
        }
        else if (cmd=="stop" || cmd=="reset")
        {
            reply.addString(m_inner_thread->stopOrReset("ext_"+cmd));
        }
        else if (cmd=="reset_home")
        {
            reply.addString(m_inner_thread->resetHome());
        }
        else if (cmd=="resume")
        {
            reply.addString(m_inner_thread->resume());
        }
        else if (cmd=="what")
        {
            reply.addString(m_inner_thread->getWhat());
        }
        else if (cmd=="where")
        {
            reply.addString(m_inner_thread->getWhere());
        }
        else if (cmd=="navpos")
        {
            if(m_inner_thread->setNavigationPosition())
            {
                reply.addString("Navpos set");
            }
            else
            {
                reply.addString("Navpos NOT set");
            }
        }
        else if (cmd=="info")
        {
            m_inner_thread->info(reply);
        }
        else if (cmd=="cmd_unk")
        {
            reply.addVocab32(Vocab32::encode("ack"));
            yCWarning(R1OBR_ORCHESTRATOR,"Uknown command received");
            m_inner_thread->askChatBotToSpeak(OrchestratorThread::cmd_unknown);
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCError(R1OBR_ORCHESTRATOR,"Error: wrong RPC command. Type 'help'");
            m_inner_thread->askChatBotToSpeak(OrchestratorThread::fallback);
        }
    }
    else if (cmd=="search")
    {
        m_inner_thread->search(request);
        m_inner_thread->setObject(request.get(1).asString());
        reply.addString("searching for '" + request.get(1).asString() + "'");
    }
    else if (cmd=="say")
    {
        string toSay = request.get(1).asString();
        reply.addString("speaking");
        say(toSay);
    }
    else if (cmd=="go")
    {
        string location_name = request.get(1).asString();;
        m_inner_thread->go(location_name);
        reply.addString("going to '" + location_name + "'");
    }
    else if (cmd=="tell")
    {
        string story;
        string story_key = request.get(1).asString();
        m_story_teller->getStory(story_key, story);
        reply.addString("Telling the story");
        say(story);
    }
    else if (cmd=="dance")
    {
        string dance_name = request.get(1).asString();
        m_inner_thread->dance(dance_name);
        reply.addString("dancing");
    }
    else if(cmd=="guide")
    {
        string location_name = request.get(1).asString();
        m_inner_thread->guide(location_name);
        reply.addString("guiding to '" + location_name + "'");
    }
    else if(cmd=="directions")
    {
        string location_name = request.get(1).asString();
        /// TODO: implement getting directions
        reply.addString("getting directions to '" + location_name + "'");
    }
    else
    {
        reply.addVocab32(Vocab32::encode("nack"));
        yCWarning(R1OBR_ORCHESTRATOR,"Error: wrong RPC command. Type 'help'");
        m_inner_thread->askChatBotToSpeak(OrchestratorThread::fallback);
    }

    if (reply.size()==0)
        reply.addVocab32(Vocab32::encode("ack"));

    return true;
}

/****************************************************************/
void Orchestrator::onRead(yarp::os::Bottle &b)
{
    if(b.size() == 2)
    {
        yCInfo(R1OBR_ORCHESTRATOR, "Received confirmation that the object has been found");
        Time::delay(2.0); //TEMPORARY: just to have a delayed vocal feedback when object found
        m_inner_thread->objectFound();
    }
    else if (b.size() == 1)
    {
        yCWarning(R1OBR_ORCHESTRATOR, "The object cannot be found anymore");
        m_inner_thread->objectActuallyNotFound();
    }

}


/****************************************************************/
bool Orchestrator::searchObject(const string& what)
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received: search %s", what.c_str());

    Bottle request;
    request.fromString("search " + what);
    m_inner_thread->search(request);
    m_inner_thread->setObject(what);

    return status()!="idle"; //return false if orchestrator status is idle
}

/****************************************************************/
bool Orchestrator::searchObjectLocation(const string& what, const string& where)
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received: search %s , %s", what.c_str(), where.c_str());

    Bottle request;
    request.fromString("search " + what + " " + where);
    m_inner_thread->search(request);
    m_inner_thread->setObject(what);

    return status()!="idle"; //return false if orchestrator status is idle
}

/****************************************************************/
bool Orchestrator::stopSearch()
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received: stop");
    m_inner_thread->stopOrReset("ext_stop");

    return status()=="idle";  //return false if orchestrator status is NOT idle
}

/****************************************************************/
bool Orchestrator::resetSearch()
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received: reset");
    m_inner_thread->stopOrReset("ext_reset");

    return status()=="idle";  //return false if orchestrator status is NOT idle
}

/****************************************************************/
bool Orchestrator::resetHome()
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received: resetHome");
    return m_inner_thread->resetHome()=="reset and sent home";
}

/****************************************************************/
bool Orchestrator::resume()
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received: resume");
    return m_inner_thread->resume()!="not resumed";
}

/****************************************************************/
string Orchestrator::status()
{
    return m_inner_thread->getStatus();
}

/****************************************************************/
string Orchestrator::what()
{
    return m_inner_thread->getWhat();
}

/****************************************************************/
string Orchestrator::where()
{
    return m_inner_thread->getWhere();
}

/****************************************************************/
bool Orchestrator::navpos()
{
    return m_inner_thread->setNavigationPosition();
}

/****************************************************************/
bool Orchestrator::go(const string& location)
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received: go %s", location.c_str());
    return m_inner_thread->go(location);
}

/****************************************************************/
bool Orchestrator::say(const string& toSay)
{
    //close microphone
    Bottle req{"stopRecording_RPC"}, rep;
    m_audiorecorderRPCPort.write(req,rep);

    //speak
    bool ret = m_additional_speaker->say(toSay);


    //wait until finish speaking
    Time::delay(0.5);
    bool audio_is_playing{true};
    while (audio_is_playing)
    {
        Bottle* player_status = m_audioPlayPort.read(false);
        if (player_status)
        {
            audio_is_playing = player_status->get(1).asInt64() > 0;
        }
        Time::delay(0.1);
    }

    //re-open microphone
    req.clear(); rep.clear();
    req.addString("startRecording_RPC");
    m_audiorecorderRPCPort.write(req,rep);

    return ret;
}

/****************************************************************/
bool Orchestrator::tell(const string& key)
{
    string story;
    m_story_teller->getStory(key, story);
    return say(story);
}

/****************************************************************/
bool Orchestrator::dance(const string& motion)
{
    return m_inner_thread->dance(motion);
}
