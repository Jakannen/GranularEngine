/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KannenGranularEngineAudioProcessorEditor::KannenGranularEngineAudioProcessorEditor (KannenGranularEngineAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Grain Density
    addAndMakeVisible(grainDensitySlider);
    grainDensitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    grainDensitySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    grainDensitySlider.setRange(1.0, 100.0);
    grainDensitySlider.setValue(30.0);
    addAndMakeVisible(grainDensityLabel);
    grainDensityLabel.setText("Grain Density", juce::dontSendNotification);
    grainDensityLabel.setJustificationType(juce::Justification::centred);

    // Grain Size
    addAndMakeVisible(grainSizeSlider);
    grainSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    grainSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    grainSizeSlider.setRange(10.0, 500.0);
    grainSizeSlider.setValue(100.0);
    addAndMakeVisible(grainSizeLabel);
    grainSizeLabel.setText("Grain Size", juce::dontSendNotification);
    grainSizeLabel.setJustificationType(juce::Justification::centred);

    // Pitch Shift
    addAndMakeVisible(pitchShiftSlider);
    pitchShiftSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchShiftSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    pitchShiftSlider.setRange(-12.0, 12.0);
    pitchShiftSlider.setValue(0.0);
    addAndMakeVisible(pitchShiftLabel);
    pitchShiftLabel.setText("Pitch Shift", juce::dontSendNotification);
    pitchShiftLabel.setJustificationType(juce::Justification::centred);

    // Feedback
    addAndMakeVisible(feedbackSlider);
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    feedbackSlider.setRange(0.0, 0.95);
    feedbackSlider.setValue(0.5);
    addAndMakeVisible(feedbackLabel);
    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    feedbackLabel.setJustificationType(juce::Justification::centred);

    // Filter Cutoff
    addAndMakeVisible(filterCutoffSlider);
    filterCutoffSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filterCutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    filterCutoffSlider.setRange(100.0, 10000.0);
    filterCutoffSlider.setValue(5000.0);
    addAndMakeVisible(filterCutoffLabel);
    filterCutoffLabel.setText("Filter Cutoff", juce::dontSendNotification);
    filterCutoffLabel.setJustificationType(juce::Justification::centred);

    // Freeze Button
    addAndMakeVisible(freezeButton);
    freezeButton.setButtonText("Freeze");
    addAndMakeVisible(freezeLabel);
    freezeLabel.setText("Freeze Mode", juce::dontSendNotification);
    freezeLabel.setJustificationType(juce::Justification::centred);

    setSize(600, 400);
}

KannenGranularEngineAudioProcessorEditor::~KannenGranularEngineAudioProcessorEditor()
{
}

//==============================================================================
void KannenGranularEngineAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    // Plugin Title
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Granular Synthesis Plugin", getLocalBounds().removeFromTop(30), juce::Justification::centredTop, 1);

    // Draw the waveform below all controls
    int waveformHeight = 120;
    int waveformTop = getHeight() - waveformHeight - 20; // Keep a margin at the bottom
    g.setColour(juce::Colours::white);
}

void KannenGranularEngineAudioProcessorEditor::resized()
{
    // Layout constants
    int margin = 20;
    int controlWidth = 120;
    int controlHeight = 120;

    // Sliders Layout (Top Section)
    grainDensitySlider.setBounds(margin, 20, controlWidth, controlHeight);
    grainDensityLabel.setBounds(margin, 150, controlWidth, 20);

    grainSizeSlider.setBounds(2 * margin + controlWidth, 20, controlWidth, controlHeight);
    grainSizeLabel.setBounds(2 * margin + controlWidth, 150, controlWidth, 20);

    pitchShiftSlider.setBounds(3 * margin + 2 * controlWidth, 20, controlWidth, controlHeight);
    pitchShiftLabel.setBounds(3 * margin + 2 * controlWidth, 150, controlWidth, 20);

    feedbackSlider.setBounds(4 * margin + 3 * controlWidth, 20, controlWidth, controlHeight);
    feedbackLabel.setBounds(4 * margin + 3 * controlWidth, 150, controlWidth, 20);

    filterCutoffSlider.setBounds(margin, 180, controlWidth, controlHeight);
    filterCutoffLabel.setBounds(margin, 310, controlWidth, 20);

    freezeButton.setBounds(2 * margin + controlWidth, 200, controlWidth, 40);
    freezeLabel.setBounds(2 * margin + controlWidth, 250, controlWidth, 20);
}
