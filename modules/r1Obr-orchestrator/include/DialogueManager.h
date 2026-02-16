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

#include <mutex>

#include <yarp/os/all.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IChatBot.h>
#include "speechSynthesizer.h"
#include <yarp/sig/AudioPlayerStatus.h>
#include <yarp/dev/ILLM.h>
#include <yarp/dev/ISpeechTranscription.h>
#include "DialogueMessage.h"

/**
 * @class DialogueManager
 * @brief Manages dialogue interactions between voice input and LLM-based responses
 *
 * This class serves as the core dialogue management system, handling:
 * - Voice command input reception and processing
 * - Communication with Large Language Models (LLM) for dialogue understanding
 * - Text-to-speech synthesis for robot responses
 * - Orchestration of robot actions based on dialogue intents
 * - Audio recording control and playback status monitoring
 * - Multi-language support for dialogue
 *
 * The system processes voice commands, interprets them using an LLM, and generates
 * appropriate robot actions or verbal responses. It maintains conversation context
 * and handles various dialogue states (SAY, GO, SEARCH, etc.) defined in DialogueMessage.
 */
class DialogueManager : public yarp::os::TypedReaderCallback<yarp::os::Bottle>
{

private:

    yarp::os::BufferedPort<Bottle>    m_voiceCommandPort;
    yarp::os::RpcClient               m_orchestratorRPCPort;
    yarp::os::RpcClient               m_audiorecorderRPCPort;
    yarp::os::RpcClient               m_wakeWordRPCPort;
    yarp::os::BufferedPort<Bottle>    m_audioPlayPort;

    yarp::dev::PolyDriver             m_polyLLM;
    yarp::dev::ILLM*                  m_iLlm = nullptr;

    yarp::dev::PolyDriver             m_polyLLMReplier;
    yarp::dev::ILLM*                  m_iLlmReplier = nullptr;

    yarp::dev::PolyDriver             m_polyTranscrption;
    yarp::dev::ISpeechTranscription*  m_iTranscription = nullptr;

    SpeechSynthesizer*                m_speaker;

    std::mutex                        m_mutex;
    std::string                       m_currentLanguage;
    std::string                       m_currentQuestion;  ///< Stored for context in replier LLM
    dlgmsg::DialogueMessage           m_currentLLMAnswer;

    // ========== Private Methods ==========
    /**
     * @brief Process message through the main LLM
     *
     * Sends a message to the main LLM and receives a structured dialogue response.
     * Parses the JSON response into a DialogueMessage object.
     *
     * @param msgIn The input message to process (typically user query or instruction)
     * @return DialogueMessage containing the LLM's response with command type and parameters
     */
    dlgmsg::DialogueMessage coreLLM(const std::string& msgIn);

    /**
     * @brief Convert DialogueMessage to YARP Bottle format
     *
     * Transforms a structured DialogueMessage into a YARP Bottle for RPC communication
     * with the orchestrator. Replaces spaces with underscores in parameters.
     *
     * @param msg The DialogueMessage to convert
     * @return A YARP Bottle formatted for orchestrator communication
     */
    yarp::os::Bottle fromMsgToBottle(const dlgmsg::DialogueMessage& msg);

    /**
     * @brief Handle invalid or unrecognized commands from LLM
     *
     * When the LLM returns an invalid command type, this method queries the LLM
     * to generate a user-friendly error message and speaks it using the synthesizer.
     */
    void manageInvalidCmd();

public:

    /**
     * @brief Default constructor
     */
    DialogueManager() = default;

    /**
     * @brief Default destructor
     */
    ~DialogueManager() = default;

    /**
     * @brief Configure the DialogueManager with parameters from ResourceFinder
     *
     * Initializes all ports, drivers, and devices needed for dialogue management.
     * Configures:
     * - Voice command input port
     * - Orchestrator RPC communication
     * - Audio recorder and wake word detection
     * - LLM drivers (main and replier)
     * - Speech transcription service
     * - Speech synthesizer
     * - Audio player status monitoring
     *
     * @param rf ResourceFinder containing configuration parameters from .ini file
     * @return true if configuration succeeds, false otherwise
     *
     * @note Configuration sections: DIALOGUE section in config file
     */
    bool configure(yarp::os::ResourceFinder &rf);

    /**
     * @brief Close all ports and cleanup resources
     *
     * Safely closes all YARP ports and drivers. Should be called before destruction.
     */
    void close();

    /**
     * @brief Callback for voice command input (inherited from TypedReaderCallback)
     *
     * Called when a new voice command arrives on the m_voiceCommandPort.
     * Extracts the command string and processes it through the dialogue manager.
     *
     * @param b The YARP Bottle containing the voice command
     */
    using yarp::os::TypedReaderCallback<yarp::os::Bottle>::onRead;
    virtual void onRead(yarp::os::Bottle& b) override;

    /**
     * @brief Process user input through the dialogue system
     *
     * Main dialogue processing loop that:
     * 1. Queries the main LLM with the user input
     * 2. Updates language if needed
     * 3. Executes the appropriate action based on command type
     * 4. Sends commands to orchestrator or generates Speech synthesis
     * 5. Seeds the replier LLM for context-aware responses
     *
     * @param msgIn The user input message (typically from voice transcription)
     */
    void interactWithDialogMng(const std::string& msgIn);

    /**
     * @brief Interact with the replier LLM for contextual responses
     *
     * Uses the replier LLM to generate more detailed or contextually aware responses
     * based on the main LLM's command. Automatically speaks the generated response.
     *
     * @param msgIn The DialogueMessage to seed the replier with
     * @param keepContext If true, maintains the conversation context from m_currentQuestion
     */
    void interactWithReplier(const dlgmsg::DialogueMessage& msgIn, bool keepContext = false);

    /**
     * @brief Synthesize and speak a message using the speech synthesizer
     *
     * Manages the complete speaking process:
     * - Waits for any current audio to finish
     * - Stops microphone recording
     * - Speaks the message
     * - Waits for speech to complete
     * - Restarts microphone recording
     *
     * @param toSay The text message to synthesize and speak
     */
    void speak(const std::string& toSay);

    /**
     * @brief Check if audio is currently playing
     *
     * Queries the audio player status port to determine if audio output is active.
     *
     * @param audio_is_playing Output parameter set to true if audio is playing
     * @return true if query succeeded, false if port not connected
     */
    bool audioIsPlaying(bool& audio_is_playing);

    /**
     * @brief Get the current dialogue language
     *
     * @return Const reference to the current language code (e.g., "it-IT")
     */
    [[nodiscard]] const std::string& getLanguage() const;
};

#endif //DIALOG_MNG_ORCHESTRATOR_H