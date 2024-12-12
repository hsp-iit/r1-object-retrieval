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

#include "DialogueManager.h"
#include <algorithm>
#include <yarp/dev/LLM_Message.h>

YARP_LOG_COMPONENT(DIALOG_MNG_ORCHESTRATOR, "r1_obr.orchestrator.DialogueManager")


// ****************************************************** //

bool DialogueManager::configure(ResourceFinder &rf)
{
    // Defaults
    std::string voiceCommandPortName     = "/r1Obr-orchestrator/voice_command:i";
    std::string orchestratorRPCPortName  = "/r1Obr-orchestrator/dialogMng:rpc";
    std::string audiorecorderRPCPortName = "/r1Obr-orchestrator/dialogMng/microphone:rpc";
    std::string audioplayerStatusPortName= "/r1Obr-orchestrator/dialogMng/audioplayerStatus:i";
    std::string local_chatBot_nwc        = "/r1Obr-orchestrator/dialogMng";
    std::string m_currentLanguage       = "it-IT";

    if(!rf.check("DIALOGUE"))
    {
        yCWarning(DIALOG_MNG_ORCHESTRATOR,"DIALOGUE section missing in ini file. Using the default values");
    }

    Searchable& config = rf.findGroup("DIALOGUE");


    // ------------------  out ------------------ //
    if(config.check("rpc_orchestrator_port")) {orchestratorRPCPortName = config.find("rpc_orchestrator_port").asString();}
    if(!m_orchestratorRPCPort.open(orchestratorRPCPortName))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open Chat Bot RPC port to orchestrator");
        return false;
    }
    std::string orchstrator_rpc_server_port_name = rf.check("input_rpc_port", Value("/r1Obr-orchestrator/rpc")).asString();
    bool ok = Network::connect(orchestratorRPCPortName.c_str(), orchstrator_rpc_server_port_name.c_str());
    if (!ok)
    {
        yCError(DIALOG_MNG_ORCHESTRATOR,"Could not connect %s to %s", orchestratorRPCPortName.c_str(), orchstrator_rpc_server_port_name.c_str());
        return false;
    }

    if(config.check("rpc_microphone_port")) {audiorecorderRPCPortName = config.find("rpc_microphone_port").asString();}
    if(!m_audiorecorderRPCPort.open(audiorecorderRPCPortName))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open Chat Bot RPC port to audio recorder");
        return false;
    }

    // ------------------  in  ------------------ //

    // Voice Command Port
    if(config.check("voice_command_port")) {voiceCommandPortName = config.find("voice_command_port").asString();}
    if (!m_voiceCommandPort.open(voiceCommandPortName))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open voice command port");
        return false;
    }
    m_voiceCommandPort.useCallback(*this);

    // ------------ LLM configuration ------------ //
    std::string llm_local{"/r1Obr-orchestrator/dialogMng/llm/rpc"}, llm_remote{"/LLM_nws/rpc"};
    Property prop;
    if(config.check("llm_local")) { llm_local = config.find("llm_local").asString();}
    if(config.check("llm_remote")) { llm_remote = config.find("llm_remote").asString();}
    prop.put("device", "LLM_nwc_yarp");
    prop.put("local", llm_local);
    prop.put("remote", llm_remote);
    if (!m_polyLLM.open(prop)) {
        yCError(DIALOG_MNG_ORCHESTRATOR) << "Cannot open LLM_nwc_yarp";
        return false;
    }

    if (!m_polyLLM.view(m_iLlm)) {
        yCError(DIALOG_MNG_ORCHESTRATOR) << "Cannot open interface from driver";
        return false;
    }

    m_iLlm->refreshConversation();

    // ------------ LLM Replier configuration ------------ //
    std::string llm_replier_local{"/r1Obr-orchestrator/dialogMng/llm_replier/rpc"}, llm_replier_remote{"/LLM_nws/rpc"};
    Property prop_replier;
    if(config.check("llm_replier_local")) { llm_replier_local = config.find("llm_replier_local").asString();}
    if(config.check("llm_replier_remote")) { llm_replier_remote = config.find("llm_replier_remote").asString();}
    prop_replier.put("device", "LLM_nwc_yarp");
    prop_replier.put("local", llm_replier_local);
    prop_replier.put("remote", llm_replier_remote);
    if (!m_polyLLMReplier.open(prop_replier)) {
        yCError(DIALOG_MNG_ORCHESTRATOR) << "Cannot open replier LLM_nwc_yarp";
        return false;
    }

    if (!m_polyLLMReplier.view(m_iLlmReplier)) {
        yCError(DIALOG_MNG_ORCHESTRATOR) << "Cannot open interface from replier driver";
        return false;
    }

    m_iLlmReplier->refreshConversation();


    // ------------ Audio Player Status Port ------------ //
    if(config.check("audioplayer_input_port")) {audioplayerStatusPortName = config.find("audioplayer_input_port").asString();}
    if(!m_audioPlayPort.open(audioplayerStatusPortName))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open audio player status port");
        return false;
    }


    // --------- SpeechSynthesizer config --------- //
    m_speaker = new SpeechSynthesizer();
    if(!m_speaker->configure(rf, ""))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR,"SpeechSynthesizer configuration failed");
        return false;
    }

    // --------- SpeechTranscription_nwc config --------- //
    std::string transcription_local{"/r1Obr-orchestrator/dialogMng/transcription/rpc"}, transcription_remote{"/speechTranscription_nws/rpc"};
    Property propTranscription;
    if(config.check("transcription_local")) { transcription_local = config.find("transcription_local").asString();}
    if(config.check("transcription_remote")) { transcription_remote = config.find("transcription_remote").asString();}
    propTranscription.put("device", "speechTranscription_nwc_yarp");
    propTranscription.put("local", transcription_local);
    propTranscription.put("remote", transcription_remote);
    if (!m_polyTranscrption.open(propTranscription)) {
        yCError(DIALOG_MNG_ORCHESTRATOR) << "Cannot open speechTranscription_nwc_yarp";
        return false;
    }
    if (!m_polyTranscrption.view(m_iTranscription)) {
        yCError(DIALOG_MNG_ORCHESTRATOR) << "Cannot open interface from transcription driver";
        return false;
    }

    return true;
}


// ****************************************************** //
void DialogueManager::close()
{
    if(!m_voiceCommandPort.isClosed())
        m_voiceCommandPort.close();

    if (m_orchestratorRPCPort.asPort().isOpen())
        m_orchestratorRPCPort.close();

    if (m_audiorecorderRPCPort.asPort().isOpen())
        m_audiorecorderRPCPort.close();

    if (!m_audioPlayPort.isClosed())
        m_audioPlayPort.close();

    if(m_polyLLM.isValid())
        m_polyLLM.close();

    m_speaker->close();
    delete m_speaker;
}


// ****************************************************** //
void DialogueManager::onRead(Bottle& b)
{
    std::string str = b.toString();

    if(str == "")
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Empty std::string received");
        return;
    }

    interactWithDialogMng(str);
}


// ****************************************************** //
void DialogueManager::interactWithDialogMng(const std::string& msgIn)
{
    if (msgIn == "")
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Empty message in received");
        return;
    }

    yarp::os::Bottle toOrchestrator;
    yarp::os::Bottle reply;

    dlgmsg::DialogueMessage replyMsg = coreLLM(msgIn);
    std::string language = replyMsg.getLanguage();
    if (language != m_currentLanguage)
    {
        m_currentLanguage = language;
        m_speaker->setLanguage(m_currentLanguage);
        m_iTranscription->setLanguage(m_currentLanguage);
    }
    dlgmsg::CmdTypes cmdType = replyMsg.getType();
    switch(cmdType){
        case dlgmsg::CmdTypes::INVALID: {
            manageInvalidCmd();
            break;
        }
        case dlgmsg::CmdTypes::SAY: {
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "SAY is the way";
            std::string toSay = replyMsg.getParams()[0];
            speak(toSay);
            break;
        }
        default: {
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "DEFAULT is the way";
            toOrchestrator = fromMsgToBottle(replyMsg);
            m_orchestratorRPCPort.write(toOrchestrator, reply);
            dlgmsg::DialogueMessage orchMsg = replyMsg;
            orchMsg.setQuery(msgIn);
            orchMsg.setComment(reply.toString());
            interactWithReplier(orchMsg);
            break;
        }
    }

    m_currentQuestion = msgIn;
    m_currentLLMAnswer = replyMsg;
}

void DialogueManager::manageInvalidCmd()
{
    yCInfo(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::interactWithDialogMng. Unknown command received from LLM.");
    std::string notify = "notify user: \"Unknown command received from LLM.\" Use language code: " + m_currentLanguage;
    dlgmsg::DialogueMessage replyMsg = coreLLM(notify);

    while(replyMsg.getType() != dlgmsg::CmdTypes::SAY)
    {
        replyMsg = coreLLM(notify);
        yarp::os::Time::delay(0.5);
    }
    speak(replyMsg.getParams()[0]);
}

yarp::os::Bottle DialogueManager::fromMsgToBottle(const dlgmsg::DialogueMessage& msg)
{
    yarp::os::Bottle toOrchestrator;
    std::vector<std::string> params = msg.getParams();
    std::string args;
    std::string command;
    command = msg.getTypeAsString();
    for(int i = 0; i < params.size(); i++)
    {
        std::string param = params[i];
        replace(param.begin(), param.end(), ' ', '_'); //replacing spaces
        args += " " + param;
    }
    toOrchestrator.clear();
    toOrchestrator.fromString(command+args);

    return toOrchestrator;
}

dlgmsg::DialogueMessage DialogueManager::coreLLM(const std::string& msgIn)
{
    yarp::dev::LLM_Message answer;
    m_iLlm->ask(msgIn, answer);
    dlgmsg::DialogueMessage replyMsg;
    if(answer.type != "assistant")
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::interactWithDialogMng. Unexpected answer type from LLM.");
        replyMsg.setType(dlgmsg::CmdTypes::INVALID);
        return replyMsg;
    }
    yCInfo(DIALOG_MNG_ORCHESTRATOR, "Contacting LLM. LLM answered: %s", answer.content.c_str());
    
    nlohmann::json replyJson = nlohmann::json::parse(answer.content);
    dlgmsg::from_json(replyJson, replyMsg);

    return replyMsg;
}

void DialogueManager::speak(const std::string& toSay)
{
    yCInfo(DIALOG_MNG_ORCHESTRATOR, "Saying: %s", toSay.c_str());

    bool audio_is_playing{true};
    while (audio_is_playing)
    {
        if(!audioIsPlaying(audio_is_playing)){
            audio_is_playing = true;
        }
        Time::delay(0.1);
    }

    //close microphone
    yarp::os::Bottle req{"stopRecording_RPC"};
    m_audiorecorderRPCPort.write(req);

    //speak
    m_speaker->say(toSay);
    while (!audio_is_playing)
    {
        if(!audioIsPlaying(audio_is_playing)){
            audio_is_playing = false;
        }
        Time::delay(0.1);
    }

    //wait until finish speaking
    Time::delay(0.5);
    audio_is_playing = true;
    while (audio_is_playing)
    {
        if(!audioIsPlaying(audio_is_playing)){
            audio_is_playing = true;
        }
        Time::delay(0.1);
    }

    //re-open microphone
    req.clear();
    req.addString("startRecording_RPC");
    m_audiorecorderRPCPort.write(req);
}


bool DialogueManager::audioIsPlaying(bool& audio_is_playing)
{
    if(m_audioPlayPort.getInputCount()<1)
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Audio player status port not connected");
        return false;
    }
    yarp::os::Bottle* player_status = m_audioPlayPort.read(false);
    if (player_status)
    {
        audio_is_playing = (unsigned int)player_status->get(1).asInt64() > 0;
        return true;
    }

    return false;
}


void DialogueManager::interactWithReplier(const dlgmsg::DialogueMessage& msgIn, bool keepContext)
{
    nlohmann::json replyJson;
    dlgmsg::to_json(replyJson, msgIn);
    yCInfo(DIALOG_MNG_ORCHESTRATOR, "Seeding replier with: %s", replyJson.dump().c_str());
    dlgmsg::DialogueMessage replyMsg{msgIn.getType(), msgIn.getParams(), msgIn.getLanguage()};
    if (replyMsg.getType() == dlgmsg::CmdTypes::INVALID)
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::interactWithReplier. Unexpected answer type from LLM.");
        manageInvalidCmd();
        return;
    }
    if(keepContext)
    {
        replyMsg.setQuery(m_currentQuestion);
    }

    replyJson = replyMsg;
    yarp::dev::LLM_Message answer;
    m_iLlmReplier->ask(replyJson.dump(), answer);
    if(answer.type != "assistant")
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::interactWithReplier. Unexpected answer type from LLM.");
        manageInvalidCmd();
        return;
    }

    yCInfo(DIALOG_MNG_ORCHESTRATOR, "Contacting LLM. LLM answered: %s", answer.content.c_str());
    speak(answer.content);

    return;
}

const std::string& DialogueManager::getLanguage() const
{
    return m_currentLanguage;
}
