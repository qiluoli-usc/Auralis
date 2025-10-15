#include "AuralisEditor.h"

#include "AuralisProcessor.h"
#include "Param/ParamIDs.h"

namespace
{
enum PresetMenuItems
{
    presetMenuNone = 0,
    presetMenuSave = 1,
    presetMenuLoad = 2
};
}

AuralisAudioProcessorEditor::AuralisAudioProcessorEditor(AuralisAudioProcessor& processor)
    : AudioProcessorEditor(&processor), processorRef(processor)
{
    setSize(640, 420);

    initialiseWaveformControl();

    addSliderControl(auralis::params::osc1DetuneCents, "Detune (cents)");
    addSliderControl(auralis::params::filterCutoffHz, "Cutoff (Hz)");
    addSliderControl(auralis::params::filterResonance, "Resonance");
    addSliderControl(auralis::params::envAttackMs, "Attack (ms)");
    addSliderControl(auralis::params::envDecayMs, "Decay (ms)");
    addSliderControl(auralis::params::envSustain, "Sustain");
    addSliderControl(auralis::params::envReleaseMs, "Release (ms)");
    addSliderControl(auralis::params::lfoRateHz, "LFO Rate (Hz)");
    addSliderControl(auralis::params::reverbMix, "Reverb Mix");

    initialisePresetMenu();
}

void AuralisAudioProcessorEditor::initialiseWaveformControl()
{
    waveformLabel.setText("Waveform", juce::dontSendNotification);
    waveformLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(waveformLabel);

    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Saw", 2);
    waveformBox.addItem("Square", 3);
    waveformBox.addItem("Triangle", 4);

    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getValueTreeState(), auralis::params::osc1Waveform, waveformBox);

    addAndMakeVisible(waveformBox);
}

void AuralisAudioProcessorEditor::initialisePresetMenu()
{
    presetLabel.setText("Preset", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    presetMenu.addItem("-- Select --", presetMenuNone);
    presetMenu.addItem("Save preset…", presetMenuSave);
    presetMenu.addItem("Load preset…", presetMenuLoad);
    presetMenu.onChange = [this]() { handlePresetSelection(presetMenu.getSelectedId()); };
    presetMenu.setSelectedId(presetMenuNone);

    addAndMakeVisible(presetMenu);
}

void AuralisAudioProcessorEditor::handlePresetSelection(int selectionID)
{
    presetMenu.setSelectedId(presetMenuNone, juce::dontSendNotification);

    juce::FileChooser chooser("Select preset file", juce::File(), "*.auralis");

    if (selectionID == presetMenuSave)
    {
        if (chooser.browseForFileToSave(true))
        {
            processorRef.saveStateToFile(chooser.getResult());
        }
    }
    else if (selectionID == presetMenuLoad)
    {
        if (chooser.browseForFileToOpen())
        {
            processorRef.loadStateFromFile(chooser.getResult());
        }
    }
}

AuralisAudioProcessorEditor::SliderControl& AuralisAudioProcessorEditor::addSliderControl(const juce::String& parameterID,
                                                                                        const juce::String& labelText)
{
    auto control = std::make_unique<SliderControl>();
    configureSlider(control->slider);
    control->label.setText(labelText, juce::dontSendNotification);
    control->label.setJustificationType(juce::Justification::centred);

    control->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getValueTreeState(), parameterID, control->slider);

    addAndMakeVisible(control->slider);
    addAndMakeVisible(control->label);

    sliderControls.push_back(std::move(control));
    return *sliderControls.back();
}

void AuralisAudioProcessorEditor::configureSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
}

void AuralisAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawFittedText("Auralis", getLocalBounds().removeFromTop(30), juce::Justification::centred, 1);
}

void AuralisAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    auto header = bounds.removeFromTop(60);
    auto presetArea = header.removeFromRight(220);

    presetLabel.setBounds(presetArea.removeFromLeft(80));
    presetMenu.setBounds(presetArea.reduced(10, 15));

    auto waveformArea = header.removeFromLeft(200);
    waveformLabel.setBounds(waveformArea.removeFromTop(20));
    waveformBox.setBounds(waveformArea.removeFromTop(30));

    const int columns = 4;
    const int rows = static_cast<int>((sliderControls.size() + columns - 1) / columns);
    const int sliderHeight = rows > 0 ? bounds.getHeight() / rows : 0;
    const int sliderWidth = columns > 0 ? bounds.getWidth() / columns : 0;

    for (size_t i = 0; i < sliderControls.size(); ++i)
    {
        auto row = static_cast<int>(i / columns);
        auto column = static_cast<int>(i % columns);

        juce::Rectangle<int> sliderArea(bounds.getX() + column * sliderWidth,
                                        bounds.getY() + row * sliderHeight,
                                        sliderWidth,
                                        sliderHeight);

        auto labelArea = sliderArea.removeFromTop(24);
        sliderControls[i]->label.setBounds(labelArea);
        sliderControls[i]->slider.setBounds(sliderArea.reduced(10));
    }
}
