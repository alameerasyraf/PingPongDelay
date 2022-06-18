/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components/RMSMeter.h"

using namespace juce;
using namespace std;

//==============================================================================
/**
*/
class PingPongDelayAudioProcessorEditor  : public juce::AudioProcessorEditor, public Timer
{
public:
    PingPongDelayAudioProcessorEditor (PingPongDelayAudioProcessor&);
    ~PingPongDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void buildElements();

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> inputGainVal;        // Attachment for Input Gain

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> delayTimeVal;        // Attachment for Delay Time
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mixVal;              // Attachment for Delay Mix Value
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> feedbackVal;         // Attachment for Feedback Value

    unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> pdOptVal;          // Attachment for Post Delay Option Value

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> distortionVal;       // Attachment for Distortion Value
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> lowpassVal;          // Attachment for Low Pass Value

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> outputGainVal;       // Attachment for Output Gain

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PingPongDelayAudioProcessor& audioProcessor;

    Slider      inputGainSlider;        // Slider for Input Gain

    Slider      delayTimeKnob;          // Knob for Delay Time
    Slider      mixKnob;                // Knob for Delay Mix
    Slider      feedbackSlider;         // Slider for Feedback

    ComboBox    postDelayOptions;       // Effect options to Delayed Signal

    Slider      distortionSlider;       // Slider for Distortion
    Slider      lowpassSlider;          // Slider for Low Pass

    Slider      outputGainSlider;       // Slider for Output Gain

    GUI::VerticalRMSMeter rmsMeterLeft, rmsMeterRight;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PingPongDelayAudioProcessorEditor)
};
