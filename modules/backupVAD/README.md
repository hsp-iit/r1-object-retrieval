# backupVAD

## General description
In case the main VAD could not be used for this project, this module can be a valid alternative way of detecting the voice of the user.
Two devices are opened: an audio Recorder and an audio Player.
Between these two, the VAD listens to the audio from the audio Recorder and extracts just the audio parts where it detects someone speaking, returning those to the audio Player.

## Software Requirements
- Download and install the library libfvad:  
```
git clone https://github.com/dpirch/libfvad (or alternatively https://github.com/elandini84/libfvad) 
cd libfvad && autoreconf -i && ./configure && sudo make install
```
- Download and install the library libsoxr: 
```
git clone https://git.code.sf.net/p/soxr/code soxr-code
cd soxr-code && ./go && cd Release && sudo make install
```
- install the alsa-utils and the sound card libraries: 
```
    sudo apt-get update
    sudo apt-get install alsa-tools alsa-utils
    sudo apt-get install libasound-dev portaudio19-dev libportaudio2 libportaudiocpp0
```
- re-build yarp, activating ccmake options portaudio, portaudioRecorder and portaudioPlayer

:warning: The backupVAD module can be run on a Docker container, while the audio-Recorder and Player devices (yarprobotinterface) should be run on a physical pc.
