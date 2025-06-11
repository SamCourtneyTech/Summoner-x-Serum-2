#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"  
#include <map>
#include <string>

class ChatBarComponent : public juce::Component
{
public:
    ChatBarComponent(SummonerXSerum2AudioProcessor& p);
    ~ChatBarComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void setCredits(int credits);

private:
    void sendPromptToGenerateParameters(const juce::String& userPrompt);
    void sendAIResponseToProcessor(const std::map<std::string, std::string>& aiResponse);
    int fetchUserCredits();  // Add declaration here

    SummonerXSerum2AudioProcessor& processor;
    juce::TextEditor chatInput;
    juce::TextButton sendButton;
    juce::Label creditsLabel;
    juce::ApplicationProperties appProps;
    bool requestInProgress = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChatBarComponent)
};