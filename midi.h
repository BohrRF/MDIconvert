#pragma once
#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <vector>
using namespace std;
//#pragma comment(lib, "winmm.lib")

class midiOut
{
    HMIDIOUT ghMidiOut;

public:
    void noteOn(const unsigned &iNote, const unsigned &iVelocity, const unsigned &iChannel);
    void noteOff(const unsigned &iNote, const unsigned &iChannel);
    void stopMusic();
    void reset();

    void sysMsg(vector<BYTE> data);
    void programChange(const unsigned &iChannel, const unsigned &iProgram) const;
    void programChange(const unsigned &iChannel, const std::pair<int, int> &iProgram) const;
    void specialEvent(const vector<unsigned> &events);

    void printMidiDevices();
    midiOut();
    ~midiOut();
};
