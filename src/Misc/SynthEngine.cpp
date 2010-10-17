/*
    SynthEngine.cpp

    Original ZynAddSubFX author Nasca Octavian Paul
    Copyright (C) 2002-2005 Nasca Octavian Paul
    Copyright 2009-2010, Alan Calvert
    Copyright 2009, James Morris

    This file is part of yoshimi, which is free software: you can redistribute
    it and/or modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    yoshimi is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.   See the GNU General Public License (version 2 or
    later) for more details.

    You should have received a copy of the GNU General Public License along with
    yoshimi; if not, write to the Free Software Foundation, Inc., 51 Franklin
    Street, Fifth Floor, Boston, MA  02110-1301, USA.

    This file is a derivative of a ZynAddSubFX original, modified October 2010
*/

#include <iostream>

//using namespace std;

#include "MasterUI.h"
#include "Misc/SynthEngine.h"

SynthEngine *synth = NULL;

char SynthEngine::random_state[256] = { 0, };
struct random_data SynthEngine::random_buf;

SynthEngine::SynthEngine() :
    muted(0),
    shutup(false),
    samplerate(48000),
    samplerate_f(samplerate),
    halfsamplerate_f(samplerate / 2),
    buffersize(0),
    buffersize_f(buffersize),
    oscilsize(1024),
    oscilsize_f(oscilsize),
    halfoscilsize(oscilsize / 2),
    halfoscilsize_f(halfoscilsize),
    ctl(NULL),
    fft(NULL),
    recordPending(false),
    tmpmixl(NULL),
    tmpmixr(NULL),
    stateXMLtree(NULL)
{
    ctl = new Controller();
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        part[npart] = NULL;
    for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        insefx[nefx] = NULL;
    for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
        sysefx[nefx] = NULL;
    shutup = false;
}


SynthEngine::~SynthEngine()
{
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        if (part[npart])
            delete part[npart];
    for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        if (insefx[nefx])
            delete insefx[nefx];
    for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
        if (sysefx[nefx])
            delete sysefx[nefx];

    if (tmpmixl)
        delete [] tmpmixl;
    if (tmpmixr)
        delete [] tmpmixr;
    if (fft)
        delete fft;
    if (ctl)
        delete ctl;
}


bool SynthEngine::Init(unsigned int audiosrate, int audiobufsize)
{
    samplerate_f = samplerate = audiosrate;
    halfsamplerate_f = samplerate / 2;
    buffersize_f = buffersize = audiobufsize;
    bufferbytes = buffersize * sizeof(float);
    oscilsize_f = oscilsize = Runtime.Oscilsize;
    halfoscilsize_f = halfoscilsize = oscilsize / 2;

    if (initstate_r(samplerate + buffersize + oscilsize, random_state,
                    sizeof(random_state), &random_buf))
        Runtime.Log("SynthEngine Init failed on general randomness");

    if (oscilsize < (buffersize / 2))
    {
        Runtime.Log("Enforcing oscilsize to half buffersize, "
                    + asString(oscilsize) + " -> " + asString(buffersize / 2));
        oscilsize = buffersize / 2;
    }

    if ((fft = new FFTwrapper(oscilsize)) == NULL)
    {
        Runtime.Log("SynthEngine failed to allocate fft");
        goto bail_out;
    }

    tmpmixl = new float[buffersize];
    tmpmixr = new float[buffersize];
    if (tmpmixl == NULL || tmpmixr == NULL)
    {
        Runtime.Log("SynthEngine tmpmix allocations failed");
        goto bail_out;
    }

    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        part[npart] = new Part(&microtonal, fft);
        if (NULL == part[npart])
        {
            Runtime.Log("Failed to allocate new Part");
            goto bail_out;
        }
        vuoutpeakpart[npart] = 1e-9;
        fakepeakpart[npart] = 0;
    }

    // Insertion Effects init
    for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
    {
        insefx[nefx] = new EffectMgr(1);
        if (NULL == insefx[nefx])
        {
            Runtime.Log("Failed to allocate new Insertion EffectMgr");
            goto bail_out;
        }
    }

    // System Effects init
    for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
    {
        sysefx[nefx] = new EffectMgr(0);
        if (NULL == sysefx[nefx])
        {
            Runtime.Log("Failed to allocate new System Effects EffectMgr");
            goto bail_out;
        }
    }
    defaults();
    if (Runtime.doRestoreJackSession)
    {
        if (!Runtime.restoreJsession(this))
        {
            Runtime.Log("Restore jack session failed");
            goto bail_out;
        }
    }
    else if (Runtime.doRestoreState)
    {
        if (!Runtime.restoreState(this))
         {
             Runtime.Log("Restore state failed");
             goto bail_out;
         }
    }
    else
    {
        if (Runtime.paramsLoad.size())
        {
            if (loadXML(Runtime.paramsLoad) >= 0)
            {
                applyparameters();
                Runtime.paramsLoad = Runtime.addParamHistory(Runtime.paramsLoad);
                Runtime.Log("Loaded " + Runtime.paramsLoad + " parameters");
            }
            else
            {
                Runtime.Log("Failed to load parameters " + Runtime.paramsLoad);
                goto bail_out;
            }
        }
        if (!Runtime.instrumentLoad.empty())
        {
            int loadtopart = 0;
            if (!part[loadtopart]->loadXMLinstrument(Runtime.instrumentLoad))
            {
                Runtime.Log("Failed to load instrument file " + Runtime.instrumentLoad);
                goto bail_out;
            }
            else
            {
                part[loadtopart]->applyparameters(true);
                Runtime.Log("Instrument file " + Runtime.instrumentLoad + " loaded");
            }
        }
    }
    return true;

bail_out:
    if (fft != NULL)
        delete fft;
    fft = NULL;
    if (tmpmixl != NULL)
        delete tmpmixl;
    tmpmixl = NULL;
    if (tmpmixr != NULL)
        delete tmpmixr;
    tmpmixr = NULL;
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        if (NULL != part[npart])
            delete part[npart];
        part[npart] = NULL;
    }
    for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
    {
        if (NULL != insefx[nefx])
            delete insefx[nefx];
        insefx[nefx] = NULL;
    }
    for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
    {
        if (NULL != sysefx[nefx])
            delete sysefx[nefx];
        sysefx[nefx] = NULL;
    }
    return false;
}


void SynthEngine::defaults(void)
{
    setPvolume(90);
    setPkeyshift(64);
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        part[npart]->defaults();
        part[npart]->midichannel = npart % NUM_MIDI_CHANNELS;
    }
    partOnOff(0, 1); // enable the first part
    for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
    {
        insefx[nefx]->defaults();
        Pinsparts[nefx] = -1;
    }
    // System Effects init
    for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
    {
        sysefx[nefx]->defaults();
        for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
            setPsysefxvol(npart, nefx, 0);
        for (int nefxto = 0; nefxto < NUM_SYS_EFX; ++nefxto)
            setPsysefxsend(nefx, nefxto, 0);
    }
    microtonal.defaults();
    ShutUp();
}


// Note On Messages ()
void SynthEngine::noteOn(unsigned char chan, unsigned char note,
                         unsigned char velocity, bool record_trigger)
{                        // velocity 0 => NoteOff
    if (!velocity)
        noteOff(chan, note);
    else if (!muted)
    {
        if (recordPending && record_trigger)
            guiMaster->record_activated();
        for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        {
            if (part[npart]->Penabled && chan == part[npart]->midichannel)
            {
                lockSharable();
                part[npart]->NoteOn(note, velocity, keyshift);
                unlockSharable();
            }
        }
    }
    else
        cerr << " Is muted" << endl;
}


void SynthEngine::noteOff(unsigned char chan, unsigned char note)
{
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        if (part[npart]->Penabled && chan == part[npart]->midichannel)
        {
            lockSharable();
            part[npart]->NoteOff(note);
            unlockSharable();
        }
    }
}


void SynthEngine::setController(unsigned char chan, unsigned char type, short int par)
{
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {   // Send the controller to all active parts assigned to the channel
        if (part[npart]->Penabled && chan == part[npart]->midichannel)
            part[npart]->SetController(type, par);
    }

    if (type == C_allsoundsoff)
    {   // cleanup insertion/system FX
        synth->lockSharable();
        for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
            sysefx[nefx]->cleanup();
        for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
            insefx[nefx]->cleanup();
        synth->unlockSharable();
    }
}


void SynthEngine::setPitchwheel(unsigned char chan, short int par)
{
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        if (part[npart]->Penabled && chan == part[npart]->midichannel)
            part[npart]->ctl->setpitchwheel(par);
}


void SynthEngine::programChange(unsigned char chan, int bankmsb, int banklsb)
{
    cerr << "Through SynthEngine::programChange, bank msb " << bankmsb
         << ", banklsb " << banklsb << endl;
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        if (part[npart]->Penabled && chan == part[npart]->midichannel)
        {
            cerr << "Part " << npart << ", chan " << (int)chan << " to change program" << endl;
            //part[npart]->programChange(bankmsb, banklsb)
        }
}


// Enable/Disable a part
void SynthEngine::partOnOff(int npart, int what)
{
    if (npart >= NUM_MIDI_PARTS)
        return;
    fakepeakpart[npart] = 0;
    lockSharable();
    if (what)
        part[npart]->Penabled = 1;
    else
    {   // disabled part
        part[npart]->Penabled = 0;
        part[npart]->cleanup();
        for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
            if (Pinsparts[nefx] == npart)
                insefx[nefx]->cleanup();
    }
    unlockSharable();
}


// Master audio out (the final sound)
void SynthEngine::MasterAudio(float *outl, float *outr)
{
    memset(outl, 0, bufferbytes);
    memset(outr, 0, bufferbytes);
    // Compute part samples and store them npart]->partoutl,partoutr
    int npart;
    for (npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        if (part[npart]->Penabled)
        {
            lockExclusive();
            part[npart]->ComputePartSmps();
            unlockExclusive();
        }
    // Insertion effects
    int nefx;
    for (nefx = 0; nefx < NUM_INS_EFX; ++nefx)
    {
        if (Pinsparts[nefx] >= 0)
        {
            int efxpart = Pinsparts[nefx];
            if (part[efxpart]->Penabled)
            {
                //lockExclusive();
                insefx[nefx]->out(part[efxpart]->partoutl, part[efxpart]->partoutr);
                //unlockExclusive();
            }
        }
    }

    // Apply the part volumes and pannings (after insertion effects)
    for (npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        if (!part[npart]->Penabled)
            continue;
        float newvol_l = part[npart]->volume;
        float newvol_r = part[npart]->volume;
        float oldvol_l = part[npart]->oldvolumel;
        float oldvol_r = part[npart]->oldvolumer;
        float pan = part[npart]->panning;
        if (pan < 0.5)
            newvol_l *= (1.0 - pan) * 2.0;
        else
            newvol_r *= pan * 2.0;

        if (aboveAmplitudeThreshold(oldvol_l, newvol_l)
            || aboveAmplitudeThreshold(oldvol_r, newvol_r))
        {   // the volume or the panning has changed and needs interpolation
            for (int i = 0; i < buffersize; ++i)
            {
                float vol_l = interpolateAmplitude(oldvol_l, newvol_l, i, buffersize);
                float vol_r = interpolateAmplitude(oldvol_r, newvol_r, i, buffersize);
                part[npart]->partoutl[i] *= vol_l;
                part[npart]->partoutr[i] *= vol_r;
            }
            part[npart]->oldvolumel = newvol_l;
            part[npart]->oldvolumer = newvol_r;
        }
        else
        {
            for (int i = 0; i < buffersize; ++i)
            {   // the volume did not change
                part[npart]->partoutl[i] *= newvol_l;
                part[npart]->partoutr[i] *= newvol_r;
            }
        }
    }
    // System effects
    for (nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
    {
        if (!sysefx[nefx]->geteffect())
            continue; // is disabled

        // Clean up the samples used by the system effects
        memset(tmpmixl, 0, bufferbytes);
        memset(tmpmixr, 0, bufferbytes);

        // Mix the channels according to the part settings about System Effect
        for (npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        {
            // skip if part is disabled or has no output to effect
            if (part[npart]->Penabled && Psysefxvol[nefx][npart])
            {
                // the output volume of each part to system effect
                float vol = sysefxvol[nefx][npart];
                for (int i = 0; i < buffersize; ++i)
                {
                    tmpmixl[i] += part[npart]->partoutl[i] * vol;
                    tmpmixr[i] += part[npart]->partoutr[i] * vol;
                }
            }
        }

        // system effect send to next ones
        for (int nefxfrom = 0; nefxfrom < nefx; ++nefxfrom)
        {
            if (Psysefxsend[nefxfrom][nefx])
            {
                float v = sysefxsend[nefxfrom][nefx];
                for (int i = 0; i < buffersize; ++i)
                {
                    //lockExclusive();
                    tmpmixl[i] += sysefx[nefxfrom]->efxoutl[i] * v;
                    tmpmixr[i] += sysefx[nefxfrom]->efxoutr[i] * v;
                    //unlockExclusive();
                }
            }
        }
        sysefx[nefx]->out(tmpmixl, tmpmixr);

        // Add the System Effect to sound output
        float outvol = sysefx[nefx]->sysefxgetvolume();
        for (int i = 0; i < buffersize; ++i)
        {
            outl[i] += tmpmixl[i] * outvol;
            outr[i] += tmpmixr[i] * outvol;
       }
    }

    // Mix all parts
    for (npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        for (int i = 0; i < buffersize; ++i)
        {   // the volume did not change
            outl[i] += part[npart]->partoutl[i];
            outr[i] += part[npart]->partoutr[i];
        }
    }

    // Insertion effects for Master Out
    for (nefx = 0; nefx < NUM_INS_EFX; ++nefx)
    {
        if (Pinsparts[nefx] == -2)
        {
            lockExclusive();
            insefx[nefx]->out(outl, outr);
            unlockExclusive();
        }
    }

    LFOParams::time++; // update the LFO's time

    meterMutex.lock();
    vuoutpeakl = 1e-12f;
    vuoutpeakr = 1e-12f;
    vurmspeakl = 1e-12f;
    vurmspeakr = 1e-12f;
    meterMutex.unlock();

    float absval;
    for (int idx = 0; idx < buffersize; ++idx)
    {
        outl[idx] *= volume; // apply Master Volume
        outr[idx] *= volume;

        if ((absval = fabsf(outl[idx])) > vuoutpeakl) // Peak computation (for vumeters)
            vuoutpeakl = absval;
        if ((absval = fabsf(outr[idx])) > vuoutpeakr)
            vuoutpeakr = absval;
        vurmspeakl += outl[idx] * outl[idx];  // RMS Peak
        vurmspeakr += outr[idx] * outr[idx];

        // Clip as necessary
        if (outl[idx] > 1.0f)
            clippedL = true;
         else if (outl[idx] < -1.0f)
            clippedL = true;
        if (outr[idx] > 1.0f)
            clippedR = true;
         else if (outr[idx] < -1.0f)
            clippedR = true;

        if (shutup) // fade-out
        {
            float fade = (float)(buffersize - idx) / (float)buffersize;
            outl[idx] *= fade;
            outr[idx] *= fade;
        }
    }
    if (shutup)
        ShutUp();

    meterMutex.lock();
    if (vumaxoutpeakl < vuoutpeakl)  vumaxoutpeakl = vuoutpeakl;
    if (vumaxoutpeakr < vuoutpeakr)  vumaxoutpeakr = vuoutpeakr;

    vurmspeakl = sqrtf(vurmspeakl / buffersize);
    vurmspeakr = sqrtf(vurmspeakr / buffersize);

    // Part Peak computation (for Part vu meters/fake part vu meters)
    for (npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        vuoutpeakpart[npart] = 1.0e-12f;
        if (part[npart]->Penabled)
        {
            float *outl = part[npart]->partoutl;
            float *outr = part[npart]->partoutr;
            for (int i = 0; i < buffersize; ++i)
            {
                float tmp = fabsf(outl[i] + outr[i]);
                if (tmp > vuoutpeakpart[npart])
                    vuoutpeakpart[npart] = tmp;
            }
            vuoutpeakpart[npart] *= volume; // how is part peak related to master volume??
        }
        else if (fakepeakpart[npart] > 1)
            fakepeakpart[npart]--;
    }
    vuOutPeakL =    vuoutpeakl;
    vuOutPeakR =    vuoutpeakr;
    vuMaxOutPeakL = vumaxoutpeakl;
    vuMaxOutPeakR = vumaxoutpeakr;
    vuRmsPeakL =    vurmspeakl;
    vuRmsPeakR =    vurmspeakr;
    vuClippedL =    clippedL;
    vuClippedR =    clippedR;
    meterMutex.unlock();
}


// Parameter control
void SynthEngine::setPvolume(char control_value)
{
    Pvolume = control_value;
    volume  = dB2rap((Pvolume - 96.0f) / 96.0f * 40.0f);
}


void SynthEngine::setPkeyshift(char Pkeyshift_)
{
    Pkeyshift = Pkeyshift_;
    keyshift = (int)Pkeyshift - 64;
}


void SynthEngine::setPsysefxvol(int Ppart, int Pefx, char Pvol)
{
    Psysefxvol[Pefx][Ppart] = Pvol;
    sysefxvol[Pefx][Ppart]  = powf(0.1f, (1.0f - Pvol / 96.0f) * 2.0f);
}


void SynthEngine::setPsysefxsend(int Pefxfrom, int Pefxto, char Pvol)
{
    Psysefxsend[Pefxfrom][Pefxto] = Pvol;
    sysefxsend[Pefxfrom][Pefxto]  = powf(0.1f, (1.0f - Pvol / 96.0f) * 2.0f);
}


// Panic! (Clean up all parts and effects)
void SynthEngine::ShutUp(void)
{
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        part[npart]->cleanup();
        fakepeakpart[npart] = 0;
    }
    for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        insefx[nefx]->cleanup();
    for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
        sysefx[nefx]->cleanup();
    vuresetpeaks();
    shutup = false;
}


// Reset peaks and clear the "clipped" flag (for VU-meter)
void SynthEngine::vuresetpeaks(void)
{
    meterMutex.lock();
    vuOutPeakL = vuoutpeakl = 1e-12f;
    vuOutPeakR = vuoutpeakr =  1e-12f;
    vuMaxOutPeakL = vumaxoutpeakl = 1e-12f;
    vuMaxOutPeakR = vumaxoutpeakr = 1e-12f;
    vuRmsPeakL = vurmspeakl = 1e-12f;
    vuRmsPeakR = vurmspeakr = 1e-12f;
    vuClippedL = vuClippedL = clippedL = clippedR = false;
    meterMutex.unlock();
}


void SynthEngine::lockExclusive(void)
{
    using namespace boost::interprocess;
    try { synthMutex.lock(); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::lockExclusive throws exception!");
    }
}


void SynthEngine::unlockExclusive(void)
{
    using namespace boost::interprocess;
    try { synthMutex.unlock(); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::unlockExclusive throws exception!");
    }
}


bool SynthEngine::trylockExclusive(void)
{
    bool ok = false;
    using namespace boost::interprocess;
    try { ok = synthMutex.try_lock(); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::trylockExclusive throws exception!");
    }
    return ok;
}


bool SynthEngine::timedlockExclusive(void)
{
    bool ok = false;
    //  wait_period = boost::posix_time::microsec(1000u);
    boost::posix_time::ptime endtime = boost::posix_time::microsec_clock::local_time()
                                       + boost::posix_time::microsec(666u);
    using namespace boost::interprocess;
    try { ok = synthMutex.timed_lock(endtime); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::timedlockExclusive throws exception!");
        ok = false;
    }
    return ok;
}


void SynthEngine::lockSharable(void)
{
    using namespace boost::interprocess;
    try { synthMutex.lock_sharable(); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::lockSharable throws exception!");
    }
}


void SynthEngine::unlockSharable(void)
{
    using namespace boost::interprocess;
    try { synthMutex.unlock_sharable(); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::unlockSharable throws exception!");
    }
}


bool SynthEngine::trylockSharable(void)
{
    bool ok = false;
    using namespace boost::interprocess;
    try {ok = synthMutex.try_lock_sharable(); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::trylockSharable throws exception!");
    }
    return ok;
}


bool SynthEngine::timedlockSharable(void)
{
    bool ok = false;
    boost::posix_time::time_duration wait_period = boost::posix_time::microsec(1000u);
    boost::posix_time::ptime endtime = boost::posix_time::microsec_clock::local_time() + lockwait;
                                       //+ boost::posix_time::microsec(500);
    using namespace boost::interprocess;
    try { ok = synthMutex.timed_lock_sharable(endtime); }
    catch (interprocess_exception &ex)
    {
        Runtime.Log("SynthEngine::timedlockSharable throws exception!");
        ok = false;
    }
    return ok;
}


void SynthEngine::applyparameters(void)
{
    ShutUp();
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        part[npart]->applyparameters(true);
}


void SynthEngine::add2XML(XMLwrapper *xml)
{
    xml->beginbranch("MASTER");
    lockSharable();
    xml->addpar("volume", Pvolume);
    xml->addpar("key_shift", Pkeyshift);
    xml->addparbool("nrpn_receive", ctl->NRPN.receive);

    xml->beginbranch("MICROTONAL");
    microtonal.add2XML(xml);
    xml->endbranch();

    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        xml->beginbranch("PART",npart);
        part[npart]->add2XML(xml);
        xml->endbranch();
    }

    xml->beginbranch("SYSTEM_EFFECTS");
    for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
    {
        xml->beginbranch("SYSTEM_EFFECT", nefx);
        xml->beginbranch("EFFECT");
        sysefx[nefx]->add2XML(xml);
        xml->endbranch();

        for (int pefx = 0; pefx < NUM_MIDI_PARTS; ++pefx)
        {
            xml->beginbranch("VOLUME", pefx);
            xml->addpar("vol", Psysefxvol[nefx][pefx]);
            xml->endbranch();
        }

        for (int tonefx = nefx + 1; tonefx < NUM_SYS_EFX; ++tonefx)
        {
            xml->beginbranch("SENDTO", tonefx);
            xml->addpar("send_vol", Psysefxsend[nefx][tonefx]);
            xml->endbranch();
        }
        xml->endbranch();
    }
    xml->endbranch();

    xml->beginbranch("INSERTION_EFFECTS");
    for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
    {
        xml->beginbranch("INSERTION_EFFECT", nefx);
        xml->addpar("part", Pinsparts[nefx]);

        xml->beginbranch("EFFECT");
        insefx[nefx]->add2XML(xml);
        xml->endbranch();
        xml->endbranch();
    }
    xml->endbranch(); // INSERTION_EFFECTS
    unlockSharable();
    xml->endbranch(); // MASTER
}


int SynthEngine::getalldata(char **data)
{
    XMLwrapper *xml = new XMLwrapper();
    add2XML(xml);
    *data = xml->getXMLdata();
    delete xml;
    return strlen(*data) + 1;
}


void SynthEngine::putalldata(char *data, int size)
{
    XMLwrapper *xml = new XMLwrapper();
    if (!xml->putXMLdata(data))
    {
        Runtime.Log("SynthEngine putXMLdata failed");
        delete xml;
        return;
    }
    if (xml->enterbranch("MASTER"))
    {
        lockSharable();
        getfromXML(xml);
        unlockSharable();
        xml->exitbranch();
    }
    else
        Runtime.Log("Master putAllData failed to enter MASTER branch");
    delete xml;
}


bool SynthEngine::saveXML(string filename)
{
    XMLwrapper *xml = new XMLwrapper();
    add2XML(xml);
    bool result = xml->saveXMLfile(filename);
    delete xml;
    return result;
}


bool SynthEngine::loadXML(string filename)
{
    XMLwrapper *xml = new XMLwrapper();
    if (NULL == xml)
    {
        Runtime.Log("failed to init xml tree");
        return false;
    }
    if (!xml->loadXMLfile(filename))
    {
        delete xml;
        return false;
    }
    defaults();
    bool isok = getfromXML(xml);
    delete xml;
    return isok;
}


bool SynthEngine::getfromXML(XMLwrapper *xml)
{
    if (!xml->enterbranch("MASTER"))
    {
        Runtime.Log("SynthEngine getfromXML, no MASTER branch");
        return false;
    }
    setPvolume(xml->getpar127("volume", Pvolume));
    setPkeyshift(xml->getpar127("key_shift", Pkeyshift));
    ctl->NRPN.receive = xml->getparbool("nrpn_receive", ctl->NRPN.receive);

    part[0]->Penabled = 0;
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        if (!xml->enterbranch("PART", npart))
            continue;
        part[npart]->getfromXML(xml);
        xml->exitbranch();
    }

    if (xml->enterbranch("MICROTONAL"))
    {
        microtonal.getfromXML(xml);
        xml->exitbranch();
    }

    sysefx[0]->changeeffect(0);
    if (xml->enterbranch("SYSTEM_EFFECTS"))
    {
        for (int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
        {
            if (!xml->enterbranch("SYSTEM_EFFECT", nefx))
                continue;
            if (xml->enterbranch("EFFECT"))
            {
                sysefx[nefx]->getfromXML(xml);
                xml->exitbranch();
            }

            for (int partefx = 0; partefx < NUM_MIDI_PARTS; ++partefx)
            {
                if (!xml->enterbranch("VOLUME", partefx))
                    continue;
                setPsysefxvol(partefx, nefx,xml->getpar127("vol", Psysefxvol[partefx][nefx]));
                xml->exitbranch();
            }

            for (int tonefx = nefx + 1; tonefx < NUM_SYS_EFX; ++tonefx)
            {
                if (!xml->enterbranch("SENDTO", tonefx))
                    continue;
                setPsysefxsend(nefx, tonefx, xml->getpar127("send_vol", Psysefxsend[nefx][tonefx]));
                xml->exitbranch();
            }
            xml->exitbranch();
        }
        xml->exitbranch();
    }

    if (xml->enterbranch("INSERTION_EFFECTS"))
    {
        for (int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        {
            if (!xml->enterbranch("INSERTION_EFFECT", nefx))
                continue;
            Pinsparts[nefx] = xml->getpar("part", Pinsparts[nefx], -2, NUM_MIDI_PARTS);
            if (xml->enterbranch("EFFECT"))
            {
                insefx[nefx]->getfromXML(xml);
                xml->exitbranch();
            }
            xml->exitbranch();
        }
        xml->exitbranch();
    }
    xml->exitbranch(); // MASTER
    return true;
}
