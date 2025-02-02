#include "Exchange.hpp"
#include "Misc/TextMsgBuffer.h"

/**
 * Send action to synth engine. (CLI method)
 *
 * Implemented from: Misc/CliFuncs.h:172
 * Usages:
 *   - CLI/CmdInterpreter.cpp: cmdInterpreterProcessCommand()
 *     - Line 6483: Command "set" (set parameter(s) in CLI)
 */
int YoshimiExchange::sendNormal(SynthEngine*  synth,
                                unsigned char action, float value, unsigned char type, unsigned char control, unsigned char part,
                                unsigned char kit,
                                unsigned char engine,
                                unsigned char insert,
                                unsigned char parameter,
                                unsigned char offset,
                                unsigned char miscmsg)
{
    if ((type & TOPLEVEL::type::Limits) && part != TOPLEVEL::section::midiLearn) {
        // readLimits(synth, value, type, control, part, kit, engine, insert, parameter, miscmsg);
        return REPLY::done_msg;
    }
    action |= TOPLEVEL::action::fromCLI;

    CommandBlock putData;

    putData.data.value     = value;
    putData.data.type      = type;
    putData.data.control   = control;
    putData.data.part      = part;
    putData.data.kit       = kit;
    putData.data.engine    = engine;
    putData.data.insert    = insert;
    putData.data.parameter = parameter;
    putData.data.offset    = offset;
    putData.data.miscmsg   = miscmsg;

    /*
     * MIDI learn settings are synced by the audio thread
     * but not passed on to any of the normal controls.
     * The type field is used for a different purpose.
     */

    if (part != TOPLEVEL::section::midiLearn) {
        putData.data.type |= TOPLEVEL::type::Limits;
        float newValue = synth->interchange.readAllData(&putData);
        if (type & TOPLEVEL::type::LearnRequest) {
            if ((putData.data.type & TOPLEVEL::type::Learnable) == 0) {
                synth->getRuntime().Log("Can't learn this control");
                return REPLY::failed_msg;
            }
        } else {
            if (putData.data.type & TOPLEVEL::type::Error)
                return REPLY::available_msg;
            if (newValue != value && (type & TOPLEVEL::type::Write)) { // checking the original type not the reported one
                putData.data.value = newValue;
                synth->getRuntime().Log("Range adjusted");
            }
        }
        action |= TOPLEVEL::action::fromCLI;
    }
    putData.data.source = action;
    putData.data.type   = type;
    if (synth->interchange.fromCLI.write(putData.bytes)) {
        synth->getRuntime().finishedCLI = false;
    } else {
        synth->getRuntime().Log("Unable to write to fromCLI buffer");
        return REPLY::failed_msg;
    }
    return REPLY::done_msg;
}

/**
 * Send direct action to synth engine. (CLI method)
 *
 * Implemented from: Misc/CliFuncs.h:246
 * Usages:
 *   - CLI/CmdInterpreter.cpp: cmdInterpreter::commandPart()
 *     - Line 5463: Set instrument
 */
int YoshimiExchange::sendDirect(SynthEngine*  synth,
                                unsigned char action, float value, unsigned char type, unsigned char control, unsigned char part,
                                unsigned char kit,
                                unsigned char engine,
                                unsigned char insert,
                                unsigned char parameter,
                                unsigned char offset,
                                unsigned char miscmsg,
                                unsigned char request)
{
    if (action == TOPLEVEL::action::fromMIDI && part != TOPLEVEL::section::midiLearn)
        request = type & TOPLEVEL::type::Default;
    CommandBlock putData;

    putData.data.value     = value;
    putData.data.control   = control;
    putData.data.part      = part;
    putData.data.kit       = kit;
    putData.data.engine    = engine;
    putData.data.insert    = insert;
    putData.data.parameter = parameter;
    putData.data.offset    = offset;
    putData.data.miscmsg   = miscmsg;

    if (type == TOPLEVEL::type::Default) {
        putData.data.type = TOPLEVEL::type::Limits;
        synth->interchange.readAllData(&putData);
        if ((putData.data.type & TOPLEVEL::type::Learnable) == 0) {
            synth->getRuntime().Log("Can't learn this control");
            return 0;
        }
    }

    if (part != TOPLEVEL::section::midiLearn)
        action |= TOPLEVEL::action::fromCLI;
    /*
     * MIDI learn is synced by the audio thread but
     * not passed on to any of the normal controls.
     * The type field is used for a different purpose.
     */
    putData.data.source = action | TOPLEVEL::action::fromCLI;
    putData.data.type   = type;
    if (request < TOPLEVEL::type::Limits) {
        putData.data.type = request | TOPLEVEL::type::Limits;
        value             = synth->interchange.readAllData(&putData);
        string name;
        switch (request) {
            case TOPLEVEL::type::Minimum:
                name = "Min ";
                break;
            case TOPLEVEL::type::Maximum:
                name = "Max ";
                break;
            default:
                name = "Default ";
                break;
        }
        type = putData.data.type;
        if ((type & TOPLEVEL::type::Integer) == 0)
            name += std::to_string(value);
        else if (value < 0)
            name += std::to_string(int(value - 0.5f));
        else
            name += std::to_string(int(value + 0.5f));
        if (type & TOPLEVEL::type::Error)
            name += " - error";
        else if (type & TOPLEVEL::type::Learnable)
            name += " - learnable";
        synth->getRuntime().Log(name);
        return 0;
    }

    if (part == TOPLEVEL::section::main && (type & TOPLEVEL::type::Write) == 0 && control >= MAIN::control::readPartPeak && control <= MAIN::control::readMainLRrms) {
        string name;
        switch (control) {
            case MAIN::control::readPartPeak:
                name = "part " + std::to_string(int(kit));
                if (engine == 0)
                    name += "L ";
                else
                    name += "R ";
                name += "peak ";
                break;
            case MAIN::control::readMainLRpeak:
                name = "main ";
                if (kit == 0)
                    name += "L ";
                else
                    name += "R ";
                name += "peak ";
                break;
            case MAIN::control::readMainLRrms:
                name = "main ";
                if (kit == 0)
                    name += "L ";
                else
                    name += "R ";
                name += "RMS ";
                break;
        }
        value = synth->interchange.readAllData(&putData);
        synth->getRuntime().Log(name + std::to_string(value));
        return 0;
    }

    if (part == TOPLEVEL::section::config && putData.data.miscmsg != UNUSED && (control == CONFIG::control::bankRootCC || control == CONFIG::control::bankCC || control == CONFIG::control::extendedProgramChangeCC)) {
        synth->getRuntime().Log("In use by " + TextMsgBuffer::instance().fetch(putData.data.miscmsg));
        return 0;
    }

    if (parameter != UNUSED && (parameter & TOPLEVEL::action::lowPrio))
        action |= (parameter & TOPLEVEL::action::muteAndLoop); // transfer low prio and loopback
    putData.data.source = action;

    if (synth->interchange.fromCLI.write(putData.bytes)) {
        synth->getRuntime().finishedCLI = false;
    } else
        synth->getRuntime().Log("Unable to write to fromCLI buffer");
    return 0; // no function for this yet
}

float YoshimiExchange::collect_readData(SynthEngine* synth, float value, unsigned char control, unsigned char part, unsigned char kititem, unsigned char engine, unsigned char insert, unsigned char parameter, unsigned char offset, unsigned char miscmsg, unsigned char request)
{
    unsigned char type   = 0;
    unsigned char action = TOPLEVEL::action::fromGUI;
    if (request < TOPLEVEL::type::Limits)
        type = request | TOPLEVEL::type::Limits; // its a limit test
    else if (request != UNUSED)
        action |= request;
    CommandBlock putData;

    putData.data.value     = value;
    putData.data.type      = type;
    putData.data.source    = action;
    putData.data.control   = control;
    putData.data.part      = part;
    putData.data.kit       = kititem;
    putData.data.engine    = engine;
    putData.data.insert    = insert;
    putData.data.parameter = parameter;
    putData.data.offset    = offset;
    putData.data.miscmsg   = miscmsg;

    float result;
    if (miscmsg != NO_MSG) {
        synth->interchange.readAllData(&putData);
        result = putData.data.miscmsg;
    } else
        result = synth->interchange.readAllData(&putData);
    return result;
}

void YoshimiExchange::collect_data(SynthEngine* synth, float value, unsigned char action, unsigned char type, unsigned char control, unsigned char part, unsigned char kititem, unsigned char engine, unsigned char insert, unsigned char parameter, unsigned char offset, unsigned char miscmsg)
{
    if (part < NUM_MIDI_PARTS && engine == PART::engine::padSynth) {
        if (YoshimiExchange::collect_readData(synth, 0, TOPLEVEL::control::partBusy, part)) {
            // alert(synth, "Part " + to_string(part + 1) + " is busy");
            return;
        }
    }

    CommandBlock putData;
    putData.data.value     = value;
    putData.data.control   = control;
    putData.data.part      = part;
    putData.data.kit       = kititem;
    putData.data.engine    = engine;
    putData.data.insert    = insert;
    putData.data.parameter = parameter;
    putData.data.offset    = offset;
    putData.data.miscmsg   = miscmsg;

#if 1
    unsigned char typetop = type & (TOPLEVEL::type::Write | TOPLEVEL::type::Integer);

    // check range & if learnable
    float newValue;
    putData.data.type = 3 | TOPLEVEL::type::Limits;
    newValue          = synth->interchange.readAllData(&putData);

    putData.data.value = newValue;
    type               = TOPLEVEL::type::Write;
    action |= TOPLEVEL::action::forceUpdate;
    // has to be write as it's 'set default'

    type |= typetop;
    action |= TOPLEVEL::action::fromCLI; // Prefer CLI ringbuffer. See comments at the end of this function
    // action |= TOPLEVEL::action::fromGUI;

#else // The following is official FLTK's implementation
    if (action == TOPLEVEL::action::fromMIDI)
        type = type | 1; // faking MIDI from virtual keyboard
    else {
        if (part != TOPLEVEL::section::midiLearn) { // midilearn UI must pass though un-modified
            unsigned char typetop = type & (TOPLEVEL::type::Write | TOPLEVEL::type::Integer);
            unsigned char buttons = Fl::event_button();
            if (part == TOPLEVEL::section::main && (control != MAIN::control::volume && control != MAIN::control::detune))
                type = 1;

            if (buttons == 3 && Fl::event_is_click()) {
                // check range & if learnable
                float newValue;
                putData.data.type = 3 | TOPLEVEL::type::Limits;
                newValue          = synth->interchange.readAllData(&putData);
                // if (newValue != value)
                // std::cout << "Gui limits " << value <<" to " << newValue << std::endl;
                if (Fl::event_state(FL_CTRL) != 0) {
                    if (putData.data.type & TOPLEVEL::type::Learnable) {
                        // identifying this for button 3 as MIDI learn
                        type = TOPLEVEL::type::LearnRequest;
                    } else {
                        alert(synth, "Can't learn this control");
                        synth->getRuntime().Log("Can't MIDI-learn this control");
                        type = TOPLEVEL::type::Learnable;
                    }
                } else {
                    putData.data.value = newValue;
                    type               = TOPLEVEL::type::Write;
                    action |= TOPLEVEL::action::forceUpdate;
                    // has to be write as it's 'set default'
                }
            } else if (buttons > 2)
                type = 1; // change scroll wheel to button 1
            type |= typetop;
            action |= TOPLEVEL::action::fromGUI;
        }
    }
#endif

    putData.data.type   = type;
    putData.data.source = action;
    // std::cerr << "collect_data value " << value << "  action " << int(action)  << "  type " << int(type) << "  control " << int(control) << "  part " << int(part) << "  kit " << int(kititem) << "  engine " << int(engine) << "  insert " << int(insert)  << "  par " << int(parameter) << " offset " << int(offset) << " msg " << int(miscmsg) << std::endl;

    /**
     * Official FLTK uses synth->interchange.fromGUI, but I don't use FLTK in reformed project,
     * because synth engine and FLTK are highly coupled.
     * Use fromCLI instead. (This requires LV2 build in yoshimi tree is disabled.)
     */
    if (!synth->interchange.fromCLI.write(putData.bytes))
        // synth->getRuntime().Log("Unable to write to fromGUI buffer.");
        synth->getRuntime().Log("Unable to write to fromCLI buffer.");
}
