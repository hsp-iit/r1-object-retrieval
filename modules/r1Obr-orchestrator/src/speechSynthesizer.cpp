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

#include "speechSynthesizer.h"

YARP_LOG_COMPONENT(SPEECH_SYNTHESIZER, "r1_obr.orchestrator.speechSynthesizer")


// ------------------------------------------------------ //
bool SpeechSynthesizer::configure(ResourceFinder &rf, string suffix)
{
    //Defaults
    string audioOutPort_name = "/r1Obr-orchestrator/speech_sythesizer" + suffix + "/audio:o";
    string textOutPort_name  = "/r1Obr-orchestrator/speech_sythesizer" + suffix + "/text:o";
    string device_nwc = "speechSynthesizer_nwc_yarp";
    string local_nwc = "/r1Obr-orchestrator/speech_sythesizer" + suffix;
    string remote_nwc = "/speechSynthesizer_nws";
    string language = "auto";
    string voice = "auto";
    double pitch = 0.0;
    double speed = 1.0;

    Property speechProp;
    if(!rf.check("SPEECH_SYNTHESIZER"))
    {
        yCWarning(SPEECH_SYNTHESIZER,"SPEECH_SYNTHESIZER section missing in ini file. Using the default values");
    }
    Searchable& speech_config = rf.findGroup("SPEECH_SYNTHESIZER");
    m_active = speech_config.check("active") ? (speech_config.find("active").asString() == "true") : false;

    if(m_active)
    {
        if(speech_config.check("audio_out_port")) audioOutPort_name = speech_config.find("audio_out_port").asString() + suffix;
        if (!m_audioOutPort.open(audioOutPort_name)){
            yCError(SPEECH_SYNTHESIZER) << "cannot open port" << audioOutPort_name;
            return false;
        }

        if(speech_config.check("text_out_port")) textOutPort_name = speech_config.find("text_out_port").asString() + suffix;
        if (!m_textOutPort.open(textOutPort_name)){
            yCError(SPEECH_SYNTHESIZER) << "cannot open port" << textOutPort_name;
            return false;
        }

        speechProp.put("device", speech_config.check("device", Value(device_nwc)));
        speechProp.put("local",  speech_config.check("local") ?  Value(speech_config.find("local").asString() + suffix) : Value(local_nwc)) ;
        speechProp.put("remote", speech_config.check("remote", Value(remote_nwc)));
        m_PolySpeech.open(speechProp);
        if(!m_PolySpeech.isValid())
        {
            yCWarning(SPEECH_SYNTHESIZER,"Error opening PolyDriver check parameters. Using the default values");
        }
        m_PolySpeech.view(m_iSpeech);
        if(!m_iSpeech){
            yCError(SPEECH_SYNTHESIZER,"Error opening ISpeechSynthesizer interface. Device not available");
            return false;
        }

        if(speech_config.check("language")) language = speech_config.find("language").asString();
        if(speech_config.check("voice")) voice = speech_config.find("voice").asString();
        if(speech_config.check("pitch")) pitch = speech_config.find("pitch").asFloat32();
        if(speech_config.check("speed")) speed = speech_config.find("speed").asFloat32();

        setLanguage(language);
        setVoice(voice);
        setPitch(pitch);
        setSpeed(speed);
    }

    return true;
}


// ------------------------------------------------------ //
void SpeechSynthesizer::close()
{
    if(m_PolySpeech.isValid())
        m_PolySpeech.close();

    if(!m_audioOutPort.isClosed())
        m_audioOutPort.close();

    if(!m_textOutPort.isClosed())
        m_textOutPort.close();

    yCInfo(SPEECH_SYNTHESIZER, "Speech synthesizer thread released");
}


// ------------------------------------------------------ //
bool SpeechSynthesizer::say(const string& sentence)
{
    if (!m_active)
    {
        yCWarning(SPEECH_SYNTHESIZER, "Speech synthesizer is not active");
        return false;
    }

    Sound& soundToSend = m_audioOutPort.prepare();
    soundToSend.clear();
    if(ReturnValue ret = m_iSpeech->synthesize(sentence,soundToSend); !ret)
    {
        yCError(SPEECH_SYNTHESIZER) << "Some error occurred synthesizing input string:" << ret.toString();
        return false;
    }
    m_audioOutPort.write();

    Bottle& textToSend = m_textOutPort.prepare();
    textToSend.clear();
    textToSend.addString(sentence);
    m_textOutPort.write();

    return true;
}


// ------------------------------------------------------ //
bool SpeechSynthesizer::setLanguage(string& language)
{
    ReturnValue ret = m_iSpeech->setLanguage(language);
    if (!ret)
        yCError(SPEECH_SYNTHESIZER) << "Unable to set the language to" << language << ":" << ret.toString();
    return static_cast<bool>(ret);
}


// ------------------------------------------------------ //
bool SpeechSynthesizer::setVoice(string& voice)
{
    ReturnValue ret = m_iSpeech->setVoice(voice);
    if (!ret)
        yCError(SPEECH_SYNTHESIZER) << "Unable to set the voice to" << voice << ":" << ret.toString();
    return static_cast<bool>(ret);
}


// ------------------------------------------------------ //
bool SpeechSynthesizer::setPitch(double pitch)
{
    ReturnValue ret = m_iSpeech->setPitch(pitch);
    if (!ret)
        yCError(SPEECH_SYNTHESIZER) << "Unable to set the pitch to" << pitch << ":" << ret.toString();
    return static_cast<bool>(ret);
}


// ------------------------------------------------------ //
bool SpeechSynthesizer::setSpeed(double speed)
{
    ReturnValue ret = m_iSpeech->setSpeed(speed);
    if (!ret)
        yCError(SPEECH_SYNTHESIZER) << "Unable to set the speed to" << speed << ":" << ret.toString();
    return static_cast<bool>(ret);
}



