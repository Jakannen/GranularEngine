/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KannenGranularEngineAudioProcessor::KannenGranularEngineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), parameters(*this, nullptr, "PARAMETERS",
                           {
                               std::make_unique<juce::AudioParameterFloat>("grainDensity", "Grain Density", 1.0f, 100.0f, 30.0f),
                               std::make_unique<juce::AudioParameterFloat>("grainSize", "Grain Size", 10.0f, 500.0f, 100.0f),
                               std::make_unique<juce::AudioParameterFloat>("pitchShift", "Pitch Shift", -12.0f, 12.0f, 0.0f),
                               std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", 0.0f, 0.95f, 0.5f),
                               std::make_unique<juce::AudioParameterBool>("freeze", "Freeze", false),
                               std::make_unique<juce::AudioParameterFloat>("filterCutoff", "Filter Cutoff", 100.0f, 10000.0f, 5000.0f)
                           })
#endif
{
    grainDensityParam = parameters.getRawParameterValue("grainDensity");
    grainSizeParam = parameters.getRawParameterValue("grainSize");
    pitchShiftParam = parameters.getRawParameterValue("pitchShift");
    feedbackParam = parameters.getRawParameterValue("feedback");
    freezeParam = parameters.getRawParameterValue("freeze");
    filterCutoffParam = parameters.getRawParameterValue("filterCutoff");
}

KannenGranularEngineAudioProcessor::~KannenGranularEngineAudioProcessor()
{
}

//==============================================================================
const juce::String KannenGranularEngineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KannenGranularEngineAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KannenGranularEngineAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KannenGranularEngineAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KannenGranularEngineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KannenGranularEngineAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int KannenGranularEngineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KannenGranularEngineAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String KannenGranularEngineAudioProcessor::getProgramName (int index)
{
    return {};
}

void KannenGranularEngineAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void KannenGranularEngineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    delayLine.setSize(2, (int)(sampleRate * 2)); // 2 seconds max
    grainFilter.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, *filterCutoffParam));

    pitchLFO.frequency = 0.5f; // Example LFO rates
    panLFO.frequency = 0.3f;
}

void KannenGranularEngineAudioProcessor::releaseResources()
{
    delayLine.clear();
}

void KannenGranularEngineAudioProcessor::scheduleGrains()
{
    activeGrains.clear();

    // Reduce the grain density to 10 for testing (can increase later)
    int grainCount = 100; // Reduce density for testing

    // Schedule grains at different positions in the delay line
    for (int i = 0; i < grainCount; ++i)
    {
        Grain grain;

        // Position grain randomly within the delay line
        grain.position = juce::Random::getSystemRandom().nextFloat() * delayLine.getNumSamples();

        // Convert grain size from milliseconds to samples
        grain.duration = *grainSizeParam / 100.0f * currentSampleRate;  // Grain size in samples

        // Assign random pitch and playback direction
        grain.pitch = juce::Random::getSystemRandom().nextFloat() * 0.5f + 0.75f;  // Slight pitch variation
        grain.playbackDirection = (juce::Random::getSystemRandom().nextFloat() > 0.5f) ? 1 : -1;  // Random direction

        // Add the grain to the active grains list
        activeGrains.push_back(grain);
    }
}

float KannenGranularEngineAudioProcessor::generateEnvelope(const Grain& grain, int envelopeType)
{
    float position = grain.position / grain.duration;
    switch (envelopeType)
    {
    case 0: return std::max(0.0f, 1.0f - position); // Linear Decay
    case 1: return 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * position)); // Raised Cosine
    default: return 1.0f; // Flat
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KannenGranularEngineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void KannenGranularEngineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto numSamples = buffer.getNumSamples();

    // Get the feedback value from the parameter
    float feedback = juce::jlimit(0.0f, 0.1f, feedbackParam->load()); // Limit feedback to a reasonable value

    // Ensure delay line is large enough for input buffer size
    if (delayLine.getNumSamples() < numSamples)
        delayLine.setSize(totalNumInputChannels, numSamples * 2);  // Double the buffer size for safety

    // Process each channel of the input
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* inputChannelData = buffer.getReadPointer(channel);  // Input audio
        auto* outputChannelData = buffer.getWritePointer(channel); // Output audio
        auto* delayData = delayLine.getWritePointer(channel);      // Delay line storage for grains

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Apply feedback: store previous output to the delay line
            delayData[delayLineWritePosition] = inputChannelData[sample] + delayData[delayLineWritePosition] * feedback;

            float outputSample = 0.0f;

            // Process all active grains (add random pitch shifting, envelope)
            for (auto& grain : activeGrains)
            {
                int grainIndex = static_cast<int>(grain.position) % delayLine.getNumSamples();

                // Apply a smooth raised cosine envelope to each grain for smooth fade-in/fade-out
                float envelope = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * grain.position / grain.duration));
                float grainSample = delayData[grainIndex] * envelope;

                // Apply pitch shifting (change grain sample based on pitch)
                grainSample *= 0.2f;  // Scale down the grain sample to prevent overloading

                // Mix the grain sample into the output
                outputSample += grainSample;

                // Move grain position (pitch shifting)
                grain.position += grain.pitch * grain.playbackDirection;

                // Ensure grain position wraps within delay line bounds
                if (grain.position >= delayLine.getNumSamples())
                    grain.position -= delayLine.getNumSamples();
                if (grain.position < 0)
                    grain.position += delayLine.getNumSamples();  // Ensure position stays within bounds
            }

            // Write the processed sample to the output buffer
            outputChannelData[sample] = juce::jlimit(-1.0f, 1.0f, outputSample);  // Ensure the output is within [-1, 1]

            // Update delay line write position
            delayLineWritePosition = (delayLineWritePosition + 1) % delayLine.getNumSamples();
        }
    }

    // Continuously schedule new grains for each block of audio
    scheduleGrains();
}

float KannenGranularEngineAudioProcessor::applyStereoPan(float sample, float pan, bool isLeft)
{
    return isLeft ? sample * std::sqrt(1.0f - pan) : sample * std::sqrt(pan);
}

float KannenGranularEngineAudioProcessor::applyFilter(float sample, juce::IIRFilter& filter)
{
    return filter.processSingleSampleRaw(sample);
}

//==============================================================================
bool KannenGranularEngineAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* KannenGranularEngineAudioProcessor::createEditor()
{
    return new KannenGranularEngineAudioProcessorEditor (*this);
}

//==============================================================================
void KannenGranularEngineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void KannenGranularEngineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KannenGranularEngineAudioProcessor();
}
