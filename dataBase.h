#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#include <vector>

class note
{
public:
    unsigned int channel = 0;
    unsigned int velocity = 0;
    unsigned int number = 0;
    unsigned int tickSpan = 0;
    bool isOffed = false;

    void operator = (const note &rval)
    {
        this->channel = rval.channel;
        this->number = rval.number;
        this->tickSpan = rval.tickSpan;
        this->velocity = rval.velocity;
    }
    bool operator == (const note &rval) const
    {
        return (this->channel == rval.channel && this->number == rval.number);
    }

    note(const note& n) : channel(n.channel), velocity(n.velocity), number(n.number), tickSpan(n.tickSpan), isOffed(n.isOffed) {}
    note() {}
};

class tickNode
{
public:
    unsigned int tickOffset = 0;

    std::vector<note> notes;
    std::vector<std::pair<unsigned, unsigned>> channelSound;
    std::vector<std::pair<unsigned, std::pair<unsigned, unsigned>>> channelBank;
    std::vector<std::vector<unsigned char>> sysMessage;
    std::vector<std::vector<unsigned>> specialEvents;
};

class beatSection
{
public:

    unsigned int tickBeatLength = 480;
    unsigned int timeSigniture_num = 4;
    unsigned int timeSigniture_den = 4;

    std::vector<tickNode> tickSet;

    inline void setTimeSigniture(unsigned int TPQN, const unsigned int &num, const unsigned int &den)
    {
        tickBeatLength = TPQN * (4.0 / den);
        timeSigniture_num = num;
        timeSigniture_den = den;
    }
};

class musicData
{
public:
    std::vector<beatSection> beatSet;
    unsigned int TPQN = 480;
};


#endif // DATABASE_H_INCLUDED
