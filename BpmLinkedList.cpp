#include "BpmLinkedList.h"
#include <iostream>
#include <conio.h>
using namespace std;

void BpmList::clear()
{
    first = pointer;
    last = first;
    n_count = 0;
}


int BpmList::push(const int64_t& time)
{
    if(time - lastBeatTimeStamp > 4 * (1e6 * 60 / (last->bpm + 1)))//keep BPM stable when beat come back
    {
        pointer->bpm = last->bpm;
    }
    else
    {
        pointer->bpm = 60 * 1000000.0 / (time - lastBeatTimeStamp);
    }
    pointer->time_stamp = time;
    lastBeatTimeStamp = time;
    pointer = pointer->next;
    if (n_count == n_max)
    {
        first = pointer;
        last = pointer->before;
    }
    else if (n_count > 0)
    {
        last = pointer->before;
        n_count++;
    }
    else
    {
        first = pointer->before;
        last = first;
        last->bpm = 0;
        n_count++;
    }
    return n_count;
}



double BpmList::calAverage(const int& length) const
{
    if(length < n_count)
    {
        auto ptr = last;
        double bpm = 0;
        for (int i = 0; i < length; i++)
        {
            bpm += ptr->bpm;
            ptr = ptr->before;
        }
        return bpm / length;
    }
    else
    {
        return 0;
    }
}

int BpmList::count_node() const
{
    return n_count;
}


bpmnode* BpmList::history(const int& his) const
{
    auto ptr = last;
    for (int i = his; i > 0; i--) ptr = ptr->before;
    return ptr;
}

void BpmList::output() const
{
    auto temp_ptr = last;

    // n_count-1 here because the speed in the first position is not usable
    for (int i = n_count - 1; i > 0; i--, temp_ptr = temp_ptr->before)
        cout << temp_ptr->bpm << endl;
}
