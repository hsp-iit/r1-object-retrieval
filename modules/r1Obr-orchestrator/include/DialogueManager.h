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
#ifndef DIALOG_MNG_ORCHESTRATOR_H
#define DIALOG_MNG_ORCHESTRATOR_H

#include <yarp/os/all.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IChatBot.h>
#include "speechSynthesizer.h"
#include <yarp/sig/AudioPlayerStatus.h>
#include <yarp/dev/ILLM.h>
#include "DialogueMessage.h"

class DialogueManager : public yarp::os::TypedReaderCallback<yarp::os::Bottle>
{

private:

    yarp::os::BufferedPort<Bottle>  m_voiceCommandPort;
    yarp::os::RpcClient             m_orchestratorRPCPort;
    yarp::os::RpcClient             m_audiorecorderRPCPort;
    yarp::os::BufferedPort<Bottle>  m_audioPlayPort;

    yarp::dev::PolyDriver           m_polyLLM;
    yarp::dev::ILLM*                m_iLlm = nullptr;

    yarp::dev::PolyDriver           m_polyLLMReplier;
    yarp::dev::ILLM*                m_iLlmReplier = nullptr;

    SpeechSynthesizer*              m_speaker;

    std::string                     m_currentLanguage;
    std::string                     m_currentQuestion;
    DialogueMessage                 m_currentLLMAnswer;

    DialogueMessage coreLLM(const std::string& msgIn);
    yarp::os::Bottle fromMsgToBottle(const DialogueMessage& msg);
    void manageInvalidCmd();

public:

    DialogueManager() = default;
    ~DialogueManager() = default;

    bool configure(yarp::os::ResourceFinder &rf);
    void close();

    //Port inherited from TypedReaderCallback
    using yarp::os::TypedReaderCallback<yarp::os::Bottle>::onRead;
    virtual void onRead(yarp::os::Bottle& b) override;

    void interactWithDialogMng(const std::string& msgIn);
    void interactWithReplier(const std::string& msgIn, bool keepContext = false);
    void speak(const std::string& toSay);
    bool audioIsPlaying(bool& audio_is_playing);
};

#endif //DIALOG_MNG_ORCHESTRATOR_H