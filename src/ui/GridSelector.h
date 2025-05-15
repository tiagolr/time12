#pragma once

#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Globals.h"

using namespace globals;

class TIME12AudioProcessor;

class GridSelector : public juce::SettableTooltipClient, public juce::Component, private juce::AudioProcessorValueTreeState::Listener {
public:
    GridSelector(TIME12AudioProcessor&);
    ~GridSelector() override;
    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

protected:
    TIME12AudioProcessor& audioProcessor;
};