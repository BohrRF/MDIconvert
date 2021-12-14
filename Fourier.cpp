#include <iostream>
#include <cmath>
#include <utility>
#include <vector>
#include <algorithm>
#include <memory>
#include <numeric>

#include "Fourier.h"

void Fourier::bitrp (Complex x[], int n) const
{
    // Bit-reversal Permutation
    int a, b, p = 0;

    for (int i = 1; i < n; i *= 2) p++;
    for (int i = 0; i < n; i ++)
    {
        a = i;
        b = 0;
        for (int j = 0; j < p; j ++)
        {
            b = (b << 1) + (a & 1);    // b = b * 2 + a % 2;
            a >>= 1;        // a = a / 2;
        }
        if(b > i) swap (x[i], x[b]);
    }
}


void Fourier::FFT() const
{
    const int n = data_list.n_max;
    int bit = 0x1;
    char temp = 0;
    while(bit)
    {
       temp += ((bit & n) == 0) ? 0 : 1;
       bit <<= 1;
    }
    if(temp != 1) std::cerr << "%d is not a power of 2!" << n << std::endl;

    auto w = std::make_unique<Complex[]>(n >> 1);
    Complex t, u;
    double arg;
    int rot, index1, index2;

    for(int i = 0; i < n; i++)
    {
        x[i].r = 0;
        x[i].i = 0;
    }
    data_list.readY(x, -average);
    bitrp (x, n);

    // 计算 1 的前 n / 2 个 n 次方根的共轭复数 W'j = wreal [j] + i * wimag [j] , j = 0, 1, ... , n / 2 - 1
    arg = - 2 * PI / n;
    t.r = cos(arg);
    t.i = sin(arg);
    w[0].r = 1.0;
    w[0].i = 0.0;
    for (int j = 1; j < n / 2; j ++)
    {
        w[j].r = w[j - 1].r * t.r - w[j - 1].i * t.i;
        w[j].i = w[j - 1].r * t.i + w[j - 1].i * t.r;
    }

    for(int m = 2; m <= n; m *= 2)
    {
        for(int k = 0; k < n; k += m)
        {
            for(int j = 0; j < m / 2; j ++)
            {
                index1 = k + j;
                index2 = index1 + m / 2;
                rot = n * j / m;    // 旋转因子 w 的实部在 wreal [] 中的下标为 t
                t.r = w[rot].r * x[index2].r - w[rot].i * x[index2].i;
                t.i = w[rot].r * x[index2].i + w[rot].i * x[index2].r;
                u = x[index1];
                x[index1] = u + t;
                x[index2] = u - t;
            }
        }
    }
}

void Fourier::push(const int64_t& time, const double& posx, const double& posy)
{
    data_list.push(time, posx, posy);
    average = (average * (data_list.n_count - 1.0) + posy) / data_list.n_count;
}

void Fourier::reset()
{
    for (int i = 0; i < N; i++)
        x[i] = 0;
    data_list.clear();
    average = 0;
}


double Fourier::getAverage() const
{
    return average;
}

void Fourier::outSpec() const
{
    for(unsigned i = 0; i < data_list.n_max; i++)
        std::cout << i+1 << '\t' << x[i] << std::endl;
}

int Fourier::getListLength() const
{
    return data_list.n_count;
}

void Fourier::find_max_freq(std::vector<std::pair<double, double>> &spec_with_freq) const
{

    int64_t span = data_list.last->data.timestamp - data_list.first->data.timestamp;
    double sample_freq = ((double)data_list.n_count - 1.0) / ((double)span / 1000000.0);
    /*
    std::cout << "Saved Frame Number:" << data_list.n_count << '\n';
    std::cout << "Time Span: " << span << '\n';
    std::cout << "Sample Frequency:" << sample_freq << "\n\n";
    */
    for (int i = 1; i < N / 2; i++)
        spec_with_freq.push_back(std::make_pair((double)i * sample_freq / (double)N, x[i].norm()));

    std::sort(spec_with_freq.begin(), spec_with_freq.end(), [](const std::pair<double, double> &lval, const std::pair<double, double>& rval) {
                                                                return lval.second > rval.second;
                                                            });
}

std::unique_ptr<double[]> Fourier::getSpec() const
{
    auto res = std::make_unique<double[]>(data_list.n_max);
    for(unsigned i = 0; i < data_list.n_max; i++)
        res[i] = x[i].norm();
    return res;
}

const Cdata& Fourier::history(const int& his)
{
    return data_list.history(his);
}

double Fourier::getSpeedVariance(const int64_t& tm) const
{
    std::vector<double> data_ary;
    data_list.readSpeedAfter(data_ary, tm);
    /*
    auto max = *std::max_element(data_ary.begin(), data_ary.end());
    auto min = *std::min_element(data_ary.begin(), data_ary.end());
    const auto rate = 1 / (max - min);
    //transform into [0-1]
    std::transform(data_ary.begin(), data_ary.end(), data_ary.begin(), [&min, &rate](const auto& e) {return ( e - min ) * rate; });
    */
    //feel stupid to trans data into 0-1 before calculate variance
    double sum = 0;
    for (auto a : data_ary)
    {
        sum += a;
        //std::cout << a << '\n';
    }
    const auto ave = sum / data_ary.size();
    //std::cout << sum << '\n';
    //system("pause");
    const auto var = std::inner_product(data_ary.begin(), data_ary.end(), data_ary.begin(), 0.0) / data_ary.size() - ave * ave;

    return var;
}


double Fourier::calCurAccel(const int64_t &startTimeStamp, const Clist &list) const
{
    std::vector<std::pair<int64_t, Cpos>> speed_ary;
    list.readSpeed(speed_ary);
    if (speed_ary.empty())
        return 0;


    Cpos low_temp = (speed_ary.begin() + 1)->second;
    Cpos max_temp = 0;

    int64_t curTime;
    int64_t endTime;
    curTime = endTime = speed_ary.front().first;

    for (auto it = speed_ary.begin() + 1; it->first > startTimeStamp && it != speed_ary.end() - 1; it++)
    {
        if (it->second.norm() > max_temp.norm())
        {
            endTime = it->first;
            max_temp = it->second;
        }
    }
    double speed = (max_temp - low_temp).norm();
    return 1e6 * speed / (curTime - endTime);

}

double Fourier::calCurAccel(const int64_t& startTimeStamp) const
{
    std::vector<std::pair<int64_t, Cpos>> speed_ary;
    data_list.readSpeed(speed_ary);
    if (speed_ary.empty())
        return 0;


    Cpos low_temp = (speed_ary.begin() + 1)->second;
    Cpos max_temp = 0;

    int64_t curTime;
    int64_t endTime;
    curTime = endTime = speed_ary.front().first;

    for (auto it = speed_ary.begin() + 1; it->first > startTimeStamp && it != speed_ary.end() - 1; it++)
    {
        if (it->second.norm() > max_temp.norm())
        {
            endTime = it->first;
            max_temp = it->second;
        }
    }
    double speed = (max_temp.norm() - low_temp.norm());
    return 1e6 * speed / (curTime - endTime);

}

bool Fourier::freqAvalible()
{
    std::pair<double, double> data[N];
    data_list.readXY(data, N);
    return true;
}


double Fourier::calHightRatio() const
{
    std::vector<double> hight_list;
    data_list.readY(hight_list);
    double min, max;
    if (hight_list.empty())
        return 0;
    min = max = hight_list.front();
    for (auto hight : hight_list)
    {
        if (hight < min) min = hight;
        if (hight > max) max = hight;
    }
    return (hight_list.front() - min) / (max - min);
}
