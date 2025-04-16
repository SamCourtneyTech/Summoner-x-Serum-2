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

    // Setup Tabs
    tabs.addTab("ChatGPT", juce::Colours::transparentBlack, &chatBar, false);
    tabs.addTab("Serum", juce::Colours::transparentBlack, &audioProcessor.getSerumInterface(), false);
    tabs.addTab("Settings", juce::Colours::transparentBlack, &settings, false);
    addAndMakeVisible(tabs);
    tabs.setVisible(false); // Start hidden until login

    // Setup Login
    addAndMakeVisible(login);
    login.onLoginSuccess = [this]()
        {
            login.setVisible(false);
            tabs.setVisible(true);
        };

    // Settings path update
    settings.onPathChanged = [this](const juce::String& newPath)
        {
            DBG("onPathChanged triggered with path: " << newPath);
            audioProcessor.setSerumPath(newPath);
        };

    // Preset switch logic
    audioProcessor.onPresetApplied = [this]()
        {
            tabs.setCurrentTabIndex(1);
        };

    // Load saved path
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