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

#ifndef YOSHIMI_EDITOR_H
#define YOSHIMI_EDITOR_H

#include "Exchange/ParamStorage.h"
#include "Misc/SynthEngine.h"

#include "DistrhoUI.hpp"
#include "ResizeHandle.hpp"

START_NAMESPACE_DISTRHO

class YoshimiEditor : public UI {
    /*
     * Yoshimi's FLTK UI is managed by synth engine... This is not a good idea.
     * But in the current period, I don't want to touch other parts.
     * So, access to DSP side is required.
     */
    SynthEngine* fSynthesizer;

    ResizeHandle fResizeHandle;

    YoshimiParamStorage fParams;

    BankEntryMap fBankEntries;
    long         fBankCurrent;

    // ----------------------------------------------------------------------------------------------------------------

public:
    YoshimiEditor();
    ~YoshimiEditor();

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks

    void parameterChanged(uint32_t index, float value) override;
    // void programLoaded(uint32_t index) override;
    void stateChanged(const char* key, const char* value) override;

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

    void onImGuiDisplay() override;

private:
    // ----------------------------------------------------------------------------------------------------------------
    // Internal helpers

    void _fetchParams();
    void _syncStateToHost();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YoshimiEditor)
};

END_NAMESPACE_DISTRHO

#endif
