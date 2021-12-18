#pragma once

#include <vector>
#include <fstream>

#include "Leap.h"
#include "Fourier.h"
#include "midi.h"
#include "playControl.h"

using namespace Leap;
using std::vector;
using std::pair;
using std::fstream;
static const int nScreenWidth = 120;
static const int nScreenHeight = 40;

class beatAnalyze
{
public:
    Fourier fft;
    control con;
    int64_t lastPeakTimeStamp;
    int64_t lastBeatTimeStamp;
    int beats = 0;
    int64_t startTimePoint = 0;
    bool isLowest = false;
    bool isHighest = false;  
    node ptr;
    double hand_peak;
    fstream fp;
    Clist fingerPosList;

    void showFrameInfo(const Controller& controller);
    beatAnalyze() : fft(N), fingerPosList(N)
    {
        fp.open("soft.txt", std::ios::out);
        if(!fp)
        {
            std::cerr << "Log file could not be open." << std::endl;
            exit(0);
        }
    }
    ~beatAnalyze()
    {
        fp.close();
    }
};

class SampleListener : public Listener, public beatAnalyze {
public:
    virtual void onInit(const Controller&);
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onExit(const Controller&);
    virtual void onFrame(const Controller&);
    virtual void onFocusGained(const Controller&);
    virtual void onFocusLost(const Controller&);
    virtual void onDeviceChange(const Controller&);
    virtual void onServiceConnect(const Controller&);
    virtual void onServiceDisconnect(const Controller&);
    /*
    void refreshGraph(const double &x, const double &y);
    
    SampleListener(wchar_t* _screen, HANDLE _hConsole)
    {
        screen = _screen;
        hConsole = _hConsole;
    }
private:
    wchar_t* screen;
    HANDLE hConsole;
    DWORD dwBytesWritten = 0;
    double originX = 0;
    double originY = 200;
    */
};