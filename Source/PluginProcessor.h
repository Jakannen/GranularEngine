/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class KannenGranularEngineAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    KannenGranularEngineAudioProcessor();
    ~KannenGranularEngineAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

    const juce::AudioBuffer<float>& getDelayBuffer() const { return delayLine; }

private:
    // Sample Rate and Buffer
    double currentSampleRate;
    juce::AudioBuffer<float> delayLine;
    int delayLineWritePosition = 0;

    struct Grain {
    float position = 0.0f;
    float age = 0.0f;
    float duration = 0.0f;
    float pitch = 1.0f;
    int playbackDirection = 1;
    int startChannel = 0;
};

    std::vector<Grain> activeGrains;

    // Grain Generation Functions
    void scheduleGrains();
    float generateEnvelope(const Grain& grain, int envelopeType);
    float applyFilter(float sample, juce::IIRFilter& filter);
    float applyStereoPan(float sample, float pan, bool isLeft);

    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* grainDensityParam = nullptr;
    std::atomic<float>* grainSizeParam = nullptr;
    std::atomic<float>* pitchShiftParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* freezeParam = nullptr;
    std::atomic<float>* filterCutoffParam = nullptr;

    // Freeze and Modulation
    bool freezeMode = false;
    float freezePosition = 0.0f;

    // LFO for Random Modulation
    struct LFO {
        float frequency, phase = 0.0f;
        float getNextSample(float sampleRate) {
            phase += juce::MathConstants<float>::twoPi * frequency / sampleRate;
            if (phase > juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
            return std::sin(phase);
        }
    } pitchLFO, panLFO;

    juce::IIRFilter grainFilter;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KannenGranularEngineAudioProcessor)
};
