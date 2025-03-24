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
#ifndef SPEECH_SYNTHESIZER_H
#define SPEECH_SYNTHESIZER_H

#include <yarp/os/all.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/ISpeechSynthesizer.h>
#include <yarp/sig/Sound.h>

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace std;

class SpeechSynthesizer
{
private:

    bool                    m_active;

    PolyDriver              m_PolySpeech;
    ISpeechSynthesizer*     m_iSpeech = nullptr;

    BufferedPort<Sound>     m_audioOutPort;
    BufferedPort<Bottle>    m_textOutPort;

public:
    SpeechSynthesizer(){};
    ~SpeechSynthesizer() = default;

    bool configure(ResourceFinder &rf, string suffix);
    void close();
    bool say(const string& sentence);

    bool setLanguage(string& language);
    bool setVoice(string& voice);
    bool setPitch(double pitch);
    bool setSpeed(double speed);

};

#endif