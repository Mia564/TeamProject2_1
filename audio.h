#pragma once

#include <xaudio2.h>
#include <mmreg.h>

#include "misc.h"

class Audio {
    WAVEFORMATEXTENSIBLE wfx = { 0 };
    XAUDIO2_BUFFER buffer = { 0 };

    IXAudio2SourceVoice* sourceVoice;

public:
    Audio(IXAudio2* xaudio2, const wchar_t* filename);
    virtual ~Audio();
    void play(int loopCount = 0/*255 : XAUDIO2_LOOP_INFINITE*/);
    void stop(bool playTails = true, bool waitForBufferToUnqueue = true);
    void volume(float volume);
    bool queuing();
};