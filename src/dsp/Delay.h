#pragma once
#include <vector>

class Delay {
public:
	void resize(int size, bool clear);
	void write(double s);
	double read(double delay);
	double read3(double delay);
	void clear();
	void reserve(int samples);
	int size = 0;

private:
	std::vector<double> buf;
	int curpos = 0;
	double curval = 0.0;
};