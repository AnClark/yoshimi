/*
    DistrhoPluginInfo

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

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_NAME "Yoshimi"
#define DISTRHO_PLUGIN_AUTHOR "Andrew Deryabin"
#define DISTRHO_PLUGIN_URI "https://github.com/yoshimi/yoshimi"
#define DISTRHO_PLUGIN_CLAP_ID "yoshimi"

#define DISTRHO_PLUGIN_HAS_UI 1
#define DISTRHO_PLUGIN_HAS_EXTERNAL_UI 1
#define DISTRHO_PLUGIN_HAS_EMBED_UI 1
#define DISTRHO_PLUGIN_IS_RT_SAFE 1
#define DISTRHO_PLUGIN_IS_SYNTH 1
#define DISTRHO_PLUGIN_NUM_INPUTS 2
#define DISTRHO_PLUGIN_NUM_OUTPUTS 2
#define DISTRHO_UI_USER_RESIZABLE 0

#define DISTRHO_PLUGIN_WANT_STATE 1
#define DISTRHO_PLUGIN_WANT_DIRECT_ACCESS 1

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED

// end of DistrhoPluginInfo.h
