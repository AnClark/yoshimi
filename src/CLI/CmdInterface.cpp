/*
    CmdInterface.cpp

    Copyright 2015-2019, Will Godfrey & others.

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

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <cstdio>
#include <cerrno>
#include <cfloat>
#include <sys/types.h>
#include <ncurses.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <list>
#include <sstream>
#include <sys/time.h>

#include "Misc/SynthEngine.h"
#include "Misc/MiscFuncs.h"
#include "Misc/Bank.h"

#include "Interface/InterChange.h"
#include "CLI/MiscCLI.h"
#include "CLI/CmdInterface.h"
#include "Interface/TextLists.h"


/*
 * There are two routes that 'write' commands can take.
 * sendDirect(synth, ) and sendNormal( synth, )
 *
 * sendDirect(synth, ) is the older form and is now mostly used for
 * numerical entry by test calls. It always returns zero.
 *
 * sendNormal( synth, ) performs 'value' range adjustment and also
 * performs some error checks, returning a response.
 *
 *
 * readControl(synth, ) provides a non-buffered way to find the
 * value of any control. It may be temporarily blocked if
 * there is a write command in progress.
 *
 * readControlText(synth, ) provides a non-buffered way to fetch
 * some text items. It is not error checked.
 */

extern SynthEngine *firstSynth;
static unsigned int currentInstance = 0;



void CmdInterface::defaults()
{
    context = LEVEL::Top;
    chan = 0;
    axis = 0;
    mline = 0;
    npart = 0;
    nFX = 0;
    nFXtype = 0;
    nFXpreset = 0;
    nFXeqBand = 0;
    kitMode = 0;
    kitNumber = 0;
    inKitEditor = false;
    voiceNumber = 0;
    insertType = 0;
    filterVowelNumber = 0;
    filterFormantNumber = 0;
}


void CmdInterface::helpLoop(list<std::string>& msg, std::string *commands, int indent, bool single)
{
    int word = 0;
    int spaces = 30 - indent;
    std::string left = "";
    std::string right = "";
    std::string dent;
    std::string blanks;

    while (commands[word] != "end")
    {
        left = commands[word];
        if (!single)
            right = commands[word + 1];
        if (left == "")
        {
            left = "  " + right;
            right = "";
        }
        if (right > "")
            left = left +(blanks.assign(spaces - left.length(), ' ') + right);
        msg.push_back(dent.assign(indent, ' ') + left);
        word += (2 - single);
    }
}


char CmdInterface::helpList(unsigned int local)
{
    if (!matchnMove(1, point, "help") && !matchnMove(1, point, "?"))
        return REPLY::todo_msg;

    int listnum = -1;
    bool named = false;

    if (point[0] != 0)
    { // 1 & 2 reserved for syseff & inseff
        if (matchnMove(3, point, "effects"))
            listnum = LISTS::eff;
        else if (matchnMove(3, point, "reverb"))
            listnum = LISTS::reverb;
        else if (matchnMove(3, point, "echo"))
            listnum = LISTS::echo;
        else if (matchnMove(3, point, "chorus"))
            listnum = LISTS::chorus;
        else if (matchnMove(3, point, "phaser"))
            listnum = LISTS::phaser;
        else if (matchnMove(3, point, "alienwah"))
            listnum = LISTS::alienwah;
        else if (matchnMove(3, point, "distortion"))
            listnum = LISTS::distortion;
        else if (matchnMove(2, point, "eq"))
            listnum = LISTS::eq;
        else if (matchnMove(3, point, "dynfilter"))
            listnum = LISTS::dynfilter;

        else if (matchnMove(1, point, "part"))
            listnum = LISTS::part;
        else if (matchnMove(2, point, "mcontrol"))
            listnum = LISTS::mcontrol;
        else if (matchnMove(3, point, "common"))
            listnum = LISTS::common;
        else if (matchnMove(3, point, "addsynth"))
            listnum = LISTS::addsynth;
        else if (matchnMove(3, point, "subsynth"))
            listnum = LISTS::subsynth;
        else if (matchnMove(3, point, "padsynth"))
            listnum = LISTS::padsynth;
        else if (matchnMove(3, point, "resonance"))
            listnum = LISTS::resonance;
        else if (matchnMove(3, point, "voice"))
            listnum = LISTS::addvoice;
        else if (matchnMove(3, point, "modulator"))
            listnum = LISTS::addmod;
        else if (matchnMove(3, point, "waveform"))
            listnum = LISTS::waveform;
        else if (matchnMove(3, point, "lfo"))
            listnum = LISTS::lfo;
        else if (matchnMove(3, point, "filter"))
            listnum = LISTS::filter;
        else if (matchnMove(3, point, "envelope"))
            listnum = LISTS::envelope;

        else if (matchnMove(1, point, "vector"))
            listnum = LISTS::vector;
        else if (matchnMove(1, point, "scale"))
            listnum = LISTS::scale;
        else if (matchnMove(1, point, "load"))
            listnum = LISTS::load;
        else if (matchnMove(1, point, "save"))
            listnum = LISTS::save;
        else if (matchnMove(1, point, "list"))
            listnum = LISTS::list;
        else if (matchnMove(1, point, "config"))
            listnum = LISTS::config;
        else if (matchnMove(1, point, "mlearn"))
            listnum = LISTS::mlearn;
        if (listnum != -1)
            named = true;
    }
    else
    {
        if(bitTest(local, LEVEL::AllFX))
        {
            switch (nFXtype)
            {
                case 0:
                    listnum = LISTS::eff;
                    break;
                case 1:
                    listnum = LISTS::reverb;
                    break;
                case 2:
                    listnum = LISTS::echo;
                    break;
                case 3:
                    listnum = LISTS::chorus;
                    break;
                case 4:
                    listnum = LISTS::phaser;
                    break;
                case 5:
                    listnum = LISTS::alienwah;
                    break;
                case 6:
                    listnum = LISTS::distortion;
                    break;
                case 7:
                    listnum = LISTS::eq;
                    break;
                case 8:
                    listnum = LISTS::dynfilter;
                    break;
            }
        }
        else if (bitTest(local, LEVEL::Envelope))
            listnum = LISTS::envelope;
        else if (bitTest(local, LEVEL::LFO))
            listnum = LISTS::lfo;
        else if (bitTest(local, LEVEL::Filter))
            listnum = LISTS::filter;
        else if (bitTest(local, LEVEL::Oscillator))
            listnum = LISTS::waveform;
        else if (bitTest(local, LEVEL::AddMod))
            listnum = LISTS::addmod;
        else if (bitTest(local, LEVEL::AddVoice))
            listnum = LISTS::addvoice;
        else if(bitTest(local, LEVEL::Resonance))
            listnum = LISTS::resonance;
        else if (bitTest(local, LEVEL::AddSynth))
            listnum = LISTS::addsynth;
        else if (bitTest(local, LEVEL::SubSynth))
            listnum = LISTS::subsynth;
        else if (bitTest(local, LEVEL::PadSynth))
            listnum = LISTS::padsynth;
        else if(bitTest(local, LEVEL::MControl))
            listnum = LISTS::mcontrol;

        else if (bitTest(local, LEVEL::Part))
            listnum = LISTS::part;
        else if (bitTest(local, LEVEL::Vector))
            listnum = LISTS::vector;
        else if (bitTest(local, LEVEL::Scale))
            listnum = LISTS::scale;
        else if (bitTest(local, LEVEL::Config))
            listnum = LISTS::config;
        else if (bitTest(local, LEVEL::Learn))
            listnum = LISTS::mlearn;
    }
    if (listnum == -1)
        listnum = LISTS::all;
    list<std::string>msg;
    if (!named)
    {
        msg.push_back("Commands:");
        helpLoop(msg, basics, 2);
    }
    switch(listnum)
    {
        case 0:
            msg.push_back(" ");
            msg.push_back("  Part [n1]   ...             - part operations");
            msg.push_back("  VEctor [n1] ...             - vector operations");
            msg.push_back("  SCale       ...             - scale (microtonal) operations");
            msg.push_back("  MLearn [n1] ...             - MIDI learn operations");
            msg.push_back("  COnfig      ...             - configuration settings");
            msg.push_back("  LIst        ...             - various available parameters");
            msg.push_back("  LOad        ...             - load various files");
            msg.push_back("  SAve        ...             - save various files");

            msg.push_back(" ");
            break;
        case LISTS::part:
            msg.push_back("Part: [n1] = part number");
            helpLoop(msg, partlist, 2);
            break;
        case LISTS::mcontrol:
            msg.push_back("Midi Control:");
            helpLoop(msg, mcontrollist, 2);
            break;
        case LISTS::common:
            msg.push_back("Part Common:");
            helpLoop(msg, commonlist, 2);
            break;
        case LISTS::addsynth:
            msg.push_back("Part AddSynth:");
            helpLoop(msg, addsynthlist, 2);
            break;
        case LISTS::subsynth:
            msg.push_back("Part SubSynth:");
            helpLoop(msg, subsynthlist, 2);
            break;
        case LISTS::padsynth:
            msg.push_back("Part PadSynth:");
            helpLoop(msg, padsynthlist, 2);
            break;
        case LISTS::resonance:
            msg.push_back("Resonance:");
            helpLoop(msg, resonancelist, 2);
            break;
        case LISTS::addvoice:
            msg.push_back("Part AddVoice:");
            helpLoop(msg, addvoicelist, 2);
            break;
        case LISTS::addmod:
            msg.push_back("AddVoice Modulator:");
            helpLoop(msg, addmodlist, 2);
            break;
        case LISTS::waveform:
            msg.push_back("Part Waveform:");
            helpLoop(msg, waveformlist, 2);
            break;

        case LISTS::lfo:
            msg.push_back("Engine LFOs:");
            helpLoop(msg, LFOlist, 2);
            break;
        case LISTS::filter:
            msg.push_back("Engine Filters:");
            helpLoop(msg, filterlist, 2);
            break;
        case LISTS::envelope:
            msg.push_back("Engine Envelopes:");
            helpLoop(msg, envelopelist, 2);
            break;

        case LISTS::eff:
            msg.push_back("Effects:");
            helpLoop(msg, fx_list, 2, true);
            break;
        case LISTS::reverb:
            msg.push_back("Reverb:");
            helpLoop(msg, reverblist, 2);
            break;
        case LISTS::echo:
            msg.push_back("Echo:");
            helpLoop(msg, echolist, 2);
            break;
        case LISTS::chorus:
            msg.push_back("Chorus:");
            helpLoop(msg, choruslist, 2);
            break;
        case LISTS::phaser:
            msg.push_back("Phaser:");
            helpLoop(msg, phaserlist, 2);
            break;
        case LISTS::alienwah:
            msg.push_back("Alienwah:");
            helpLoop(msg, alienwahlist, 2);
            break;
        case LISTS::distortion:
            msg.push_back("Distortion:");
            helpLoop(msg, distortionlist, 2);
            break;
        case LISTS::eq:
            msg.push_back("EQ:");
            helpLoop(msg, eqlist, 2);
            break;
        case LISTS::dynfilter:
            msg.push_back("Dynfilter:");
            helpLoop(msg, dynfilterlist, 2);
            break;

        case LISTS::vector:
            msg.push_back("Vector: [n1] = base channel:");
            helpLoop(msg, vectlist, 2);
            break;
        case LISTS::scale:
            msg.push_back("Scale:");
            helpLoop(msg, scalelist, 2);
            break;
        case LISTS::load:
            msg.push_back("Load:");
            helpLoop(msg, loadlist, 2);
            break;
        case LISTS::save:
            msg.push_back("Save:");
            helpLoop(msg, savelist, 2);
            break;
        case LISTS::list:
            msg.push_back("List:");
            helpLoop(msg, listlist, 2);
            break;
        case LISTS::config:
            msg.push_back("Config:");
            helpLoop(msg, configlist, 2);
            msg.push_back("'*' entries need to be saved and Yoshimi restarted to activate");
            break;
        case LISTS::mlearn:
            msg.push_back("Mlearn: [n1] = line number");
            helpLoop(msg, learnlist, 2);
            break;
    }

    if (listnum == LISTS::all)
    {
        helpLoop(msg, toplist, 2);
        msg.push_back("'...' is a help sub-menu");
    }

    if (synth->getRuntime().toConsole)
        // we need this in case someone is working headless
        std::cout << "\nSet CONfig REPorts [s] - set report destination (gui/stderr)\n";

    synth->cliOutput(msg, LINES);
    return REPLY::exit_msg;
}


void CmdInterface::historyList(int listnum)
{
    list<std::string>msg;
    int start = TOPLEVEL::XML::Instrument;
    int end = TOPLEVEL::XML::MLearn;
    bool found = false;

    if (listnum >= 0) // its a single list we want
    {
        start = listnum;
        end = listnum;
    }
    for (int type = start; type <= end; ++type)
    {
        vector<std::string> listType = *synth->getHistory(type);
        if (listType.size() > 0)
        {
            msg.push_back(" ");
            switch (type)
            {
                case TOPLEVEL::XML::Instrument:
                    msg.push_back("Recent Instruments:");
                    break;
                case TOPLEVEL::XML::Patch:
                    msg.push_back("Recent Patch Sets:");
                    break;
                case TOPLEVEL::XML::Scale:
                    msg.push_back("Recent Scales:");
                    break;
                case TOPLEVEL::XML::State:
                    msg.push_back("Recent States:");
                    break;
                case TOPLEVEL::XML::Vector:
                    msg.push_back("Recent Vectors:");
                    break;
                case TOPLEVEL::XML::MLearn:
                    msg.push_back("Recent MIDI learned:");
                    break;
            }
            int itemNo = 0;
            for (vector<std::string>::iterator it = listType.begin(); it != listType.end(); ++it, ++ itemNo)
                msg.push_back(std::to_string(itemNo + 1) + "  " + *it);
            found = true;
        }
    }
    if (!found)
        msg.push_back("\nNo Saved History");

    synth->cliOutput(msg, LINES);
}


std::string CmdInterface::historySelect(int listnum, int selection)
{
    vector<std::string> listType = *synth->getHistory(listnum - 1);
    if (listType.size()== 0)
    {
        synth->getRuntime().Log("No saved entries");
        return "";
    }
    else
    {
        vector<std::string>::iterator it = listType.begin();
        int itemNo = 0;
        while (it != listType.end() && itemNo != selection)
        {
            ++ it;
            ++ itemNo;
        }
        if (it != listType.end())
            return *it;
    }
    synth->getRuntime().Log("No such entry");
    return "";
}


int CmdInterface::effectsList(bool presets)
{
    list<std::string>msg;

    size_t presetsPos;
    size_t presetsLast;
    int presetsCount;
    std::string blanks;
    std::string left;
    bool all;

    if (bitTest(context, LEVEL::AllFX) && presets == true)
    {
         synth->getRuntime().Log("Type " + fx_list[nFXtype] + "\nPresets -" + fx_presets[nFXtype].substr(fx_presets[nFXtype].find(',') + 1));
         return REPLY::done_msg;
    }
    else if (presets)
    {
        synth->getRuntime().Log("No effect selected");
        return REPLY::done_msg;
    }
    else
        all = matchnMove(1, point, "all");
    if (!all)
        msg.push_back("  effect     presets");
    for (int i = 0; i < 9; ++ i)
    {
        presetsPos = 1;
        presetsLast = fx_presets [i].find(',') + 1; // skip over count
        presetsCount = 0;
        if (all)
        {
            msg.push_back("  " + fx_list[i]);
            msg.push_back("    presets");
            while (presetsPos != std::string::npos)
            {
                presetsPos = fx_presets [i].find(',', presetsLast);
                msg.push_back("      " + asString(presetsCount + 1) + " =" + fx_presets [i].substr(presetsLast, presetsPos - presetsLast));
                presetsLast = presetsPos + 1;
                ++ presetsCount;
            }
        }
        else
        {
            left = fx_list[i];
            msg.push_back("    " + left + blanks.assign(12 - left.length(), ' ') + fx_presets [i].substr(0, presetsLast - 1));
        }
    }

    synth->cliOutput(msg, LINES);
    return REPLY::done_msg;
}


int CmdInterface::effects(unsigned char controlType)
{
    Config &Runtime = synth->getRuntime();
    int nFXavail;
    int par = nFX;
    int value;
    std::string dest = "";

    if (bitTest(context, LEVEL::Part))
    {
        nFXavail = NUM_PART_EFX;
        nFXtype = synth->part[npart]->partefx[nFX]->geteffect();
    }
    else if (bitTest(context, LEVEL::InsFX))
    {
        nFXavail = NUM_INS_EFX;
        nFXtype = synth->insefx[nFX]->geteffect();
    }
    else
    {
        nFXavail = NUM_SYS_EFX;
        nFXtype = synth->sysefx[nFX]->geteffect();
        int tmp = toggle(point);
        if (tmp >= 0)
            return sendNormal( synth, 0, tmp, controlType, EFFECT::sysIns::effectEnable, TOPLEVEL::section::systemEffects, UNUSED, nFX);
    }

    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    value = string2int(point);

    if (value > 128)
    {
        std::cout << "Eff status " << int(readControl(synth, 0, value, npart, EFFECT::type::none + nFXtype, nFX)) << std::endl;
        return REPLY::done_msg;
    }


    if (value > 0)
    {
        value -= 1;
        point = skipChars(point);
        if (value >= nFXavail)
            return REPLY::range_msg;

        if (value != nFX)
        { // partially updates GUI
            nFX = value;
            if (bitTest(context, LEVEL::Part))
            {
                sendDirect(synth, 0, nFX, TOPLEVEL::type::Write, PART::control::effectNumber, npart, UNUSED, nFX, TOPLEVEL::insert::partEffectSelect);
                nFXtype = synth->part[npart]->partefx[nFX]->geteffect();
                sendDirect(synth, 0, nFXtype, TOPLEVEL::type::Write, PART::control::effectType, npart, UNUSED, nFX, TOPLEVEL::insert::partEffectSelect);
            }
            else if (bitTest(context, LEVEL::InsFX))
            {
                sendDirect(synth, 0, nFX, TOPLEVEL::type::Write, EFFECT::sysIns::effectNumber, TOPLEVEL::section::insertEffects, UNUSED, nFX);

                nFXtype = synth->insefx[nFX]->geteffect();
                sendDirect(synth, 0, nFXtype, TOPLEVEL::type::Write, EFFECT::sysIns::effectType, TOPLEVEL::section::insertEffects, UNUSED, nFX);
            }
            else
            {
                sendDirect(synth, 0, nFX, TOPLEVEL::type::Write, EFFECT::sysIns::effectNumber, TOPLEVEL::section::systemEffects, UNUSED, nFX);

                nFXtype = synth->sysefx[nFX]->geteffect();
                sendDirect(synth, 0, nFXtype, TOPLEVEL::type::Write, EFFECT::sysIns::effectType, TOPLEVEL::section::systemEffects, UNUSED, nFX);
            }
        }
        if (lineEnd(point, controlType))
        {
            Runtime.Log("efx number set to " + asString(nFX + 1));
            return REPLY::done_msg;
        }
    }

    bool effType = false;
    for (int i = 0; i < 9; ++ i)
    {
        //Runtime.Log("command " + (std::string) point + "  list " + fx_list[i]);
        if (matchnMove(2, point, fx_list[i].c_str()))
        {
            nFXtype = i;
            effType = true;
            break;
        }
    }
    if (effType)
    {
        //std::cout << "nfx " << nFX << std::endl;
        nFXpreset = 0; // always set this on type change
        if (bitTest(context, LEVEL::Part))
        {
            sendDirect(synth, 0, nFXtype, TOPLEVEL::type::Write, PART::control::effectType, npart, UNUSED, nFX);
            return REPLY::done_msg; // TODO find out why not sendNormal
        }
        else if (bitTest(context, LEVEL::InsFX))
            return sendNormal( synth, 0, nFXtype, TOPLEVEL::type::Write, EFFECT::sysIns::effectType, TOPLEVEL::section::insertEffects, UNUSED, nFX);
        else
            return sendNormal( synth, 0, nFXtype, TOPLEVEL::type::Write, EFFECT::sysIns::effectType, TOPLEVEL::section::systemEffects, UNUSED, nFX);
    }

    if (nFXtype > 0)
    {
        int selected = -1;
        int value = -1;
        std::string name = std::string(point).substr(0, 3);
        /*
         * We can't do a skipChars here as we don't yet know
         * if 'selected' will be valid. For some controls we
         * need to do an on-the-spot skip, otherwise we do so
         * at the end when we know we have a valid result but
         * 'value' has not been set.
         * If it's not valid we don't block, but pass on to
         * other command tests routines.
         */
        switch (nFXtype)
        {
            case 1:
            {
                selected = stringNumInList(name, effreverb, 3);
                if (selected != 7) // EQ
                    nFXeqBand = 0;
                if (selected == 10) // type
                {
                    point = skipChars(point);
                    if (matchnMove(1, point, "random"))
                        value = 0;
                    else if (matchnMove(1, point, "freeverb"))
                        value = 1;
                    else if (matchnMove(1, point, "bandwidth"))
                        value = 2;
                    else
                        return REPLY::value_msg;
                }
                break;
            }
            case 2:
                selected = stringNumInList(name, effecho, 3);
                break;
            case 3:
            {
                selected = stringNumInList(name, effchorus, 3);
                if (selected == 4) // filtershape
                {
                    point = skipChars(point);
                    if (matchnMove(1, point, "sine"))
                        value = 0;
                    else if (matchnMove(1, point, "triangle"))
                        value = 1;
                    else return REPLY::value_msg;
                }
                else if (selected == 11) // subtract
                {
                    point = skipChars(point);
                    value = (toggle(point) == 1);
                }
                break;
            }
            case 4:
            {
                selected = stringNumInList(name, effphaser, 3);
                if (selected == 4) // filtershape
                {
                    point = skipChars(point);
                    if (matchnMove(1, point, "sine"))
                        value = 0;
                    else if (matchnMove(1, point, "triangle"))
                        value = 1;
                    else return REPLY::value_msg;
                }
                else if (selected == 10 || selected == 12 || selected == 14) // LFO, SUB, ANA
                {
                    point = skipChars(point);
                    value = (toggle(point) == 1);
                }
                break;
            }
            case 5:
            {
                selected = stringNumInList(name, effalienwah, 3);
                if (selected == 3) // filtershape
                {
                    point = skipChars(point);
                    if (matchnMove(1, point, "sine"))
                        value = 0;
                    else if (matchnMove(1, point, "triangle"))
                        value = 1;
                    else return REPLY::value_msg;
                }
                break;
            }
            case 6:
            {
                selected = stringNumInList(name, effdistortion, 3);
                if (selected == 5) // filtershape
                {
                    point = skipChars(point);
                    std::string name = std::string(point).substr(0,3);
                    value = stringNumInList(name, filtershapes, 3) - 1;
                    if (value < 0)
                        return REPLY::value_msg;
                }
                else if (selected == 6 || selected == 9 || selected == 10) // invert, stereo, prefilter
                {
                    point = skipChars(point);
                    value = (toggle(point) == 1);
                }
                break;
            }
            case 7: // TODO band and type no GUI update
            {
                selected = stringNumInList(name, effeq, 2);
                if (selected == 1) // band
                {
                    if (controlType == TOPLEVEL::type::Write)
                    {
                        point = skipChars(point);
                        value = string2int(point);
                        if (value < 0 || value >= MAX_EQ_BANDS)
                            return REPLY::range_msg;
                        nFXeqBand = value;
                    }
                }
                else if (selected == 2) // type
                {
                    point = skipChars(point);
                    std::string name = std::string(point).substr(0,3);
                    value = stringNumInList(name, eqtypes, 3);
                    if (value < 0)
                        return REPLY::value_msg;
                }

                if (selected > 1)
                {
                    selected += 8;
                }
                break;
            }
            case 8:
            {
                selected = stringNumInList(name, effdynamicfilter, 3);
                if (selected == 4) // filtershape
                {
                    point = skipChars(point);
                    if (matchnMove(1, point, "sine"))
                        value = 0;
                    else if (matchnMove(1, point, "triangle"))
                        value = 1;
                    else return REPLY::value_msg;
                }
                else if (selected == 8) // invert
                {
                    point = skipChars(point);
                    value = (toggle(point) == 1);
                }
                else if (selected == 10) // filter entry
                {
                    bitSet(context, LEVEL::Filter);
                    return REPLY::done_msg;
                }
            }
        }
        if (selected > -1)
        {
            if (value == -1)
            {
                point = skipChars(point);
                value = string2int(point);
            }
            //std::cout << "Val " << value << "  type " << controlType << "  cont " << selected << "  part " << context << "  efftype " << int(nFXtype) << "  num " << int(nFX) << std::endl;
            if (bitTest(context, LEVEL::Part))
                return sendNormal( synth, 0, value, controlType, selected, npart, EFFECT::type::none + nFXtype, nFX);
            else if (bitTest(context, LEVEL::InsFX))
                return sendNormal( synth, 0, value, controlType, selected, TOPLEVEL::section::insertEffects, EFFECT::type::none + nFXtype, nFX);
            else
                return sendNormal( synth, 0, value, controlType, selected, TOPLEVEL::section::systemEffects, EFFECT::type::none + nFXtype, nFX);
        }
        // Continue cos it's not for us.
    }

    if (matchnMove(2, point, "send"))
    {
        if (lineEnd(point, controlType))
            return REPLY::parameter_msg;

        if (bitTest(context, LEVEL::InsFX))
        {
            if (matchnMove(1, point, "master"))
                value = -2;
            else if (matchnMove(1, point, "off"))
                value = -1;
            else
            {
                value = string2int(point) - 1;
                if (value >= Runtime.NumAvailableParts || value < 0)
                    return REPLY::range_msg;
            }
        }
        else
        {
            par = string2int(point) - 1;
            point = skipChars(point);
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            value = string2int127(point);
        }

        int control;
        int partno;
        int engine = nFX;
        int insert = UNUSED;

        if (bitTest(context, LEVEL::Part))
        {
            partno = npart;
            control = PART::control::partToSystemEffect1 + par;
            engine = UNUSED;
        }
        else if (bitTest(context, LEVEL::InsFX))
        {
            partno = TOPLEVEL::section::insertEffects;
            control = EFFECT::sysIns::effectDestination;
        }
        else
        {
            if (par <= nFX || par >= NUM_SYS_EFX)
                return REPLY::range_msg;
            partno = TOPLEVEL::section::systemEffects;
            control = EFFECT::sysIns::toEffect1 + par - 1; // TODO this needs sorting
            engine = nFX;
            insert = TOPLEVEL::insert::systemEffectSend;
        }
        return sendNormal( synth, 0, value, TOPLEVEL::type::Write, control, partno, UNUSED, engine, insert);
    }

    if (matchnMove(3, point, "preset"))
    {
        /*
         * Using constant strings and bedding the number into the list
         * of presets provides a very simple way to keep track of a
         * moving target with minimal code and data space.
         * However, all of this should really be in src/Effects
         * not here *and* in the gui code!
         */
        int partno;
        nFXpreset = string2int127(point) - 1;
        if (bitTest(context, LEVEL::Part))
            partno = npart;
        else if (bitTest(context, LEVEL::InsFX))
            partno = TOPLEVEL::section::insertEffects;
        else
            partno = TOPLEVEL::section::systemEffects;
        return sendNormal( synth, 0, nFXpreset, TOPLEVEL::type::Write, 16, partno,  EFFECT::type::none + nFXtype, nFX);
    }
    return REPLY::op_msg;
}


int CmdInterface::midiControllers(unsigned char controlType)
{
    int value = -1;
    int cmd = -1;
    std::cout << "here" << std::endl;
    if (matchnMove(2, point, "volume"))
    {
        value = toggle(point);
        cmd = PART::control::volumeEnable;

        if (value == -1)
        {
            value = string2int127(point);
            cmd = PART::control::volumeRange;
        }
    }
    if ((cmd == -1) && matchnMove(2, point, "pan"))
    {
        value = string2int127(point);
        cmd = PART::control::panningWidth;
    }
    if ((cmd == -1) && matchnMove(2, point, "modwheel"))
    {
        value = toggle(point);
        cmd = PART::control::exponentialModWheel;

        if (value == -1)
        {
            value = string2int127(point);
            cmd = PART::control::modWheelDepth;
        }
    }
    if ((cmd == -1) && matchnMove(2, point, "expression"))
    {
        value = toggle(point);
        if (value == -1)
            return REPLY::value_msg;
        cmd = PART::control::expressionEnable;
    }
    if ((cmd == -1) && matchnMove(2, point, "sustain"))
    {
        value = toggle(point);
        if (value == -1)
            return REPLY::value_msg;
        cmd = PART::control::sustainPedalEnable;
    }
    if ((cmd == -1) && matchnMove(2, point, "pwheel"))
    {
        value = string2int(point);
        cmd = PART::control::pitchWheelRange;
    }
    if ((cmd == -1) && matchnMove(2, point, "breath"))
    {
        value = toggle(point);
        if (value == -1)
            return REPLY::value_msg;
        cmd = PART::control::breathControlEnable;
    }
    if ((cmd == -1) && matchnMove(2, point, "fcutoff"))
    {
        value = string2int127(point);
        cmd = PART::control::filterCutoffDepth;
    }
    if ((cmd == -1) && matchnMove(2, point, "fq"))
    {
        value = string2int127(point);
        cmd = PART::control::filterQdepth;
    }
    if ((cmd == -1) && matchnMove(2, point, "bandwidth"))
    {
        value = toggle(point);
        cmd = PART::control::exponentialBandwidth;

        if (value == -1)
        {
            value = string2int127(point);
            cmd = PART::control::bandwidthDepth;
        }
    }
    if ((cmd == -1) && matchnMove(2, point, "fmamplitude"))
    {
        value = toggle(point);
        if (value == -1)
            return REPLY::value_msg;
        cmd = PART::control::FMamplitudeEnable;
    }
    if ((cmd == -1) && matchnMove(2, point, "rcenter"))
    {
        value = string2int127(point);
        cmd = PART::control::resonanceCenterFrequencyDepth;
    }
    if ((cmd == -1) && matchnMove(2, point, "rband"))
    {
        value = string2int127(point);
        cmd = PART::control::resonanceBandwidthDepth;
    }

    // portamento controls
    if (cmd == -1)
    {
        if (matchnMove(2, point, "proportional"))
        {
            value = toggle(point);
            if (value == -1)
                return REPLY::value_msg;
            cmd = PART::control::receivePortamento;
        }
        else if(matchnMove(2, point, "psweep"))
        {
            value = string2int127(point);
            cmd = PART::control::portamentoTime;
        }
        else if (matchnMove(2, point, "pratio"))
        {
            value = string2int127(point);
            cmd = PART::control::portamentoTimeStretch;
        }
        else if (matchnMove(2, point, "pdifference"))
        {
            value = string2int127(point);
            cmd = PART::control::portamentoThreshold;
        }
        else if (matchnMove(2, point, "pinvert"))
        {
            value = toggle(point);
            if (value == -1)
                return REPLY::value_msg;
            cmd = PART::control::portamentoThresholdType;
        }
        else if (matchnMove(2, point, "pproportional"))
        {
            value = toggle(point);
            if (value == -1)
                return REPLY::value_msg;
            cmd = PART::control::enableProportionalPortamento;
        }
        else if (matchnMove(2, point, "pextent"))
        {
            value = string2int127(point);
            cmd = PART::control::proportionalPortamentoRate;
        }
        else if (matchnMove(2, point, "poffset"))
        {
            value = string2int127(point);
            cmd = PART::control::proportionalPortamentoDepth;
        }
    }

    if ((cmd == -1) && matchnMove(2, point, "clear"))
    {
        value = 0;
        cmd = PART::control::resetAllControllers;
    }

    // midi controllers
    if (cmd == -1)
    {
        if (matchnMove(3, point, "moemulate"))
        {
            value = string2int127(point);
            cmd = PART::control::midiModWheel;
        }
        else if (matchnMove(3, point, "exemulate"))
        {
            value = string2int127(point);
            cmd = PART::control::midiExpression;
        }
        else if (matchnMove(3, point, "bremulate"))
        {
            value = string2int127(point);
            cmd = PART::control::midiBreath;
        }
        else if (matchnMove(3, point, "fcemulate"))
        {
            value = string2int127(point);
            cmd = PART::control::midiFilterCutoff;
        }
        else if (matchnMove(3, point, "fqemulate"))
        {
            value = string2int127(point);
            cmd = PART::control::midiFilterQ;
        }
        else if (matchnMove(3, point, "baemulate"))
        {
            value = string2int127(point);
            cmd = PART::control::midiBandwidth;
        }
    }

    if (cmd > -1)
        return sendNormal( synth, 0, value, controlType, cmd, npart);
    return REPLY::available_msg;
}


int CmdInterface::partCommonControls(unsigned char controlType)
{
    // TODO integrate modulator controls properly
    int cmd = -1;
    int engine = contextToEngines(context);
    int insert = UNUSED;
    int kit = UNUSED;
    if (engine == PART::engine::addVoice1 || engine == PART::engine::addMod1)
        engine += voiceNumber; // voice numbers are 0 to 7

    if (inKitEditor)
        kit = kitNumber;

    if (bitFindHigh(context) != LEVEL::Part)
    {
        // these are all common to Add, Sub, Pad, Voice
        int value = 0;
        if (matchnMove(3, point, "detune"))
        {
            if (matchnMove(1, point, "fine"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                if (engine >= PART::engine::addMod1)
                    cmd = ADDVOICE::control::modulatorDetuneFrequency;
                else
                    cmd = ADDSYNTH::control::detuneFrequency;
            }
            else if (matchnMove(1, point, "coarse"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                if (engine >= PART::engine::addMod1)
                    cmd = ADDVOICE::control::modulatorCoarseDetune;
                else
                    cmd = ADDSYNTH::control::coarseDetune;
            }
            else if (matchnMove(1, point, "type"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                std::string name = std::string(point).substr(0,3);
                value = stringNumInList(name, detuneType, 3);
                if (value > -1 && engine < PART::engine::addVoice1)
                    value -= 1;
                if (value == -1)
                    return REPLY::range_msg;
                if (engine >= PART::engine::addMod1)
                    cmd = ADDVOICE::control::modulatorDetuneType;
                else
                    cmd = ADDSYNTH::control::detuneType;
            }
        }
        else if (matchnMove(3, point, "octave"))
        {
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            value = string2int(point);
            if (engine >= PART::engine::addMod1)
                cmd = ADDVOICE::control::modulatorOctave;
            else
                cmd = ADDSYNTH::control::octave;
        }

        if (cmd == -1 && matchnMove(3, point, "lfo"))
        {
            if(engine == PART::engine::subSynth)
                return REPLY::available_msg;
            bitSet(context, LEVEL::LFO);
            return LFOselect(controlType);
        }
        if (cmd == -1 && matchnMove(3, point, "filter"))
        {
            bitSet(context, LEVEL::Filter);
            return filterSelect(controlType);
        }
        if (cmd == -1 && matchnMove(3, point, "envelope"))
        {
            bitSet(context, LEVEL::Envelope);
            return envelopeSelect(controlType);
        }

        // not AddVoice
        if (cmd == -1 && (matchnMove(3, point, "stereo") && bitFindHigh(context) != LEVEL::AddVoice))
        {
            cmd = ADDSYNTH::control::stereo;
            value = (toggle(point) == 1);
        }
        // not AddSynth
        if (cmd == -1 && (bitFindHigh(context) != LEVEL::AddSynth))
        {
            int tmp_cmd = -1;
            if (matchnMove(3, point, "fixed"))
            {
                value = (toggle(point) == 1);
                cmd = SUBSYNTH::control::baseFrequencyAs440Hz;
            }
            else if (matchnMove(3, point, "equal"))
                tmp_cmd = SUBSYNTH::control::equalTemperVariation;
            else if (matchnMove(3, point, "bend"))
            {
                if (matchnMove(1, point, "adjust"))
                    tmp_cmd = SUBSYNTH::control::pitchBendAdjustment;
                else if (matchnMove(1, point, "offset"))
                    tmp_cmd = SUBSYNTH::control::pitchBendOffset;
            }
            if (tmp_cmd > -1)
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                cmd = tmp_cmd;
            }
        }
        // Add/Pad only
        if (cmd == -1 && (bitFindHigh(context) == LEVEL::AddSynth || bitFindHigh(context) == LEVEL::PadSynth))
        {
            int tmp_cmd = -1;
            if (matchnMove(3, point, "depop"))
                tmp_cmd = ADDSYNTH::control::dePop;
            else if (matchnMove(2, point, "punch"))
            {
                if (matchnMove(1, point, "power"))
                    tmp_cmd = ADDSYNTH::control::punchStrength;
                else if (matchnMove(1, point, "duration"))
                    tmp_cmd = ADDSYNTH::control::punchDuration;
                else if (matchnMove(1, point, "stretch"))
                    tmp_cmd = ADDSYNTH::control::punchStretch;
                else if (matchnMove(1, point, "velocity"))
                    tmp_cmd = ADDSYNTH::control::punchVelocity;
            }
            if (tmp_cmd > -1)
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                cmd = tmp_cmd;
            }
        }
        // Sub/Pad only
        if (cmd == -1 && (bitFindHigh(context) == LEVEL::SubSynth || bitFindHigh(context) == LEVEL::PadSynth))
        {
            value = -1;
            if (matchnMove(2, point, "overtone"))
            {
                if (matchnMove(1, point, "Position"))
                {
                    if (matchnMove(2, point, "harmonic"))
                        value = 0;
                    else if(matchnMove(2, point, "usine"))
                        value = 1;
                    else if(matchnMove(2, point, "lsine"))
                        value = 2;
                    else if(matchnMove(2, point, "upower"))
                        value = 3;
                    else if(matchnMove(2, point, "lpower"))
                        value = 4;
                    else if(matchnMove(2, point, "sine"))
                        value = 5;
                    else if(matchnMove(2, point, "power"))
                        value = 6;
                    else if(matchnMove(2, point, "shift"))
                        value = 7;
                    else
                        return REPLY::range_msg;
                    cmd = SUBSYNTH::control::overtonePosition;
                }
                else
                {
                    if (matchnMove(1, point, "First"))
                        cmd = SUBSYNTH::control::overtoneParameter1;
                    else if (matchnMove(1, point, "Second"))
                        cmd = SUBSYNTH::control::overtoneParameter2;
                    else if (matchnMove(1, point, "Harmonic"))
                        cmd = SUBSYNTH::control::overtoneForceHarmonics;
                    if (cmd > -1)
                    {
                        if (lineEnd(point, controlType))
                            return REPLY::value_msg;
                        value = string2int(point);
                    }
                }
            }
        }

        if (cmd > -1)
        {
            sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, engine);
            return REPLY::done_msg;
        }
    }

    if (matchnMove(1, point, "volume"))
        cmd = PART::control::volume;
    else if(matchnMove(1, point, "pan"))
        cmd = PART::control::panning;
    else if (matchnMove(2, point, "velocity"))
        cmd = PART::control::velocitySense;

    if (cmd != -1)
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;

        if (bitFindHigh(context) == LEVEL::Part)
            kit = UNUSED;
        else
            kit = kitNumber;

        return sendNormal( synth, 0, string2float(point), controlType, cmd, npart, kit, engine);
    }

    if (cmd == -1 && bitFindHigh(context) == LEVEL::Part)
    { // the following can only be done at part/kit level
        int value = 0;
        if (matchnMove(2, point, "min"))
        {
            cmd = PART::control::minNote;
            if(controlType == TOPLEVEL::type::Write)
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                if (matchnMove(1, point, "last"))
                {
                    cmd = PART::control::minToLastKey;
                }
                else
                {
                    value = string2int(point);
                    if (value > synth->part[npart]->Pmaxkey)
                        return REPLY::high_msg;
                }
            }

        }
        else if (matchnMove(2, point, "max"))
        {
            cmd = PART::control::maxNote;
            if(controlType == TOPLEVEL::type::Write)
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                if (matchnMove(1, point, "last"))
                {
                    cmd = PART::control::maxToLastKey;
                }
                else
                {
                    value = string2int(point);
                    if (value < synth->part[npart]->Pminkey)
                        return REPLY::low_msg;
                }
            }

        }
        if (cmd > -1)
        {
            if (inKitEditor)
                insert = TOPLEVEL::insert::kitGroup;
            else
                kit = UNUSED;
            return sendNormal( synth, 0, value, controlType, cmd, npart, kit, UNUSED, insert);
        }
    }
    //std::cout << ">> value " << value << "  type " << controlType << "  cmd " << int(cmd) << "  part " << int(npart) << "  kit " << int(kitNumber) << "  engine " << int(engine) << "  insert " << int(insert) << std::endl;
    return REPLY::todo_msg;
}


int CmdInterface::LFOselect(unsigned char controlType)
{
    int cmd = -1;
    float value = -1;
    int group = -1;
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    int engine = contextToEngines(context);
    if (engine == PART::engine::addVoice1)
        engine += voiceNumber;

    if (matchnMove(2, point, "amplitude"))
        group = TOPLEVEL::insertType::amplitude;
    else if (matchnMove(2, point, "frequency"))
        group = TOPLEVEL::insertType::frequency;
    else if (matchnMove(2, point, "filter"))
        group = TOPLEVEL::insertType::filter;
    if (group > -1)
        insertType = group;
    else
        group = insertType;
    switch (group)
    {
        case TOPLEVEL::insertType::amplitude:
            cmd = ADDVOICE::control::enableAmplitudeLFO;
            break;
        case TOPLEVEL::insertType::frequency:
            cmd = ADDVOICE::control::enableFrequencyLFO;
            break;
        case TOPLEVEL::insertType::filter:
            cmd = ADDVOICE::control::enableFilterLFO;
            break;
    }

    value = toggle(point);
    if (value > -1)
    {
        if (engine != PART::engine::addVoice1 + voiceNumber)
            return REPLY::available_msg;
        return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, engine);
    }
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    value = -1;
    cmd = -1;

    if (matchnMove(1, point, "rate"))
        cmd = LFOINSERT::control::speed;
    else if (matchnMove(1, point, "intensity"))
        cmd = LFOINSERT::control::depth;
    else if (matchnMove(1, point, "start"))
        cmd = LFOINSERT::control::start;
    else if (matchnMove(1, point, "delay"))
        cmd = LFOINSERT::control::delay;
    else if (matchnMove(1, point, "expand"))
        cmd = LFOINSERT::control::stretch;
    else if (matchnMove(1, point, "continuous"))
    {
        value = (toggle(point) == 1);
        cmd = LFOINSERT::control::continuous;
    }
    else if (matchnMove(1, point, "type"))
    {
        if (lineEnd(point, controlType))
            return REPLY::what_msg;
        if (matchnMove(2, point, "sine"))
            value = 0;
        else if (matchnMove(1, point, "triangle"))
            value = 1;
        else if (matchnMove(2, point, "square"))
            value = 2;
        else if (matchnMove(2, point, "rup"))
            value = 3;
        else if (matchnMove(2, point, "rdown"))
            value = 4;
        else if (matchnMove(1, point, "e1dn"))
            value = 5;
        else if (matchnMove(1, point, "e2dn"))
            value = 6;
        cmd = LFOINSERT::control::type;
    }
    else if (matchnMove(2, point, "ar"))
        cmd = LFOINSERT::control::amplitudeRandomness;
    else if (matchnMove(2, point, "fr"))
        cmd = LFOINSERT::control::frequencyRandomness;

    //std::cout << ">> base cmd " << int(cmd) << "  part " << int(npart) << "  kit " << int(kitNumber) << "  engine " << int(engine) << "  parameter " << int(group) << std::endl;

    if (value == -1)
        value = string2float(point);
    return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, engine, TOPLEVEL::insert::LFOgroup, group);
}


int CmdInterface::filterSelect(unsigned char controlType)
{
    int cmd = -1;
    float value = -1;
    int thisPart = npart;
    int kit = kitNumber;
    int param = UNUSED;
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    int engine = contextToEngines(context);
    if (engine == PART::engine::addVoice1)
        engine += voiceNumber;
    bool isDyn = false;
    if (bitTest(context, LEVEL::AllFX) && nFXtype == 8)
    {
        kit = EFFECT::type::dynFilter;
        engine = 0;
        if (bitTest(context, LEVEL::InsFX))
        {
            thisPart = TOPLEVEL::section::insertEffects;
        }
        else if (!bitTest(context, LEVEL::Part))
        {
            thisPart = TOPLEVEL::section::systemEffects;
        }
        isDyn = true;
    }

    if (!isDyn && (engine == PART::engine::subSynth || engine == PART::engine::addVoice1 + voiceNumber))
    {
        value = toggle(point);
        if (value > -1)
        {
            if (engine == PART::engine::subSynth)
                cmd = SUBSYNTH::control::enableFilter;
            else
                cmd = ADDVOICE::control::enableFilter;
            readControl(synth, 0, FILTERINSERT::control::baseType, thisPart, kitNumber, engine, TOPLEVEL::insert::filterGroup);

            return sendNormal( synth, 0, value, controlType, cmd, thisPart, kit, engine);
        }
        value = -1; // leave it as if not set
    }

    if (matchnMove(2, point, "center"))
        cmd = FILTERINSERT::control::centerFrequency;
    else if (matchnMove(1, point, "q"))
        cmd = FILTERINSERT::control::Q;
    else if (matchnMove(1, point, "velocity"))
        cmd = FILTERINSERT::control::velocitySensitivity;
    else if (matchnMove(2, point, "slope"))
        cmd = FILTERINSERT::control::velocityCurve;
    else if (matchnMove(1, point, "gain"))
        cmd = FILTERINSERT::control::gain;
    else if (matchnMove(2, point, "tracking"))
        cmd = FILTERINSERT::control::frequencyTracking;
    else if (matchnMove(1, point, "range"))
    {
        value = (toggle(point) == 1);
        cmd = FILTERINSERT::control::frequencyTrackingRange;
    }
    else if (matchnMove(2, point, "category"))
    {
        if (matchnMove(1, point, "analog"))
            value = 0;
        else if(matchnMove(1, point, "formant"))
        {
            value = 1;
            filterVowelNumber = 0;
            filterFormantNumber = 0;
        }
        else if(matchnMove(1, point, "state"))
            value = 2;
        else
            return REPLY::range_msg;
        cmd = FILTERINSERT::control::baseType;
    }
    else if (matchnMove(2, point, "stages"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        value = string2int(point) - 1;
        cmd = FILTERINSERT::control::stages;
    }

    if (cmd == -1)
    {
        int baseType = readControl(synth, 0, FILTERINSERT::control::baseType, thisPart, kit, engine, TOPLEVEL::insert::filterGroup);
        //std::cout << "baseType " << baseType << std::endl;
        if (baseType == 1) // formant
        {
            if (matchnMove(1, point, "invert"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = (toggle(point) == 1);
                cmd = FILTERINSERT::control::negateInput;
            }
            else if (matchnMove(2, point, "fcenter"))
                cmd = FILTERINSERT::control::formantCenter;
            else if (matchnMove(2, point, "frange"))
                cmd = FILTERINSERT::control::formantOctave;
            else if (matchnMove(1, point, "expand"))
                cmd = FILTERINSERT::control::formantStretch;
            else if (matchnMove(1, point, "lucidity"))
                cmd = FILTERINSERT::control::formantClearness;
            else if (matchnMove(1, point, "morph"))
                cmd = FILTERINSERT::control::formantSlowness;
            else if (matchnMove(2, point, "size"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                cmd = FILTERINSERT::control::sequenceSize;
            }
            else if (matchnMove(2, point, "count"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                cmd = FILTERINSERT::control::numberOfFormants;
            }
            else if (matchnMove(2, point, "vowel"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                filterVowelNumber = string2int(point);
                filterFormantNumber = 0;
                return REPLY::done_msg;
            }
            else if (matchnMove(1, point, "point"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                point = skipChars(point);
                int position = string2int(point);
                //std::cout << "val " << value << "  pos " << position << std::endl;
                return sendNormal( synth, 0, value, controlType, FILTERINSERT::control::vowelPositionInSequence, thisPart, kit, engine, TOPLEVEL::insert::filterGroup, position);
            }
            else if (matchnMove(2, point, "formant"))
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                filterFormantNumber = string2int(point);
                return REPLY::done_msg;
            }
            else
            {
                if (matchnMove(2, point, "ffrequency"))
                    cmd = FILTERINSERT::control::formantFrequency;
                else if (matchnMove(2, point, "fq"))
                    cmd = FILTERINSERT::control::formantQ;
                else if (matchnMove(2, point, "fgain"))
                    cmd = FILTERINSERT::control::formantAmplitude;
                if (cmd == -1)
                    return REPLY::range_msg;
                value = string2int(point);
                return sendNormal( synth, 0, value, controlType, cmd, thisPart, kit, engine, TOPLEVEL::insert::filterGroup, filterFormantNumber, filterVowelNumber);
            }
        }
        else if (matchnMove(2, point, "type"))
        {
            switch (baseType)
            {
                case 0: // analog
                {
                    if (matchnMove(2, point, "l1"))
                        value = 0;
                    else if (matchnMove(2, point, "h1"))
                        value = 1;
                    else if (matchnMove(2, point, "l2"))
                        value = 2;
                    else if (matchnMove(2, point, "h2"))
                        value = 3;
                    else if (matchnMove(2, point, "bpass"))
                        value = 4;
                    else if (matchnMove(2, point, "stop"))
                        value = 5;
                    else if (matchnMove(2, point, "peak"))
                        value = 6;
                    else if (matchnMove(2, point, "lshelf"))
                        value = 7;
                    else if (matchnMove(2, point, "hshelf"))
                        value = 8;
                    else
                        return REPLY::range_msg;
                    cmd = FILTERINSERT::control::analogType;
                    break;
                }
                case 2: // state variable
                {
                    if (matchnMove(1, point, "low"))
                        value = 0;
                    else if (matchnMove(1, point, "high"))
                        value = 1;
                    else if (matchnMove(1, point, "band"))
                        value = 2;
                    else if (matchnMove(1, point, "stop"))
                        value = 3;
                    else
                        return REPLY::range_msg;
                    cmd = FILTERINSERT::control::stateVariableType;
                    break;
                }
                default:
                    return REPLY::available_msg;
                    break;
            }
        }
    }

    //std::cout << ">> base cmd " << int(cmd) << "  part " << int(thisPart) << "  kit " << int(kit) << "  engine " << int(engine) << "  parameter " << int(param) << std::endl;

    if (value == -1)
        value = string2float(point);

    return sendNormal( synth, 0, value, controlType, cmd, thisPart, kit, engine, TOPLEVEL::insert::filterGroup, param);
}


int CmdInterface::envelopeSelect(unsigned char controlType)
{
    int cmd = -1;
    float value = -1;
    int group = -1;
    unsigned char insert = TOPLEVEL::insert::envelopeGroup;
    unsigned char offset = UNUSED;
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    int engine = contextToEngines(context);
    if (engine == PART::engine::addVoice1 || engine == PART::engine::addMod1)
        engine += voiceNumber;

    if (matchnMove(2, point, "amplitute"))
        group = TOPLEVEL::insertType::amplitude;
    else if (matchnMove(2, point, "frequency"))
        group = TOPLEVEL::insertType::frequency;
    else if (matchnMove(2, point, "filter"))
        group = TOPLEVEL::insertType::filter;
    else if (matchnMove(2, point, "bandwidth"))
    {
        if(bitTest(context, LEVEL::SubSynth))
            group = TOPLEVEL::insertType::bandwidth;
        else
            return REPLY::available_msg;
    }

    if (group > -1)
        insertType = group;
    else
        group = insertType;

    switch (insertType)
    {
        case TOPLEVEL::insertType::amplitude:
            if (engine < PART::engine::addMod1)
                cmd = ADDVOICE::control::enableAmplitudeEnvelope;
            else
                cmd = ADDVOICE::control::enableModulatorAmplitudeEnvelope;
            break;
        case TOPLEVEL::insertType::frequency:
            if (engine < PART::engine::addMod1)
                cmd = ADDVOICE::control::enableFrequencyEnvelope;
            else
                cmd = ADDVOICE::control::enableModulatorFrequencyEnvelope;
            break;
        case TOPLEVEL::insertType::filter:
            cmd = ADDVOICE::control::enableFilterEnvelope;
            break;
        case TOPLEVEL::insertType::bandwidth:
            cmd = SUBSYNTH::control::enableBandwidthEnvelope;
            break;
    }
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    value = toggle(point);
    if (value > -1)
    {
        if (engine != PART::engine::addSynth && engine != PART::engine::padSynth)
            return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, engine);
        else
            return REPLY::available_msg;
    }

    if (matchnMove(2, point, "fmode"))
    {
        return sendNormal( synth, 0, (toggle(point) == 1), controlType, ENVELOPEINSERT::control::enableFreeMode, npart, kitNumber, engine, TOPLEVEL::insert::envelopeGroup, insertType);
    }

    // common controls
    value = -1;
    cmd = -1;
    if (matchnMove(2, point, "expand"))
        cmd = ENVELOPEINSERT::control::stretch;
    else if (matchnMove(1, point, "force"))
    {
        cmd = ENVELOPEINSERT::control::forcedRelease;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(2, point, "linear"))
    {
        cmd = ENVELOPEINSERT::control::linearEnvelope;
        value = (toggle(point) == 1);
    }

    bool freeMode = readControl(synth, 0, ENVELOPEINSERT::control::enableFreeMode, npart, kitNumber, engine, TOPLEVEL::insert::envelopeGroup, insertType);

    if (freeMode && cmd == -1)
    {
        int pointCount = readControl(synth, 0, ENVELOPEINSERT::control::points, npart, kitNumber, engine, insert, insertType);
        if (matchnMove(1, point, "Points"))
        {
            value = 0; // dummy value
            cmd = ENVELOPEINSERT::control::points;
            // not using already fetched value to get normal reporting
        }
        else if (matchnMove(1, point, "Sustain"))
        {
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            value = string2int(point);
            if (value == 0)
            {
                    synth->getRuntime().Log("Sustain can't be at first point");
                    return REPLY::done_msg;
            }
            else if (value >= (pointCount - 1))
            {
                    synth->getRuntime().Log("Sustain can't be at last point");
                    return REPLY::done_msg;
            }
            else if (value < 0)
                return REPLY::range_msg;
            cmd = ENVELOPEINSERT::control::sustainPoint;
        }
        else
        {
            if (matchnMove(1, point, "insert"))
            {
                if ((MAX_ENVELOPE_POINTS - pointCount) < 2)
                {
                    synth->getRuntime().Log("Max points already defined");
                    return REPLY::done_msg;
                }
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;

                cmd = string2int(point); // point number
                if (cmd == 0)
                {
                    synth->getRuntime().Log("Can't add at first point");
                    return REPLY::done_msg;
                }
                if (cmd < 0 || cmd >= pointCount)
                    return REPLY::range_msg;
                point = skipChars(point);
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;

                offset = string2int(point); // X
                point = skipChars(point);
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;

                value = string2int(point); // Y
                insert = TOPLEVEL::insert::envelopePoints;

            }
            else if (matchnMove(1, point, "delete"))
            {
                if (pointCount <= 3)
                {
                    synth->getRuntime().Log("Can't have less than three points");
                    return REPLY::done_msg;
                }
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;

                cmd = string2int(point); // point number
                if (cmd == 0)
                {
                    synth->getRuntime().Log("Can't delete first point");
                    return REPLY::done_msg;
                }
                if (cmd >= (pointCount - 1))
                {
                    synth->getRuntime().Log("Can't delete last point");
                    return REPLY::done_msg;
                }
                if (cmd < 0 || cmd >= (MAX_ENVELOPE_POINTS - 1))
                    return REPLY::range_msg;
                insert = TOPLEVEL::insert::envelopePoints;
            }
            else if (matchnMove(1, point, "change"))
            {
                if (lineEnd(point, controlType))
                return REPLY::value_msg;

                cmd = string2int(point); // point number
                if (cmd < 0 || cmd >= (pointCount - 1))
                    return REPLY::range_msg;
                point = skipChars(point);
                if (lineEnd(point, controlType))
                return REPLY::value_msg;

                offset = string2int(point); // X
                point = skipChars(point);
                if (lineEnd(point, controlType))
                return REPLY::value_msg;

                value = string2int(point); // Y
                insert = TOPLEVEL::insert::envelopePointChange;
            }
        }
    }
    else if (cmd == -1)
    {
        if (matchnMove(1, point, "attack"))
        {
            if (matchnMove(1, point, "level"))
                cmd = ENVELOPEINSERT::control::attackLevel;
            else if (matchnMove(1, point, "time"))
                cmd = ENVELOPEINSERT::control::attackTime;
        }
        else if (matchnMove(1, point, "decay"))
        {
            if (matchnMove(1, point, "level"))
                cmd = ENVELOPEINSERT::control::decayLevel;
            else if (matchnMove(1, point, "time"))
                cmd = ENVELOPEINSERT::control::decayTime;
        }
        else if (matchnMove(1, point, "sustain"))
            cmd = ENVELOPEINSERT::control::sustainLevel;
        else if (matchnMove(1, point, "release"))
        {
            if (matchnMove(1, point, "level"))
                cmd = ENVELOPEINSERT::control::releaseLevel;
            else if (matchnMove(1, point, "time"))
                cmd = ENVELOPEINSERT::control::releaseTime;
        }
    }

    if (cmd == -1)
        return REPLY::op_msg;

    if (value == -1)
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        value = string2float(point);
    }

    //std::cout << ">> base cmd " << int(cmd) << "  part " << int(npart) << "  kit " << int(kitNumber) << "  engine " << int(engine) << "  parameter " << int(insertType) << std::endl;

    return sendNormal( synth, 0, string2float(point), controlType, cmd, npart, kitNumber, engine, insert, insertType, offset);
}

int CmdInterface::commandGroup()
{
    string line;
    float value = string2int(point);
    if (point[0] == 0)
    {
        synth->getRuntime().Log("\nInstrument Groups");
        for (int i = 0; i < 17; ++ i)
        {
            line = "  " + instrumentGroupType[i];
            synth->getRuntime().Log(line);
        }
        return REPLY::done_msg;
    }



    std::string name = std::string(point);
    value = stringNumInList(name, instrumentGroupType, 2) + 1;
    cout << value << endl;
    if (value < 1)
        return REPLY::range_msg;


    list<std::string> msg;
    // having two lists is messy but the list routine clears 'msg'
    // while we need 'instrumentGroup' kept for actual part loads
    point = skipChars(point);
    bool full = (matchnMove(1, point, "location"));

    int count = 0;
    if (!instrumentGroup.empty())
        instrumentGroup.clear();
    do {
        ++ count;
        line = miscMsgPop(readControl(synth, 0, BANK::control::findInstrumentName, TOPLEVEL::section::bank, UNUSED, UNUSED, UNUSED, value - 1));
        if (line != "*")
        {
            instrumentGroup.push_back(line);
            if (!full && line.length() > 16)
                line = line.substr(15);
            line = to_string(count) + "| " + line;
            msg.push_back(line);
        }
    } while (line != "*");
    synth->cliOutput(msg, LINES);
    return REPLY::done_msg;
}


int CmdInterface::commandList()
{
    Config &Runtime = synth->getRuntime();
    int ID;
    int tmp;
    list<std::string> msg;

    if (matchnMove(1, point, "instruments") || matchnMove(2, point, "programs"))
    {
        if (point[0] == 0)
            ID = 128;
        else
            ID = string2int(point);
        synth->ListInstruments(ID, msg);
        synth->cliOutput(msg, LINES);
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "banks"))
    {
        if (point[0] == 0)
            ID = 128;
        else
            ID = string2int(point);
        synth->ListBanks(ID, msg);
        synth->cliOutput(msg, LINES);
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "roots"))
    {
        synth->ListPaths(msg);
        synth->cliOutput(msg, LINES);
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "vectors"))
    {
        synth->ListVectors(msg);
        synth->cliOutput(msg, LINES);
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "parts"))
    {
        listCurrentParts(msg);
        synth->cliOutput(msg, LINES);
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "config"))
    {
        synth->ListSettings(msg);
        synth->cliOutput(msg, LINES);
        return REPLY::done_msg;
    }

    if (matchnMove(2, point, "mlearn"))
    {
        if (point[0] == '@')
        {
            point += 1;
            point = skipSpace(point);
            tmp = string2int(point);
            if (tmp > 0)
                synth->midilearn.listLine(tmp - 1);
            else
                return REPLY::value_msg;
        }
        else
        {
            synth->midilearn.listAll(msg);
            synth->cliOutput(msg, LINES);
        }
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "tuning"))
    {
        Runtime.Log("Tuning:\n" + synth->microtonal.tuningtotext());
        return REPLY::done_msg;
    }
    if (matchnMove(1, point, "keymap"))
    {
        Runtime.Log("Keymap:\n" + synth->microtonal.keymaptotext());
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "history"))
    {
        if (matchnMove(1, point, "instruments") || matchnMove(2, point, "program") )
            historyList(TOPLEVEL::XML::Instrument);
        else if (matchnMove(1, point, "patchsets"))
            historyList(TOPLEVEL::XML::Patch);
        else if (matchnMove(2, point, "scales"))
            historyList(TOPLEVEL::XML::Scale);
        else if (matchnMove(2, point, "states"))
            historyList(TOPLEVEL::XML::State);
        else if (matchnMove(1, point, "vectors"))
            historyList(TOPLEVEL::XML::Vector);
        else if (matchnMove(2, point, "mlearn"))
            historyList(TOPLEVEL::XML::MLearn);
        else
            historyList(-1);
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "effects") || matchnMove(1, point, "efx"))
        return effectsList();
    if (matchnMove(3, point, "presets"))
        return effectsList(true);

    msg.push_back("Lists:");
    helpLoop(msg, listlist, 2);
    if (synth->getRuntime().toConsole)
        // we need this in case someone is working headless
        std::cout << "\nSet CONfig REPorts [s] - set report destination (gui/stderr)\n";
    synth->cliOutput(msg, LINES);
    return REPLY::done_msg;
}


void CmdInterface::listCurrentParts(list<std::string>& msg_buf)
{
    int dest;
    std::string name = "";
    int avail = readControl(synth, 0, MAIN::control::availableParts, TOPLEVEL::section::main);
    bool full = matchnMove(1, point, "more");
    if (bitFindHigh(context) == LEVEL::Part)
    {
        if (!readControl(synth, 0, PART::control::kitMode, TOPLEVEL::section::part1 + npart))
        {
            if (readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + npart, UNUSED, PART::engine::addSynth))
            {
                name += " AddSynth ";
                if (full)
                {
                    std::string found = "";
                    for(int voice = 0; voice < NUM_VOICES; ++voice)
                    {
                        if (readControl(synth, 0, ADDSYNTH::control::enable, TOPLEVEL::section::part1 + npart, 0, PART::engine::addVoice1 + voice))
                            found += (" " + std::to_string(voice + 1));
                    }
                    if (found > "")
                        name += ("Voices" + found + " ");
                }
            }
            if (readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + npart, UNUSED, PART::engine::subSynth))
                name += " SubSynth ";
            if (readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + npart, UNUSED, PART::engine::padSynth))
                name += " PadSynth ";
            if (name == "")
                name = "no engines active!";
            msg_buf.push_back(name);
            return;
        }
        msg_buf.push_back("kit items");
        for(int item = 0; item < NUM_KIT_ITEMS; ++item)
        {
            name = "";
            if (readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + npart, item, UNUSED, TOPLEVEL::insert::kitGroup))
            {
                name = "  " + std::to_string(item) + " ";
                {
                if (readControl(synth, 0, PART::control::kitItemMute, TOPLEVEL::section::part1 + npart, item, UNUSED, TOPLEVEL::insert::kitGroup))
                    name += "Quiet";
                else
                {
                    if (full)
                    {
                        name += "  key Min ";
                        int min = int(readControl(synth, 0, PART::control::minNote, TOPLEVEL::section::part1 + npart, item, UNUSED, TOPLEVEL::insert::kitGroup));
                        if (min < 10)
                            name += "  ";
                        else if (min < 100)
                            name += " ";
                        name += std::to_string(min);
                        name += "  Max ";
                        int max = int(readControl(synth, 0, PART::control::maxNote, TOPLEVEL::section::part1 + npart, item, UNUSED, TOPLEVEL::insert::kitGroup));
                        if (max < 10)
                            name += "  ";
                        else if (max < 100)
                            name += " ";

                        name += (std::to_string(max) + "  ");
                        std::string text = readControlText(synth, TOPLEVEL::action::lowPrio, PART::control::instrumentName, TOPLEVEL::section::part1 + npart, item, UNUSED, TOPLEVEL::insert::kitGroup);
                        if (text > "")
                            name += text;
                        msg_buf.push_back(name);
                        name = "    ";
                    }
                    if (readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + npart, item, PART::engine::addSynth, TOPLEVEL::insert::kitGroup))
                    {
                        name += "AddSynth ";
                        if (full)
                        {
                            std::string found = "";
                            for(int voice = 0; voice < NUM_VOICES; ++voice)
                            {
                                if (readControl(synth, 0, ADDSYNTH::control::enable, TOPLEVEL::section::part1 + npart, item, PART::engine::addVoice1 + voice))
                                found += (" " + std::to_string(voice + 1));
                            }
                            if (found > "")
                                name += ("Voices" + found + " ");
                        }
                    }
                    if (readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + npart, item, PART::engine::subSynth, TOPLEVEL::insert::kitGroup))
                        name += "SubSynth ";
                    if (readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + npart, item, PART::engine::padSynth, TOPLEVEL::insert::kitGroup))
                        name += "PadSynth ";
                    if (name == "")
                        name = "no engines active!";
                }
            }
            if (name > "")
                msg_buf.push_back(name);
            }
        }
        return;
    }
    msg_buf.push_back(asString(avail) + " parts available");
    for (int partno = 0; partno < NUM_MIDI_PARTS; ++partno)
    {
        std::string text = readControlText(synth, TOPLEVEL::action::lowPrio, PART::control::instrumentName, TOPLEVEL::section::part1 + partno);
        bool enabled = readControl(synth, 0, PART::control::enable, TOPLEVEL::section::part1 + partno);
        if (text != "Simple Sound" || enabled)
        {
            if (partno < 9)
                name = " ";
            else
                name = "";
            if (enabled && partno < avail)
                name += "+";
            else
                name += " ";
            name += std::to_string(partno + 1);
            dest = readControl(synth, 0, PART::control::audioDestination, TOPLEVEL::section::part1 + partno);
            if (partno >= avail)
                name += " - " + text;
            else
            {
                if(dest == 1)
                    name += " Main";
                else if(dest == 2)
                    name += " Part";
                else
                    name += " Both";
                name += "  Chan ";
                int ch = int(readControl(synth, 0, PART::control::midiChannel, TOPLEVEL::section::part1 + partno) + 1);
                if (ch < 10)
                    name += " ";
                name += std::to_string(ch);
                if (full)
                {
                    name += "  key Min ";
                    int min = int(readControl(synth, 0, PART::control::minNote, TOPLEVEL::section::part1 + partno));
                    if (min < 10)
                        name += "  ";
                    else if (min < 100)
                        name += " ";
                    name += std::to_string(min);
                    name += "  Max ";
                    int max = int(readControl(synth, 0, PART::control::maxNote, TOPLEVEL::section::part1 + partno));
                    if (max < 10)
                        name += "  ";
                    else if (max < 100)
                        name += " ";
                    name += std::to_string(max);
                    name += "  Shift ";
                    int shift = int(readControl(synth, TOPLEVEL::action::lowPrio, PART::control::keyShift, TOPLEVEL::section::part1 + partno));
                    if (shift >= 10)
                        name += " ";
                    else if (shift >= 0)
                        name += "  ";
                    else if (shift >= -10)
                        name += " ";
                    name += std::to_string(shift);

                }
                name +=  ("  " + text);
                int mode = readControl(synth, 0, PART::control::kitMode, TOPLEVEL::section::part1 + partno);
                if (mode != PART::kitType::Off)
                    name += " > ";
                switch (mode)
                {
                    case PART::kitType::Multi:
                        name += "Multi";
                        break;
                    case PART::kitType::Single:
                        name += "Single";
                        break;
                    case PART::kitType::CrossFade:
                        name += "Crossfade";
                        break;
                }
            }
            msg_buf.push_back(name);
            if (full)
            {
                name = "    Drum ";
                int drum = readControl(synth, 0, PART::control::drumMode, TOPLEVEL::section::part1 + partno);
                if (drum)
                    name += " on";
                else
                    name += "off";
                name += " Portamento ";
                if (readControl(synth, 0, PART::control::portamento, TOPLEVEL::section::part1 + partno))
                    name += " on";
                else name += "off";
                int key = readControl(synth, 0, PART::control::keyMode, TOPLEVEL::section::part1 + partno);
                switch (key)
                {
                    case 0:
                        name += "  Polphonic";
                        break;
                    case 1:
                        name += "  Monophonic";
                        break;
                    case 2:
                        name += "  Legato";
                        if (drum)
                            name += " (drum blocked)";
                        break;
                }
                msg_buf.push_back(name);
            }
        }
    }
}


int CmdInterface::commandMlearn(unsigned char controlType)
{
    Config &Runtime = synth->getRuntime();
    list<std::string> msg;
    bitSet(context, LEVEL::Learn);

    if (controlType != TOPLEVEL::type::Write)
    {
        Runtime.Log("Write only");
        return REPLY::done_msg; // will eventually be readable
    }

    if (isdigit(point[0]) || point[0] == '-') // negative should never happen!
    {
        int lineNo = string2int(point);
        point = skipChars(point);
        if (lineNo <= 0)
            return REPLY::value_msg;
        else
            mline = lineNo -1;
    }
    int tmp = synth->midilearn.findSize();
    if (tmp == 0 || tmp <= mline)
    {
        if (tmp == 0)
            Runtime.Log("No learned lines");
        else
            Runtime.Log("Line " + std::to_string(mline + 1) + " Not found");
        mline = 0;
        return (REPLY::done_msg);
    }
    if (lineEnd(point, controlType))
        return REPLY::done_msg;
    {
        unsigned char type = 0;
        unsigned char control = 0;
        unsigned char kit = UNUSED;
        unsigned char engine = UNUSED;
        unsigned char insert = UNUSED;
        unsigned char parameter = UNUSED;

        if (matchnMove(2, point, "cc"))
        {
            if (!isdigit(point[0]))
                return REPLY::value_msg;
            kit = string2int(point);
            if (kit > 129)
            {
                Runtime.Log("Max CC value is 129");
                return REPLY::done_msg;
            }
            control = MIDILEARN::control::CCorChannel;
            Runtime.Log("Lines may be re-ordered");
        }
        else if (matchnMove(2, point, "channel"))
        {
            engine = string2int(point) - 1;
            if (engine > 16)
                engine = 16;
            control = MIDILEARN::control::CCorChannel;
            Runtime.Log("Lines may be re-ordered");;
        }
        else if (matchnMove(2, point, "minimum"))
        {
            insert = int((string2float(point)* 2.0f) + 0.5f);
            if (insert > 200)
                return REPLY::value_msg;
            control = MIDILEARN::control::minimum;
        }
        else if (matchnMove(2, point, "maximum"))
        {
            parameter = int((string2float(point)* 2.0f) + 0.5f);
            if (parameter > 200)
                return REPLY::value_msg;
            control = MIDILEARN::control::maximum;
        }
        else if (matchnMove(2, point, "mute"))
        {
            type = (toggle(point) == 1) * 4;
            control = MIDILEARN::control::mute;
        }
        else if (matchnMove(2, point, "limit"))
        {
            type = (toggle(point) == 1) * 2;
            control = MIDILEARN::control::limit;
        }
        else if (matchnMove(2, point, "block"))
        {
            type = (toggle(point) == 1);
            control = MIDILEARN::control::block;
        }
        else if (matchnMove(2, point, "seven"))
        {
            type = (toggle(point) == 1) * 16;
            control = MIDILEARN::control::sevenBit;
        }
        sendNormal( synth, 0, mline, type, control, TOPLEVEL::section::midiLearn, kit, engine, insert, parameter);
        return REPLY::done_msg;
    }
    return REPLY::op_msg;
}


int CmdInterface::commandVector(unsigned char controlType)
{
    Config &Runtime = synth->getRuntime();
    list<std::string> msg;
    int tmp;
    bitSet(context, LEVEL::Vector);
    if (controlType != TOPLEVEL::type::Write)
    {
        if (synth->SingleVector(msg, chan))
            synth->cliOutput(msg, LINES);
        else
            Runtime.Log("No vector on channel " + asString(chan + 1));
        return REPLY::done_msg;
    }
    if (lineEnd(point, controlType))
    {
        if (!Runtime.vectordata.Enabled[chan])
            Runtime.Log("No vector on channel " + asString(chan + 1));
        return REPLY::done_msg;
    }

    unsigned char ch = string2int127(point);
    if (ch > 0)
    {
        ch -= 1;
        if (ch >= NUM_MIDI_CHANNELS)
            return REPLY::range_msg;
        point = skipChars(point);
        if (chan != ch)
        {
            chan = ch;
            axis = 0;
        }

        Runtime.Log("Vector channel set to " + asString(chan + 1));
    }

    if (matchWord(1, point, "off"))
    {
        sendDirect(synth, 0, 0,controlType,VECTOR::control::erase, TOPLEVEL::section::vector, UNUSED, UNUSED, chan);
        axis = 0;
        bitClear(context, LEVEL::Vector);
        return REPLY::done_msg;
    }
    if (matchnMove(1, point, "xaxis"))
        axis = 0;
    else if (matchnMove(1, point, "yaxis"))
    {
        if (!Runtime.vectordata.Enabled[chan])
        {
            Runtime.Log("Vector X must be set first");
            return REPLY::done_msg;
        }
        axis = 1;
    }

    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    if (matchnMove(2, point, "cc"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;

        tmp = string2int(point);
        if (axis == 0)
        {
            sendDirect(synth, 0, tmp, controlType, VECTOR::control::Xcontroller, TOPLEVEL::section::vector, UNUSED, UNUSED, chan);
            bitSet(context, LEVEL::Vector);
            return REPLY::done_msg;
        }
        if (Runtime.vectordata.Enabled[chan])
        {
            sendDirect(synth, 0, tmp, controlType, VECTOR::control::Ycontroller, TOPLEVEL::section::vector, UNUSED, UNUSED, chan);
            return REPLY::done_msg;
        }
    }

    if (!Runtime.vectordata.Enabled[chan])
    {
        Runtime.Log("Vector X CC must be set first");
        return REPLY::done_msg;
    }

    if (axis == 1 && (Runtime.vectordata.Yaxis[chan] > 0x7f))
    {
        Runtime.Log("Vector Y CC must be set first");
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "name"))
    {
        std::string name = "!";
        if (controlType == TOPLEVEL::type::Write)
        {
            name = std::string(point);
            if (name <= "!")
                return REPLY::value_msg;
        }
        sendDirect(synth, TOPLEVEL::action::lowPrio, 0, controlType, VECTOR::control::name, TOPLEVEL::section::vector, UNUSED, UNUSED, chan, UNUSED, UNUSED, miscMsgPush(name));
        return REPLY::done_msg;
    }

    if (matchnMove(1, point, "features"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        int feat = string2int(point) - 1;
        if (feat < 0 || feat > 3)
            return REPLY::range_msg;
        point = skipChars(point);
        int enable = 0;
        if (toggle(point) == 1)
            enable = 1;
        else if (feat > 1 && matchnMove(1, point, "reverse"))
            enable = 2;
        sendDirect(synth, 0, enable, controlType, VECTOR::control::Xfeature0 + (axis * (VECTOR::control::Ycontroller - VECTOR::control::Xcontroller)) + feat , TOPLEVEL::section::vector, UNUSED, UNUSED, chan);
        return REPLY::done_msg;
    }

    if (matchnMove(2, point, "program") || matchnMove(1, point, "instrument"))
    {
        int hand = point[0] | 32;
        point = skipChars(point); // in case they type the entire word
        if ((axis == 0 && (hand == 'd' || hand == 'u')) || (axis == 1 && (hand == 'l' || hand == 'r')))
        {
            Runtime.Log("Bad direction for this axis");
            return REPLY::done_msg;
        }
        if (hand == 'l' || hand == 'd')
            hand = 0;
        else if (hand == 'r' || hand == 'u')
            hand = 1;
        else
            return REPLY::op_msg;
        tmp = string2int(point);
        sendDirect(synth, 0, tmp, controlType, VECTOR::control::XleftInstrument + hand + (axis * (VECTOR::control::Ycontroller - VECTOR::control::Xcontroller)), TOPLEVEL::section::vector, UNUSED, UNUSED, chan);
        return REPLY::done_msg;
    }

    // this disabled for now - needs a lot of work.
    /*if (!matchnMove(1, point, "control"))
        return REPLY::op_msg;
    if(isdigit(point[0]))
    {
        int cmd = string2int(point);
        if (cmd < 2 || cmd > 4)
            return REPLY::range_msg;
        point = skipChars(point);
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        tmp = string2int(point);
        if (!synth->vectorInit(axis * 3 + cmd + 6, chan, tmp))
        {
            synth->vectorSet(axis * 3 + cmd + 6, chan, tmp);
            return REPLY::done_msg;
        }
        else
            return REPLY::value_msg;
    }*/

    return REPLY::op_msg;
}


int CmdInterface::commandConfig(unsigned char controlType)
{
    float value = 0;
    unsigned char command = UNUSED;
    unsigned char action = 0;
    unsigned char miscmsg = UNUSED;

    if (matchnMove(1, point, "oscillator"))
    {
        command = CONFIG::control::oscillatorSize;
        if (controlType == TOPLEVEL::type::Write && point[0] == 0)
            return REPLY::value_msg;
        value = string2int(point);
    }
    else if (matchnMove(2, point, "buffer"))
    {
        command = CONFIG::control::bufferSize;
        if (controlType == TOPLEVEL::type::Write && point[0] == 0)
            return REPLY::value_msg;
        value = string2int(point);
    }
    else if (matchnMove(2, point, "padsynth"))
    {
        command = CONFIG::control::padSynthInterpolation;
        value = !matchnMove(1, point, "linear");
    }
    else if (matchnMove(1, point, "virtual"))
    {
        command = CONFIG::control::virtualKeyboardLayout;
        if (controlType == TOPLEVEL::type::Write && point[0] == 0)
            return REPLY::value_msg;
        value = string2int(point);
    }
    else if (matchnMove(1, point, "xml"))
    {
        command = CONFIG::control::XMLcompressionLevel;
        if (controlType == TOPLEVEL::type::Write && point[0] == 0)
            return REPLY::value_msg;
        value = string2int(point);
    }
    else if (matchnMove(2, point, "reports"))
    {
        command = CONFIG::control::reportsDestination;
        value = !matchnMove(1, point, "stdout");
    }
    else if (matchnMove(2, point, "saved"))
    {
        command = CONFIG::control::savedInstrumentFormat;
        if (matchnMove(1, point, "legacy"))
            value = 1;
        else if (matchnMove(1, point, "yoshimi"))
            value = 2;
        else if (matchnMove(1, point, "both"))
            value = 3;
        else if (controlType == TOPLEVEL::type::Write)
            return REPLY::value_msg;
    }

    else if (matchnMove(2, point, "state"))
    {
        command = CONFIG::control::defaultStateStart;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(2, point, "single"))
    {
        command = CONFIG::control::enableSinglePath;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "hide"))
    {
        command = CONFIG::control::hideNonFatalErrors;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "display"))
    {
        command = CONFIG::control::showSplash;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "time"))
    {
        command = CONFIG::control::logInstrumentLoadTimes;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "include"))
    {
        command = CONFIG::control::logXMLheaders;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "keep"))
    {
        command = CONFIG::control::saveAllXMLdata;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "gui"))
    {
        command = CONFIG::control::enableGUI;
        value = toggle(point);
        if (value == -1)
            return REPLY::value_msg;
    }
    else if (matchnMove(1, point, "cli"))
    {
        command = CONFIG::control::enableCLI;
        value = toggle(point);
        if (value == -1)
            return REPLY::value_msg;
    }

    else if (matchnMove(3, point, "expose"))
    {
        value = toggle(point);
        if (value == -1 && matchnMove(2, point, "prompt"))
            value = 2;
        if (value == -1)
            return REPLY::value_msg;
        command = CONFIG::control::exposeStatus;
    }

    else if (matchnMove(1, point, "jack"))
    {
        if (matchnMove(1, point, "midi"))
        {
            command = CONFIG::control::jackMidiSource;
            action = TOPLEVEL::action::lowPrio;
            if (controlType != TOPLEVEL::type::Write || point[0] != 0)
            {
                if (controlType == TOPLEVEL::type::Write)
                    miscmsg = miscMsgPush(std::string(point));
            }
            else
                return REPLY::value_msg;
        }
        else if (matchnMove(1, point, "server"))
        {
            command = CONFIG::control::jackServer;
            action = TOPLEVEL::action::lowPrio;
            if (controlType != TOPLEVEL::type::Write || point[0] != 0)
            {
                if (controlType == TOPLEVEL::type::Write)
                    miscmsg = miscMsgPush(std::string(point));
            }
            else
                return REPLY::value_msg;
        }
        else if (matchnMove(1, point, "auto"))
        {
            command = CONFIG::control::jackAutoConnectAudio;
            value = (toggle(point) == 1);
        }
        else
            return REPLY::op_msg;
    }

    else if (matchnMove(2, point, "alsa"))
    {
        if (matchnMove(1, point, "midi"))
        {
            command = CONFIG::control::alsaMidiSource;
            action = TOPLEVEL::action::lowPrio;
            if (controlType != TOPLEVEL::type::Write || point[0] != 0)
            {
                if (controlType == TOPLEVEL::type::Write)
                    miscmsg = miscMsgPush(std::string(point));
            }
            else
                return REPLY::value_msg;
        }
        else if (matchnMove(1, point, "audio"))
        {
            command = CONFIG::control::alsaAudioDevice;
            action = TOPLEVEL::action::lowPrio;
            if (controlType != TOPLEVEL::type::Write || point[0] != 0)
            {
                if (controlType == TOPLEVEL::type::Write)
                    miscmsg = miscMsgPush(std::string(point));
            }
            else
                return REPLY::value_msg;
        }
        else if (matchnMove(1, point, "s"))
        {
            command = CONFIG::control::alsaSampleRate;
            if (controlType == TOPLEVEL::type::Write)
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int(point);
                if (value < 0 || value > 3)
                    return REPLY::range_msg;
            }
        }
        else
            return REPLY::op_msg;
    }

    else if (matchnMove(2, point, "midi"))
    {
        value = 1;
        if (matchnMove(1, point, "alsa"))
            command = CONFIG::control::alsaPreferredMidi;
        else if (controlType != TOPLEVEL::type::Write || matchnMove(1, point, "jack"))
            command = CONFIG::control::jackPreferredMidi;
        else
            return REPLY::value_msg;
    }

    else if (matchnMove(2, point, "audio"))
    {
        value = 1;
        if (matchnMove(1, point, "alsa"))
            command = CONFIG::control::alsaPreferredAudio;
        else if (controlType != TOPLEVEL::type::Write || matchnMove(1, point, "jack"))
            command = CONFIG::control::jackPreferredAudio;
        else
            return REPLY::value_msg;
    }

    else if (matchnMove(2, point, "root"))
    {
        command = CONFIG::control::bankRootCC;
        //if (controlType != TOPLEVEL::type::Write)
            value = 128; // ignored by range check
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        if (matchnMove(1, point, "msb"))
            value = 0;
        else if (matchnMove(1, point, "lsb"))
            value = 32;
    }
    else if (matchnMove(2, point, "bank"))
    {
        command = CONFIG::control::bankCC;
        //if (controlType != TOPLEVEL::type::Write)
            value = 128; // ignored by range check
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        if (matchnMove(1, point, "msb"))
            value = 0;
        else if (matchnMove(1, point, "lsb"))
            value = 32;
    }
    else if (matchnMove(2, point, "program") || matchnMove(2, point, "instrument"))
    {
        command = CONFIG::control::enableProgramChange;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(2, point, "activate"))
    {
        command = CONFIG::control::instChangeEnablesPart;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(3, point, "extend"))
    {
        command = CONFIG::control::extendedProgramChangeCC;
        if (controlType != TOPLEVEL::type::Write)
            value = 128; // ignored by range check
        else if (lineEnd(point, controlType))
            return REPLY::value_msg;
        else
            value = string2int(point);
    }
    else if (matchnMove(1, point, "quiet"))
    {
        command = CONFIG::control::ignoreResetAllCCs;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "log"))
    {
        command = CONFIG::control::logIncomingCCs;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(2, point, "show"))
    {
        command = CONFIG::control::showLearnEditor;
        value = (toggle(point) == 1);
    }
    else if (matchnMove(1, point, "nrpn"))
    {
        command = CONFIG::control::enableNRPNs;
        value = (toggle(point) == 1);
    }

    else if (matchnMove(3, point, "lock"))
    {
        command = CONFIG::control::historyLock;
        value = (toggle(point));
        std::string name = std::string(point).substr(0,2);
        int selected = stringNumInList(name, historyGroup, 2);
        if (selected == -1)
            return REPLY::range_msg;
        point = skipChars(point);
        value = (toggle(point));
        if (controlType == TOPLEVEL::type::Write && value == -1)
            return REPLY::value_msg;
        return sendDirect(synth, TOPLEVEL::action::lowPrio, value, controlType, command, TOPLEVEL::section::config, selected);
    }

    else
        return  REPLY::op_msg;

    sendDirect(synth, action, value, controlType, command, TOPLEVEL::section::config, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscmsg);
    return REPLY::done_msg;
}


int CmdInterface::commandScale(unsigned char controlType)
{
    if (lineEnd(point, controlType))
        return REPLY::done_msg;
    Config &Runtime = synth->getRuntime();
    float value = 0;
    unsigned char command = UNUSED;
    unsigned char action = 0;
    unsigned char miscmsg = UNUSED;
    if (controlType != TOPLEVEL::type::Write)
        return REPLY::done_msg;

    std::string name;

    if (matchnMove(1, point, "tuning"))
        command = SCALES::control::tuning;
    else if (matchnMove(1, point, "keymap"))
        command = SCALES::control::keyboardMap;
    else if (matchnMove(2, point, "name"))
        command = SCALES::control::name;
    else if (matchnMove(2, point, "description"))
        command = SCALES::control::comment;

    if (command >= SCALES::control::tuning && command <= SCALES::control::comment)
    {
        if (controlType != TOPLEVEL::type::Write && command <= SCALES::control::importKbm)
        {
            Runtime.Log("Write only - use list");
            return REPLY::done_msg;
        }
        if (command <= SCALES::control::keyboardMap)
        {
            if (matchnMove(3, point, "import"))
                command += (SCALES::control::importKbm - SCALES::control::keyboardMap);
        }
        name = (std::string)point;
        if (name == "")
            return REPLY::value_msg;
        action = TOPLEVEL::action::lowPrio;
        miscmsg = miscMsgPush(name);
    }
    else
    {
        int min = 0;
        int max = 127;
        if (matchnMove(2, point, "frequency"))
        {
            command = SCALES::control::Afrequency;
            min = 1;
            max = 20000;
            controlType &= ~TOPLEVEL::type::Integer; // float
        }
        else if(matchnMove(2, point, "note"))
            command = SCALES::control::Anote;
        else if(matchnMove(1, point, "invert"))
        {
            command = SCALES::control::invertScale;
            max = 1;
        }
        else if(matchnMove(2, point, "center"))
            command = SCALES::control::invertedScaleCenter;
        else if(matchnMove(2, point, "shift"))
        {
            command = SCALES::control::scaleShift;
            min = -63;
            max = 64;
        }
        else if(matchnMove(2, point, "scale"))
        {
            command = SCALES::control::enableMicrotonal;
            max = 1;
        }
        else if(matchnMove(2, point, "mapping"))
        {
            command = SCALES::control::enableKeyboardMap;
            max = 1;
        }
        else if(matchnMove(2, point, "first"))
            command = SCALES::control::lowKey;
        else if(matchnMove(2, point, "middle"))
            command = SCALES::control::middleKey;
        else if(matchnMove(1, point, "last"))
            command = SCALES::control::highKey;
        else if(matchnMove(3, point, "CLEar"))
        {
            point -=1; // sneaky way to force a zero :)
            command = SCALES::control::clearAll;
        }
        else
            return REPLY::todo_msg;

        if (controlType == TOPLEVEL::type::Write)
        {
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            if ((toggle(point) == 1))
                value = 1;
            else//if (isdigit(point[0]))
            {
                value = string2float(point);
                if (value < min || value > max)
                    return REPLY::value_msg;
            }
        }
    }
    sendDirect(synth, action, value, controlType, command, TOPLEVEL::section::scales, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscmsg);
    return REPLY::done_msg;
}


int CmdInterface::modulator(unsigned char controlType)
{
    if (lineEnd(point, controlType))
        return REPLY::done_msg;
    int value = -1;
    int cmd = -1;
    if (matchnMove(3, point, "off"))
            value = 0;
        else if (matchnMove(2, point, "morph"))
            value = 1;
        else if (matchnMove(2, point, "ring"))
            value = 2;
        else if (matchnMove(2, point, "phase"))
            value = 3;
        else if (matchnMove(2, point, "frequency"))
            value = 4;
        else if (matchnMove(2, point, "pulse"))
            value = 5;
        if (value != -1)
            cmd = ADDVOICE::control::modulatorType;
    if (cmd == -1)
    {
        if (readControl(synth, 0, ADDVOICE::control::modulatorType, npart, kitNumber, PART::engine::addVoice1 + voiceNumber) == 0)
            return REPLY::inactive_msg;
        if (matchnMove(2, point, "waveform"))
        {
            bitSet(context, LEVEL::Oscillator);
            return waveform(controlType);
        }

        if (matchnMove(2, point, "source"))
        {
            if (matchnMove(1, point, "local"))
                value = 0;
            else
            {
                int tmp = point[0] - char('0');
                if (tmp > 0)
                    value = tmp;
            }
            if (value == -1 || value > voiceNumber)
                return REPLY::range_msg;
            if (value == 0)
                value = 0xff;
            else
                value -= 1;
            cmd = ADDVOICE::control::externalModulator;
        }

        if (matchnMove(3, point, "oscillator"))
        {
            if (matchnMove(1, point, "internal"))
                value = 0;
            else
            {
                int tmp = point[0] - char('0');
                if (tmp > 0)
                    value = tmp;
            }
            if (value == -1 || value > voiceNumber)
                return REPLY::range_msg;
            if (value == 0)
                value = 0xff;
            else
                value -= 1;
            cmd = ADDVOICE::control::modulatorOscillatorSource;
        }

        else if (matchnMove(3, point, "follow"))
        {
            value = (toggle(point) == 1);
            cmd = ADDVOICE::control::modulatorDetuneFromBaseOsc;
        }

        else if (matchnMove(1, point, "volume"))
            cmd = ADDVOICE::control::modulatorAmplitude;
        else if(matchnMove(2, point, "velocity"))
            cmd = ADDVOICE::control::modulatorVelocitySense;
        else if(matchnMove(2, point, "damping"))
            cmd = ADDVOICE::control::modulatorHFdamping;
    }

    if (cmd == -1)
    {
        if (readControl(synth, 0, ADDVOICE::control::externalModulator, npart, kitNumber, PART::engine::addVoice1 + voiceNumber) != -1)
            return  REPLY::inactive_msg;

        if (matchnMove(2, point, "local"))
        {
            if (matchnMove(1, point, "internal"))
                value = 0;
            else
            {
                int tmp = point[0] - char('0');
                if (tmp > 0)
                    value = tmp;
            }
            if (value == -1 || value > voiceNumber)
                return REPLY::range_msg;
            if (value == 0)
                value = 0xff;
            else
                value -= 1;
            cmd = ADDVOICE::control::modulatorOscillatorSource;
        }
        if (matchnMove(2, point, "shift"))
            cmd = ADDVOICE::control::modulatorOscillatorPhase;
    }

    if (cmd > -1)
    {
        if (value == -1)
            value = string2int(point);
        else if (value == 0xff)
            value = -1; // special case for modulator sources
        return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, PART::engine::addVoice1 + voiceNumber);
    }

/*
 * The following control need to be integrated with
 * partCommonControls(), but this needs checking for
 * possible clashes. The envelope enable controls can
 * then also be more fully integrated.
 */

    if (matchnMove(3, point, "envelope"))
    {
        bitSet(context, LEVEL::Envelope);
        return envelopeSelect(controlType);
    }

    if (cmd == -1)
        return partCommonControls(controlType);//REPLY::available_msg;

    return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, PART::engine::addVoice1 + voiceNumber);
}


int CmdInterface::addVoice(unsigned char controlType)
{
    if (isdigit(point[0]))
    {
        int tmp = string2int(point) - 1;
        if (tmp < 0 || tmp >= NUM_VOICES)
            return REPLY::range_msg;
        voiceNumber = tmp;
        point = skipChars(point);
    }
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    int enable = (toggle(point));
    if (enable > -1)
    {
        sendNormal( synth, 0, enable, controlType, ADDVOICE::control::enableVoice, npart, kitNumber, PART::engine::addVoice1 + voiceNumber);
        return REPLY::done_msg;
    }
    if (!lineEnd(point, controlType) && !readControl(synth, 0, ADDVOICE::control::enableVoice, npart, kitNumber, PART::engine::addVoice1 + voiceNumber))
        return REPLY::inactive_msg;

    if (matchnMove(2, point, "modulator"))
    {
        bitSet(context, LEVEL::AddMod);
        return modulator(controlType);
    }
    else if (matchnMove(2, point, "waveform"))
    {
        bitSet(context, LEVEL::Oscillator);
        return waveform(controlType);
    }

    int value = -1;
    int cmd = -1;
    int result = partCommonControls(controlType);
    if (result != REPLY::todo_msg)
        return result;

    if (cmd == -1)
    {
        if (matchnMove(1, point, "type"))
        {
            if (matchnMove(1, point, "oscillator"))
                value = 0;
            else if (matchnMove(1, point, "white"))
                value = 1;
            else if (matchnMove(1, point, "pink"))
                value = 2;
            else if (matchnMove(1, point, "spot"))
                value = 3;
            else
                return REPLY::range_msg;
            cmd = ADDVOICE::control::soundType;
        }
        else if (matchnMove(3, point, "oscillator"))
        {
            if (matchnMove(1, point, "internal"))
                value = 0;
            else
            {
                int tmp = point[0] - char('0');
                if (tmp > 0)
                    value = tmp;
            }
            if (value == -1 || value > voiceNumber)
                return REPLY::range_msg;
            if (value == 0)
                value = 0xff;
            else
                value -= 1;
            cmd = ADDVOICE::control::voiceOscillatorSource;
        }
        else if (matchnMove(3, point, "source"))
        {
            if (matchnMove(1, point, "local"))
                value = 0;
            else
            {
                int tmp = point[0] - char('0');
                if (tmp > 0)
                    value = tmp;
            }
            if (value == -1 || value > voiceNumber)
                return REPLY::range_msg;
            if (value == 0)
                value = 0xff;
            else
                value -= 1;
            cmd = ADDVOICE::control::externalOscillator;
        }
        else if (matchnMove(1, point, "phase"))
            cmd = ADDVOICE::control::voiceOscillatorPhase;
        else if (matchnMove(1, point, "minus"))
        {
            value = (toggle(point) == 1);
            cmd = ADDVOICE::control::invertPhase;
        }
        else if (matchnMove(3, point, "delay"))
            cmd = ADDVOICE::control::delay;
        else if (matchnMove(1, point, "resonance"))
        {
            value = (toggle(point) == 1);
            cmd = ADDVOICE::control::enableResonance;
        }
        else if (matchnMove(2, point, "bypass"))
        {
            value = (toggle(point) == 1);
            cmd = ADDVOICE::control::bypassGlobalFilter;
        }
        else if (matchnMove(1, point, "unison"))
        {
            value = toggle(point);
            if (value > -1)
                cmd = ADDVOICE::control::enableUnison;
            else
            {
                if (matchnMove(1, point, "size"))
                    cmd = ADDVOICE::control::unisonSize;
                else if(matchnMove(1, point, "frequency"))
                    cmd = ADDVOICE::control::unisonFrequencySpread;
                else if(matchnMove(1, point, "phase"))
                    cmd = ADDVOICE::control::unisonPhaseRandomise;
                else if(matchnMove(1, point, "width"))
                    cmd = ADDVOICE::control::unisonStereoSpread;
                else if(matchnMove(1, point, "vibrato"))
                    cmd = ADDVOICE::control::unisonVibratoDepth;
                else if(matchnMove(1, point, "rate"))
                    cmd = ADDVOICE::control::unisonVibratoSpeed;
                else if(matchnMove(1, point, "invert"))
                {
                    if (matchnMove(1, point, "none"))
                        value = 0;
                    else if (matchnMove(1, point, "random"))
                        value = 1;
                    else if (matchnMove(1, point, "half"))
                        value = 2;
                    else if (matchnMove(1, point, "third"))
                        value = 3;
                    else if (matchnMove(1, point, "quarter"))
                        value = 4;
                    else if (matchnMove(1, point, "fifth"))
                        value = 5;
                    else
                        return REPLY::value_msg;
                    cmd = ADDVOICE::control::unisonPhaseInvert;
                }
            }
            if (cmd == -1)
                return REPLY::op_msg;
        }
        else
            return REPLY::op_msg;
    }

    if (value == -1)
        value = string2int(point);
    else if (value == 0xff)
            value = -1; // special case for voice and oscillator sources
    return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, PART::engine::addVoice1 + voiceNumber);
}


int CmdInterface::addSynth(unsigned char controlType)
{
    int kit = UNUSED;
    int insert = UNUSED;
    if (kitMode)
    {
        kit = kitNumber;
        insert = TOPLEVEL::insert::kitGroup;
    }
    int enable = (toggle(point));
    if (enable > -1)
    {
        sendNormal( synth, 0, enable, controlType, PART::control::enable, npart, kit, PART::engine::addSynth, insert);
        return REPLY::done_msg;
    }
    if (!lineEnd(point, controlType) && !readControl(synth, 0, PART::control::enable, npart, kit, PART::engine::addSynth, insert))
        return REPLY::inactive_msg;

    if (matchnMove(2, point, "resonance"))
    {
        bitSet(context, LEVEL::Resonance);
        return resonance(controlType);
    }
    if (matchnMove(3, point, "voice"))
    {
        bitSet(context, LEVEL::AddVoice);
        insertType = TOPLEVEL::insertType::amplitude;
        return addVoice(controlType);
    }
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    int result = partCommonControls(controlType);
    if (result != REPLY::todo_msg)
        return result;

    int cmd = -1;
    int value;
    if (matchnMove(2, point, "bandwidth"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        value = string2int(point);
        cmd = ADDSYNTH::control::relativeBandwidth;
    }
    else if (matchnMove(2, point, "group"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        value = (toggle(point) == 1);
        cmd = ADDSYNTH::control::randomGroup;
    }
    if (cmd == -1)
        return REPLY::available_msg;

    return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, PART::engine::addSynth);
}


int CmdInterface::subSynth(unsigned char controlType)
{
    int kit = UNUSED;
    int insert = UNUSED;
    if (kitMode)
    {
        kit = kitNumber;
        insert = TOPLEVEL::insert::kitGroup;
    }
    int enable = (toggle(point));
    if (enable > -1)
    {
        sendNormal( synth, 0, enable, controlType, PART::control::enable, npart, kit, PART::engine::subSynth, insert);
        return REPLY::done_msg;
    }
    if (!lineEnd(point, controlType) && !readControl(synth, 0, PART::control::enable, npart, kit, PART::engine::subSynth, insert))
        return REPLY::inactive_msg;

    if (lineEnd(point, controlType))
        return REPLY::done_msg;
    int result = partCommonControls(controlType);
    if (result != REPLY::todo_msg)
        return result;

    int cmd = -1;
    if (matchnMove(2, point, "harmonic"))
    {
        if (matchnMove(1, point, "stages"))
            cmd = SUBSYNTH::control::filterStages;
        else if (matchnMove(1, point, "mag"))
            cmd = SUBSYNTH::control::magType;
        else if (matchnMove(1, point, "position"))
            cmd = SUBSYNTH::control::startPosition;
        if (cmd != -1)
        {
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            return sendNormal( synth, 0, string2int(point), controlType, cmd, npart, kitNumber, PART::engine::subSynth);
        }

        int control = -1;
        unsigned char insert = UNUSED;
        bool set = false;
        if (lineEnd(point, controlType))
            return REPLY::parameter_msg;
        control = string2int(point) - 1;
        point = skipChars(point);
        if (matchnMove(1, point, "amplitude"))
        {
            insert = TOPLEVEL::insert::harmonicAmplitude;
            set = true;
        }
        else if (matchnMove(1, point, "bandwidth"))
        {
            insert = TOPLEVEL::insert::harmonicPhaseBandwidth;
            set = true;
        }
        if (set)
        {
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            return sendNormal( synth, 0, string2int(point), controlType, control, npart, kitNumber, PART::engine::subSynth, insert);
        }
    }

    float value = -1;
    if (cmd == -1)
    {
        if (matchnMove(2, point, "band"))
        {
            if (matchnMove(1, point, "width"))
                cmd = SUBSYNTH::control::bandwidth;
            else if (matchnMove(1, point, "scale"))
                cmd = SUBSYNTH::control::bandwidthScale;
            else if (matchnMove(1, point, "envelope"))
            {
                value = (toggle(point) == 1);
                cmd = SUBSYNTH::control::enableBandwidthEnvelope;
            }
        }
        else if (matchnMove(2, point, "frequency"))
        {
            if (matchnMove(1, point, "envelope"))
            {
                value = (toggle(point) == 1);
                cmd = SUBSYNTH::control::enableFrequencyEnvelope;
            }
        }
        else if (matchnMove(2, point, "filter"))
        {
            value = (toggle(point) == 1);
            cmd = SUBSYNTH::control::enableFilter;
        }

    }

    if (cmd != -1)
    {
        //std::cout << "control " << int(cmd) << "  part " << int(npart) << "  kit " << int(kitNumber) << "  engine " << int(PART::engine::subSynth) << std::endl;
        if (value == -1)
        {
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            value = string2int(point);
        }
        return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, PART::engine::subSynth);
    }
    return REPLY::available_msg;
}


int CmdInterface::padSynth(unsigned char controlType)
{
    int kit = UNUSED;
    int insert = UNUSED;
    if (kitMode)
    {
        kit = kitNumber;
        insert = TOPLEVEL::insert::kitGroup;
    }
    int enable = (toggle(point));
    if (enable > -1)
    {
        sendNormal( synth, 0, enable, controlType, PART::control::enable, npart, kit, PART::engine::padSynth, insert);
        return REPLY::done_msg;
    }
    if (!lineEnd(point, controlType) && !readControl(synth, 0, PART::control::enable, npart, kit, PART::engine::padSynth, insert))
        return REPLY::inactive_msg;

    if (matchnMove(2, point, "resonance"))
    {
        bitSet(context, LEVEL::Resonance);
        return resonance(controlType);
    }
    if (matchnMove(2, point, "waveform"))
    {
        bitSet(context, LEVEL::Oscillator);
        return waveform(controlType);
    }
    if (lineEnd(point, controlType))
        return REPLY::done_msg;
    int result = partCommonControls(controlType);
    if (result != REPLY::todo_msg)
        return result;

    if (matchnMove(2, point, "xport"))
    {
        if (controlType != TOPLEVEL::type::Write)
            return REPLY::writeOnly_msg;
        if (point[0] == 0)
            return REPLY::value_msg;
        std::string name = point;
        sendDirect(synth, TOPLEVEL::action::lowPrio, 0, controlType, MAIN::control::exportPadSynthSamples, TOPLEVEL::section::main, kitNumber, 2, npart, UNUSED, UNUSED, miscMsgPush(name));
        return REPLY::done_msg;
    }

    int cmd = -1;
    float value = -1;
    if (matchnMove(2, point, "profile"))
    {
        if (matchnMove(1, point, "gauss"))
            value = 0;
        else if (matchnMove(1, point, "square"))
            value = 1;
        else if (matchnMove(1, point, "double"))
            value = 2;
        else
            return REPLY::value_msg;

        cmd = PADSYNTH::control::baseType;
    }
    else if (matchnMove(2, point, "width"))
    {
        cmd = PADSYNTH::control::baseWidth;
    }
    else if (matchnMove(2, point, "count"))
    {
        cmd = PADSYNTH::control::frequencyMultiplier;
    }
    else if (matchnMove(2, point, "expand"))
    {
        cmd = PADSYNTH::control::modulatorStretch;
    }
    else if (matchnMove(2, point, "frequency"))
    {
        cmd = PADSYNTH::control::modulatorFrequency;
    }
    else if (matchnMove(2, point, "size"))
    {
        cmd = PADSYNTH::control::size;
    }
    else if (matchnMove(2, point, "cross"))
    {
        if (matchnMove(1, point, "full"))
            value = 0;
        else if (matchnMove(1, point, "upper"))
            value = 1;
        else if (matchnMove(1, point, "lower"))
            value = 2;
        else
            return REPLY::value_msg;

        cmd = PADSYNTH::control::harmonicSidebands;
    }
    else if (matchnMove(2, point, "multiplier"))
    {
        if (matchnMove(1, point, "off"))
            value = 0;
        else if (matchnMove(1, point, "gauss"))
            value = 1;
        else if (matchnMove(1, point, "sine"))
            value = 2;
        else if (matchnMove(1, point, "double"))
            value = 3;
        else
            return REPLY::value_msg;

        cmd = PADSYNTH::control::amplitudeMultiplier;
    }
    else if (matchnMove(2, point, "mode"))
    {
        if (matchnMove(1, point, "Sum"))
            value = 0;
        else if (matchnMove(1, point, "mult"))
            value = 1;
        else if (matchnMove(1, point, "d1"))
            value = 2;
        else if (matchnMove(1, point, "d2"))
            value = 3;
        else
            return REPLY::value_msg;

        cmd = PADSYNTH::control::amplitudeMode;
    }
    else if (matchnMove(2, point, "center"))
    {
        cmd = PADSYNTH::control::spectralWidth;
    }
    else if (matchnMove(3, point, "relative"))
    {
        cmd = PADSYNTH::control::spectralAmplitude;
    }
    else if (matchnMove(2, point, "auto"))
    {
        value = (toggle(point) > 0);
        cmd = PADSYNTH::control::autoscale;
    }
    else if (matchnMove(3, point, "base"))
    {
        std::string found = point;
        for (int i = 0; i < 9; ++ i)
        {
            if (found == basetypes[i])
            {
                value = i;
                cmd = PADSYNTH::control::harmonicBase;
                break;
            }
        }
        if (cmd == -1)
            return REPLY::range_msg;
    }
    else if (matchnMove(2, point, "samples"))
    {
        unsigned char sizes[] {1, 2, 4, 6, 8, 12, 24};
        value = string2float(point);
        int tmp = value * 2;
        for (int i = 0; i < 7; ++i)
        {
            if (tmp == sizes[i])
            {
                value = i;
                cmd = PADSYNTH::control::samplesPerOctave;
                break;
            }
        }
        if (cmd == -1)
            return REPLY::range_msg;
    }
    else if (matchnMove(2, point, "range"))
    {
        cmd = PADSYNTH::control::numberOfOctaves;
    }
    else if (matchnMove(2, point, "length"))
    {
        value = bitFindHigh(string2int(point)) - 4;
        if (value > 6)
            return REPLY::range_msg;
        cmd = PADSYNTH::control::sampleSize;
    }
    else if (matchnMove(2, point, "bandwidth"))
    {
        cmd = PADSYNTH::control::bandwidth;
    }
    else if (matchnMove(2, point, "scale"))
    {
        if (matchnMove(1, point, "normal"))
            value = 0;
        else if (matchnMove(1, point, "equalhz"))
            value = 1;
        else if (matchnMove(1, point, "quarter"))
            value = 2;
        else if (matchnMove(1, point, "half"))
            value = 3;
        else if (matchnMove(1, point, "threequart"))
            value = 4;
        else if (matchnMove(1, point, "oneandhalf"))
            value = 5;
        else if (matchnMove(1, point, "double"))
            value = 6;
        else if (matchnMove(1, point, "inversehalf"))
            value = 7;
        else
            return REPLY::range_msg;

        cmd = PADSYNTH::control::bandwidthScale;
    }
    else if (matchnMove(2, point, "spectrum"))
    {
        if (matchnMove(1, point, "bandwidth"))
            value = 0;
        else if (matchnMove(1, point, "discrete"))
            value = 1;
        else if (matchnMove(1, point, "continuous"))
            value = 2;
        else
            return REPLY::range_msg;

        cmd = PADSYNTH::control::spectrumMode;
    }

    if (matchnMove(2, point, "apply"))
    {
        value = 0; // dummy
        cmd = PADSYNTH::control::applyChanges;
    }

    if (cmd > -1)
    {
        if (value == -1)
            value = string2int(point);
        return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, PART::engine::padSynth);
    }
    return REPLY::available_msg;
}


int CmdInterface::resonance(unsigned char controlType)
{
    int value = toggle(point);
    int cmd = -1;
    int engine = contextToEngines(context);
    int insert = TOPLEVEL::insert::resonanceGroup;
    if (value > -1)
    {
        sendNormal( synth, 0, value, controlType, RESONANCE::control::enableResonance, npart, kitNumber, engine, insert);
        return REPLY::done_msg;
    }
    if (lineEnd(point, controlType))
        return REPLY::done_msg;

    if (matchnMove(1, point, "random"))
    {
        if (matchnMove(1, point, "coarse"))
            value = 0;
        else if (matchnMove(1, point, "medium"))
            value = 1;
        else if (matchnMove(1, point, "fine"))
            value = 2;
        else
            return REPLY::value_msg;
        cmd = RESONANCE::control::randomType;
    }
    else if (matchnMove(2, point, "protect"))
    {
        value = (toggle(point) == 1);
        cmd = RESONANCE::control::protectFundamental;
    }
    else if (matchnMove(1, point, "maxdb"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        cmd = RESONANCE::control::maxDb;
        value = string2int(point);
    }
    else if (matchnMove(2, point, "center"))
    {
        value = string2int(point);
        cmd = RESONANCE::control::centerFrequency;
    }
    else if (matchnMove(1, point, "octaves"))
    {
        value = string2int(point);
        cmd = RESONANCE::control::octaves;
    }
    else if (matchnMove(1, point, "interpolate"))
    {
        if (matchnMove(1, point, "linear"))
            value = 1;
        else if (matchnMove(1, point, "smooth"))
            value = 0;
        else return REPLY::value_msg;
        cmd = RESONANCE::control::interpolatePeaks;
    }
    else if (matchnMove(1, point, "smooth"))
        cmd = RESONANCE::control::smoothGraph;
    else if (matchnMove(1, point, "clear"))
        cmd = RESONANCE::control::clearGraph;

    else if (matchnMove(2, point, "points"))
    {
        insert = TOPLEVEL::insert::resonanceGraphInsert;
        if (point[0] == 0) // need to catch reading as well
        {
            if (controlType & TOPLEVEL::type::Limits)
                return sendNormal( synth, 0, 0, controlType, 1, npart, kitNumber, engine, insert);
            else
            {
                for (int i = 0; i < MAX_RESONANCE_POINTS; i += 8)
                {
                    std::string line = asAlignedString(i + 1, 4) + ">";
                    for (int j = 0; j < (MAX_RESONANCE_POINTS / 32); ++ j)
                    {
                        line += asAlignedString(readControl(synth, 0, i + j, npart, kitNumber, engine, insert), 4);
                    }
                    synth->getRuntime().Log(line);
                }
            }
            return REPLY::done_msg;
        }

        cmd = string2int(point) - 1;
        if (cmd < 1 || cmd >= MAX_RESONANCE_POINTS)
            return REPLY::range_msg;
        point = skipChars(point);
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        value = string2int(point);
    }
    if (cmd > -1)
        return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, engine, insert);
    return REPLY::available_msg;
}


int CmdInterface::waveform(unsigned char controlType)
{
    if (lineEnd(point, controlType))
        return REPLY::done_msg;
    float value = -1;
    int cmd = -1;
    int engine = contextToEngines(context);
    unsigned char insert = TOPLEVEL::insert::oscillatorGroup;

    std::string name = std::string(point).substr(0,3);
    value = stringNumInList(name, wavebase, 3);
    if (value != -1)
        cmd = OSCILLATOR::control::baseFunctionType;
    else if (matchnMove(1, point, "harmonic"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;

        if (matchnMove(1, point, "shift"))
            cmd = OSCILLATOR::control::harmonicShift;
        else if (matchnMove(1, point, "before"))
        {
            value = (toggle(point) == 1);
            cmd = OSCILLATOR::control::shiftBeforeWaveshapeAndFilter;
        }
        else
        {
            cmd = string2int(point) - 1;
            if (cmd < 0 || cmd >= MAX_AD_HARMONICS)
                return REPLY::range_msg;
            point = skipChars(point);

            if (matchnMove(1, point, "amp"))
                insert = TOPLEVEL::insert::harmonicAmplitude;
            else if (matchnMove(1, point, "phase"))
                insert = TOPLEVEL::insert::harmonicPhaseBandwidth;

            if (lineEnd(point, controlType))
                return REPLY::value_msg;
        }
        if (value == -1)
            value = string2int(point);
        return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, engine + voiceNumber, insert);
    }

    else if (matchnMove(2, point, "convert"))
    {
        value = 0; // dummy
        cmd = OSCILLATOR::control::convertToSine;
    }

    else if (matchnMove(2, point, "clear"))
    {
        value = 0; // dummy
        cmd = OSCILLATOR::control::clearHarmonics;
    }

    else if (matchnMove(2, point, "shape"))
    {
        if (matchnMove(1, point, "type"))
        {
            std::string name = std::string(point).substr(0,3);
            value = stringNumInList(name, filtershapes, 3);
            if (value == -1)
                return REPLY::value_msg;
            cmd = OSCILLATOR::control::waveshapeType;
        }
        else if (matchnMove(1, point, "par"))
            cmd = OSCILLATOR::control::waveshapeParameter;
        else return REPLY::op_msg;
    }

    else if (matchnMove(1, point, "filter"))
    {
        if (matchnMove(1, point, "type"))
        {
            std::string name = std::string(point).substr(0,3);
            value = stringNumInList(name, filtertype, 3);
            if (value == -1)
                return REPLY::value_msg;
            cmd = OSCILLATOR::control::filterType;
        }
        else if (matchnMove(1, point, "par"))
        {
            switch (point[0])
            {
                case char('1'):
                    cmd = OSCILLATOR::control::filterParameter1;
                    break;
                case char('2'):
                    cmd = OSCILLATOR::control::filterParameter2;
                    break;
                default:
                    return REPLY::op_msg;
            }
            point = skipChars(point);
        }
        else if (matchnMove(1, point, "before"))
        {
            value = (toggle(point) == 1);
            cmd = OSCILLATOR::control::filterBeforeWaveshape;
        }
        else return REPLY::op_msg;
    }

    else if (matchnMove(1, point, "base"))
    {
        if(matchnMove(1, point, "par"))
            cmd = OSCILLATOR::control::baseFunctionParameter;
        else if (matchnMove(1, point, "convert"))
        {
            value = (toggle(point) != 0);
            cmd = OSCILLATOR::control::useAsBaseFunction;
        }
        else if (matchnMove(1, point, "mod"))
        {
            if(matchnMove(1, point, "type"))
            {
                if(matchnMove(3, point, "off"))
                    value = 0;
                else if(matchnMove(1, point, "Rev"))
                    value = 1;
                else if(matchnMove(1, point, "Sine"))
                    value = 2;
                else if(matchnMove(1, point, "Power"))
                    value = 3;
                else
                    return REPLY::value_msg;
                cmd = OSCILLATOR::control::baseModulationType;
            }
            else if(matchnMove(1, point, "par"))
            {
                switch (point[0])
                {
                    case char('1'):
                        cmd = OSCILLATOR::control::baseModulationParameter1;
                        break;
                    case char('2'):
                        cmd = OSCILLATOR::control::baseModulationParameter2;
                        break;
                    case char('3'):
                        cmd = OSCILLATOR::control::baseModulationParameter3;
                        break;
                    default:
                        return REPLY::range_msg;
                }
                point = skipChars(point);
            }
            else
                return REPLY::op_msg;
        }
        else
            return REPLY::op_msg;
    }

    else if (matchnMove(2, point, "spectrum"))
    {
        if (matchnMove(1, point, "type"))
        {
            if (matchnMove(3, point, "OFF"))
                value = 0;
            else if (matchnMove(3, point, "Power"))
                value = 1;
            else if (matchnMove(1, point, "Down"))
                value = 2;
            else if (matchnMove(1, point, "Up"))
                value = 3;
            else
                return REPLY::value_msg;
            cmd = OSCILLATOR::control::spectrumAdjustType;
        }
        else if (matchnMove(1, point, "par"))
            cmd = OSCILLATOR::control::spectrumAdjustParameter;
        else return REPLY::op_msg;
    }

    else if (matchnMove(2, point, "adaptive"))
    {
        if (matchnMove(1, point, "type"))
        {
            std::string name = std::string(point).substr(0,3);
            value = stringNumInList(name, adaptive, 3);
            if (value == -1)
                return REPLY::value_msg;
            cmd = OSCILLATOR::control::adaptiveHarmonicsType;
        }
        else if (matchnMove(1, point, "base"))
            cmd = OSCILLATOR::control::adaptiveHarmonicsBase;
        else if (matchnMove(1, point, "level"))
            cmd = OSCILLATOR::control::adaptiveHarmonicsPower;
        else if (matchnMove(1, point, "par"))
            cmd = OSCILLATOR::control::adaptiveHarmonicsParameter;
        else
            return REPLY::op_msg;
    }

    else if (matchnMove(2, point, "apply"))
    {
        if (engine != PART::engine::padSynth)
            return REPLY::available_msg;
        value = 0; // dummy
        insert = UNUSED;
        cmd = PADSYNTH::control::applyChanges;
    }
    if (cmd == -1)
        return REPLY::available_msg;
    if (value == -1)
        value = string2float(point);
    return sendNormal( synth, 0, value, controlType, cmd, npart, kitNumber, engine + voiceNumber, insert);
}


int CmdInterface::commandPart(unsigned char controlType)
{
    Config &Runtime = synth->getRuntime();
    int tmp = -1;
    if (bitTest(context, LEVEL::AllFX))
        return effects(controlType);
    if (lineEnd(point, controlType))
        return REPLY::done_msg;
    if (kitMode == PART::kitType::Off)
        kitNumber = UNUSED; // always clear it if not kit mode
    if (matchnMove(2, point, "effects") || matchnMove(2, point, "efx"))
    {
        context = LEVEL::Top;
        bitSet(context, LEVEL::AllFX);
        bitSet(context, LEVEL::Part);
        return effects(controlType);
    }

    if (isdigit(point[0]))
    {
        tmp = string2int127(point);
        point = skipChars(point);
        if (tmp > 0)
        {
            tmp -= 1;
            if (!inKitEditor)
            {
                if (tmp >= Runtime.NumAvailableParts)
                {
                    Runtime.Log("Part number too high");
                    return REPLY::done_msg;
                }

                if (npart != tmp)
                {
                    npart = tmp;
                    if (controlType == TOPLEVEL::type::Write)
                    {
                        context = LEVEL::Top;
                        bitSet(context, LEVEL::Part);
                        kitMode = PART::kitType::Off;
                        kitNumber = 0;
                        voiceNumber = 0; // must clear this too!
                        sendNormal( synth, 0, npart, TOPLEVEL::type::Write, MAIN::control::partNumber, TOPLEVEL::section::main);
                    }
                }
                if (lineEnd(point, controlType))
                    return REPLY::done_msg;
            }
            else
            {
                if (controlType == TOPLEVEL::type::Write)
                {
                    if (tmp >= NUM_KIT_ITEMS)
                        return REPLY::range_msg;
                    kitNumber = tmp;
                    voiceNumber = 0;// to avoid confusion
                }
                Runtime.Log("Kit item number " + std::to_string(kitNumber + 1));
                return REPLY::done_msg;
            }
        }
    }

    if (!inKitEditor)
    {
        int enable = toggle(point);
        if (enable != -1)
        {
            int result = sendNormal( synth, 0, enable, controlType, PART::control::enable, npart);
            if (lineEnd(point, controlType))
                return result;
        }
    }

    if (matchnMove(2, point, "program") || matchnMove(1, point, "instrument"))
    {
        if (controlType != TOPLEVEL::type::Write)
        {
            Runtime.Log("Part name is " + synth->part[npart]->Pname);
            return REPLY::done_msg;
        }
        if (matchnMove(2, point, "clear"))
        {
            sendDirect(synth, 0, 0, controlType, PART::control::defaultInstrument, npart, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, tmp);
            return REPLY::done_msg;
        }
        if (point[0] != 0) // force part not channel number
        {
            if(matchnMove(1, point, "group"))
            {
                if(instrumentGroup.empty())
                {
                    Runtime.Log("No list entries, or list not seen");
                    return REPLY::done_msg;
                }
                size_t value = string2int(point);
                string line;
                if(value < 1 || value > instrumentGroup.size())
                    return REPLY::range_msg;
                -- value;

                std::list<string>:: iterator it = instrumentGroup.begin();
                size_t lineNo = 0;
                while (lineNo < value && it != instrumentGroup.end())
                {
                    ++ it;
                    ++ lineNo;
                }
                if (it == instrumentGroup.end())
                    return REPLY::range_msg;
                line = *it;

                int root = string2int(line.substr(0, 3));
                int bank = string2int(line.substr(5, 3));
                int inst = (string2int(line.substr(10, 3))) - 1;

                sendDirect(synth, 0, inst, controlType, MAIN::control::loadInstrumentFromBank, TOPLEVEL::section::main, npart, bank, root);
                return REPLY::done_msg;
            }
            tmp = string2int(point) - 1;
            if (tmp < 0 || tmp >= MAX_INSTRUMENTS_IN_BANK)
                return REPLY::range_msg;
            sendDirect(synth, 0, tmp, controlType, MAIN::control::loadInstrumentFromBank, TOPLEVEL::section::main, npart);
            return REPLY::done_msg;
        }
        else
            return REPLY::value_msg;
    }


    if (!readControl(synth, 0, PART::control::enable, npart))
        return REPLY::inactive_msg;

    tmp = -1;
    if (matchnMove(2, point, "disable"))
        tmp = PART::kitType::Off;
    else if(matchnMove(2, point, "multi"))
        tmp = PART::kitType::Multi;
    else if(matchnMove(2, point, "single"))
        tmp = PART::kitType::Single;
    else if(matchnMove(2, point, "crossfade"))
        tmp = PART::kitType::CrossFade;
    else if(matchnMove(3, point, "kit"))
    {
        if (kitMode == PART::kitType::Off)
            return REPLY::inactive_msg;
        inKitEditor = true;
        return REPLY::done_msg;
    }

    if (tmp != -1)
    {
        kitNumber = 0;
        voiceNumber = 0; // must clear this too!
        kitMode = tmp;
        inKitEditor = (kitMode != PART::kitType::Off);
        return sendNormal( synth, 0, kitMode, controlType, PART::control::kitMode, npart);
    }
    if (inKitEditor)
    {
        int value = toggle(point);
        if (value >= 0)
        {
            if (kitNumber == 0 && bitFindHigh(context) == LEVEL::Part)
            {
                synth->getRuntime().Log("Kit item 1 always on.");
                return REPLY::done_msg;
            }
            sendNormal( synth, 0, value, controlType, PART::control::enable, npart, kitNumber, UNUSED, TOPLEVEL::insert::kitGroup);
        }
        if (!readControl(synth, 0, PART::control::enable, npart, kitNumber, UNUSED, TOPLEVEL::insert::kitGroup))
            return REPLY::inactive_msg;
    }

    if (bitTest(context, LEVEL::AllFX))
        return effects(controlType);

    if (matchnMove(3, point, "addsynth"))
    {
        bitSet(context, LEVEL::AddSynth);
        insertType = TOPLEVEL::insertType::amplitude;
        return addSynth(controlType);
    }

    if (matchnMove(3, point, "subsynth"))
    {
        bitSet(context, LEVEL::SubSynth);
        insertType = TOPLEVEL::insertType::amplitude;
        return subSynth(controlType);
    }

    if (matchnMove(3, point, "padsynth"))
    {
        bitSet(context, LEVEL::PadSynth);
        insertType = TOPLEVEL::insertType::amplitude;
        return padSynth(controlType);
    }

    if (matchnMove(3, point, "mcontrol"))
    {
        bitSet(context, LEVEL::MControl);
        return midiControllers(controlType);
    }

    if (inKitEditor)
    {
        int value;
        if (matchnMove(2, point, "drum"))
            return sendNormal( synth, 0, (toggle(point) != 0), controlType, PART::control::drumMode, npart);
        if (matchnMove(2, point, "quiet"))
            return sendNormal( synth, 0, (toggle(point) != 0), controlType, PART::control::kitItemMute, npart, kitNumber, UNUSED, TOPLEVEL::insert::kitGroup);
        if (matchnMove(2, point,"effect"))
        {
            if (controlType == TOPLEVEL::type::Write && point[0] == 0)
                return REPLY::value_msg;
            value = string2int(point);
            if (value < 0 || value > NUM_PART_EFX)
                return REPLY::range_msg;
            return sendNormal( synth, 0, value, controlType, PART::control::kitEffectNum, npart, kitNumber, UNUSED, TOPLEVEL::insert::kitGroup);
        }
        if (matchnMove(2, point,"name"))
        {
            int miscmsg = NO_MSG;
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            if (controlType == TOPLEVEL::type::Write)
                miscmsg = miscMsgPush(point);
            return sendNormal( synth, TOPLEVEL::action::muteAndLoop, 0, controlType, PART::control::instrumentName, npart, kitNumber, UNUSED, TOPLEVEL::insert::kitGroup, UNUSED, UNUSED, miscmsg);
        }
    }

    tmp = partCommonControls(controlType);
    if (tmp != REPLY::todo_msg)
        return tmp;

    if (matchnMove(2, point, "shift"))
    {
        if (controlType == TOPLEVEL::type::Write && point[0] == 0)
            return REPLY::value_msg;
        int value = string2int(point);
        if (value < MIN_KEY_SHIFT)
            value = MIN_KEY_SHIFT;
        else if(value > MAX_KEY_SHIFT)
            value = MAX_KEY_SHIFT;
        return sendNormal( synth, TOPLEVEL::action::lowPrio, value, controlType, PART::control::keyShift, npart);
    }

    if (matchnMove(2, point, "LEvel"))
    {
        tmp = string2int127(point);
        if(controlType == TOPLEVEL::type::Write && tmp < 1)
            return REPLY::value_msg;
        return sendNormal( synth, 0, tmp, controlType, PART::control::velocityOffset, npart);
    }

    if (matchnMove(1, point, "channel"))
    {
        tmp = string2int127(point);
        if(controlType == TOPLEVEL::type::Write && tmp < 1)
            return REPLY::value_msg;
        tmp -= 1;
        return sendNormal( synth, 0, tmp, controlType, PART::control::midiChannel, npart);
    }
    if (matchnMove(1, point, "destination"))
    {
        int dest = 0;
        if (controlType == TOPLEVEL::type::Write)
        {
            if (matchnMove(1, point, "main"))
                dest = 1;
            else if (matchnMove(1, point, "part"))
                dest = 2;
            else if (matchnMove(1, point, "both"))
                dest = 3;
            if (dest == 0)
                return REPLY::range_msg;
        }
        return sendNormal( synth, TOPLEVEL::action::muteAndLoop, dest, controlType, PART::control::audioDestination, npart);
    }
    if (matchnMove(1, point, "note"))
    {
        int value = 0;
        if(controlType == TOPLEVEL::type::Write)
        {
            if (lineEnd(point, controlType))
                return REPLY::value_msg;
            value = string2int(point);
            if (value < 1 || (value > POLIPHONY - 20))
                return REPLY::range_msg;
        }
        return sendNormal( synth, 0, value, controlType, PART::control::maxNotes, npart);
    }

    if (matchnMove(1, point, "mode"))
    {
        int value = 0;
        if(controlType == TOPLEVEL::type::Write)
        {
            if (matchnMove(1, point, "poly"))
                value = 0;
            else if (matchnMove(1, point, "mono"))
                value = 1;
            else if (matchnMove(1, point, "legato"))
                value = 2;
            else
                return REPLY::name_msg;
        }
        return sendNormal( synth, 0, value, controlType, 6, npart);
    }
    if (matchnMove(2, point, "portamento"))
        return sendNormal( synth, 0, (toggle(point) == 1), controlType, PART::control::portamento, npart);
    if (matchnMove(2, point, "name"))
    {
        std::string name;
        unsigned char miscmsg = NO_MSG;
        if (controlType == TOPLEVEL::type::Write)
        {
            name = (std::string) point;
            if (name.size() < 3)
            {
                Runtime.Log("Name too short");
                return REPLY::done_msg;
            }
            else if ( name == "Simple Sound")
            {
                Runtime.Log("Cant use name of default sound");
                return REPLY::done_msg;
            }
            else
                miscmsg = miscMsgPush(name);
        }
        return sendNormal( synth, TOPLEVEL::action::lowPrio, 0, controlType, PART::control::instrumentName, npart, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscmsg);
    }
    return REPLY::op_msg;
}


int CmdInterface::commandReadnSet(unsigned char controlType)
{
    Config &Runtime = synth->getRuntime();
    std::string name;


    /*CommandBlock getData;
    getData.data.value.F = 0;
    getData.data.part = TOPLEVEL::section::copyPaste;
    getData.data.kit = 0;
    getData.data.engine = 135;
    getData.data.insert = UNUSED;
    std::cout << synth->unifiedpresets.findSectionName(&getData) << std::endl;*/


    if (matchnMove(2, point, "yoshimi"))
    {
        if (controlType != TOPLEVEL::type::Write)
        {
            //Runtime.Log("Instance " + asString(currentInstance), 1);
            Runtime.Log("Instance " + std::to_string(synth->getUniqueId()));
            return REPLY::done_msg;
        }
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        currentInstance = string2int(point);
        synth = firstSynth->getSynthFromId(currentInstance);
        unsigned int newID = synth->getUniqueId();
        if (newID != currentInstance)
        {
            Runtime.Log("Instance " + std::to_string(currentInstance) + " not found. Set to " + std::to_string(newID), 1);
            currentInstance = newID;
        }
        defaults();
        return REPLY::done_msg;
    }

 // these must all be highest (relevant) bit first
    if (bitTest(context, LEVEL::Config))
        return commandConfig(controlType);
    if (bitTest(context, LEVEL::Scale))
        return commandScale(controlType);
    if (bitTest(context, LEVEL::Envelope))
        return envelopeSelect(controlType);
    if (bitTest(context, LEVEL::Filter))
        return filterSelect(controlType);
    if (bitTest(context, LEVEL::LFO))
        return LFOselect(controlType);
    if (bitTest(context, LEVEL::Resonance))
        return resonance(controlType);
    if (bitTest(context, LEVEL::Oscillator))
        return waveform(controlType);
    if (bitTest(context, LEVEL::AddMod))
        return modulator(controlType);
    if (bitTest(context, LEVEL::AddVoice))
        return addVoice(controlType);
    if (bitTest(context, LEVEL::AddSynth))
        return addSynth(controlType);
    if (bitTest(context, LEVEL::SubSynth))
        return subSynth(controlType);
    if (bitTest(context, LEVEL::PadSynth))
        return padSynth(controlType);
    if (bitTest(context, LEVEL::MControl))
        return midiControllers(controlType);
    if (bitTest(context, LEVEL::Part))
        return commandPart(controlType);
    if (bitTest(context, LEVEL::Vector))
        return commandVector(controlType);
    if (bitTest(context, LEVEL::Learn))
        return commandMlearn(controlType);

    if (matchnMove(3, point, "mono"))
    {
        return sendNormal( synth, 0, (toggle(point) == 1), controlType, MAIN::control::mono, TOPLEVEL::section::main);
    }

    if (matchnMove(2, point, "config"))
    {
        context = LEVEL::Top;
        bitSet(context, LEVEL::Config);
        return commandConfig(controlType);
    }

    if (matchnMove(1, point, "scale"))
    {
        context = LEVEL::Top;
        bitSet(context, LEVEL::Scale);
        return commandScale(controlType);
    }

    if (matchnMove(1, point, "part"))
    {
        nFX = 0; // effects number limit changed
        if (controlType != TOPLEVEL::type::Write && point[0] == 0)
        {
            if (synth->partonoffRead(npart))
                name = " enabled";
            else
                name = " disabled";
            Runtime.Log("Current part " + asString(npart) + name, 1);
            return REPLY::done_msg;
        }
        context = LEVEL::Top;
        bitSet(context, LEVEL::Part);
        nFXtype = synth->part[npart]->partefx[nFX]->geteffect();
        return commandPart(controlType);
    }

    if (matchnMove(2, point, "vector"))
    {
        context = LEVEL::Top;
        return commandVector(controlType);
    }

    if (matchnMove(2, point, "mlearn"))
    {
        context = LEVEL::Top;
        return commandMlearn(controlType);
    }

    if ((context == LEVEL::Top || bitTest(context, LEVEL::InsFX)) && matchnMove(3, point, "system"))
    {
        bitSet(context,LEVEL::AllFX);
        bitClear(context, LEVEL::InsFX);
        nFX = 0; // effects number limit changed
        matchnMove(2, point, "effects"); // clear it if given
        matchnMove(2, point, "efx");
        nFXtype = synth->sysefx[nFX]->geteffect();
        return effects(controlType);
    }
    if ((context == LEVEL::Top || bitTest(context, LEVEL::AllFX)) && !bitTest(context, LEVEL::Part) && matchnMove(3, point, "insert"))
    {
        bitSet(context,LEVEL::AllFX);
        bitSet(context,LEVEL::InsFX);
        nFX = 0; // effects number limit changed
        matchnMove(2, point, "effects"); // clear it if given
        matchnMove(2, point, "efx");
        nFXtype = synth->insefx[nFX]->geteffect();
        return effects(controlType);
    }
    if (bitTest(context, LEVEL::AllFX))
        return effects(controlType);

    if (matchnMove(1, point, "root"))
    {
        if (controlType != TOPLEVEL::type::Write)
        {
            Runtime.Log("Root is ID " + asString(synth->ReadBankRoot()), 1);
            return REPLY::done_msg;
        }
        if (point[0] != 0)
        {
            sendDirect(synth, TOPLEVEL::action::muteAndLoop, 255, controlType, 8, TOPLEVEL::section::midiIn, 0, UNUSED, string2int(point));
            return REPLY::done_msg;
        }
        else
            return REPLY::value_msg;
    }

    if (matchnMove(1, point, "bank"))
    {
        if (controlType != TOPLEVEL::type::Write)
        {
            Runtime.Log("Bank is ID " + asString(synth->ReadBank()), 1);
            return REPLY::done_msg;
        }
        if (point[0] != 0)
        {
            sendDirect(synth, TOPLEVEL::action::muteAndLoop, 255, TOPLEVEL::type::Write, 8, TOPLEVEL::section::midiIn, 0, string2int(point));
            return REPLY::done_msg;
        }
        else
            return REPLY::value_msg;
    }

    if (matchnMove(1, point, "volume"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        return sendNormal( synth, 0, string2int127(point), controlType, MAIN::control::volume, TOPLEVEL::section::main);
    }
    if (matchnMove(2, point, "detune"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        return sendNormal( synth, TOPLEVEL::action::lowPrio, string2int127(point), controlType, MAIN::control::detune, TOPLEVEL::section::main);
    }

    if (matchnMove(2, point, "shift"))
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        int value = string2int(point);
        return sendNormal( synth, TOPLEVEL::action::lowPrio, value, controlType, MAIN::control::keyShift, TOPLEVEL::section::main);
    }

    if (matchnMove(2, point, "solo"))
    {
        int value = 0; // disable

        if (matchnMove(2, point, "cc"))
        {
            if (controlType == TOPLEVEL::type::Write)
            {
                if (lineEnd(point, controlType))
                    return REPLY::value_msg;
                value = string2int127(point);
                std::string otherCC = Runtime.masterCCtest(value);
                if (otherCC > "")
                {
                    Runtime.Log("In use for " + otherCC);
                    return REPLY::done_msg;
                }
            }
            return sendNormal( synth, 0, value, controlType, MAIN::control::soloCC, TOPLEVEL::section::main);
        }

        else if (matchnMove(1, point, "row"))
            value = 1;
        else if (matchnMove(1, point, "column"))
            value = 2;
        else if (matchnMove(1, point, "loop"))
            value = 3;
        else if (matchnMove(1, point, "twoway"))
            value = 4;
        return sendNormal( synth, 0, value, controlType, MAIN::control::soloType, TOPLEVEL::section::main);
    }
    if (matchnMove(2, point, "available")) // 16, 32, 64
    {
        if (lineEnd(point, controlType))
            return REPLY::value_msg;
        int value = string2int(point);
        if (controlType == TOPLEVEL::type::Write && value != 16 && value != 32 && value != 64)
            return REPLY::range_msg;
        return sendNormal( synth, 0, value, controlType, MAIN::control::availableParts, TOPLEVEL::section::main);
    }
    return REPLY::op_msg;
}


int CmdInterface::cmdIfaceProcessCommand(char *cCmd)
{
    // in case it's been changed from elsewhere
    synth = firstSynth->getSynthFromId(currentInstance);
    unsigned int newID = synth->getUniqueId();
    if (newID != currentInstance)
    {
        currentInstance = newID;
        defaults();
    }

    Config &Runtime = synth->getRuntime();

    int tmp;
    point = cCmd;
    point = skipSpace(point); // just to be sure
    tmp = strlen(cCmd) - 1;
    while (point[tmp] < '!' && tmp > 0)
    {
        point[tmp] = 0; // also trailing spaces
        -- tmp;
    }

    list<std::string> msg;

    findStatus(synth, context, false);

#ifdef REPORT_NOTES_ON_OFF
    if (matchnMove(3, point, "report")) // note test
    {
        std::cout << "note on sent " << Runtime.noteOnSent << std::endl;
        std::cout << "note on seen " << Runtime.noteOnSeen << std::endl;
        std::cout << "note off sent " << Runtime.noteOffSent << std::endl;
        std::cout << "note off seen " << Runtime.noteOffSeen << std::endl;
        std::cout << "notes hanging sent " << Runtime.noteOnSent - Runtime.noteOffSent << std::endl;
        std::cout << "notes hanging seen " << Runtime.noteOnSeen - Runtime.noteOffSeen << std::endl;
        return REPLY::done_msg;
    }
#endif
    if (matchnMove(2, point, "exit"))
    {
        if (currentInstance > 0)
        {
            Runtime.Log("Can only exit from instance 0", 1);
            return REPLY::done_msg;
        }
        std::string message;
        if (Runtime.configChanged)
            message = "System config has been changed. Still exit";
        else
            message = "All data will be lost. Still exit";
        if (query(message, false))
        {
            // this seems backwards but it *always* saves.
            // seeing configChanged makes it reload the old config first.
            Runtime.runSynth = false;
            return REPLY::exit_msg;
        }
        return REPLY::done_msg;
    }
    if (point[0] == '/')
    {
        ++ point;
        point = skipSpace(point);
        defaults();
        if (point[0] == 0)
            return REPLY::done_msg;
    }

    if (matchnMove(3, point, "reset"))
    {
        int control = MAIN::control::masterReset;
        if (matchnMove(3, point, "all"))
            control = MAIN::control::masterResetAndMlearn;
        if (query("Restore to basic settings", false))
        {
            sendDirect(synth, TOPLEVEL::action::muteAndLoop, 0, TOPLEVEL::type::Write, control, TOPLEVEL::section::main);
            defaults();
        }
        return REPLY::done_msg;
    }
    if (point[0] == '.' && point[1] == '.')
    {
        point += 2;
        point = skipSpace(point);
        /*
         * kit mode is a pseudo context level so the code
         * below emulates normal 'back' actions
         */
        if (bitFindHigh(context) == LEVEL::Part && kitMode != PART::kitType::Off)
        {
            int newPart = npart;
            char *oldPoint = point;
            defaults();
            npart = newPart;
            bitSet(context, LEVEL::Part);
            if (matchnMove(1, point, "set"))
            {
                if (!isdigit(point[0]))
                    point = oldPoint;
                else
                {
                    tmp = string2int(point);
                    if (tmp < 1 || tmp > Runtime.NumAvailableParts)
                        return REPLY::range_msg;

                    npart = tmp -1;
                    return REPLY::done_msg;
                }
            }
            else
                return REPLY::done_msg;
        }

        if (bitFindHigh(context) == LEVEL::AllFX || bitFindHigh(context) == LEVEL::InsFX)
            defaults();
        else if (bitFindHigh(context) == LEVEL::Part)
        {
            int temPart = npart;
            if (bitTest(context, LEVEL::AllFX) || bitTest(context, LEVEL::InsFX))
            {
                defaults();
                bitSet(context, LEVEL::Part); // restore part level
            }
            else
                defaults();
            npart = temPart;
        }
        else
        {
            bitClearHigh(context);
        }
        if (point[0] == 0)
            return REPLY::done_msg;
    }

    if (helpList(context))
        return REPLY::done_msg;

    if (matchnMove(2, point, "stop"))
        return sendNormal( synth, 0, 0, TOPLEVEL::type::Write,MAIN::control::stopSound, TOPLEVEL::section::main);
    if (matchnMove(1, point, "list"))
    {
        if (matchnMove(1, point, "group"))
            return commandGroup();
        return commandList();
    }

    if (matchnMove(3, point, "run"))
    {
        std::string filename = std::string(point);
        if (filename > "!")
        {
            char to_send[COMMAND_SIZE];
            char *mark;
            int count = 0;
            bool isok = true;

            std::string text = loadText(filename);
            if (text != "")
            {
                size_t linePoint = 0;
                context = LEVEL::Top; // start from top level
                while (linePoint < text.length() && isok)
                {
                    if (!C_lineInText(text, linePoint, to_send))
                        Runtime.Log("*** Error: line " + to_string(count) + " too long");
                    ++ count;
                    mark = skipSpace(to_send);
                    if ( mark[0] < ' ' || mark [0] == '#')
                        continue;
                    if (matchnMove(3, mark, "run"))
                    {
                        isok = false;
                        Runtime.Log("*** Error: scripts are not recursive @ line " + std::to_string(count) + " ***");
                        continue;
                    }
                    if (matchnMove(4, mark, "wait"))
                    {
                        int tmp = string2int(mark);
                        if (tmp < 1)
                            tmp = 1;
                        else if (tmp > 1000)
                            tmp = 1000;
                        Runtime.Log("Waiting " + std::to_string(tmp) + "mS");
                        usleep((tmp - 1) * 1000);
                        // total processing may add up to another 1 mS
                    }
                    else
                    {
                        usleep(2000); // the loop is too fast otherwise!
                        reply = cmdIfaceProcessCommand(mark);
                    }
                    if (reply > REPLY::done_msg)
                    {
                        isok = false;
                        Runtime.Log("*** Error: " + replies[reply] + " @ line " + std::to_string(count) + " ***");
                    }
                }
            }
            else
                Runtime.Log("Can't read file " + filename);
            return REPLY::done_msg;
        }
        replyString = "Exec";
        return REPLY::what_msg;
    }

    if (matchnMove(1, point, "set"))
    {
        if (point[0] != 0)
            return commandReadnSet(TOPLEVEL::type::Write);
        else
        {
            replyString = "set";
            return REPLY::what_msg;
        }
    }

    if (matchnMove(1, point, "read") || matchnMove(1, point, "get"))
    {
        if (point[0] != 0)
            return commandReadnSet(TOPLEVEL::type::Read);
        else
        {
            replyString = "read";
            return REPLY::what_msg;
        }
    }

    if (matchnMove(3, point, "minimum"))
    {
        if (point[0] != 0)
            return commandReadnSet(TOPLEVEL::type::Minimum | TOPLEVEL::type::Limits);
        else
        {
            replyString = "minimum";
            return REPLY::what_msg;
        }
    }

    if (matchnMove(3, point, "maximum"))
    {
        if (point[0] != 0)
            return commandReadnSet(TOPLEVEL::type::Maximum | TOPLEVEL::type::Limits);
        else
        {
            replyString = "maximum";
            return REPLY::what_msg;
        }
    }

    if (matchnMove(3, point, "default"))
    {
        if (point[0] != 0)
            return commandReadnSet(TOPLEVEL::type::Default | TOPLEVEL::type::Limits);
        else
        {
            replyString = "default";
            return REPLY::what_msg;
        }
    }

    if (matchnMove(2, point, "mlearn"))
    {
        if (point[0] != 0)
            return commandReadnSet(TOPLEVEL::type::LearnRequest);
        else
        {
            replyString = "mlearn";
            return REPLY::what_msg;
        }
    }

    if (matchnMove(3, point, "add"))
    {
        if (matchnMove(1, point, "root"))
        {
            int found = synth->getBankRef().addRootDir(point);
            if (!found)
            {
                Runtime.Log("Can't find path " + (std::string) point);
            }
#ifdef GUI_FLTK
            else
            {
                GuiThreadMsg::sendMessage(synth, GuiThreadMsg::UpdatePaths, 0);
                Runtime.Log("Added new root ID " + asString(found) + " as " + (std::string) point);
                synth->saveBanks();
            }
#endif
            return REPLY::done_msg;
        }
        if (matchnMove(1, point, "bank"))
        {
            int slot;
            for (slot = 0; slot < MAX_BANKS_IN_ROOT; ++slot)
            {
                if (synth->getBankRef().getBankName(slot).empty())
                    break;
            }
            if (!synth->getBankRef().newIDbank(point, (unsigned int)slot))
            {
                Runtime.Log("Could not create bank " + (std::string) point + " for ID " + asString(slot));
            }

            Runtime.Log("Created  new bank " + (std::string) point + " with ID " + asString(slot));
#ifdef GUI_FLTK
            GuiThreadMsg::sendMessage(synth, GuiThreadMsg::UpdatePaths, 0);
#endif
            return REPLY::done_msg;
        }
        if (matchnMove(2, point, "yoshimi"))
        {
            if (currentInstance !=0)
            {
                Runtime.Log("Only instance 0 can start others");
                return REPLY::done_msg;
            }
            int forceId = string2int(point);
            if (forceId < 1 || forceId >= 32)
                forceId = 0;
            sendDirect(synth, TOPLEVEL::action::lowPrio, forceId, TOPLEVEL::type::Write, MAIN::control::startInstance, TOPLEVEL::section::main);
            return REPLY::done_msg;
        }
        else
        {
            replyString = "add";
            return REPLY::what_msg;
        }
    }
    if (matchWord(3, point, "import") || matchWord(3, point, "export") )
    { // need the double test to find which then move along line
        int type = 0;
        if (matchnMove(3, point, "import"))
        {
            type = MAIN::control::importBank;
            replyString = "import";
        }
        else if (matchnMove(3, point, "export"))
        {
            type = MAIN::control::exportBank;
            replyString = "export";
        }

        int root = UNUSED;
        if (matchnMove(1, point, "root"))
        {
            if (isdigit(point[0]))
            {
                root = string2int(point);
                point = skipChars(point);
            }
            else
                root = 200; // force invalid root error
        }
        int value = string2int(point);
        point = skipChars(point);
        std::string name = std::string(point);
        if (root < 0 || (root > 127 && root != UNUSED) || value < 0 || value > 127 || name <="!")
            return REPLY::what_msg;
        else
        {
            sendDirect(synth, TOPLEVEL::action::lowPrio, value, TOPLEVEL::type::Write, type, TOPLEVEL::section::main, root, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(name));
            return REPLY::done_msg;
        }
    }

    if (matchnMove(3, point, "remove"))
    {
        if  (matchnMove(1, point, "root"))
        {
            if (isdigit(point[0]))
            {
                int rootID = string2int(point);
                if (rootID >= MAX_BANK_ROOT_DIRS)
                    return REPLY::range_msg;
                else
                {
                    std::string rootname = synth->getBankRef().getRootPath(rootID);
                    if (rootname.empty())
                        Runtime.Log("Can't find path " + asString(rootID));
                    else
                    {
                        synth->getBankRef().removeRoot(rootID);
#ifdef GUI_FLTK
                        GuiThreadMsg::sendMessage(synth, GuiThreadMsg::UpdatePaths, 0);
#endif
                        Runtime.Log("Un-linked " + rootname);
                        synth->saveBanks();
                    }
                    return REPLY::done_msg;
                }
            }
            else
                return REPLY::value_msg;
        }
        if (matchnMove(1, point, "bank"))
        {
            int rootID = UNUSED;
            if (matchnMove(1, point, "root"))
            {
                if (isdigit(point[0]))
                    rootID = string2int(point);
                if (rootID >= MAX_BANK_ROOT_DIRS)
                    return REPLY::range_msg;
            }
            if (isdigit(point[0]))
            {
                point = skipChars(point);
                int bankID = string2int(point);
                if (bankID >= MAX_BANKS_IN_ROOT)
                    return REPLY::range_msg;
                else
                {
                    std::string filename = synth->getBankRef().getBankName(bankID);
                    if (filename.empty())
                        Runtime.Log("No bank at this location");
                    else
                    {
                        tmp = synth->getBankRef().getBankSize(bankID);
                        if (tmp)
                        {
                            Runtime.Log("Bank " + filename + " has " + asString(tmp) + " Instruments");
                            if (query("Delete bank and all of these", false))
                                tmp = 0;
                            else
                                Runtime.Log("Aborted");
                        }
                        if (tmp == 0)
                        {
                            sendDirect(synth, TOPLEVEL::action::lowPrio, bankID, TOPLEVEL::type::Write, MAIN::control::deleteBank, TOPLEVEL::section::main, rootID);
                        }
                    }

                }
                return REPLY::done_msg;
            }
            else
                return REPLY::value_msg;
        }
        if(matchnMove(2, point, "yoshimi"))
        {
            if (point[0] == 0)
            {
                replyString = "remove";
                return REPLY::what_msg;
            }
            else
            {
                unsigned int to_close = string2int(point);
                if (to_close == 0)
                    Runtime.Log("Use 'Exit' to close main instance");
                else if (to_close == currentInstance)
                    Runtime.Log("Instance can't close itself");
                else
                {
                    sendDirect(synth, TOPLEVEL::action::lowPrio, to_close, TOPLEVEL::type::Write, MAIN::control::stopInstance, TOPLEVEL::section::main);
                }
                return REPLY::done_msg;
            }
        }
        if (matchnMove(2, point, "mlearn"))
        {
            if (matchnMove(3, point, "all"))
            {
                sendNormal( synth, 0, 0, 0, MIDILEARN::control::clearAll, TOPLEVEL::section::midiLearn);
                return REPLY::done_msg;
            }
            else if (point[0] == '@')
            {
                point += 1;
                point = skipSpace(point);
                tmp = string2int(point);
                if (tmp == 0)
                    return REPLY::value_msg;
                sendNormal( synth, 0, tmp - 1, 0, MIDILEARN::control::deleteLine, TOPLEVEL::section::midiLearn);
                return REPLY::done_msg;
            }
        }
        if (matchnMove(2, point, "instrument") || matchnMove(2, point, "program"))
        {
            tmp = string2int(point);
            if (tmp <= 0 || tmp >= MAX_INSTRUMENTS_IN_BANK)
                    return REPLY::range_msg;
            sendDirect(synth, TOPLEVEL::action::lowPrio, tmp - 1, TOPLEVEL::type::Write, BANK::control::deleteInstrument, TOPLEVEL::section::bank);
            return REPLY::done_msg;
        }
        replyString = "remove";
        return REPLY::what_msg;
    }

    else if (matchnMove(2, point, "load"))
    {
        if(matchnMove(2, point, "mlearn"))
        {
            if (point[0] == '@')
            {
                point += 1;
                tmp = string2int(point);
                if (tmp == 0)
                    return REPLY::value_msg;
                sendNormal( synth, 0, tmp - 1, TOPLEVEL::type::Write, MIDILEARN::control::loadFromRecent, TOPLEVEL::section::midiLearn);
                return REPLY::done_msg;
            }
            if ((std::string) point == "")
                return REPLY::name_msg;
            sendNormal( synth, 0, 0, TOPLEVEL::type::Write, MIDILEARN::control::loadList, TOPLEVEL::section::midiLearn, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush((std::string) point));
            return REPLY::done_msg;
        }
        if(matchnMove(2, point, "vector"))
        {
            std::string loadChan;
            unsigned char ch;
            if(matchnMove(1, point, "channel"))
            {
                ch = string2int127(point);
                if (ch > 0)
                {
                    ch -= 1;
                    point = skipChars(point);
                }
                else
                    ch = chan;
                loadChan = "channel " + asString(ch + 1);
            }
            else
            {
                ch = UNUSED;
                loadChan = "source channel";
            }
            if (ch != UNUSED && ch >= NUM_MIDI_CHANNELS)
                return REPLY::range_msg;
            if (point[0] == 0)
                return REPLY::name_msg;
            std::string name;
            if (point[0] == '@')
            {
                point += 1;
                point = skipSpace(point);
                tmp = string2int(point);
                if (tmp <= 0)
                    return REPLY::value_msg;
                name = historySelect(5, tmp - 1);
                if (name == "")
                    return REPLY::done_msg;
            }
            else
            {
                name = (std::string)point;
                if (name == "")
                    return REPLY::name_msg;
            }
            sendDirect(synth, TOPLEVEL::action::muteAndLoop, 0, TOPLEVEL::type::Write, MAIN::control::loadNamedVector, TOPLEVEL::section::main, UNUSED, UNUSED, ch, UNUSED, UNUSED, miscMsgPush(name));
            return REPLY::done_msg;
        }
        if(matchnMove(2, point, "state"))
        {
            if (point[0] == 0)
                return REPLY::name_msg;
            std::string name;
            if (point[0] == '@')
            {
                point += 1;
                point = skipSpace(point);
                tmp = string2int(point);
                if (tmp <= 0)
                    return REPLY::value_msg;
                name = historySelect(4, tmp - 1);
                if (name == "")
                    return REPLY::done_msg;
            }
            else
            {
                name = (std::string)point;
                if (name == "")
                        return REPLY::name_msg;
            }
            sendDirect(synth, TOPLEVEL::action::muteAndLoop, 0, TOPLEVEL::type::Write, MAIN::control::loadNamedState, TOPLEVEL::section::main, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(name));
            return REPLY::done_msg;
        }
        if (matchnMove(2, point, "scale"))
        {
            if (point[0] == 0)
                return REPLY::name_msg;
            std::string name;
            if (point[0] == '@')
            {
                point += 1;
                point = skipSpace(point);
                tmp = string2int(point);
                if (tmp <= 0)
                    return REPLY::value_msg;
                name = historySelect(3, tmp - 1);
                if (name == "")
                    return REPLY::done_msg;
            }
            else
            {
                name = (std::string)point;
                if (name == "")
                    return REPLY::name_msg;
            }
            sendDirect(synth, TOPLEVEL::action::lowPrio, 0, TOPLEVEL::type::Write, MAIN::control::loadNamedScale, TOPLEVEL::section::main, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(name));
            return REPLY::done_msg;
        }
        if (matchnMove(1, point, "patchset"))
        {
            if (point[0] == 0)
                return REPLY::name_msg;
            std::string name;
            if (point[0] == '@')
            {
                point += 1;
                point = skipSpace(point);
                tmp = string2int(point);
                if (tmp <= 0)
                    return REPLY::value_msg;
                name = historySelect(2, tmp - 1);
                if (name == "")
                    return REPLY::done_msg;
            }
            else
            {
                name = (std::string)point;
                if (name == "")
                    return REPLY::name_msg;
            }
            sendDirect(synth, TOPLEVEL::action::muteAndLoop, 0, TOPLEVEL::type::Write, MAIN::control::loadNamedPatchset, TOPLEVEL::section::main, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(name));
            return REPLY::done_msg;
        }
        if (matchnMove(1, point, "instrument"))
        {
            if (point[0] == 0)
                return REPLY::name_msg;
            std::string name;
            if (point[0] == '@')
            {
                point += 1;
                point = skipSpace(point);
                tmp = string2int(point);
                if (tmp <= 0)
                    return REPLY::value_msg;
                name = historySelect(1, tmp - 1);
                if (name == "")
                    return REPLY::done_msg;
            }
            else
            {
                name = (std::string)point;
                if (name == "")
                    return REPLY::name_msg;
            }

            sendDirect(synth, 0, 0, TOPLEVEL::type::Write, MAIN::control::loadInstrumentByName, TOPLEVEL::section::main, npart, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(name));
            return REPLY::done_msg;
        }
        replyString = "load";
        return REPLY::what_msg;
    }

    if (matchnMove(2, point, "save"))
    {
        if(matchnMove(2, point, "mlearn"))
        {
            if (point[0] == 0)
                return REPLY::name_msg;

            sendNormal( synth, 0, 0, TOPLEVEL::type::Write, MIDILEARN::control::saveList, TOPLEVEL::section::midiLearn, 0, 0, 0, 0, UNUSED, miscMsgPush((std::string) point));
            return REPLY::done_msg;
        }
        if(matchnMove(2, point, "vector"))
        {
            tmp = chan;
            if(matchnMove(1, point, "channel"))
            {
                tmp = string2int127(point) - 1;
                point = skipChars(point);
            }
            if (tmp >= NUM_MIDI_CHANNELS || tmp < 0)
                return REPLY::range_msg;
            if (point[0] == 0)
                return REPLY::name_msg;
            chan = tmp;
            sendDirect(synth, TOPLEVEL::action::lowPrio, 0, TOPLEVEL::type::Write, MAIN::control::saveNamedVector, TOPLEVEL::section::main, UNUSED, UNUSED, chan, UNUSED, UNUSED, miscMsgPush((std::string) point));
            return REPLY::done_msg;
        }
        if(matchnMove(2, point, "state"))
        {
            if (point[0] == 0)
                return REPLY::value_msg;
            sendDirect(synth, TOPLEVEL::action::lowPrio, 0, TOPLEVEL::type::Write, MAIN::control::saveNamedState, TOPLEVEL::section::main, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(std::string(point)));
            return REPLY::done_msg;
        }
        if(matchnMove(1, point, "config"))
        {
            sendDirect(synth, TOPLEVEL::action::lowPrio, 0, TOPLEVEL::type::Write, CONFIG::control::saveCurrentConfig, TOPLEVEL::section::config, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush("DUMMY"));
            return REPLY::done_msg;
        }

        if (matchnMove(2, point, "scale"))
        {
            if (point[0] == 0)
                return REPLY::name_msg;
            sendDirect(synth, TOPLEVEL::action::lowPrio, 0, TOPLEVEL::type::Write, MAIN::control::saveNamedScale, TOPLEVEL::section::main, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(std::string(point)));
            return REPLY::done_msg;
        }
        else if (matchnMove(1, point, "patchset"))
        {
            if (point[0] == 0)
                return REPLY::name_msg;
            sendDirect(synth, TOPLEVEL::action::lowPrio, 0, TOPLEVEL::type::Write, MAIN::control::saveNamedPatchset, TOPLEVEL::section::main, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(std::string(point)));
            return REPLY::done_msg;
        }
        if (matchnMove(1, point, "instrument"))
        {
            if (synth->part[npart]->Pname == "Simple Sound")
            {
                Runtime.Log("Nothing to save!");
                return REPLY::done_msg;
            }
            if (point[0] == 0)
                return REPLY::name_msg;
            sendDirect(synth, TOPLEVEL::action::lowPrio, npart, TOPLEVEL::type::Write, MAIN::control::saveNamedInstrument, TOPLEVEL::section::main, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscMsgPush(std::string(point)));
            return REPLY::done_msg;
        }
        replyString = "save";
        return REPLY::what_msg;
    }

    // legacyCLIaccess goes here

    return REPLY::unrecognised_msg;
}


void CmdInterface::cmdIfaceCommandLoop()
{
    // Initialise the history functionality
    // Set up the history filename
    std::string hist_filename;

    { // put this in a block to lose the passwd afterwards
        struct passwd *pw = getpwuid(getuid());
        hist_filename = std::string(pw->pw_dir) + std::string("/.yoshimi_history");
    }
    using_history();
    stifle_history(80); // Never more than 80 commands
    if (read_history(hist_filename.c_str()) != 0) // reading failed
    {
        perror(hist_filename.c_str());
        ofstream outfile (hist_filename.c_str()); // create an empty file
    }
    cCmd = NULL;
    bool exit = false;
    sprintf(welcomeBuffer, "yoshimi> ");
    synth = firstSynth;
    while(!exit)
    {
        cCmd = readline(welcomeBuffer);
        if (cCmd)
        {
            if (strlen(cCmd) >= COMMAND_SIZE)
                std::cout << "*** Error: line too long" << std::endl;
            else if(cCmd[0] != 0)
            {
                reply = REPLY::todo_msg;
                replyString = "";
                int reply = cmdIfaceProcessCommand(cCmd);
                exit = (reply == REPLY::exit_msg);

                if (reply == REPLY::what_msg)
                    synth->getRuntime().Log(replyString + replies[REPLY::what_msg]);
                else if (reply > REPLY::done_msg)
                    synth->getRuntime().Log(replies[reply]);
                add_history(cCmd);
            }
            free(cCmd);
            cCmd = NULL;

            if (!exit)
            {
                do
                { // create enough delay for most ops to complete
                    usleep(2000);
                }
                while (synth->getRuntime().runSynth && !synth->getRuntime().finishedCLI);
            }
            if (synth->getRuntime().runSynth)
            {
                std::string prompt = "yoshimi";
                if (currentInstance > 0)
                    prompt += (":" + asString(currentInstance));
                int expose = readControl(synth, 0, CONFIG::control::exposeStatus, TOPLEVEL::section::config);
                if (expose == 1)
                {
                    std::string status = findStatus(synth, context, true);
                    if (status == "" )
                        status = " Top";
                    synth->getRuntime().Log("@" + status, 1);
                }
                else if (expose == 2)
                    prompt += findStatus(synth, context, true);
                prompt += "> ";
                sprintf(welcomeBuffer,"%s",prompt.c_str());
            }
        }
        if (!exit && synth->getRuntime().runSynth)
            usleep(20000);
    }

    if (write_history(hist_filename.c_str()) != 0) // writing of history file failed
    {
        perror(hist_filename.c_str());
    }
}