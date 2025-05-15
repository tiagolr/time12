#pragma once

#include <JuceHeader.h>
#include <functional>

class TIME12AudioProcessor;

class SettingsButton : public juce::Component {
public:
    SettingsButton(TIME12AudioProcessor& p) : audioProcessor(p) {}
    ~SettingsButton() override {}

    void mouseDown(const juce::MouseEvent& e) override;
    void paint(Graphics& g) override;

    std::function<void()> onScaleChange;
    std::function<void()> toggleUIComponents;
    std::function<void()> toggleAbout;

private:
    TIME12AudioProcessor& audioProcessor;
};