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
   float density = *grainDensityParam;
   float blockTime = getBlockSize() / currentSampleRate;
   int grainsToAdd = static_cast<int>(density * blockTime + 0.5f);

   for (int i = 0; i < grainsToAdd; ++i)
   {
       Grain grain;
       grain.startChannel = juce::Random::getSystemRandom().nextInt(getTotalNumInputChannels());
       grain.position = juce::Random::getSystemRandom().nextFloat() * delayLine.getNumSamples();
       grain.duration = (*grainSizeParam / 1000.0f) * currentSampleRate;
       grain.pitch = pow(2.0f, *pitchShiftParam / 12.0f); // Semitones to ratio
       grain.playbackDirection = juce::Random::getSystemRandom().nextBool() ? 1 : -1;
       activeGrains.add(grain);
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

void KannenGranularEngineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // Update filter if needed
    if (*filterCutoffParam != lastFilterCutoff)
    {
        grainFilter.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, *filterCutoffParam));
        lastFilterCutoff = *filterCutoffParam;
    }

    // Write input to delay line with feedback
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        const float* input = buffer.getReadPointer(channel);
        float* delayData = delayLine.getWritePointer(channel);
        int writePos = delayLineWritePosition;

        for (int i = 0; i < numSamples; ++i)
        {
            float feedbackSample = delayData[(writePos + i) % delayLine.getNumSamples()];
            delayData[(writePos + i) % delayLine.getNumSamples()] = input[i] + feedbackSample * *feedbackParam;
        }
    }

    // Clear output buffer
    buffer.clear();

    // Process each sample
    for (int i = 0; i < numSamples; ++i)
    {
        // Process each grain
        for (int g = 0; g < activeGrains.size(); ++g)
        {
            auto& grain = activeGrains.getReference(g);
            if (grain.age >= grain.duration)
                continue;

            // Calculate envelope
            float envelope = 0.5f * (1.0f + cosf(juce::MathConstants<float>::pi * grain.age / grain.duration));

            // Read from delay line with interpolation
            int channel = grain.startChannel;
            const float* delayData = delayLine.getReadPointer(channel);
            int pos = static_cast<int>(grain.position);
            float frac = grain.position - pos;
            float sample1 = delayData[pos % delayLine.getNumSamples()];
            float sample2 = delayData[(pos + 1) % delayLine.getNumSamples()];
            float interpolated = sample1 + frac * (sample2 - sample1);

            // Apply envelope and pitch
            interpolated *= envelope;

            // Apply filter
            interpolated = grainFilter.processSingleSampleRaw(interpolated);

            // Add to output
            for (int outChan = 0; outChan < totalNumOutputChannels; ++outChan)
                buffer.addSample(outChan, i, interpolated * (outChan == channel ? 1.0f : 0.5f));

            // Update grain position and age
            grain.position += grain.pitch * grain.playbackDirection;
            grain.age += 1.0f;

            // Wrap position
            if (grain.position >= delayLine.getNumSamples())
                grain.position -= delayLine.getNumSamples();
            else if (grain.position < 0)
                grain.position += delayLine.getNumSamples();
        }
    }

    // Schedule new grains for next block
    scheduleGrains();

    // Remove expired grains
    for (int g = activeGrains.size(); --g >= 0;)
        if (activeGrains[g].age >= activeGrains[g].duration)
            activeGrains.remove(g);

    // Update delay line write position
    delayLineWritePosition = (delayLineWritePosition + numSamples) % delayLine.getNumSamples();
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
