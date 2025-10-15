#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <memory>
#include <vector>

class AuralisAudioProcessor;

class AuralisAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AuralisAudioProcessorEditor(AuralisAudioProcessor&);
    ~AuralisAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct SliderControl
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    AuralisAudioProcessor& processorRef;

    juce::ComboBox osc1WaveformBox;
    juce::ComboBox osc2WaveformBox;
    juce::Label osc1WaveformLabel;
    juce::Label osc2WaveformLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2WaveformAttachment;

    juce::ComboBox presetMenu;
    juce::Label presetLabel;

    std::vector<std::unique_ptr<SliderControl>> sliderControls;

    SliderControl& addSliderControl(const juce::String& parameterID, const juce::String& labelText);
    void configureSlider(juce::Slider& slider);
    void initialiseWaveformControls();
    void initialisePresetMenu();

    void handlePresetSelection(int selectionID);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuralisAudioProcessorEditor)
};
