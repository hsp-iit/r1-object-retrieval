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
#ifndef CHAT_BOT_ORCHESTRATOR_H
#define CHAT_BOT_ORCHESTRATOR_H

#include <yarp/os/all.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IChatBot.h>
#include "speechSynthesizer.h"
#include <yarp/sig/AudioPlayerStatus.h>
#include <yarp/dev/ILLM.h>

using namespace yarp::os;
using namespace yarp::dev;
using namespace std;

class ChatBot : public TypedReaderCallback<Bottle>
{

private:

    BufferedPort<Bottle>    m_voiceCommandPort;
    RpcClient               m_orchestratorRPCPort;
    RpcClient               m_audiorecorderRPCPort;
    BufferedPort<Bottle>    m_audioPlayPort;

    bool                    m_chatBot_active;
    PolyDriver              m_PolyCB;
    IChatBot*               m_iChatBot = nullptr;

    bool                    m_llm_active;
    PolyDriver              m_polyLLM;
    ILLM*                   m_iLlm = nullptr;

    SpeechSynthesizer*      m_speaker;

    string                  m_language_chatbot;

public:

    ChatBot() = default;
    ~ChatBot() = default;

    bool configure(ResourceFinder &rf);
    void close();

    //Port inherited from TypedReaderCallback
    using TypedReaderCallback<Bottle>::onRead;
    virtual void onRead(Bottle& b) override;

    void interactWithChatBot(const string& msgIn);

};

#endif