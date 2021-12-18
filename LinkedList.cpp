#include <cmath>
#include <cstdio>

#include "LinkedList.h"

void Clist::clear()
{
    first = pointer;
    last = first;
    n_count = 0;
}

int Clist::readY(double data_ary[], const double &bias) const
{
    auto temp_ptr = pointer->before;
    for(int i = n_count - 1; i >= 0; i--)
    {
        data_ary[i] = temp_ptr->data.position.y + bias;
        temp_ptr = temp_ptr->before;
    }
    return n_count;
}

int Clist::readY(Complex data_ary[], const double &bias) const
{
    auto temp_ptr = pointer->before;
    for(int i = n_count - 1; i >= 0; i--)
    {
        data_ary[i] = temp_ptr->data.position.y + bias;
        temp_ptr = temp_ptr->before;
    }
    return n_count;
}

int Clist::readY(std::vector<double> &data_ary) const
{
    auto temp_ptr = last;
    for (int i = n_count; i > 0; i--, temp_ptr = temp_ptr->before)
        data_ary.push_back(temp_ptr->data.position.y);

    return data_ary.size();
}

int Clist::readXY(std::pair<double, double> data_ary[], size_t n) const
{
    auto temp_ptr = pointer->before;
    n = (n >= n_count) ? n_count : n;
    for (int i = n - 1; i >= 0; i--)
    {
        data_ary[i] = std::make_pair( temp_ptr->data.position.x , temp_ptr->data.position.y );
        temp_ptr = temp_ptr->before;
    }
    return n;
}

int Clist::readXYAfter(std::vector<std::pair<int64_t, Cpos>> &data_ary, const int64_t &tm) const
{
    int i = 0;
    auto temp_ptr = last;

    // n_count-1 here because the speed in the first position is not usable
    for (i = n_count - 1; i > 0 && temp_ptr->data.timestamp >= tm; i--, temp_ptr = temp_ptr->before)
        data_ary.push_back(std::make_pair(temp_ptr->data.timestamp, temp_ptr->data.position));

    return data_ary.size();
}


int Clist::readSpeed(std::vector<std::pair<int64_t, Cpos>>& data_ary) const
{
    auto temp_ptr = last;

    // n_count-1 here because the speed in the first position is not usable
    for (int i = n_count - 1; i > 0; i--, temp_ptr = temp_ptr->before)
        data_ary.push_back(std::make_pair(temp_ptr->data.timestamp, temp_ptr->data.speed));

    return data_ary.size();
}


int Clist::readSpeedAfter(std::vector<double> &data_ary, const int64_t &tm) const
{
    int i = 0;
    auto temp_ptr = last;

    // n_count-1 here because the speed in the first position is not usable
    for (i = n_count-1; i > 0 && temp_ptr->data.timestamp >= tm; i--, temp_ptr = temp_ptr->before)
        data_ary.push_back(temp_ptr->data.speed.norm());

    return data_ary.size();
}


int Clist::push(const int64_t& time, const double& posx, const double& posy)
{
    Cpos temp(posx, posy);
    pointer->data.timestamp = time;
    pointer->data.position = { posx, posy };
    pointer->data.speed = (temp - last->data.position) / (time - last->data.timestamp) * 1000;
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
        n_count++;
    }
    return n_count;
}


int Clist::count_node() const
{
    return n_count;
}


const Cdata& Clist::history(const int& his)
{
    auto ptr = last;
    for (int i = his; i > 0; i--) ptr = ptr->before;
    return ptr->data;
}

