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

    // I use unique pointer for synthesizer. So firstSynth seems to be unneeded
    //if (_synth->getUniqueId() == 0)
    //{
    //    firstSynth = _synth;
    //    //firstSynth->getRuntime().Log("Started first");
    //}

    _synth->getRuntime().showGui = false;
    _synth->getRuntime().runSynth = true;

    synth->getRuntime().Log("Starting in DPF plugin mode");
    _inited = true;
}

YoshimiMusicIO::~YoshimiMusicIO()
{
    delete beatTracker;
}
