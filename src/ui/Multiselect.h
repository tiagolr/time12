/*
  ==============================================================================

    Multiselect.h
    Author:  tiagolr

  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include <iostream>
#include "../Globals.h"

using namespace globals;
class TIME12AudioProcessor;

struct SelPoint {
    uint64_t id;
    double x; // 0..1 in relation to viewport
    double y;
    double areax; // 0..1 in relation to selection area
    double areay;
};

enum MouseHover {
    area,
    topLeft,
    topMid,
    topRight,
    midLeft,
    midRight,
    bottomLeft,
    bottomMid,
    bottomRight
};

struct Vec2 {
    double x, y;

    Vec2() : x(0), y(0) {}
    Vec2(double x_, double y_) : x(x_), y(y_) {}
    Point<int> toPoint() { return Point<int>((int)x , (int)y); }

    Vec2 operator*(double scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
};

using Quad = std::array<Vec2, 4>;

inline Vec2 bilinearInterpolate(const Quad& quad, double u, double v) {
    Vec2 A = quad[0] * (1 - u) + quad[1] * u;  // Top edge interpolation
    Vec2 B = quad[2] * (1 - u) + quad[3] * u;  // Bottom edge interpolation
    Vec2 P = A * (1 - v) + B * v;      // Final vertical interpolation
    return P;
}

class Multiselect
{
public:
	Multiselect(TIME12AudioProcessor& p) : audioProcessor(p) {}
	~Multiselect() {}

    void setViewBounds(int _x, int _y, int _w, int _h);

	void mouseDown(const MouseEvent& e);
    void mouseMove(const MouseEvent& e);
    void mouseDrag(const MouseEvent& e);
    void mouseUp(const MouseEvent& e);
	void drawBackground(Graphics& g);
	void draw(Graphics& g);
    void drawHandles(Graphics& g);

    void recalcSelectionArea();
    void clearSelection();
    void makeSelection(const MouseEvent& e, Point<int>selectionStart, Point<int>selectionEnd);
    void deleteSelectedPoints();
    void selectAll();

    int mouseHover = -1; // flag for hovering selection drag handles, 0 area, 1 top left corner, 2 top center etc..
    std::vector<SelPoint> selectionPoints;

private:
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;
    // select quad coordinates
    Quad quad = { Vec2(0.0,0.0),Vec2(1.0,0.0),Vec2(0.0,1.0),Vec2(1.0,1.0) };
    // select quad relative coordinates to selection area
    Quad quadrel = { Vec2(0.0, 0.0), Vec2(0.0, 0.0), Vec2(0.0, 0.0), Vec2(0.0, 0.0) };
    bool invertx = false;
    bool inverty = false;

    void dragArea(const MouseEvent& e);
    void dragQuad(const MouseEvent& e);
    void updatePointsToSelection();

    Quad getQuadExpanded(double expand = 0.0);
    void calcRelativeQuadCoords(Rectangle<double> area);
    void applyRelativeQuadCoords(Rectangle<double> area);

    Rectangle<double> selectionAreaStart = Rectangle<double>(); // used to drag or scale selection area
    Quad selectionQuadStart = {Vec2(0.0,0.0), Vec2(1.0, 0.0), Vec2(0.0, 1.0), Vec2(1.0,1.0)};
    TIME12AudioProcessor& audioProcessor;

    bool isSnapping(const MouseEvent& e);
    Vec2 pointToVec(Point<double> p);
    Rectangle<double> quadToRect(Quad q);
    bool isCollinear(const std::vector<SelPoint>& p, bool xaxis);
};