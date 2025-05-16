#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class TIME12AudioProcessor;

class PaintToolWidget : public juce::Component, private juce::Timer {
public:
    PaintToolWidget(TIME12AudioProcessor& p);
    ~PaintToolWidget() override {}

    TextButton paintEditButton;
    TextButton paintNextButton;
    TextButton paintPrevButton;
    Label paintPageLabel;

    void toggleUIComponents();
    void timerCallback() override;
    void paint(Graphics& g) override;
    void resized() override;
    void drawPattern(Graphics& g, Rectangle<int> bounds, int index, Colour color);
    void mouseDown(const juce::MouseEvent& e) override;
    std::vector<Rectangle<int>> getPatRects();

private:
    TIME12AudioProcessor& audioProcessor;
};