#include "Delay.h"
#include <cmath>

void Delay::resize(int size_)
{
    buf.clear();
    buf.resize(size_, 0);
    size = (int)buf.size();
    curpos = 0;
    curval = 0;
}

void Delay::setSize(int sz)
{
    if (sz <= buf.size()) {
        size = std::max(sz, 1);
    }
}

void Delay::reserve(int samples)
{
    buf.reserve(samples);
}

void Delay::clear()
{
    std::fill(buf.begin(), buf.end(), 0.0);
}

void Delay::write(double s)
{
    buf[curpos] = s;
    curpos += 1;
    if (curpos == size)
        curpos = 0;
}

double Delay::read(double delay)
{
    if (delay >= size) delay -= size;
    double pos = (double)curpos - delay;
    if (pos < 0) {
        pos += (double)size;
        if (pos == size) { // avoids error with floating point arithmetics
            pos = 0;
        }
    }
    curval = buf[int(pos)];
    return curval;
}
/*
  Interpolated delay read
*/
double Delay::read3(double delay)
{
    if (delay >= size) delay -= size;
    double pos = (double)curpos - delay;
    if (pos < 0) {
        pos += (double)size;
        if (pos == size) pos = 0; // avoids error with floating point arithmetics
    }
    int i = (int)pos;
    double f = pos - i;
    double x0, x1, x2, x3, a0, a1, a2, a3;
    x0 = i == 0 ? buf[size - 1] : buf[(i - 1)];
    x1 = buf[i];
    x2 = i == size - 1 ? buf[0] : buf[i + 1];
    x3 = i >= size - 2 ? buf[i - size + 2] : buf[i + 2];
    a3 = f * f; a3 -= 1.0; a3 *= (1.0 / 6.0);
    a2 = (f + 1.0) * 0.5; a0 = a2 - 1.0;
    a1 = a3 * 3.0; a2 -= a1; a0 -= a3; a1 -= f;
    a0 *= f; a1 *= f; a2 *= f; a3 *= f; a1 += 1.0;
    curval = a0 * x0 + a1 * x1 + a2 * x2 + a3 * x3;
    return curval;
}