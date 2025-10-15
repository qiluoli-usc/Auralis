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
    setSize(680, 520);

    initialiseWaveformControls();

    addSliderControl(auralis::params::osc1DetuneCents, "Osc 1 Detune (cents)");
    addSliderControl(auralis::params::osc2DetuneCents, "Osc 2 Detune (cents)");
    addSliderControl(auralis::params::oscMix, "Osc Mix");
    addSliderControl(auralis::params::filterCutoffHz, "Cutoff (Hz)");
    addSliderControl(auralis::params::filterResonance, "Resonance");
    addSliderControl(auralis::params::envAttackMs, "Attack (ms)");
    addSliderControl(auralis::params::envDecayMs, "Decay (ms)");
    addSliderControl(auralis::params::envSustain, "Sustain");
    addSliderControl(auralis::params::envReleaseMs, "Release (ms)");
    addSliderControl(auralis::params::lfoRateHz, "LFO Rate (Hz)");
    addSliderControl(auralis::params::lfoDepthHz, "LFO Depth (Hz)");
    addSliderControl(auralis::params::reverbMix, "Reverb Mix");

    initialisePresetMenu();
    initialisePromptControls();

    startTimerHz(15);
}

void AuralisAudioProcessorEditor::initialiseWaveformControls()
{
    auto configureWaveformBox = [](juce::ComboBox& box)
    {
        box.addItem("Sine", 1);
        box.addItem("Saw", 2);
        box.addItem("Square", 3);
        box.addItem("Triangle", 4);
    };

    osc1WaveformLabel.setText("Osc 1 Waveform", juce::dontSendNotification);
    osc1WaveformLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osc1WaveformLabel);

    configureWaveformBox(osc1WaveformBox);
    osc1WaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getValueTreeState(), auralis::params::osc1Waveform, osc1WaveformBox);
    addAndMakeVisible(osc1WaveformBox);

    osc2WaveformLabel.setText("Osc 2 Waveform", juce::dontSendNotification);
    osc2WaveformLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osc2WaveformLabel);

    configureWaveformBox(osc2WaveformBox);
    osc2WaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getValueTreeState(), auralis::params::osc2Waveform, osc2WaveformBox);
    addAndMakeVisible(osc2WaveformBox);
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

void AuralisAudioProcessorEditor::initialisePromptControls()
{
    promptLabel.setText("Prompt", juce::dontSendNotification);
    promptLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(promptLabel);

    promptInput.setMultiLine(true);
    promptInput.setReturnKeyStartsNewLine(true);
    promptInput.setScrollbarsShown(true);
    promptInput.setTextToShowWhenEmpty("Describe the sound you want…");
    addAndMakeVisible(promptInput);

    dryRunToggle.setButtonText("Dry run (preview only)");
    dryRunToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(dryRunToggle);

    applyPromptButton.setButtonText("Generate Patch");
    applyPromptButton.onClick = [this]()
    {
        patchPreview.setText("Queued prompt for mapping…", juce::dontSendNotification);
        processorRef.queuePrompt(promptInput.getText(), dryRunToggle.getToggleState());
    };
    addAndMakeVisible(applyPromptButton);

    previewLabel.setText("JSON Patch", juce::dontSendNotification);
    previewLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(previewLabel);

    patchPreview.setMultiLine(true);
    patchPreview.setReadOnly(true);
    patchPreview.setScrollbarsShown(true);
    patchPreview.setCaretVisible(false);
    patchPreview.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    patchPreview.setTextToShowWhenEmpty("Patch preview will appear here.");
    addAndMakeVisible(patchPreview);
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

    auto waveformArea = header.removeFromLeft(280);
    auto osc1Area = waveformArea.removeFromLeft(waveformArea.getWidth() / 2);
    auto osc2Area = waveformArea;

    osc1WaveformLabel.setBounds(osc1Area.removeFromTop(20));
    osc1WaveformBox.setBounds(osc1Area.removeFromTop(30));

    osc2WaveformLabel.setBounds(osc2Area.removeFromTop(20));
    osc2WaveformBox.setBounds(osc2Area.removeFromTop(30));

    auto promptArea = bounds.removeFromBottom(220);

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

    auto promptHeader = promptArea.removeFromTop(24);
    auto promptLabelArea = promptHeader.removeFromLeft(promptHeader.getWidth() / 2);
    promptLabel.setBounds(promptLabelArea);
    previewLabel.setBounds(promptHeader);

    auto promptControls = promptArea.removeFromTop(30);
    dryRunToggle.setBounds(promptControls.removeFromLeft(180));
    applyPromptButton.setBounds(promptControls.removeFromRight(160).reduced(0, 4));

    auto inputArea = promptArea.removeFromLeft(promptArea.getWidth() / 2);
    promptInput.setBounds(inputArea.reduced(5, 5));
    patchPreview.setBounds(promptArea.reduced(5, 5));
}

void AuralisAudioProcessorEditor::timerCallback()
{
    juce::String preview;
    if (processorRef.fetchLatestPreview(preview))
    {
        if (preview != lastPreviewText)
        {
            lastPreviewText = preview;
            patchPreview.setText(preview, juce::dontSendNotification);
        }
    }
}
