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
    fSynthesizer = std::make_unique<SynthEngine>(dummy, LV2PluginTypeSingle);   // Set as single-output plugin
    fSynthInited = true;

    fMusicIo = std::make_unique<YoshimiMusicIO>(&(*fSynthesizer), (uint32_t)getSampleRate(), (uint32_t)getBufferSize());    
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
    if (!fSynthesizer->getRuntime().isRuntimeSetupCompleted())
    {
        // TODO: How to stop processing if error occurs?
        fSynthesizer->getRuntime().LogError("Synthesizer runtime setup failed");
        fSynthesizer.reset();  // delete synth instance

        fSynthInited = false;
        return;
    }

    if (fMusicIo->hasInited())
    {
        /*
        * Perform further global initialisation.
        * For stand-alone the equivalent init happens in main(),
        * after mainCreateNewInstance() returned successfully.
        */
        fSynthesizer->installBanks();
        fSynthesizer->loadHistory();
    }
    else
    {
        fSynthesizer->getRuntime().LogError("Failed to create Yoshimi DPF plugin");
        fSynthesizer.reset();   // delete synth instance
        fMusicIo.reset();

        fSynthInited = false;
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
    //if (!flatbankprgs.empty())
    //{
    //    fMusicIO->getProgram(flatbankprgs.size() + 1);
    //}
    fSynthesizer->getRuntime().runSynth = false;
    fSynthesizer->getRuntime().Log("EXIT plugin");
    fSynthesizer->getRuntime().Log("Goodbye - Play again soon?");

    /*
    * fSynthesizer and fMusicIo will be automatically cleaned up by unique_ptr.
    */
}

void YoshimiPlugin::initState(uint32_t index, State& state) {
    /*
    * Yoshimi use 1 state to store configurations.
    */
    state.key = "state";
    state.defaultValue = defaultState;
}

String YoshimiPlugin::getState(const char* key) const {
    if (strcmp(key, "state") == 0)
    {
        return String(_getState(), false);
    }

    return String();
}

void YoshimiPlugin::setState(const char* key, const char* value) {
    if (strcmp(key, "state") == 0)
    {
        fSynthesizer->putalldata(value, sizeof(value));
    }
}

void YoshimiPlugin::initParameter(uint32_t index, Parameter& parameter) {
    // Yoshimi does not use parameters
    (void)index;
    (void)parameter;
}

float YoshimiPlugin::getParameterValue(uint32_t index) const {
    // Yoshimi does not use parameters
    return index;
}

void YoshimiPlugin::setParameterValue(uint32_t index, float value) {
    // Yoshimi does not use parameters
    (void)index;
    (void)value;
}

void YoshimiPlugin::run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    // Do not process if synth or MusicIO is not initialised.
    // otherwise plugin may crash.
    if (!fSynthInited || !fMusicIoInited)
        return;

    fMusicIo->process(inputs, outputs, frames, midiEvents, midiEventCount);
}

void YoshimiPlugin::bufferSizeChanged(uint32_t newBufferSize)
{
    fMusicIo->setBufferSize(newBufferSize);
}

void YoshimiPlugin::sampleRateChanged(double newSampleRate)
{
    fMusicIo->setSamplerate(newSampleRate);
}

void YoshimiPlugin::activate() {
    fMusicIo->Start();
}

void YoshimiPlugin::deactivate() {
    fMusicIo->Close();
}

char* YoshimiPlugin::_getState() const {
    char *data = nullptr;
    fSynthesizer->getalldata(&data);

    return data;
}

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new YoshimiPlugin();
}

END_NAMESPACE_DISTRHO

int mainCreateNewInstance(unsigned int) //stub
{
    return 0;
}


void mainRegisterAudioPort(SynthEngine *, int ) //stub
{
}
