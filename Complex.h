#ifndef COMPLEX_H_INCLUDED
#define COMPLEX_H_INCLUDED

#include <ostream>
#include <cmath>
#include <iomanip>
class Complex
{
public:
    double r;
    double i;

    Complex(const double &real = 0, const double &image = 0)
    {
        r = real;
        i = image;
    }

    Complex operator = (const Complex &rval)
    {
        this->r = rval.r;
        this->i = rval.i;
        return *this;
    }

    void operator = (const double &rval)
    {
        this->r = rval;
    }

    Complex operator + (const Complex &rval)
    {
        Complex temp;
        temp.r = this->r + rval.r;
        temp.i = this->i + rval.i;
        return temp;
    }

    Complex operator - (const Complex &rval)
    {
        Complex temp;
        temp.r = this->r - rval.r;
        temp.i = this->i - rval.i;
        return temp;
    }

    friend std::ostream& operator<<(std::ostream& os, const Complex &c)
    {
        return os << "(" << std::fixed << std::setprecision(2) << std::setw(5) << c.r << ", " << std::setw(5) << c.i << ")";
    }

    double norm() const
    {
        return sqrt(r*r + i*i);
    }
};


#endif // COMPLEX_H_INCLUDED
