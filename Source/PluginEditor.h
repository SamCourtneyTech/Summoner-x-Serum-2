#pragma once
#include <JuceHeader.h>
#include <atomic>
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
    class ChatLoginOverlay : public juce::Component, public juce::Timer
    {
    public:
        ChatLoginOverlay()
        {
            floatingBoxes.reserve(35); // Reserve space for up to 35 boxes
            startTimer(50); // 20 FPS for smooth animation
        }
        
        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black);
            
            // Draw mystical floating boxes
            for (const auto& box : floatingBoxes)
            {
                if (box.alpha > 0.0f)
                {
                    g.setColour(box.color.withAlpha(box.alpha));
                    g.fillRect(box.x, box.y, box.size, box.size);
                }
            }
        }
        
        void timerCallback() override
        {
            updateFloatingBoxes();
            
            // Randomly create new boxes
            if (random.nextFloat() < 0.12f && floatingBoxes.size() < 35) // 12% chance per frame, max 35 boxes
            {
                createRandomBox();
            }
            
            repaint();
        }
        
    private:
        // Mystical floating boxes effect
        struct FloatingBox
        {
            float x, y;
            float size;
            float alpha;
            float targetAlpha;
            float fadeSpeed;
            juce::Colour color;
            int lifeTime;
            int maxLifeTime;
        };
        
        std::vector<FloatingBox> floatingBoxes;
        std::array<int, 4> quadrantCounts = {0, 0, 0, 0}; // Track boxes per quadrant
        juce::Random random;
        
        void updateFloatingBoxes()
        {
            for (auto it = floatingBoxes.begin(); it != floatingBoxes.end();)
            {
                auto& box = *it;
                
                // Update lifetime
                box.lifeTime++;
                
                // Update alpha towards target
                float alphaDiff = box.targetAlpha - box.alpha;
                box.alpha += alphaDiff * box.fadeSpeed;
                
                // Change target alpha based on lifecycle
                if (box.lifeTime < box.maxLifeTime * 0.3f)
                {
                    // Fade in phase
                    box.targetAlpha = 0.6f;
                }
                else if (box.lifeTime > box.maxLifeTime * 0.7f)
                {
                    // Fade out phase
                    box.targetAlpha = 0.0f;
                }
                else
                {
                    // Stable phase with gentle pulsing
                    box.targetAlpha = 0.4f + 0.2f * std::sin(box.lifeTime * 0.1f);
                }
                
                // Remove boxes that have faded out and exceeded their lifetime
                if (box.alpha < 0.01f && box.lifeTime > box.maxLifeTime)
                {
                    // Update quadrant count before removing
                    int quadrant = getQuadrant(box.x, box.y);
                    quadrantCounts[quadrant]--;
                    it = floatingBoxes.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
        
        void createRandomBox()
        {
            FloatingBox box;
            
            // Find the least populated quadrant for balanced distribution
            int minQuadrant = 0;
            int minCount = quadrantCounts[0];
            for (int i = 1; i < 4; ++i)
            {
                if (quadrantCounts[i] < minCount)
                {
                    minCount = quadrantCounts[i];
                    minQuadrant = i;
                }
            }
            
            // Generate position in the chosen quadrant
            float halfWidth = getWidth() * 0.5f;
            float halfHeight = getHeight() * 0.5f;
            
            switch (minQuadrant)
            {
                case 0: // Top-left
                    box.x = random.nextFloat() * halfWidth;
                    box.y = random.nextFloat() * halfHeight;
                    break;
                case 1: // Top-right
                    box.x = halfWidth + random.nextFloat() * halfWidth;
                    box.y = random.nextFloat() * halfHeight;
                    break;
                case 2: // Bottom-left
                    box.x = random.nextFloat() * halfWidth;
                    box.y = halfHeight + random.nextFloat() * halfHeight;
                    break;
                case 3: // Bottom-right
                    box.x = halfWidth + random.nextFloat() * halfWidth;
                    box.y = halfHeight + random.nextFloat() * halfHeight;
                    break;
            }
            
            // Random size between 3-12 pixels
            box.size = 3.0f + random.nextFloat() * 9.0f;
            
            // Start invisible
            box.alpha = 0.0f;
            box.targetAlpha = 0.0f;
            
            // Random fade speed
            box.fadeSpeed = 0.02f + random.nextFloat() * 0.08f;
            
            // Random mystical colors (blues, purples, whites)
            float colorChoice = random.nextFloat();
            if (colorChoice < 0.4f)
            {
                // Blue tones
                box.color = juce::Colour::fromRGB(100 + random.nextInt(100), 150 + random.nextInt(100), 255);
            }
            else if (colorChoice < 0.7f)
            {
                // Purple tones
                box.color = juce::Colour::fromRGB(150 + random.nextInt(100), 100 + random.nextInt(100), 255);
            }
            else
            {
                // White/silver tones
                int brightness = 200 + random.nextInt(55);
                box.color = juce::Colour::fromRGB(brightness, brightness, brightness);
            }
            
            // Random lifetime (2-8 seconds at 20 FPS)
            box.maxLifeTime = 40 + random.nextInt(120); // 40-160 frames
            box.lifeTime = 0;
            
            // Update quadrant count
            int quadrant = getQuadrant(box.x, box.y);
            quadrantCounts[quadrant]++;
            
            floatingBoxes.push_back(box);
        }
        
        int getQuadrant(float x, float y)
        {
            float halfWidth = getWidth() * 0.5f;
            float halfHeight = getHeight() * 0.5f;
            
            if (x < halfWidth && y < halfHeight) return 0; // Top-left
            if (x >= halfWidth && y < halfHeight) return 1; // Top-right
            if (x < halfWidth && y >= halfHeight) return 2; // Bottom-left
            return 3; // Bottom-right
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
    std::atomic<bool> creditsFetchInProgress{false};
    
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