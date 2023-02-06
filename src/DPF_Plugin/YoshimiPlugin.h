/*
    YoshimiPlugin

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

#ifndef YOSHIMI_PLUGIN_H
#define YOSHIMI_PLUGIN_H

#include "Misc/SynthEngine.h"
#include "Interface/InterChange.h"
#include "Interface/Data2Text.h"
#include "Interface/RingBuffer.h"

#include "DistrhoPlugin.hpp"
#include <memory>

// Forward decls.
class YoshimiMusicIO;

START_NAMESPACE_DISTRHO

class YoshimiPlugin : public Plugin {
    std::unique_ptr<SynthEngine> fSynthesizer;
    std::unique_ptr<YoshimiMusicIO> fMusicIo;
    bool fSynthInited, fMusicIoInited;

    String defaultState;

    // Let the UI side access DSP side (mainly for synth instance)
    friend class YoshimiEditor;

public:
    YoshimiPlugin();
    ~YoshimiPlugin();

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information

    /**
        Get the plugin label.@n
        This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const noexcept override
    {
        return "Yoshimi";
    }

    /**
        Get an extensive comment/description about the plugin.@n
        Optional, returns nothing by default.
    */
    const char* getDescription() const override
    {
        return "A sophisticated soft-synth originally forked from ZynAddSubFX";
    }

    /**
        Get the plugin author/maker.
    */
    const char* getMaker() const noexcept override
    {
        return "Andrew Deryabin";
    }

    /**
           Get the plugin license (a single line of text or a URL).@n
           For commercial plugins this should return some short copyright information.
         */
    const char* getLicense() const noexcept override
    {
        return "GPLv2";
    }

    /**
        Get the plugin version, in hexadecimal.
        @see d_version()
        */
    uint32_t getVersion() const noexcept override
    {
        return d_version(2, 2, 2);
    }

    /**
           Get the plugin unique Id.@n
           This value is used by LADSPA, DSSI and VST plugin formats.
           @see d_cconst()
         */
    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('y', 'o', 's', 'm');
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override;
    void initState(uint32_t index, State& state) override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override;
    void setParameterValue(uint32_t index, float value) override;
    String getState(const char* key) const;
    void setState(const char* key, const char* value) override;

    // ----------------------------------------------------------------------------------------------------------------
    // Audio/MIDI Processing

    void activate() override;
    void deactivate() override;
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks (optional)

    void sampleRateChanged(double newSampleRate) override;
    void bufferSizeChanged(uint32_t newBufferSize) override;

    // ----------------------------------------------------------------------------------------------------------------

private:
    char* _getState() const;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YoshimiPlugin)
};

END_NAMESPACE_DISTRHO

#endif
