#pragma once
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

    tabs.addTab("ChatGPT", juce::Colours::transparentBlack, &chatBar, false);
    tabs.addTab("Serum", juce::Colours::transparentBlack, &audioProcessor.getSerumInterface(), false);
    tabs.addTab("Settings", juce::Colours::transparentBlack, &settings, false);
    addAndMakeVisible(tabs);
    tabs.setVisible(loggedIn);

    addAndMakeVisible(login);
    login.setVisible(!loggedIn);
    login.onLoginSuccess = [this]() {
        appProps.getUserSettings()->setValue("isLoggedIn", true);
        appProps.saveIfNeeded();
        login.setVisible(false);
        tabs.setVisible(true);
        };

    settings.onLogout = [this]() {
        handleLogout();
        };

    settings.onPathChanged = [this](const juce::String& newPath)
        {
            DBG("onPathChanged triggered with path: " << newPath);
            audioProcessor.setSerumPath(newPath);
        };

    audioProcessor.onPresetApplied = [this]()
        {
            tabs.setCurrentTabIndex(1);
        };

    auto initialPath = settings.loadSavedPath();
    settings.updatePathDisplay(initialPath);
    audioProcessor.setSerumPath(initialPath);
}


SummonerXSerum2AudioProcessorEditor::~SummonerXSerum2AudioProcessorEditor()
{
}

void SummonerXSerum2AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(30.0f));
    g.drawFittedText("SummonerXSerum2 (SC1)", getLocalBounds(), juce::Justification::centred, 1);
    if (isLoading)
    {
        loadingManager->showLoadingScreen(true);
        repaint();
    }
}

void SummonerXSerum2AudioProcessorEditor::resized()
{
    login.setBounds(getLocalBounds());
    tabs.setBounds(getLocalBounds());
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
    if (loadingManager)
        loadingManager->showLoadingScreen(show);
}

void SummonerXSerum2AudioProcessorEditor::handleLogout()
{
    appProps.getUserSettings()->setValue("isLoggedIn", false);
    appProps.saveIfNeeded();
    tabs.setVisible(false);
    login.setVisible(true);
}