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

#include "MasterUI.h"

YoshimiEditor::YoshimiEditor()
    : UI(600, 300)
    , fSynthesizer(nullptr)
    , fMasterUI(nullptr)
    , fUiInited(false)
{
    // Get synth engine instance
    YoshimiPlugin* fDspInstance = (YoshimiPlugin*)UI::getPluginInstancePointer();
    fSynthesizer                = &(*fDspInstance->fSynthesizer);

    // Set GUI close callback
    // TODO: May not needed on DPF?
    // fSynthesizer->setGuiClosedCallback(YoshimiLV2PluginUI::static_guiClosed, this);

    /*
     * Adapted from YoshimiLV2PluginUI::show().
     */
    fSynthesizer->getRuntime().showGui = true;
    if (fMasterUI == NULL)
        fUiInited = true;
    fMasterUI = fSynthesizer->getGuiMaster();
    if (fMasterUI == NULL) {
        fSynthesizer->getRuntime().LogError("Failed to instantiate gui");
        return;
    }
    if (fUiInited)
        fMasterUI->Init("YoshimiEditor");

    // TODO: Use OS API to reparent FLTK window
}

YoshimiEditor::~YoshimiEditor()
{
    /*
     * Adapted from YoshimiLV2PluginUI::static_guiClosed().
     */
    fMasterUI = NULL;
    fSynthesizer->closeGui();
}

void YoshimiEditor::parameterChanged(uint32_t index, float value)
{
}

void YoshimiEditor::stateChanged(const char* key, const char* value)
{
    // TODO: Refresh UI when loading a new state
    d_stderr("TODO: Stated changed. Should refresh UI!");
}

void YoshimiEditor::visibilityChanged(const bool visible)
{
    if (!fUiInited)
        return;

    if (visible) {
        fMasterUI->masterwindow->show();
    } else {
        fMasterUI->masterwindow->hide();
    }
}

void YoshimiEditor::uiIdle()
{
    if (!fUiInited)
        return;

    /*
     * Adapted from YoshimiLV2PluginUI::run().
     */
    if (fMasterUI != NULL) {
        fMasterUI->checkBuffer();
        Fl::check();
    }
}

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new YoshimiEditor();
}

END_NAMESPACE_DISTRHO
