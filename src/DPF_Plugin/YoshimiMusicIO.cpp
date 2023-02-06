/*
    YoshimiMusicIO

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

#include "YoshimiMusicIO.h"
#include "DistrhoPlugin.hpp"

YoshimiMusicIO::YoshimiMusicIO(SynthEngine *synth, uint32_t initSampleRate, uint32_t initBufferSize) 
:MusicIO(synth, new SinglethreadedBeatTracker),
_synth(synth), _sampleRate(initSampleRate), _bufferSize(initBufferSize)
{
    /*
    * Adapted YoshimiLV2Plugin::init() (member function).
    * It actually initialises MusicIO part.
    *
    * Notice: In YoshimiLV2Plugin, _bufferSize is assinged in constructor rather than init().
    *         Here we assign it via YoshimiMusicIO's constructor.
    */

    if (!prepBuffers()) {
        _synth->getRuntime().LogError("Cannot prepare buffers");
        _inited = false;
        return;
    }

    if (!_synth->Init(_sampleRate, _bufferSize))
    {
        _synth->getRuntime().LogError("Cannot init synth engine");
        _inited = false;
	    return;
    }

    _synth->getRuntime().showGui = false;
    _synth->getRuntime().runSynth = true;

    synth->getRuntime().Log("Starting in DPF plugin mode");
    _inited = true;
}

YoshimiMusicIO::~YoshimiMusicIO()
{
    delete beatTracker;
}

void YoshimiMusicIO::process(const float** inputs, float** outputs, uint32_t sample_count, const DISTRHO::MidiEvent *midi_events, uint32_t midi_event_count)
{
    if (sample_count == 0)
    {
        return;
    }

    /*
     * Our implmentation of LV2 has a problem with envelopes. In general
     * the bigger the buffer size the shorter the envelope, and whichever
     * is the smallest (host size or Yoshimi size) determines the time.
     *
     * However, Yoshimi is always correct when working standalone.
     */

    int offs = 0;
    uint32_t next_frame = 0;
    uint32_t processed = 0;
    BeatTracker::BeatValues beats(beatTracker->getRawBeatValues());
    uint32_t beatsAt = 0;
    bool bpmProvided = false;
    float *tmpLeft [NUM_MIDI_PARTS + 1];
    float *tmpRight [NUM_MIDI_PARTS + 1];

    for (uint32_t i = 0; i < NUM_MIDI_PARTS + 1; ++i)
    {
        tmpLeft [i] = zynLeft [i];
        tmpRight [i] = zynRight [i];
    }

    for (uint32_t i = 0; i < midi_event_count; i++)
    {
        DISTRHO::MidiEvent event = midi_events[i];  // NOTICE: DPF's MidiEvent is never null

        if (event.size <= 0)
            continue;

        if (event.size > sizeof(event.kDataSize))
            continue;

        next_frame = event.frame;
        if (next_frame >= sample_count)
            continue;

        uint32_t to_process = next_frame - offs;

        if ((to_process > 0)
            && (processed < sample_count)
            && (to_process <= (sample_count - processed)))
        {
            int mastered = 0;
            offs = next_frame;
            while (to_process - mastered > 0)
            {
                float bpmInc = (float)(processed + mastered - beatsAt) * beats.bpm / (synth->samplerate_f * 60.f);
                synth->setBeatValues(beats.songBeat + bpmInc, beats.monotonicBeat + bpmInc, beats.bpm);
                int mastered_chunk = _synth->MasterAudio(tmpLeft, tmpRight, to_process - mastered);
                for (uint32_t i = 0; i < NUM_MIDI_PARTS + 1; ++i)
                {
                    tmpLeft [i] += mastered_chunk;
                    tmpRight [i] += mastered_chunk;
                }

                mastered += mastered_chunk;
            }
            processed += to_process;
        }

        // Process this midi event
        const uint8_t *msg = (const uint8_t*)event.data;
        //if (_bFreeWheel != NULL)  // I don't know how to implement freewheel. It's always empty.
        processMidiMessage(msg);
    }

    if (processed < sample_count)
    {
        uint32_t to_process = sample_count - processed;
        int mastered = 0;
        offs = next_frame;
        while (to_process - mastered > 0)
        {
            float bpmInc = (float)(processed + mastered - beatsAt) * beats.bpm / (synth->samplerate_f * 60.f);
            synth->setBeatValues(beats.songBeat + bpmInc, beats.monotonicBeat + bpmInc, beats.bpm);
            int mastered_chunk = _synth->MasterAudio(tmpLeft, tmpRight, to_process - mastered);
            for (uint32_t i = 0; i < NUM_MIDI_PARTS + 1; ++i)
            {
                tmpLeft [i] += mastered_chunk;
                tmpRight [i] += mastered_chunk;
            }
            mastered += mastered_chunk;
        }
        processed += to_process;

    }

    float bpmInc = (float)(sample_count - beatsAt) * beats.bpm / (synth->samplerate_f * 60.f);
    beats.songBeat += bpmInc;
    beats.monotonicBeat += bpmInc;
    if (!bpmProvided)
        beats.bpm = synth->PbpmFallback;
    beatTracker->setBeatValues(beats);

#if 0  // notify host about plugin's changes (May not exactly required by DPF!)
    LV2_Atom_Sequence *aSeq = static_cast<LV2_Atom_Sequence *>(_notifyDataPortOut);
    size_t neededAtomSize = sizeof(LV2_Atom_Event) + sizeof(LV2_Atom_Object_Body);
    size_t paddedSize = (neededAtomSize + 7U) & (~7U);
    if (synth->getNeedsSaving() && _notifyDataPortOut && aSeq->atom.size >= paddedSize) //notify host about plugin's changes
    {
        synth->setNeedsSaving(false);
        aSeq->atom.type = _atom_type_sequence;
        aSeq->atom.size = sizeof(LV2_Atom_Sequence_Body);
        aSeq->body.unit = 0;
        aSeq->body.pad = 0;
        LV2_Atom_Event *ev = reinterpret_cast<LV2_Atom_Event *>(aSeq + 1);
        ev->time.frames = 0;
        LV2_Atom_Object *aObj = reinterpret_cast<LV2_Atom_Object *>(&ev->body);
        aObj->atom.type = _atom_object;
        aObj->atom.size = sizeof(LV2_Atom_Object_Body);
        aObj->body.id = 0;
        aObj->body.otype =_atom_state_changed;

        aSeq->atom.size += paddedSize;
    }
    else if (aSeq)
    {
        aSeq->atom.size = sizeof(LV2_Atom_Sequence_Body);
    }
#endif

    /*
    * Currently only 2-channel edition is supported.
    */
	memcpy(outputs[0],  zynLeft[NUM_MIDI_PARTS],  sample_count * sizeof(float));
	memcpy(outputs[1], zynRight[NUM_MIDI_PARTS], sample_count * sizeof(float));
}

void YoshimiMusicIO::processMidiMessage(const uint8_t * msg)
{
    bool in_place = _bFreeWheel ? ((*_bFreeWheel == 0) ? false : true) : false;
    setMidi(msg[0], msg[1], msg[2], in_place);
}
