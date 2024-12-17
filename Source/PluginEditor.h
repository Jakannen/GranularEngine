/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class KannenGranularEngineAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    KannenGranularEngineAudioProcessorEditor (KannenGranularEngineAudioProcessor&);
    ~KannenGranularEngineAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    juce::Slider grainDensitySlider, grainSizeSlider, pitchShiftSlider, feedbackSlider, filterCutoffSlider;
    juce::ToggleButton freezeButton;
    juce::Label grainDensityLabel, grainSizeLabel, pitchShiftLabel, feedbackLabel, filterCutoffLabel, freezeLabel;


    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    KannenGranularEngineAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KannenGranularEngineAudioProcessorEditor)
};
