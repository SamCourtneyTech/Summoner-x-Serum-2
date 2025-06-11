#include "SerumInterfaceComponent.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h" 

class SerumButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
        bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        auto font = juce::Font("Press Start 2P", 10.0f, juce::Font::plain);
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
        juce::Colour fillColour = isButtonDown ? juce::Colours::darkgrey
            : isMouseOverButton ? juce::Colours::lightgrey
            : backgroundColour;
        g.setColour(fillColour);
        g.fillRect(bounds);
    }
};

SerumInterfaceComponent::SerumInterfaceComponent(juce::AudioProcessor& processor)
    : parentProcessor(processor)
{
    static SerumButtonLookAndFeel customSerumButtons;
    formatManager.addDefaultFormats();

    if (formatManager.getNumFormats() > 0 && formatManager.getFormat(0) != nullptr)
    {
        DBG("Plugin format added: " << formatManager.getFormat(0)->getName());
    }
    else
    {
        DBG("No plugin formats available!");
    }

    juce::AudioProcessor::BusesLayout layout;
    if (!isBusesLayoutSupported(layout))
    {
        DBG("Unsupported bus layout");
        return;
    }
    addAndMakeVisible(nextButton);
    addAndMakeVisible(prevButton);
    addAndMakeVisible(responseCounter);
    nextButton.setButtonText("Next");
    nextButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    nextButton.setColour(juce::TextButton::textColourOnId, juce::Colours::whitesmoke);
    nextButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    nextButton.setLookAndFeel(&customSerumButtons);
    prevButton.setButtonText("Previous");
    prevButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    prevButton.setColour(juce::TextButton::textColourOnId, juce::Colours::whitesmoke);
    prevButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    prevButton.setLookAndFeel(&customSerumButtons);
    responseCounter.setJustificationType(juce::Justification::centred);
    responseCounter.setFont(juce::Font("Press Start 2P", 12.0f, juce::Font::plain));
    responseCounter.setColour(juce::Label::textColourId, juce::Colours::white);
    responseCounter.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    nextButton.onClick = [this]() {
        if (auto* proc = dynamic_cast<SummonerXSerum2AudioProcessor*>(&parentProcessor))
        {
            proc->nextResponse();
            updateResponseCounter();
        }
        };

    prevButton.onClick = [this]() {
        if (auto* proc = dynamic_cast<SummonerXSerum2AudioProcessor*>(&parentProcessor))
        {
            proc->previousResponse();
            updateResponseCounter();
        }
        };

    responseCounter.setJustificationType(juce::Justification::centred);
    responseCounter.setFont(juce::Font("Press Start 2P", 12.0f, juce::Font::plain));
    updateResponseCounter();
}

SerumInterfaceComponent::~SerumInterfaceComponent()
{
    const juce::ScopedLock lock(criticalSection);
    serumEditor = nullptr;
    serumInstance = nullptr;
}

void SerumInterfaceComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    if (!serumInstance)
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font("Press Start 2P", 12.0f, juce::Font::plain));
        g.drawText("Serum.vst3 not detected-Check the plugin path in the settings tab.",
            getLocalBounds(), juce::Justification::centred, true);
    }
}

void SerumInterfaceComponent::setPluginInstance(juce::AudioPluginInstance* newPlugin)
{
    if (!newPlugin)
    {
        DBG("setPluginInstance received a null pointer.");
        return;
    }

    if (serumInstance.get() == newPlugin)
    {
        DBG("setPluginInstance called, but instance is already set.");
        return;
    }
    serumInstance.reset(newPlugin);
    serumEditor.reset(serumInstance->createEditorIfNeeded());
    if (serumEditor)
    {
        DBG("Editor successfully created in SerumInterfaceComponent.");
        addAndMakeVisible(serumEditor.get());
        resized();
    }
    else
    {
        DBG("Failed to create editor in SerumInterfaceComponent.");
    }
}

void SerumInterfaceComponent::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (serumInstance != nullptr)
    {
        DBG("Preparing Serum with sample rate: " << sampleRate << " and block size: " << samplesPerBlock);
        serumInstance->prepareToPlay(sampleRate, samplesPerBlock);
    }
    else
    {
        DBG("Cannot prepare Serum. Instance is null.");
    }
}

void SerumInterfaceComponent::processMidiAndAudio(juce::AudioBuffer<float>& audioBuffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    //DBG("processMidiAndAudio called!");
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
            DBG("Note On for Serum: " << message.getNoteNumber());
        else if (message.isNoteOff())
            DBG("Note Off for Serum: " << message.getNoteNumber());
    }
    if (!midiMessages.isEmpty())
    {
        //DBG("MIDI buffer is not empty and forwarded correctly.");
    }
    else
    {
        //DBG("MIDI buffer is empty. No notes sent to Serum.");
    }
    if (serumInstance != nullptr)
    {
        juce::ScopedLock lock(criticalSection);
        //DBG("processMidiAndAudio called!");
        //DBG("Audio buffer before Serum: " << audioBuffer.getMagnitude(0, audioBuffer.getNumSamples()));
        if (!midiMessages.isEmpty())
        {
            //DBG("Forwarding MIDI to Serum: " << midiMessages.getNumEvents() << " events.");
        }
        else
        {
            //DBG("MIDI buffer is empty before forwarding to Serum.");
        }
        serumInstance->processBlock(audioBuffer, midiMessages);
        //DBG("Audio buffer after Serum: " << audioBuffer.getMagnitude(0, audioBuffer.getNumSamples()));

        if (audioBuffer.getMagnitude(0, audioBuffer.getNumSamples()) > 0.0f)
        {
            //DBG("Audio data present in buffer.");
        }
        else
        {
            //DBG("No audio generated by Serum.");
        }
        if (!midiMessages.isEmpty())
            DBG("MIDI forwarded to Serum!");
    }
    else
    {
        //DBG("Serum instance is null.");
        audioBuffer.clear();
    }
}

void SerumInterfaceComponent::loadSerum(const juce::File& pluginPath)
{
    if (serumInstance != nullptr)
    {
        serumEditor.reset();
        serumInstance.reset();
        DBG("Unloaded previous plugin instance.");
    }
    if (!pluginPath.exists())
    {
        DBG("Plugin path does not exist: " << pluginPath.getFullPathName());
        return;
    }
    if (pluginPath.isDirectory())
    {
        DBG("Plugin path is a directory: " << pluginPath.getFullPathName());
    }
    else if (pluginPath.existsAsFile())
    {
        DBG("Plugin path is a file: " << pluginPath.getFullPathName());
    }
    else
    {
        DBG("Plugin path is neither a valid file nor directory: " << pluginPath.getFullPathName());
        return;
    }
    juce::AudioProcessor::BusesLayout layout;
    if (!isBusesLayoutSupported(layout))
    {
        DBG("Unsupported bus layout");
        return;
    }
    DBG("Bus layout is supported, proceeding to load plugin.");
    double sampleRate = parentProcessor.getSampleRate();
    int blockSize = parentProcessor.getBlockSize();
    DBG("Sample Rate: " << sampleRate);
    DBG("Block Size: " << blockSize);
    auto* format = formatManager.getFormat(0);
    if (format == nullptr)
    {
        DBG("No plugin formats available!");
        return;
    }
    juce::KnownPluginList pluginList;
    juce::PluginDescription pluginDescription;
    juce::String errorMessage;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    if (!pluginList.scanAndAddFile(pluginPath.getFullPathName(),
        true,
        descriptions,
        *format))
    {
        DBG("Failed to scan and add plugin: " << pluginPath.getFullPathName());
        return;
    }
    if (descriptions.isEmpty())
    {
        DBG("No plugin descriptions found!");
        return;
    }
    pluginDescription = *descriptions.getFirst();
    if (!pluginDescription.fileOrIdentifier.isEmpty())
    {
        DBG("Plugin Name: " << pluginDescription.name);
        DBG("Manufacturer: " << pluginDescription.manufacturerName);
        DBG("File Path: " << pluginDescription.fileOrIdentifier);
        DBG("Plugin Format: " << pluginDescription.pluginFormatName);
        DBG("Version: " << pluginDescription.version);
        DBG("Unique ID: " << pluginDescription.uniqueId);
        DBG("Category: " << pluginDescription.category);
        DBG("Is Instrument: " << (pluginDescription.isInstrument ? "true" : "false"));
    }
    else
    {
        DBG("Plugin description is invalid.");
    }
    std::unique_ptr<juce::AudioPluginInstance> instance;
    instance = formatManager.createPluginInstance(pluginDescription, sampleRate, blockSize, errorMessage);
    if (instance == nullptr)
    {
        DBG("Error loading plugin: " << errorMessage);
        return;
    }
    //DBG("Plugin loaded successfully: " << pluginDescriptionantiago::replace("Name: ", instance->getName());
    DBG("Manufacturer: " << instance->getName());
    DBG("Plugin loaded successfully!");
    serumInstance = std::move(instance);
    DBG("Serum instance initialized!");
    serumEditor.reset(serumInstance->createEditorIfNeeded());
    if (serumEditor != nullptr)
    {
        DBG("Editor created successfully!");
        addAndMakeVisible(serumEditor.get());
        resized();
    }
    else
    {
        DBG("Failed to create plugin editor.");
    }
}

void SerumInterfaceComponent::resized()
{
    const juce::ScopedLock lock(criticalSection);
    auto bounds = getLocalBounds();
    if (serumEditor != nullptr)
    {
        serumEditor->setBounds(getLocalBounds());
    }
    const int buttonWidth = 100;
    const int buttonHeight = 30;
    const int counterWidth = 100;
    const int spacing = 10;
    const int margin = 10;
    int totalWidth = buttonWidth * 2 + counterWidth + spacing * 2;
    int controlsY = bounds.getHeight() - buttonHeight - margin;
    int controlsX = (bounds.getWidth() - totalWidth) / 2;
    prevButton.setBounds(controlsX, controlsY, buttonWidth, buttonHeight);
    responseCounter.setBounds(controlsX + buttonWidth + spacing, controlsY, counterWidth, buttonHeight);
    nextButton.setBounds(controlsX + buttonWidth + counterWidth + spacing * 2, controlsY, buttonWidth, buttonHeight);
}

void SerumInterfaceComponent::updateResponseCounter()
{
    if (auto* proc = dynamic_cast<SummonerXSerum2AudioProcessor*>(&parentProcessor))
    {
        int currentIndex = proc->getCurrentResponseIndex();
        int responseCount = proc->getResponseCount();
        if (responseCount <= 0)
        {
            responseCounter.setText("0/0", juce::dontSendNotification);
            DBG("No responses available, setting counter to 0/0");
            return;
        }
        if (currentIndex < 0 || currentIndex >= responseCount)
        {
            currentIndex = 0;
            DBG("Warning: Invalid currentIndex, resetting to 0");
        }
        juce::String text = juce::String(currentIndex + 1) + "/" + juce::String(responseCount);
        responseCounter.setText(text, juce::dontSendNotification);
        DBG("Response counter updated: " << text);
    }
    else
    {
        responseCounter.setText("0/0", juce::dontSendNotification);
        DBG("Warning: Failed to cast processor to SummonerXSerum2AudioProcessor!");
    }
}

bool SerumInterfaceComponent::isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput = layouts.getMainInputChannelSet();
    if (!mainInput.isDisabled() && mainInput != mainOutput)
        return false;
    if (mainOutput.size() > 2)
        return false;
    return true;
}