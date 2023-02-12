#pragma once

#include "Misc/SynthEngine.h"

/**
 * Unlike common synthesizers, Yoshimi use messages to communicate between front-end and back-end.
 * There are two kinds of front-ends:
 *   - FLTK: Use collect_readData() and collect_data(). Defined in MiscGui.h
 *           UI components packs calls to them via fetchData() and send_data().
 *   - CLI:  Use sendNormal() and other possible functions.
 * Both of them can be successfully implemented here, with FLTK-related calls removed.
 * But only CLI's method can actually apply parameters.
 * So in Yoshimi Reform, I will mainly use CLI's calls to perform "UI-to-synth-engine"
 * communications.
 */

namespace YoshimiExchange {
    // ----------------------------------------------------------------------------------------------------------------
    // FLTK communicators (not usable)

    float collect_readData(SynthEngine* synth, float value, unsigned char control, unsigned char part, unsigned char kititem = 0xff, unsigned char engine = 0xff, unsigned char insert = 0xff, unsigned char parameter = 0xff, unsigned char offset = 0xff, unsigned char miscmsg = 0xff, unsigned char request = 0xff);
    void  collect_data(SynthEngine* synth, float value, unsigned char action, unsigned char type, unsigned char control, unsigned char part, unsigned char kititem = 0xff, unsigned char engine = 0xff, unsigned char insert = 0xff, unsigned char parameter = 0xff, unsigned char offset = 0xff, unsigned char miscmsg = 0xff);

    // ----------------------------------------------------------------------------------------------------------------
    // CLI communicators

    int sendNormal(SynthEngine*  synth,
                   unsigned char action, float value, unsigned char type, unsigned char control, unsigned char part,
                   unsigned char kit       = UNUSED,
                   unsigned char engine    = UNUSED,
                   unsigned char insert    = UNUSED,
                   unsigned char parameter = UNUSED,
                   unsigned char offset    = UNUSED,
                   unsigned char miscmsg   = NO_MSG);

    // ----------------------------------------------------------------------------------------------------------------
    // FLTK communicators for each UI component (not usable)

    namespace MasterUI {
        void  send_data(SynthEngine* synth, int action, int control, float value, int type, int part = UNUSED, int engine = UNUSED, int insert = UNUSED, int parameter = UNUSED, int miscmsg = UNUSED);
        float fetchData(SynthEngine* synth, float value, int control, int part, int kititem = UNUSED, int engine = UNUSED, int insert = UNUSED, int parameter = UNUSED, int offset = UNUSED, int miscmsg = UNUSED, int request = UNUSED);
    }

}
