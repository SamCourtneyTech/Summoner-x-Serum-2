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
    void sendPromptToChatGPT(const juce::String& userPrompt,
        std::function<void(std::vector<std::map<std::string, std::string>>)> callback);
    void sendAIResponseToProcessor(const std::map<std::string, std::string>& aiResponse);
    void setCredits(int credits);

private:
    SummonerXSerum2AudioProcessor& processor;
    juce::TextEditor chatInput;
    juce::TextButton sendButton;
    juce::Label creditsLabel;
    bool requestInProgress = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChatBarComponent)
};