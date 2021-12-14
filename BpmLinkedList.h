#pragma once
#include <vector>
#include <iostream>
#include "Complex.h"


class bpmnode
{
public:
    double bpm = 0;
    int64_t time_stamp = 0;
    bpmnode* next = nullptr;
    bpmnode* before = nullptr;
    bpmnode(){}
};


class BpmList
{
    bpmnode* pointer; //pointing at the next node of current last node
    int n_count = 0;
    int n_max;
    int64_t lastBeatTimeStamp = 0;
public:
    bpmnode* first;
    bpmnode* last;

    BpmList(const int& length) : n_max(length)
    {
        pointer = new bpmnode;
        pointer->next = pointer;
        pointer->before = pointer;
        first = last = pointer;

        auto temp_ptr = pointer;
        for (int i = 1; i < length; i++)
        {
            temp_ptr->next = new bpmnode;
            temp_ptr->next->next = pointer;
            temp_ptr->next->before = temp_ptr;
            temp_ptr = temp_ptr->next;
        }
        temp_ptr->next = pointer;
        pointer->before = temp_ptr;
    }
    ~BpmList()
    {
        bpmnode* temp;
        while (pointer)
        {
            temp = pointer->next;
            delete pointer;
            pointer = temp;
        }
    }

    void clear();
    int push(const int64_t& time);
    double calAverage(const int& length = 1) const;
    int count_node() const;
    void output() const;
    bpmnode* history(const int& his = 0) const;
};
