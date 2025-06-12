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
    ChatLoginOverlay chatLoginOverlay;
    UIState currentUIState = UIState::FirstTime;
    
    // Welcome/Login Screen Components
    juce::Label welcomeTitle;
    juce::Label welcomeMessage;
    juce::Label loggedOutTitle;
    juce::Label loggedOutMessage;
    juce::TextButton welcomeLoginButton;
    juce::TextButton loggedOutLoginButton;

    void loadPluginFromSettings(const juce::String& path);
    void handleLogout();
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