#ifndef PLAYCONTROL_H_INCLUDED
#define PLAYCONTROL_H_INCLUDED

#include <string>
#include <list>
#include <iostream>
#include "dataBase.h"
#include "midi.h"
#include "Fourier.h"
#include "bpmLinkedList.h"


const int BPM_LEN = 12;
const double LOW_HAND_AMP = 10;

class onPlayNote
{
public:
    note onNote;
    int64_t endTimeStamp = 0;
    onPlayNote() {}
    onPlayNote(const note &n, const int64_t & ts) : onNote(n), endTimeStamp(ts) {}
};

class control
{
    musicData music;
    std::vector<tickNode>::iterator node_ptr;
    std::vector<beatSection>::iterator beat_ptr;

    std::list<onPlayNote> onPlayList;

    unsigned int timetrans(const unsigned int& len);
    void write_beat(beatSection &beat, const std::vector<int> &notes);

    simpleSound playSound;

    BpmList bpmList;

public:
    bool autoplayMode = true;
    bool musicLoop = true;
    bool beatRecieved = false;
    bool speedFix = false;
    unsigned int gearFactor = 1;

    unsigned char playState = 0;
    double timeOffset = 0;

    int curBeatPos = 1;
    double handBpm = 0;
    double curAccel = 1;
    double curBpm = 120;
    double curSpanFactor = 1;
    double curVelocityFactor = 1;
    double dif = 0;
    double speedBias = 0;

    int64_t curNodeTimeStamp = 0;
    int64_t lastBeatTimeStamp = 0;
    int64_t nextBeatTimeStamp = 0;
    int64_t pauseTimeStamp = 0;

    void initial_music();
    int readMIDI(std::string FILENAME);

    void printMusic() const;
    void outParameter() const;
    void readOnPlayList(unsigned char ary[]) const;

    void pause(const int64_t &curTimeStamp);
    void resume(const int64_t &curTimeStamp);
    void restartMusic(const int64_t &curTimeStamp);
    void setplayState(const int64_t& curTimeStamp, const int& mode);
    void resetBeat();
    void resetBpmList();
    void resetAll();

    void onBeat(const int64_t& TimeStamp, const double& hand_amp, const double& hand_accel, const Fourier& fft);
    int refresh(const int64_t& TimeStamp, const Fourier& fft);

    control() : bpmList(BPM_LEN){}
};

#endif // PLAYCONTROL_H_INCLUDED
