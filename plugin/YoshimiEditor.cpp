/*
    YoshimiEditor

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

#include "YoshimiEditor.h"
#include "YoshimiPlugin.h"

#include "Exchange/Exchange.hpp"

// these two are both zero and repesented by an enum entry
constexpr unsigned char TYPE_READ = TOPLEVEL::type::Adjust;

YoshimiEditor::YoshimiEditor()
    : UI(600, 400)
    , fSynthesizer(nullptr)
    , fResizeHandle(this)
{
    // Get synth engine instance
    YoshimiPlugin* fDspInstance = (YoshimiPlugin*)UI::getPluginInstancePointer();
    fSynthesizer                = &(*fDspInstance->fSynthesizer);

    // hide handle if UI is resizable
    if (isResizable())
        fResizeHandle.hide();

    // Fetch initial params from synth side
    _fetchParams();
}

YoshimiEditor::~YoshimiEditor()
{
}

void YoshimiEditor::parameterChanged(uint32_t index, float value)
{
}

void YoshimiEditor::stateChanged(const char* key, const char* value)
{
    // TODO: Refresh UI when loading a new state
    d_stderr(">>> Stated changed. Should refresh UI!");

    // Refresh parameters
    _fetchParams();
}

void YoshimiEditor::onImGuiDisplay()
{
    const float width  = getWidth();
    const float height = getHeight();
    const float margin = 20.0f * getScaleFactor();

    ImGui::SetNextWindowPos(ImVec2(margin, margin));
    ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

    if (ImGui::Begin("Yoshimi Demo", nullptr, ImGuiWindowFlags_NoResize)) {
        static char aboutText[256] = "This is a demo UI for Yoshimi, based on Dear ImGui.\n";
        ImGui::InputTextMultiline("About", aboutText, sizeof(aboutText));

        if (ImGui::SliderFloat("Master Volume", &fParams.pVolume, 0.0f, 127.0f)) {
            // NOTICE: Methods from FLTK UI does not take effects. Use CLI-provided one instead.
            // YoshimiExchange::MasterUI::send_data(fSynthesizer, 0, MAIN::control::volume, fParams.pVolume, 0, TOPLEVEL::section::main);

            // Interpreted from CLI/CmdInterpreter.cpp:6103
            YoshimiExchange::sendNormal(fSynthesizer, 0, fParams.pVolume, TOPLEVEL::type::Write, MAIN::control::volume, TOPLEVEL::section::main);
        }

        if (ImGui::SliderFloat("Global Detune", &fParams.pGlobalDetune, 0.0f, 127.0f)) {
            YoshimiExchange::sendNormal(fSynthesizer, TOPLEVEL::action::lowPrio, fParams.pGlobalDetune, TOPLEVEL::type::Write, MAIN::control::detune, TOPLEVEL::section::main);
        }

        if (ImGui::SliderInt("Key Shift", &fParams.pKeyShift, -36, 36)) {
            YoshimiExchange::sendNormal(fSynthesizer, TOPLEVEL::action::lowPrio, fParams.pKeyShift, TOPLEVEL::type::Write, MAIN::control::keyShift, TOPLEVEL::section::main);
        }

        if (ImGui::IsItemDeactivated()) {
            _syncStateToHost();
        }
#if 0
        if (ImGui::SliderFloat("Gain (dB)", &fGain, -90.0f, 30.0f)) {
            if (ImGui::IsItemActivated())
                editParameter(0, true);

            setParameterValue(0, fGain);
        }

        if (ImGui::IsItemDeactivated()) {
            editParameter(0, false);
        }
#endif
    }

    ImGui::End();
}

// TODO:
// - Consider moving this to separate file
// - Implement returns_update() instead of simply fetching values (in case of outdated data)
//   * Reference: MasterUI::returns_update() (view its call hierachy)
void YoshimiEditor::_fetchParams()
{
    // Some default values can be directly accessed from synth instance
    fParams.pVolume       = fSynthesizer->Pvolume;
    fParams.pGlobalDetune = fSynthesizer->microtonal.Pglobalfinedetune;
    fParams.pKeyShift     = fSynthesizer->Pkeyshift - 64;
}

void YoshimiEditor::_syncStateToHost()
{
    char* data = nullptr;
    fSynthesizer->getalldata(&data);

    setState("state", data);

    delete data;
}

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new YoshimiEditor();
}

END_NAMESPACE_DISTRHO
