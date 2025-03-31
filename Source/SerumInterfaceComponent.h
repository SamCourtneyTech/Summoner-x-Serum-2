#pragma once
#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>

class SerumInterfaceComponent : public juce::Component
{
public:
    SerumInterfaceComponent(juce::AudioProcessor& processor);
    ~SerumInterfaceComponent() override;

    void setPluginInstance(juce::AudioPluginInstance* newPlugin);
    void paint(juce::Graphics&) override;
    void resized() override;
    void loadSerum(const juce::File& pluginPath);
    void processMidiAndAudio(juce::AudioBuffer<float>& audioBuffer, juce::MidiBuffer& midiMessages, double sampleRate);
    juce::AudioPluginInstance* getSerumInstance() const { return serumInstance.get(); }
    std::unique_ptr<juce::AudioPluginInstance> serumInstance;
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void updateResponseCounter();

private:
    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioProcessorEditor> serumEditor;
    juce::AudioProcessor& parentProcessor;
    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const;
    juce::CriticalSection criticalSection;
    juce::TextButton nextButton{ "Next" };
    juce::TextButton prevButton{ "Previous" };
    juce::Label responseCounter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SerumInterfaceComponent)
};