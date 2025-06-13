#pragma once
#include <JuceHeader.h>
#include <array>

class SummonerXSerum2AudioProcessor;

class SettingsComponent : public juce::Component, public juce::Timer
{
public:
    void updatePathDisplay(const juce::String& newPath)
    {
        pathDisplay.setText(newPath, juce::dontSendNotification);
    }
    juce::String getPathDisplayText() const
    {
        return pathDisplay.getText();
    }
    explicit SettingsComponent(SummonerXSerum2AudioProcessor& processor);
    ~SettingsComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void resetSavedPath();
    std::function<void(const juce::String&)> onPathChanged;
    juce::String loadSavedPath();
    juce::String getPluginPath() const;

    std::function<void()> onLogout;
    std::function<void()> onLogin;
    
    void updateLoginState(bool isLoggedIn);
    void setCredits(int credits);
    int getCredits() const;
    void timerCallback() override;

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
    void updateFloatingBoxes();
    void createRandomBox();
    int getQuadrant(float x, float y);
    juce::Random random;
    juce::Label pathLabel;
    juce::Label pathDisplay;
    juce::TextButton browseButton;
    juce::TextButton resetButton;
    juce::ApplicationProperties applicationProperties;
    juce::String defaultPath;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String savedPath;
    void browseForPath();
    void savePath(const juce::String& path);
    juce::Label pluginPathLabel;
    juce::TextButton logoutButton;
    juce::Label creditsLabel;
    juce::TextButton purchaseCreditsButton;
    int currentCredits = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};