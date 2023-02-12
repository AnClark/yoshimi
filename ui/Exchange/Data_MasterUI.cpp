#include "Exchange.hpp"

void YoshimiExchange::MasterUI::send_data(SynthEngine* synth, int action, int control, float value, int type, int part, int engine, int insert, int parameter, int miscmsg)
{
    //
    type |= TOPLEVEL::type::Write;
    /*
        The following variations are due to the way the section was built
        up over time. It really needs the whole lot expanding for the calls
        to natively include all parameters.
    */
    if (control == MAIN::control::loadInstrumentByName && part == TOPLEVEL::section::main && miscmsg < NO_MSG) {
        collect_data(synth, 0, action, type, control, part, engine, UNUSED, UNUSED, UNUSED, UNUSED, miscmsg);
        return;
    }

    // if (control <= MAIN::control::soloType)
    //     type |= Fl::event_button();
    if (parameter == 0) {
        collect_data(synth, 0, action, type, control, part, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, miscmsg);
        return;
    }

    collect_data(synth, value, action, type, control, part, UNUSED, engine, insert, parameter, UNUSED, miscmsg);
}

float YoshimiExchange::MasterUI::fetchData(SynthEngine* synth, float value, int control, int part, int kititem, int engine, int insert, int parameter, int offset, int miscmsg, int request)
{
    //
    return collect_readData(synth, value, control, part, kititem, engine, insert, parameter, offset, miscmsg, request);
}
