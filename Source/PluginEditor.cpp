#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "LoadingComponent.h"

SummonerXSerum2AudioProcessorEditor::SummonerXSerum2AudioProcessorEditor(SummonerXSerum2AudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    settings(p),
    chatBar(p),
    loadingManager(std::make_unique<LoadingScreenManager>(this))
{
    setName("SummonerXSerum2AudioProcessorEditor");
    setSize(1192, 815);

    juce::PropertiesFile::Options options;
    options.applicationName = "SummonerXSerum2";
    options.filenameSuffix = ".settings";
    options.folderName = "SummonerXSerum2App";
    options.osxLibrarySubFolder = "Application Support";
    appProps.setStorageParameters(options);

    bool loggedIn = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
    juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
    int credits = appProps.getUserSettings()->getIntValue("credits", 0);

    tabs.addTab("ChatGPT", juce::Colours::transparentBlack, &chatBar, false);
    tabs.addTab("Serum", juce::Colours::transparentBlack, &audioProcessor.getSerumInterface(), false);
    tabs.addTab("Settings", juce::Colours::transparentBlack, &settings, false);
    addAndMakeVisible(tabs);
    tabs.setVisible(loggedIn);
    DBG("Initial visibility - loggedIn: " + juce::String(loggedIn ? "true" : "false") + ", tabs visible: " + juce::String(tabs.isVisible() ? "true" : "false"));

    addAndMakeVisible(login);
    login.setVisible(!loggedIn);
    DBG("Setting onLoginSuccess callback for LoginComponent");
    login.onLoginSuccess = [this](juce::String token, int credits) {
        DBG("onLoginSuccess called with Token=" + token + ", Credits=" + juce::String(credits));
        appProps.getUserSettings()->setValue("isLoggedIn", true);
        appProps.getUserSettings()->setValue("accessToken", token);
        appProps.getUserSettings()->setValue("credits", credits);
        appProps.saveIfNeeded();
        login.setVisible(false);
        tabs.setVisible(true);
        chatBar.setCredits(credits);
        DBG("After login - login visible: " + juce::String(login.isVisible() ? "true" : "false") + ", tabs visible: " + juce::String(tabs.isVisible() ? "true" : "false"));
        repaint();
        resized(); // Ensure layout updates
        };

    settings.onLogout = [this]() {
        handleLogout();
        };

    settings.onPathChanged = [this](const juce::String& newPath) {
        DBG("onPathChanged triggered with path: " + newPath);
        audioProcessor.setSerumPath(newPath);
        };

    audioProcessor.onPresetApplied = [this]() {
        tabs.setCurrentTabIndex(1);
        };

    auto initialPath = settings.loadSavedPath();
    settings.updatePathDisplay(initialPath);
    audioProcessor.setSerumPath(initialPath);

    if (loggedIn && credits > 0) {
        chatBar.setCredits(credits);
    }
}

SummonerXSerum2AudioProcessorEditor::~SummonerXSerum2AudioProcessorEditor()
{
}

void SummonerXSerum2AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    if (!tabs.isVisible())
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(30.0f));
        g.drawFittedText("SummonerXSerum2 (SC1)", getLocalBounds(), juce::Justification::centred, 1);
    }
    if (isLoading)
    {
        DBG("Painting with isLoading=true, calling showLoadingScreen");
        loadingManager->showLoadingScreen(true);
        repaint();
    }
}

void SummonerXSerum2AudioProcessorEditor::resized()
{
    login.setBounds(getLocalBounds());
    tabs.setBounds(getLocalBounds());
    DBG("resized() called - login bounds: " + login.getBounds().toString() + ", tabs bounds: " + tabs.getBounds().toString());
}

void SummonerXSerum2AudioProcessorEditor::loadPluginFromSettings(const juce::String& path)
{
    juce::File pluginFile(path);
    if (pluginFile.existsAsFile())
    {
        audioProcessor.setSerumPath(path);
        DBG("Loaded plugin from: " + path);
    }
    else
    {
        DBG("Invalid plugin path: " + path);
    }
}

void SummonerXSerum2AudioProcessorEditor::showLoadingScreen(bool show)
{
    DBG("showLoadingScreen called with show=" + juce::String(show ? "true" : "false"));
    isLoading = show;
    if (loadingManager)
    {
        loadingManager->showLoadingScreen(show);
    }
    repaint();
}

void SummonerXSerum2AudioProcessorEditor::handleLogout()
{
    appProps.getUserSettings()->setValue("isLoggedIn", false);
    appProps.getUserSettings()->setValue("accessToken", "");
    appProps.getUserSettings()->setValue("credits", 0);
    appProps.saveIfNeeded();

    chatBar.setCredits(0);
    tabs.setVisible(false);
    login.setVisible(true);
    DBG("After logout - login visible: " + juce::String(login.isVisible() ? "true" : "false") + ", tabs visible: " + juce::String(tabs.isVisible() ? "true" : "false"));
    repaint();
    resized();
}