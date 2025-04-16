#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ChatBarComponent.h"
#include "SerumInterfaceComponent.h"
#include "SettingsComponent.h"
#include "LoadingComponent.h"
#include "LoginComponent.h"

class SummonerXSerum2AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    SummonerXSerum2AudioProcessorEditor(SummonerXSerum2AudioProcessor&);
    ~SummonerXSerum2AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void showLoadingScreen(bool show);

private:
    bool isLoading = false;
    SummonerXSerum2AudioProcessor& audioProcessor;
    juce::TabbedComponent tabs{ juce::TabbedButtonBar::TabsAtTop };
    ChatBarComponent chatBar;
    SettingsComponent settings;
    std::unique_ptr<LoadingScreenManager> loadingManager;

    LoginComponent login;
    juce::ApplicationProperties appProps;

    void loadPluginFromSettings(const juce::String& path);
    void handleLogout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SummonerXSerum2AudioProcessorEditor)
};
