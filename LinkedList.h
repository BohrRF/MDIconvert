#ifndef LINKEDLIST_H_INCLUDED
#define LINKEDLIST_H_INCLUDED

#include <vector>
#include <iostream>
#include "Complex.h"

class Cpos
{
public:
    double x;
    double y;

    double norm() const
    {
        //std::cout << x << ' ' << y << "\n";
        return sqrt(x * x + y * y);
    }

    const Cpos& operator = (const Cpos& rval)
    {
        x = rval.x;
        y = rval.y;
        return rval;
    }

    const Cpos operator / (const int64_t& rval) const
    {
        Cpos temp;
        temp.x = this->x / rval;
        temp.y = this->y / rval;
        return temp;
    }

    const Cpos operator * (const int64_t& rval) const
    {
        Cpos temp;
        temp.x = this->x * rval;
        temp.y = this->y * rval;
        return temp;
    }

    const Cpos operator - (const Cpos& rval) const
    {
        Cpos temp;
        temp.x = this->x - rval.x;
        temp.y = this->y - rval.y;
        return temp;
    }

    Cpos(const double& posx = 0, const double& posy = 0)
    {
        x = posx;
        y = posy;
    }

    Cpos(const Cpos& pos)
    {
        x = pos.x;
        y = pos.y;
    }
};

class Cdata
{
public:
    int64_t timestamp;
    Cpos position;
    Cpos speed;

    Cdata()
    {
        timestamp = 0;
        position = { 0, 0 };
    }
    const Cdata& operator =(const Cdata& rval)
    {
        timestamp = rval.timestamp;
        position = rval.position;
        return rval;
    }
};


class node
{
public:
    Cdata data;//
    node* next;
    node* before;
    node() :next(nullptr), before(nullptr) {}
};


class Clist
{
    node *pointer; //pointing at the next node of current last node
    unsigned n_count;
    unsigned n_max;
public:
    node *first;
    node *last;

    Clist(const int& length)
    {
        n_count = 0;
        n_max = length;
        pointer = new node;
        pointer->next = pointer;
        pointer->before = pointer;
        first = last = pointer;

        auto temp_ptr = pointer;
        for (int i = 1; i < length; i++)
        {
            temp_ptr->next = new node;
            temp_ptr->next->next = pointer;
            temp_ptr->next->before = temp_ptr;
            temp_ptr = temp_ptr->next;
        }
        temp_ptr->next = pointer;
        pointer->before = temp_ptr;
    }
    ~Clist()
    {
        node *temp;
        while (pointer)
        {
            temp = pointer->next;
            delete pointer;
            pointer = temp;
        }
    }

    void clear();
    int readY(double data_ary[], const double &bias = 0) const;//
    int readY(Complex data_ary[], const double &bias = 0) const;
    int readY(std::vector<double> &data_ary) const;
    int readXY(std::pair<double, double> data_ary[], size_t n) const;
    int readSpeed(std::vector<std::pair<int64_t, Cpos>> &data_ary) const;
    int readSpeedAfter(std::vector<double> &data_ary, const int64_t &tm) const;
    int push(const int64_t &time, const double& posx, const double& posy);
    int count_node() const;
    const Cdata& history(const int &his = 0);
    friend class Fourier;
};

#endif // LINKEDLIST_H_INCLUDED
