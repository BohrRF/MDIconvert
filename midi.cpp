#include"midi.h"

void simpleSound::noteOn(const unsigned &iNote, const unsigned &iVelocity, const unsigned &iChannel)
{
    unsigned packdata = (0x90 + iChannel) | (iNote << 8) | (iVelocity << 16);
    midiOutShortMsg(ghMidiOut, packdata);
}

void simpleSound::noteOff(const unsigned &iNote, const unsigned &iChannel)
{
    unsigned packdata = (0x80 + iChannel) | (iNote << 8);
    midiOutShortMsg(ghMidiOut, packdata);
}

void simpleSound::programChange(const unsigned &iChannel, const unsigned &iProgram) const
{
    unsigned packdata = (0xC0 + iChannel) | (iProgram << 8);
    midiOutShortMsg(ghMidiOut, packdata);
}

void simpleSound::programChange(const unsigned &iChannel, const std::pair<int, int> &iProgram) const
{
    unsigned packdata = (0xB0 + iChannel) | (iProgram.first << 16);
//    printf("%08x ", packdata);
    midiOutShortMsg(ghMidiOut, packdata);
    packdata = (0xB0 + iChannel) | (0x20 << 8) | (iProgram.second << 16);
//    printf("%08x ", packdata);getchar();
    midiOutShortMsg(ghMidiOut, packdata);
}

void simpleSound::sysMsg(vector<BYTE> data)
{
    MIDIHDR pMidiHdr={0};
    BYTE msg[data.size() + 1];
    msg[0] = 0xF0;
    int i = 1;
    for(auto a : data) msg[i++] = a;
    //for(int i = 0; i < data.size()+1; i++) printf("%x ", msg[i]);
    //putchar('\n');
    pMidiHdr.lpData=reinterpret_cast<LPSTR>(msg);
    pMidiHdr.dwBufferLength=pMidiHdr.dwBytesRecorded=data.size() + 1;
    midiOutPrepareHeader(ghMidiOut, &pMidiHdr, sizeof(pMidiHdr));
    midiOutLongMsg(ghMidiOut, &pMidiHdr, sizeof(pMidiHdr));
    while((pMidiHdr.dwFlags&MHDR_DONE) == 0);
    midiOutUnprepareHeader(ghMidiOut, &pMidiHdr, sizeof(pMidiHdr));
}


void simpleSound::stopMusic()
{
    for(int i = 0; i < 16; i++)
    {
        midiOutShortMsg(ghMidiOut, 0x78b0 + i);
    }
}

void simpleSound::specialEvent(const vector<unsigned> &events)
{
    unsigned packdata = 0;
    int cnt = 0;
    for(const auto &a : events)
    {
        packdata |= a << (8 * cnt++);
    }
    midiOutShortMsg(ghMidiOut, packdata);
}

simpleSound::simpleSound()
{
    MMRESULT mmres = midiOutOpen(&ghMidiOut, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
    if (mmres != MMSYSERR_NOERROR)
        fprintf(stderr, "MIDI‚ª—˜—p‚Å‚«‚Ü‚¹‚ñB\n");
//
}



simpleSound::~simpleSound()
{
    midiOutClose(ghMidiOut);
}
