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
    void showCreditsModal();
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    // Custom Credits Modal
    class CreditsModalWindow : public juce::Component
    {
    public:
        CreditsModalWindow();
        void paint(juce::Graphics& g) override;
        void resized() override;
        std::function<void()> onCloseClicked;
        std::function<void()> onPurchaseClicked;
        
    private:
        class PurchaseButtonLookAndFeel : public juce::LookAndFeel_V4
        {
        public:
            void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
            {
                auto font = juce::Font("Press Start 2P", 12.0f, juce::Font::plain);
                g.setFont(font);
                g.setColour(button.findColour(juce::TextButton::textColourOffId));
                g.drawFittedText(button.getButtonText(), button.getLocalBounds(),
                    juce::Justification::centred, 1);
            }
            
            void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                const juce::Colour& backgroundColour,
                bool isMouseOverButton, bool isButtonDown) override
            {
                auto bounds = button.getLocalBounds().toFloat();
                juce::Colour fillColour = isButtonDown ? backgroundColour.darker(0.3f)
                    : isMouseOverButton ? backgroundColour.brighter(0.2f)
                    : backgroundColour;
                g.setColour(fillColour);
                g.fillRect(bounds);
            }
        };
        
        class CloseButtonLookAndFeel : public juce::LookAndFeel_V4
        {
        public:
            void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
            {
                auto font = juce::Font("Press Start 2P", 12.0f, juce::Font::plain);
                g.setFont(font);
                g.setColour(button.findColour(juce::TextButton::textColourOffId));
                g.drawFittedText(button.getButtonText(), button.getLocalBounds(),
                    juce::Justification::centred, 1);
            }
            
            void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                const juce::Colour& backgroundColour,
                bool isMouseOverButton, bool isButtonDown) override
            {
                auto bounds = button.getLocalBounds().toFloat();
                juce::Colour fillColour = isButtonDown ? backgroundColour.darker(0.3f)
                    : isMouseOverButton ? backgroundColour.brighter(0.2f)
                    : backgroundColour;
                g.setColour(fillColour);
                g.fillRect(bounds);
            }
        };
        
        PurchaseButtonLookAndFeel purchaseButtonLookAndFeel;
        CloseButtonLookAndFeel closeButtonLookAndFeel;
        juce::TextButton closeButton;
        juce::Label titleLabel;
        juce::Label infoLabel;
        juce::TextButton purchaseButton;
    };
    
    std::unique_ptr<CreditsModalWindow> creditsModal;
    void sendPromptToGenerateParameters(const juce::String& userPrompt);
    void sendAIResponseToProcessor(const std::map<std::string, std::string>& aiResponse);
    int fetchUserCredits();  // Add declaration here

    SummonerXSerum2AudioProcessor& processor;
    juce::TextEditor chatInput;
    juce::TextButton sendButton;
    juce::Label creditsLabel;
    juce::ApplicationProperties appProps;
    bool requestInProgress = false;
    bool creditsLabelHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChatBarComponent)
};