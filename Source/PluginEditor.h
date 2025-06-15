#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ChatBarComponent.h"
#include "SerumInterfaceComponent.h"
#include "SettingsComponent.h"
#include "LoadingComponent.h"
#include "LoginComponent.h"
#include "LoginState.h"

class SummonerXSerum2AudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    SummonerXSerum2AudioProcessorEditor(SummonerXSerum2AudioProcessor&);
    ~SummonerXSerum2AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void showLoadingScreen(bool show);
    void timerCallback() override;
    void refreshAccessToken();

private:
    // Custom TabbedComponent to handle tab changes
    class CustomTabbedComponent : public juce::TabbedComponent
    {
    public:
        CustomTabbedComponent(juce::TabbedButtonBar::Orientation orientation)
            : juce::TabbedComponent(orientation) {}
        
        std::function<void()> onTabChanged;
        
    protected:
        void currentTabChanged(int newCurrentTabIndex, const juce::String& newCurrentTabName) override
        {
            juce::TabbedComponent::currentTabChanged(newCurrentTabIndex, newCurrentTabName);
            if (onTabChanged)
                onTabChanged();
        }
    };

    bool isLoading = false;
    SummonerXSerum2AudioProcessor& audioProcessor;
    CustomTabbedComponent tabs{ juce::TabbedButtonBar::TabsAtTop };
    ChatBarComponent chatBar;
    SettingsComponent settings;
    std::unique_ptr<LoadingScreenManager> loadingManager;

    LoginComponent login;
    juce::ApplicationProperties appProps;
    
    // UI State Management
    enum class UIState {
        FirstTime,
        LoggedOut,
        LoggingIn,
        LoggedIn
    };
    
    // Chat-specific login overlay
    class ChatLoginOverlay : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black);
        }
    };
    
    // Custom LookAndFeel for login buttons
    class LoginButtonLookAndFeel : public juce::LookAndFeel_V4
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
            juce::Colour fillColour = isButtonDown ? juce::Colours::navy
                : isMouseOverButton ? juce::Colours::blue
                : backgroundColour;
            g.setColour(fillColour);
            g.fillRect(bounds);
        }
    };
    
    ChatLoginOverlay chatLoginOverlay;
    LoginButtonLookAndFeel customLoginButtonLookAndFeel;
    UIState currentUIState = UIState::FirstTime;
    UIState previousUIState = UIState::FirstTime;
    bool loginInitiatedFromSettings = false;
    
    // Welcome/Login Screen Components
    juce::Label welcomeTitle;
    juce::Label welcomeMessage;
    juce::Label loggedOutTitle;
    juce::Label loggedOutMessage;
    juce::TextButton welcomeLoginButton;
    juce::TextButton loggedOutLoginButton;

    void loadPluginFromSettings(const juce::String& path);
    void handleLogout();
    void handleLoginCancel();
    void fetchAndUpdateCredits(const juce::String& accessToken);
    void updateUIState();
    void setupWelcomeScreen();
    void setupLoggedOutScreen();
    void setupChatLoginOverlay();
    void startLoginProcess();
    bool isFirstTimeUser();
    void bringPluginToFront();
    void updateChatLoginOverlay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SummonerXSerum2AudioProcessorEditor)
};