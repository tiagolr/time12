#pragma once

#include <JuceHeader.h>
#include "GridSelector.h"
#include "../Globals.h"

using namespace globals;
class TIME12AudioProcessor;

class SequencerWidget : public juce::Component {
public:
    SequencerWidget(TIME12AudioProcessor& p);
    ~SequencerWidget() override {}
    void resized() override;

    std::unique_ptr<GridSelector> stepSelector;

    TextButton maxBtn;
    TextButton skewBtn;
    TextButton tenBtn;
    TextButton flipXBtn;

    TextButton silenceBtn;
    TextButton lpointBtn;
    TextButton rpointBtn;
    TextButton lineBtn;
    TextButton ptoolBtn;

    TextButton randomBtn;
    TextButton randomMenuBtn;
    Slider randomRange;
    TextButton clearBtn;

    TextButton applyBtn;
    TextButton resetBtn;

    TextButton linkStepBtn;

    double randomMin = 0.0;
    double randomMax = 1.0;

    void updateButtonsState();
    void paint(Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void drawChain(Graphics& g, Rectangle<int> bounds, Colour color, Colour background);

private:
    TIME12AudioProcessor& audioProcessor;
};