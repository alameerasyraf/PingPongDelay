/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace std;

//==============================================================================
PingPongDelayAudioProcessor::PingPongDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                        ), parameters(*this, nullptr, "Parameter", createParameters()),
                           lowPassFilter(dsp::IIR::Coefficients<float>::makeLowPass(48000, 20000.0f, 0.8f))
#endif
{
}

PingPongDelayAudioProcessor::~PingPongDelayAudioProcessor()
{
}

//==============================================================================
const juce::String PingPongDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PingPongDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PingPongDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PingPongDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PingPongDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PingPongDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PingPongDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PingPongDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PingPongDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void PingPongDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PingPongDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Reset RMS Levels for new outgoing signal
    rmslevelLeft.reset(sampleRate, 0.5);
    rmslevelRight.reset(sampleRate, 0.5);

    rmslevelLeft.setCurrentAndTargetValue(-100.f);
    rmslevelRight.setCurrentAndTargetValue(-100.f);

    // Reset Delay Buffer information
    float maxDelayTime = parameters.getParameterRange("delayTime").end;
    delayBufferChannels = getTotalNumInputChannels();
    delayBufferSamples = (int)(maxDelayTime * (float)sampleRate) + 1;

    if (delayBufferSamples < 1) { delayBufferSamples = 1; }

    delayBuffer.setSize(delayBufferChannels, delayBufferSamples);
    delayBuffer.clear();
    delayWritePosition = 0;

    //Pre-processing for LOW PASS FILTER
    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    lowPassFilter.prepare(spec);
    lowPassFilter.reset();
}

void PingPongDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PingPongDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() || layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo())
    {
        if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()) { return true; }
        else { return false; }
    }
    else { return false; }

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PingPongDelayAudioProcessor::updateFilter()
{
    auto lowpassFreq = parameters.getParameterAsValue("lowpass");
    float currentCutOff = lowpassFreq.getValue();

    *lowPassFilter.state = *dsp::IIR::Coefficients<float>::makeLowPass(lastSampleRate, currentCutOff, 0.8f);
}

void PingPongDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //========= Variables ===================================//
    ScopedNoDenormals noDenormals;
    const int numInputChannels  = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples        = buffer.getNumSamples();

    auto dTime      = parameters.getRawParameterValue("delayTime");
    auto mix        = parameters.getRawParameterValue("mix");
    auto feedback   = parameters.getRawParameterValue("feedback");
    auto distort           =  parameters.getParameterAsValue("distortion");

    float currentDelayTime  = (dTime->load()) * (float)getSampleRate();
    float currentMix        = mix->load();
    float currentFeedback   = feedback->load();
    float currentThreshold  = distort.getValue();

    int localWritePosition = delayWritePosition;

    float* leftchannelData  = buffer.getWritePointer(0);
    float* rightchannelData = buffer.getWritePointer(1);
    float* leftdelayData    = delayBuffer.getWritePointer(0);
    float* rightdelayData   = delayBuffer.getWritePointer(1);

    //========== Processing =================================//

    // Gain control of input signal
    inputGainControl(buffer);

    // Perform DSP below
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Input samples for each channel
        const float leftsampleInput = leftchannelData[sample];
        const float rightsampleInput = rightchannelData[sample];

        // Output samples for each channel
        float leftsampleOutput = 0.0f;
        float rightsampleOutput = 0.0f;

        // Obtain the position to read and write from and to the buffer
        float readPosition = fmodf((float)localWritePosition - currentDelayTime + (float)delayBufferSamples, delayBufferSamples);
        int localReadPosition = floorf(readPosition);

        if (localReadPosition != localWritePosition)
        {
            //================================PROCESSING DELAY==========================================//
            float fraction = readPosition - (float)localReadPosition;

            float delayed1L = leftdelayData[(localReadPosition + 0)];
            float delayed1R = rightdelayData[(localReadPosition + 0)];

            float delayed2L = leftdelayData[(localReadPosition + 1) % delayBufferSamples];
            float delayed2R = rightdelayData[(localReadPosition + 1) % delayBufferSamples];

            leftsampleOutput = delayed1L + fraction * (delayed2L - delayed1L);
            rightsampleOutput = delayed1R + fraction * (delayed2R - delayed1R);

            //==========================PROCESSING DISTORTION============================================//
            float leftsampleDelayDistorted = hard_clip(leftsampleOutput - leftsampleInput, currentThreshold);
            float rightsampleDelayDistorted = hard_clip(rightsampleOutput - rightsampleInput, currentThreshold);

            //=========================MIX AND OUTPUT FOR CURRENT SAMPLE================================//
            leftchannelData[sample] = leftsampleInput + currentMix * leftsampleDelayDistorted;
            rightchannelData[sample] = rightsampleInput + currentMix * rightsampleDelayDistorted;

            leftdelayData[localWritePosition] = leftsampleInput + rightsampleOutput * currentFeedback;
            rightdelayData[localWritePosition] = rightsampleInput + leftsampleOutput * currentFeedback;
        }

        if (++localWritePosition >= delayBufferSamples) { localWritePosition -= delayBufferSamples; }

    }

    delayWritePosition = localWritePosition;

    lpFilter(buffer);

    // Gain control of output signal
    outputGainControl(buffer);

    // Calculate and display the RMS Meter
    setRMSdisplay(buffer);

    // This is here to avoid people getting screaming feedback when they first compile a plugin
    for (auto i = numInputChannels; i < numOutputChannels; ++i) { buffer.clear(i, 0, buffer.getNumSamples()); }
}

void PingPongDelayAudioProcessor::lpFilter(AudioBuffer<float>& inBuffer)
{
    dsp::AudioBlock <float> block(inBuffer);
    updateFilter();
    lowPassFilter.process(dsp::ProcessContextReplacing<float>(block));
}

float PingPongDelayAudioProcessor::hard_clip(const float& sample, float thresh)
{
    float val;

    if (thresh >= 0.01)
    {
        if (sample < -thresh)
            val = -thresh;
        else if (sample > thresh)
            val = thresh;
        else
            val = sample;
    }
    else { val = sample; }
    
    return val;
}

void PingPongDelayAudioProcessor::inputGainControl(AudioBuffer<float>& buffer)
{
    auto gain = parameters.getRawParameterValue("inGain");
    float gainValue = gain->load();
    if (gainValue == startGain)
    {
        buffer.applyGain(gainValue);
    }
    else
    {
        buffer.applyGainRamp(0, buffer.getNumSamples(), startGain, gainValue);
        startGain = gainValue;
    }
}

void PingPongDelayAudioProcessor::outputGainControl(AudioBuffer<float>& buffer)
{
    auto gain = parameters.getRawParameterValue("outGain");
    float gainValue = gain->load();
    if (gainValue == finalGain)
    {
        buffer.applyGain(gainValue);
    }
    else
    {
        buffer.applyGainRamp(0, buffer.getNumSamples(), finalGain, gainValue);
        finalGain = gainValue;
    }
}

void PingPongDelayAudioProcessor::setRMSdisplay(juce::AudioBuffer<float>& buffer)
{
    rmslevelLeft.skip(buffer.getNumSamples());
    rmslevelRight.skip(buffer.getNumSamples());

    {
        const auto leftlevelValue = Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));

        if (leftlevelValue < rmslevelLeft.getCurrentValue())
        {
            rmslevelLeft.setTargetValue(leftlevelValue);
        }
        else
        {
            rmslevelLeft.setCurrentAndTargetValue(leftlevelValue);
        }
    }

    {
        const auto rightlevelValue = Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, buffer.getNumSamples()));

        if (rightlevelValue < rmslevelLeft.getCurrentValue())
        {
            rmslevelRight.setTargetValue(rightlevelValue);
        }
        else
        {
            rmslevelRight.setCurrentAndTargetValue(rightlevelValue);
        }
    }
}

//==============================================================================
bool PingPongDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PingPongDelayAudioProcessor::createEditor()
{
    return new PingPongDelayAudioProcessorEditor (*this);
}

//==============================================================================
void PingPongDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PingPongDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType())) { parameters.replaceState(ValueTree::fromXml(*xmlState)); }
    }
}

float PingPongDelayAudioProcessor::getRMSValue(const int channel) const
{
    // Ensuring it is a stereo input
    jassert(channel == 0 || channel == 1);

    if (channel == 0)       { return rmslevelLeft.getCurrentValue(); }
    else if (channel == 1)  { return rmslevelRight.getCurrentValue(); }
    else                    { return 0.f; }
}

AudioProcessorValueTreeState::ParameterLayout PingPongDelayAudioProcessor::createParameters()
{
    // StringArray of Options
    StringArray choices;
    choices.insert(1, "Distortion"); choices.insert(2, "Low Pass"); choices.insert(3, "Distortion + Low Pass");

    // Parameter Vector
    vector<unique_ptr<RangedAudioParameter>> parameterVector;

    // Input Gain
    parameterVector.push_back(make_unique<AudioParameterFloat>("inGain",                "Input Gain",   0.0f, 2.0f, 1.0f));

    // Delay Time and Mix
    parameterVector.push_back(make_unique<AudioParameterFloat>("delayTime",             "Delay Time",   0.0f, 4.0f, 2.0f));
    parameterVector.push_back(make_unique<AudioParameterFloat>("mix",                   "Mix",          0.0f, 1.0f, 0.5f));
    parameterVector.push_back(make_unique<AudioParameterFloat>("feedback",              "Feedback",     0.0f, 0.9f, 0.5f));

    // Post Delay Options
    parameterVector.push_back(make_unique<AudioParameterChoice>("post_delay_option",    "Delay Options", choices, 0));

    // Distortion and Low Pass
    parameterVector.push_back(make_unique<AudioParameterFloat>("distortion",            "Distortion",   0.01f, 1.0f, 0.5f));
    parameterVector.push_back(make_unique<AudioParameterFloat>("lowpass",               "Low Pass",     1000.0f, 20000.0f, 5000.0f));

    // Output Gain
    parameterVector.push_back(make_unique<AudioParameterFloat>("outGain",               "Output Gain",  0.0f, 2.0f, 1.0f));

    return { parameterVector.begin(), parameterVector.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PingPongDelayAudioProcessor();
}
