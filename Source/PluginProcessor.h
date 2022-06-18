/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;
using namespace std;

//==============================================================================
/**
*/
class PingPongDelayAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    PingPongDelayAudioProcessor();
    ~PingPongDelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void updateFilter();

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void lpFilter(AudioBuffer<float>& buffer);

    float hard_clip(const float& sample, float thresh);

    void inputGainControl(AudioBuffer<float>& buffer);

    void outputGainControl(AudioBuffer<float>& buffer);

    void setRMSdisplay(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getRMSValue(const int channel) const;
    

    AudioProcessorValueTreeState    parameters;
    
private:

    // Variables
    LinearSmoothedValue<float>  rmslevelLeft, rmslevelRight;
    float                       startGain, finalGain, lastSampleRate{48000};
    int                         delayBufferSamples, delayBufferChannels, delayWritePosition;

    AudioSampleBuffer           delayBuffer;

    dsp::ProcessorDuplicator<dsp::IIR::Filter <float>, dsp::IIR::Coefficients <float>> lowPassFilter;

    // Functions
    AudioProcessorValueTreeState::ParameterLayout createParameters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PingPongDelayAudioProcessor)
};
