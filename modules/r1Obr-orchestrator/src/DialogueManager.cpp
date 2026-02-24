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


bool DialogueManager::configure(ResourceFinder &rf)
{
    // Set default port names and language before reading config
    std::string voiceCommandPortName     = "/r1Obr-orchestrator/voice_command:i";
    std::string orchestratorRPCPortName  = "/r1Obr-orchestrator/dialogMng:rpc";
    std::string audiorecorderRPCPortName = "/r1Obr-orchestrator/dialogMng/microphone:rpc";
    std::string wakeWordRPCPortName = "/r1Obr-orchestrator/dialogMng/wakeWord:rpc";
    std::string audioplayerStatusPortName= "/r1Obr-orchestrator/dialogMng/audioplayerStatus:i";
    std::string local_chatBot_nwc        = "/r1Obr-orchestrator/dialogMng";
    std::string m_currentLanguage       = "it-IT";

    if(!rf.check("DIALOGUE"))
    {
        yCWarning(DIALOG_MNG_ORCHESTRATOR,"DIALOGUE section missing in ini file. Using the default values");
    }

    Searchable& config = rf.findGroup("DIALOGUE");

    // Open RPC port to orchestrator and establish connection
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

    // Open RPC port for wake word detection control
    if(config.check("rpc_wakeword_port")) {wakeWordRPCPortName = config.find("rpc_wakeword_port").asString();}
    if(!m_wakeWordRPCPort.open(wakeWordRPCPortName))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open Chat Bot RPC port to wakeword");
        return false;
    }

    // ------------------  in  ------------------ //

    // Open input port for receiving voice commands from transcription system
    // Voice Command Port
    if(config.check("voice_command_port")) {voiceCommandPortName = config.find("voice_command_port").asString();}
    if (!m_voiceCommandPort.open(voiceCommandPortName))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open voice command port");
        return false;
    }
    m_voiceCommandPort.useCallback(*this);

    // Configure main LLM driver for command interpretation
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

    // Configure replier LLM - uses a separate LLM instance for context-aware detailed responses
    // This allows maintaining two independent conversation contexts
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

    // Setup audio player status monitoring
    if(config.check("audioplayer_input_port")) {audioplayerStatusPortName = config.find("audioplayer_input_port").asString();}
    if(!m_audioPlayPort.open(audioplayerStatusPortName))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open audio player status port");
        return false;
    }


    // Initialize speech synthesizer
    m_speaker = new SpeechSynthesizer();
    if(!m_speaker->configure(rf, ""))
    {
        yCError(DIALOG_MNG_ORCHESTRATOR,"SpeechSynthesizer configuration failed");
        return false;
    }

    // Configure speech transcription service
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


void DialogueManager::onRead(Bottle& b)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string str = b.get(0).asString();

    if(str == "" || str == " ")
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Empty std::string received");
        // Skip empty commands and restart microphone
        yarp::os::Bottle req;
        req.clear();
        req.addString("startRecording_RPC");
        m_audiorecorderRPCPort.write(req);
        return;
    }

    interactWithDialogMng(str);
}


// ****************************************************** //
void DialogueManager::interactWithDialogMng(const std::string& msgIn)
{
    yCInfo(DIALOG_MNG_ORCHESTRATOR) << "----------------------";
    yCInfo(DIALOG_MNG_ORCHESTRATOR) << "Dialog got:" << msgIn;
    yCInfo(DIALOG_MNG_ORCHESTRATOR) << "----------------------";
    if (msgIn == "")
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Empty message in received");
        return;
    }

    yarp::os::Bottle toOrchestrator;
    yarp::os::Bottle reply;

    // Query main LLM to interpret user command and get structured response
    dlgmsg::DialogueMessage replyMsg = coreLLM(msgIn);

    // Update language settings if different from current
    std::string language = replyMsg.getLanguage();
    if (language != m_currentLanguage)
    {
        m_currentLanguage = language;
        m_speaker->setLanguage(m_currentLanguage);
        m_iTranscription->setLanguage(m_currentLanguage);
    }

    // Execute appropriate action based on LLM command type
    dlgmsg::CmdTypes cmdType = replyMsg.getType();
    switch(cmdType){
        // Invalid command - query LLM again to generate user-friendly error message
        case dlgmsg::CmdTypes::INVALID: {
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "INVALID is the way" << msgIn;
            manageInvalidCmd();
            break;
        }
        case dlgmsg::CmdTypes::SAY: {
            // Direct speech response - synthesize and speak
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "SAY is the way" << msgIn;
            std::string toSay = replyMsg.getParams()[0];
            speak(toSay);
            break;
        }
        case dlgmsg::CmdTypes::IGNORE: {
            // Ignore command - restart recording without further processing
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "IGNORE is the way" << msgIn;
            //re-open microphone
            yarp::os::Bottle req;
            req.clear();
            req.addString("startRecording_RPC");
            m_audiorecorderRPCPort.write(req);
            return;
        }
        case dlgmsg::CmdTypes::FAREWELL: {
            // Farewell - stop wake word detection and reset LLM conversation contexts
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "FAREWELL is the way" << msgIn;
            yarp::os::Bottle toWakeWord;
            toWakeWord.clear();
            toWakeWord.addString("stop");
            m_wakeWordRPCPort.write(toWakeWord,reply);
            if (!reply.isNull() && reply.get(0).asString() == "nack")
            {
                yCError(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::interactWithDialogMng. Orchestrator returned NACK.");
                return;
            }
            m_iLlm->refreshConversation();
            m_iLlmReplier->refreshConversation();
        }
        case dlgmsg::CmdTypes::GRASP: {
            // Grasping - send command to /gb-new-task/rpc:i
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "GRASP is the way" << msgIn;
            yarp::os::Bottle command, response;
            command.addString("start_video");
            command.addString("grasp_position");
            yarp::os::RpcClient gbNewTaskPort;
            if (!gbNewTaskPort.open("/dialogueManager/gbNewTask:rpc")) {
                yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to open RPC client port for /gb-new-task/rpc:i");
                return;
            }
            if (!yarp::os::Network::connect(gbNewTaskPort.getName(), "/gb-new-task/rpc:i")) {
                yCError(DIALOG_MNG_ORCHESTRATOR, "Unable to connect to /gb-new-task/rpc:i");
                gbNewTaskPort.close();
                return;
            }
            gbNewTaskPort.write(command, response);
            yCInfo(DIALOG_MNG_ORCHESTRATOR, "Response from /gb-new-task/rpc:i: %s", response.toString().c_str());
            gbNewTaskPort.close();
            break;
        }
        default: {
            // Execute command through orchestrator, get response, and seed replier LLM
            yCWarning(DIALOG_MNG_ORCHESTRATOR) << "DEFAULT is the way" << msgIn;
            toOrchestrator = fromMsgToBottle(replyMsg);
            m_orchestratorRPCPort.write(toOrchestrator, reply);
            if (!reply.isNull() && reply.get(0).asString() == "nack")
            {
                yCError(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::interactWithDialogMng. Orchestrator returned NACK.");
                return;
            }
            dlgmsg::DialogueMessage orchMsg = replyMsg;
            orchMsg.setQuery(msgIn);
            orchMsg.setComment(reply.toString());
            yCInfo(DIALOG_MNG_ORCHESTRATOR) << "----------------------";
            yCInfo(DIALOG_MNG_ORCHESTRATOR) << "Replier from dialoguemanager" << msgIn;
            yCInfo(DIALOG_MNG_ORCHESTRATOR) << "----------------------";
            interactWithReplier(orchMsg);
            if(cmdType != dlgmsg::CmdTypes::RESUME &&
               cmdType != dlgmsg::CmdTypes::STOP)
            {
                m_currentQuestion = msgIn;
            }
            break;
        }
    }

    m_currentLLMAnswer = replyMsg;
}

void DialogueManager::manageInvalidCmd()
{
    // Query LLM to generate user-friendly error message in current language
    yCInfo(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::interactWithDialogMng. Unknown command received from LLM.");
    std::string notify = "notify user: \"Unknown command received from LLM.\" Use language code: " + m_currentLanguage;
    dlgmsg::DialogueMessage replyMsg = coreLLM(notify);

    // Retry until LLM returns a SAY command
    while(replyMsg.getType() != dlgmsg::CmdTypes::SAY)
    {
        replyMsg = coreLLM(notify);
        yarp::os::Time::delay(0.5);
    }
    speak(replyMsg.getParams()[0]);
}

yarp::os::Bottle DialogueManager::fromMsgToBottle(const dlgmsg::DialogueMessage& msg)
{
    // Convert DialogueMessage to YARP Bottle format for RPC communication
    // Format: command_type param1 param2 ... (spaces in params replaced with underscores)
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

    yCWarning(DIALOG_MNG_ORCHESTRATOR, "DialogueManager::fromMsgToBottle. Bottle toOrchestrator: %s", toOrchestrator.toString().c_str());

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

    // Wait for any previous audio playback to finish
    bool audio_is_playing{true};
    while (audio_is_playing)
    {
        if(!audioIsPlaying(audio_is_playing)){
            audio_is_playing = true;
        }
        Time::delay(0.1);
    }

    // Stop microphone recording before speaking (avoid echo/interference)
    yarp::os::Bottle red_rec{"isRecording_RPC"};
    yarp::os::Bottle reply;
    yarp::os::Bottle req_stop{"stopRecording_RPC"};
    reply.clear();
    m_audiorecorderRPCPort.write(red_rec, reply);
    yCInfo(DIALOG_MNG_ORCHESTRATOR, "isReconrding_RPC reply: %s", reply.toString().c_str());
    if(reply.get(1).asString() == "ok")
    {
        yCInfo(DIALOG_MNG_ORCHESTRATOR, "Microphone is recording, stopping it");
        reply.clear();
        m_audiorecorderRPCPort.write(req_stop,reply);
    }
    else
    {
        yCInfo(DIALOG_MNG_ORCHESTRATOR, "Microphone is not recording, no need to stop it");
    }

    // Synthesize and play speech
    m_speaker->say(toSay);

    // Wait for speech to start playing (audio_is_playing becomes true)
    while (!audio_is_playing)
    {
        if(!audioIsPlaying(audio_is_playing)){
            audio_is_playing = false;
        }
        Time::delay(0.1);
    }

    // Wait for speech to finish playing
    Time::delay(0.5);
    audio_is_playing = true;
    while (audio_is_playing)
    {
        if(!audioIsPlaying(audio_is_playing)){
            audio_is_playing = true;
        }
        Time::delay(0.1);
    }

    // Restart microphone recording for next command
    yarp::os::Bottle req;
    req.clear();
    req.addString("startRecording_RPC");
    m_audiorecorderRPCPort.write(req);
}


bool DialogueManager::audioIsPlaying(bool& audio_is_playing)
{
    // Check if audio player status port is connected
    if(m_audioPlayPort.getInputCount()<1)
    {
        yCError(DIALOG_MNG_ORCHESTRATOR, "Audio player status port not connected");
        return false;
    }

    // Non-blocking read of audio player status
    // Extracts boolean flag from the second element of the status message
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
    // Serialize command for replier LLM - replier receives both command and orchestrator response as context
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

    // Optionally add conversation context (current user question) for better replies
    if(keepContext)
    {
        replyMsg.setQuery(m_currentQuestion);
    }

    // Query replier LLM with full command context for detailed, aware responses
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
