/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace std;

//==============================================================================
PingPongDelayAudioProcessorEditor::PingPongDelayAudioProcessorEditor (PingPongDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    rmsMeterLeft([&]()  { return audioProcessor.getRMSValue(0); }),
    rmsMeterRight([&]() { return audioProcessor.getRMSValue(1); })
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    buildElements();
    setSize(1200, 700);

    startTimerHz(24);
}

PingPongDelayAudioProcessorEditor::~PingPongDelayAudioProcessorEditor()
{
}

//==============================================================================
void PingPongDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    Image background = ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);
    g.drawImageAt(background, 0, 0);
    
    //Draw the semi-transparent rectangle around components
    const Rectangle<float> area(20, 80, 1160, 540);
    g.setColour(Colours::ghostwhite);
    g.drawRoundedRectangle(area, 5.0f, 3.0f);

    //Draw background for rectangle
    g.setColour(Colours::black);
    g.setOpacity(0.5f);
    g.fillRoundedRectangle(area, 5.0f);

    //Draw text labels for each component
    g.setColour(Colours::white);
    g.setFont(18.0f);

    // Input Gain
    g.drawText("Input Gain",    (getWidth() / 3) - 398, (getHeight() / 2) + 215,    200, 50, Justification::centred, false);

    // Delay Time and Mix and Feedback
    g.drawText("Delay Time",    (getWidth() / 3) - 208, (getHeight() / 2) - 175,    200, 50, Justification::centred, false);
    g.drawText("Mix",           (getWidth() / 3) - 208, (getHeight() / 2) + 70,     200, 50, Justification::centred, false);
    g.drawText("Feedback",      (getWidth() / 3) + 125, (getHeight() / 2) + 185,    200, 50, Justification::centred, false);

    // Distortion
    g.drawText("Distortion",     (getWidth() / 2) - 182, (getHeight() / 2) - 50,     200, 50, Justification::centred, false);

    // Low Pass
    g.drawText("Low Pass",      (getWidth() / 2) + 43,  (getHeight() / 2) - 50,     200, 50, Justification::centred, false);

    // Output Gain
    g.drawText("Output Gain",   getWidth() - 368, (getHeight() / 2) + 215, 200, 50, Justification::centred, false);  

    // L and R and RMS Level
    g.setFont(25.0f);
    g.drawText("L",             getWidth() - 252,       (getHeight() / 2) + 163,    200, 50, Justification::centred, false);
    g.drawText("R",             getWidth() - 213,       (getHeight() / 2) + 163,    200, 50, Justification::centred, false);
    g.drawText("RMS",           getWidth() - 231,       (getHeight() / 2) + 193,    200, 50, Justification::centred, false);
    g.drawText("Level",         getWidth() - 231,       (getHeight() / 2) + 218,    200, 50, Justification::centred, false);

    // Title of PlugIn
    g.setFont(35.0f);
    g.drawText("Ping-Pong Delay", 20, 0, 1160, 75, Justification::centred, false);

    // Hide or Display Knobs based on selection
    if (postDelayOptions.getSelectedId() == 0)
    {
        postDelayOptions.setTextWhenNothingSelected("Please select an effect to apply on delayed signal...");

        distortionSlider.setValue(1.0f);
        lowpassSlider.setValue(20000.0f);

        distortionSlider.setVisible(false);
        lowpassSlider.setVisible(false);
    }
    else if (postDelayOptions.getSelectedId() == 1)
    {
        lowpassSlider.setValue(20000.0f);

        distortionSlider.setVisible(true);
        lowpassSlider.setVisible(false);
    }
    else if (postDelayOptions.getSelectedId() == 2)
    {
        distortionSlider.setValue(1.0f);

        distortionSlider.setVisible(false);
        lowpassSlider.setVisible(true);
    }
    else
    {
        distortionSlider.setVisible(true);
        lowpassSlider.setVisible(true);
    }
}

void PingPongDelayAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any subcomponents in your editor.

    // Input Gain Slider
    inputGainSlider.setBounds   ((getWidth() / 3) - 400,    (getHeight() / 2) - 250,    200, 475);

    // Delay Time Knob and Mix Knob
    delayTimeKnob.setBounds     ((getWidth() / 3) - 200,    (getHeight() / 2) - 250,    185, 225);
    mixKnob.setBounds           ((getWidth() / 3) - 200,    (getHeight() / 2) - 10,     185, 225);
    feedbackSlider.setBounds    ((getWidth() / 3) + 50,     (getHeight() / 2) + 150,    350, 50);

    // List of Options
    postDelayOptions.setBounds  ((getWidth() / 2) - 175,    (getHeight() / 2) - 200,    400, 50);

    // Distortion Knob and Low Pass Knob
    distortionSlider.setBounds  ((getWidth() / 2) - 175,    (getHeight() / 2) - 125,    185, 225);
    lowpassSlider.setBounds     ((getWidth() / 2) + 50,     (getHeight() / 2) - 125,    185, 225);

    // Output Gain Slider
    outputGainSlider.setBounds  (getWidth() - 370,          (getHeight() / 2) - 250,    200, 475);

    rmsMeterLeft.setBounds      (getWidth() - 170,          (getHeight() / 2) - 225,    35, 400);
    rmsMeterRight.setBounds     (getWidth() - 130,          (getHeight() / 2) - 225,    35, 400);

}

void PingPongDelayAudioProcessorEditor::timerCallback()
{
}

void PingPongDelayAudioProcessorEditor::buildElements()
{
    // StringArray for Options
    StringArray choices; choices.insert(1, "Distortion"); choices.insert(2, "Low Pass"); choices.insert(3, "Distortion + Low Pass");

    addAndMakeVisible(rmsMeterLeft);
    addAndMakeVisible(rmsMeterRight);

    //Building the Input Gain
    inputGainVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "inGain", inputGainSlider);
    inputGainSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    inputGainSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    inputGainSlider.setRange(0.0f, 2.0f); addAndMakeVisible(&inputGainSlider);

    //Building the Delay Time Knob
    delayTimeVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "delayTime", delayTimeKnob);
    delayTimeKnob.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    delayTimeKnob.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    delayTimeKnob.setRange(0.0f, 4.0f); delayTimeKnob.setTextValueSuffix(" s"); addAndMakeVisible(&delayTimeKnob);

    //Building the Mix Knob
    mixVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "mix", mixKnob);
    mixKnob.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    mixKnob.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    mixKnob.setRange(0.0f, 1.0f); mixKnob.setTextValueSuffix(" %"); addAndMakeVisible(&mixKnob);

    //Building the Feedback Knob
    feedbackVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "feedback", feedbackSlider);
    feedbackSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    feedbackSlider.setTextBoxStyle(Slider::TextBoxAbove, false, 100, 20);
    feedbackSlider.setRange(0.0f, 1.0f); addAndMakeVisible(&feedbackSlider);

    //Building the ComboBox List
    pdOptVal = make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "post_delay_option", postDelayOptions);
    postDelayOptions.setEditableText(false); postDelayOptions.addItemList(choices, 1); addAndMakeVisible(&postDelayOptions);

    //Building the Distortion Slider
    distortionVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "distortion", distortionSlider);
    distortionSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    distortionSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    distortionSlider.setRange(0.01f, 1.0f); addChildComponent(&distortionSlider);

    //Building the Low Pass Slider
    lowpassVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "lowpass", lowpassSlider);
    lowpassSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    lowpassSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    lowpassSlider.setRange(1000.0f, 20000.0f); lowpassSlider.setTextValueSuffix(" Hz"); addChildComponent(&lowpassSlider);

    //Building the Output Gain
    outputGainVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "outGain", outputGainSlider);
    outputGainSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    outputGainSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    outputGainSlider.setRange(0.0f, 2.0f); addAndMakeVisible(&outputGainSlider);
}
