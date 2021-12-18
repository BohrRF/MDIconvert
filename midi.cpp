#include "midi.h"
#include <mmeapi.h>

void midiOut::noteOn(const unsigned &iNote, const unsigned &iVelocity, const unsigned &iChannel)
{
    unsigned packdata = (0x90 + iChannel) | (iNote << 8) | (iVelocity << 16);
    midiOutShortMsg(ghMidiOut, packdata);
}

void midiOut::noteOff(const unsigned &iNote, const unsigned &iChannel)
{
    unsigned packdata = (0x80 + iChannel) | (iNote << 8);
    midiOutShortMsg(ghMidiOut, packdata);
}

void midiOut::programChange(const unsigned &iChannel, const unsigned &iProgram) const
{
    unsigned packdata = (0xC0 + iChannel) | (iProgram << 8);
    midiOutShortMsg(ghMidiOut, packdata);
}

void midiOut::programChange(const unsigned &iChannel, const std::pair<int, int> &iProgram) const
{
    unsigned packdata = (0xB0 + iChannel) | (iProgram.first << 16);
    midiOutShortMsg(ghMidiOut, packdata);
    packdata = (0xB0 + iChannel) | (0x20 << 8) | (iProgram.second << 16);
    midiOutShortMsg(ghMidiOut, packdata);
}

void midiOut::specialEvent(const vector<unsigned> &events)
{
    unsigned packdata = 0;
    int cnt = 0;
    for(const auto &a : events) packdata |= a << (8 * cnt++);
    midiOutShortMsg(ghMidiOut, packdata);
}

void midiOut::sysMsg(vector<BYTE> data)
{
    MIDIHDR pMidiHdr={0};
    BYTE* msg = new BYTE[data.size() + 1];
    msg[0] = 0xF0;
    int i = 1;
    for(auto a : data) msg[i++] = a;

    pMidiHdr.lpData=reinterpret_cast<LPSTR>(msg);
    pMidiHdr.dwBufferLength=pMidiHdr.dwBytesRecorded=data.size() + 1;
    midiOutPrepareHeader(ghMidiOut, &pMidiHdr, sizeof(pMidiHdr));
    midiOutLongMsg(ghMidiOut, &pMidiHdr, sizeof(pMidiHdr));
    while((pMidiHdr.dwFlags&MHDR_DONE) == 0);
    midiOutUnprepareHeader(ghMidiOut, &pMidiHdr, sizeof(pMidiHdr));

    //for(int i = 0; i < data.size()+1; i++) printf("%x ", msg[i]);
    //putchar('\n');
}

void midiOut::stopMusic()
{
    for(int i = 0; i < 16; i++)
    {
        midiOutShortMsg(ghMidiOut, 0x78b0 + i);
    }
}

void midiOut::reset()
{
    midiOutReset(ghMidiOut);
}


void midiOut::printMidiDevices()
{
    MIDIOUTCAPS device;
    LPMIDIOUTCAPS p = &device;
    UINT num = midiOutGetNumDevs();
    for(unsigned i = 0; i < num; i++)
    {
        midiOutGetDevCaps(i, p, sizeof(MIDIOUTCAPS));
        cout << "#"<< i << " " << p->szPname << '\n'
        << "Mid: " << p->wMid << '\n'
        << "Pid: " << p->wPid << '\n'
        << "DriverVersion: " << p->vDriverVersion << '\n'
        << "Technology: " << p->wTechnology << '\n'
        << "Voices: " << p->wVoices << '\n'
        << "Notes: " << p->wNotes << '\n'
        << "ChannelMask: " << p->wChannelMask << '\n'
        << "Support: " << p->dwSupport << '\n' << endl;
    }
}

midiOut::midiOut()
{
    UINT id;
    MIDIOUTCAPS device;
    LPMIDIOUTCAPS p = &device;
    printMidiDevices();
   // cin >> id; getchar();
    MMRESULT mmres = midiOutOpen(&ghMidiOut, 0, 0, 0, CALLBACK_NULL);
    if (mmres != MMSYSERR_NOERROR)
        fprintf(stderr, "MIDI not avaliable.\n");
    midiOutGetID(ghMidiOut, &id);
    midiOutGetDevCaps(id, p, sizeof(MIDIOUTCAPS));
    cout << "Current MIDI output device: [" << id << "] " << p->szPname << '\n' << endl;
}

midiOut::~midiOut()
{
    midiOutClose(ghMidiOut);
}
