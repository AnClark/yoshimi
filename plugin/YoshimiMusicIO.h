/*
    YoshimiMusicIO

    Copyright 2014, Andrew Deryabin <andrewderyabin@gmail.com>
    Copyright 2020-2021, Will Godfrey & others
    Copyright 2023, AnClark Liu <anclarkliu@outlook.com>

    This file is part of yoshimi, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 2 of the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef YOSHIMI_MUSICIO_H
#define YOSHIMI_MUSICIO_H

#include "MusicIO/MusicIO.h"

// Forward decls.
namespace DISTRHO {
    struct MidiEvent;
}

class YoshimiMusicIO : public MusicIO {
private:
    SynthEngine* _synth;
    uint32_t     _sampleRate;
    uint32_t     _bufferSize;
    bool         _inited;

    float* _bFreeWheel; // TODO: How to implement this?

public:
    YoshimiMusicIO(SynthEngine* synth, uint32_t initSampleRate, uint32_t initBufferSize);
    ~YoshimiMusicIO();

    // ----------------------------------------------------------------------------------------------------------------
    // Access from plugin interface
    bool hasInited() { return _inited; }
    void setSamplerate(uint32_t newSampleRate);
    void setBufferSize(uint32_t newBufferSize);

    // ----------------------------------------------------------------------------------------------------------------
    // Virtual methods from MusicIO
    unsigned int getSamplerate(void) { return _sampleRate; }
    int          getBuffersize(void) { return _bufferSize; }
    bool         Start(void) { return true; }
    void         Close(void) { ; }

    bool openAudio() { return true; }
    bool openMidi() { return true; }

    virtual std::string audioClientName(void) { return "DPF plugin"; }
    virtual int         audioClientId(void) { return 0; }
    virtual std::string midiClientName(void) { return "DPF plugin"; }
    virtual int         midiClientId(void) { return 0; }

    virtual void registerAudioPort(int) { }

    // ----------------------------------------------------------------------------------------------------------------
    // Process audio / MIDI
    void process(const float** inputs, float** outputs, uint32_t sample_count, const DISTRHO::MidiEvent* midi_events, uint32_t midi_event_count);
    void processMidiMessage(const uint8_t* msg);

private:
    // ----------------------------------------------------------------------------------------------------------------
    // Workarounds
    void _deinitSynthParts();
};

#endif
