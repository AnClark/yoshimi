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

#include "YoshimiPlugin.h"
#include "YoshimiMusicIO.h"

YoshimiPlugin::YoshimiPlugin()
    : Plugin(0, 0, 1) // parameters, programs, states
{
    /*
     * Initialize synthesizer and MusicIO.
     */
    std::list<string> dummy;
    fSynthesizer = std::make_unique<SynthEngine>(dummy, LV2PluginTypeSingle); // Set as single-output plugin
    fSynthInited = true;

    fMusicIo       = std::make_unique<YoshimiMusicIO>(&(*fSynthesizer), (uint32_t)getSampleRate(), (uint32_t)getBufferSize());
    fMusicIoInited = true;

    // Adaptered from YoshimiLV2Plugin constructor.
    // that constructor mainly configures LV2 interface and buffer size.
    fSynthesizer->setBPMAccurate(true);

    /*
     * Setup runtime.
     *
     * Adapted from YoshimiLV2Plugin::instantiate() (static method).
     * That function is the entrance of LV2 plugin instance.
     */
    if (!fSynthesizer->getRuntime().isRuntimeSetupCompleted()) {
        // TODO: How to stop processing if error occurs?
        fSynthesizer->getRuntime().LogError("Synthesizer runtime setup failed");
        fSynthesizer.reset(); // delete synth instance

        fSynthInited = false;
        return;
    }

    if (fMusicIo->hasInited()) {
        /*
         * Perform further global initialisation.
         * For stand-alone the equivalent init happens in main(),
         * after mainCreateNewInstance() returned successfully.
         */
        fSynthesizer->installBanks();
        fSynthesizer->loadHistory();
    } else {
        fSynthesizer->getRuntime().LogError("Failed to create Yoshimi DPF plugin");
        fSynthesizer.reset(); // delete synth instance
        fMusicIo.reset();

        fSynthInited   = false;
        fMusicIoInited = false;
        return;
    }

    /*
     * Initialize default state value.
     */
    defaultState = _getState();

    fSynthesizer->getRuntime().Log("Now Yoshimi is ready!");
}

YoshimiPlugin::~YoshimiPlugin()
{
    /*
     * Since synthesizer instance is managed by unique_ptr, we can assume it exists.
     * No need to check if it is empty or not (fSynthesizer != NULL is always satisfied).
     */
    // if (!flatbankprgs.empty())
    //{
    //     fMusicIO->getProgram(flatbankprgs.size() + 1);
    // }
    fSynthesizer->getRuntime().runSynth = false;
    fSynthesizer->getRuntime().Log("EXIT plugin");
    fSynthesizer->getRuntime().Log("Goodbye - Play again soon?");

    /*
     * fSynthesizer and fMusicIo will be automatically cleaned up by unique_ptr.
     */
}

// ----------------------------------------------------------------------------------------------------------------
// Init

void YoshimiPlugin::initState(uint32_t index, State& state)
{
    /*
     * Yoshimi use 1 state to store configurations.
     */

    YOSHIMI_INIT_SAFE_CHECK()

    state.key          = "state";
    state.defaultValue = defaultState;
}

void YoshimiPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    // Yoshimi does not use parameters
    (void)index;
    (void)parameter;
}

// ----------------------------------------------------------------------------------------------------------------
// Internal data

String YoshimiPlugin::getState(const char* key) const
{
    YOSHIMI_INIT_SAFE_CHECK(String())

    if (strcmp(key, "state") == 0) {
        return String(_getState(), false);
    }

    return String();
}

void YoshimiPlugin::setState(const char* key, const char* value)
{
    YOSHIMI_INIT_SAFE_CHECK()

    if (strcmp(key, "state") == 0) {
        fSynthesizer->putalldata(value, sizeof(value));
    }
}

float YoshimiPlugin::getParameterValue(uint32_t index) const
{
    // Yoshimi does not use parameters
    return index;
}

void YoshimiPlugin::setParameterValue(uint32_t index, float value)
{
    // Yoshimi does not use parameters
    (void)index;
    (void)value;
}

// ----------------------------------------------------------------------------------------------------------------
// Audio/MIDI Processing

void YoshimiPlugin::activate()
{
    YOSHIMI_INIT_SAFE_CHECK()

    fMusicIo->Start();
}

void YoshimiPlugin::deactivate()
{
    YOSHIMI_INIT_SAFE_CHECK()

    fMusicIo->Close();
}

void YoshimiPlugin::run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    YOSHIMI_INIT_SAFE_CHECK()

    fMusicIo->process(inputs, outputs, frames, midiEvents, midiEventCount);
}

// ----------------------------------------------------------------------------------------------------------------
// Callbacks

void YoshimiPlugin::bufferSizeChanged(uint32_t newBufferSize)
{
    /*
     * Buffer size changes MUST be handled properly!
     * See: YoshimiMusicIO::setBufferSize().
     */

    YOSHIMI_INIT_SAFE_CHECK()

    // Back up all states
    const char* state_backup(_getState());

    // Reinit synth engine with new buffer size
    fMusicIo->setBufferSize(newBufferSize);

    // Restore states
    setState("state", state_backup);
}

void YoshimiPlugin::sampleRateChanged(double newSampleRate)
{
    /*
     * Sample rate changes MUST be handled properly!
     * See: YoshimiMusicIO::setSampleRate().
     */

    YOSHIMI_INIT_SAFE_CHECK()

    // Back up all states
    const char* state_backup(_getState());

    // Reinit synth engine with new sample rate
    fMusicIo->setSamplerate(newSampleRate);

    // Restore states
    setState("state", state_backup);
}

// ----------------------------------------------------------------------------------------------------------------
// Internal helpers

char* YoshimiPlugin::_getState() const
{
    char* data = nullptr;
    fSynthesizer->getalldata(&data);

    return data;
}

// ----------------------------------------------------------------------------------------------------------------
// Plugin entry point

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new YoshimiPlugin();
}

END_NAMESPACE_DISTRHO

// ----------------------------------------------------------------------------------------------------------------
// Yoshimi entry points (stub, needed when linking)

int mainCreateNewInstance(unsigned int) // stub
{
    return 0;
}

void mainRegisterAudioPort(SynthEngine*, int) // stub
{
}
