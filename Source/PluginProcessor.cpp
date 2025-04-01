#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <string>
#include <juce_core/juce_core.h>
#include <regex>
#include "ParameterNormalizer.h"

SummonerXSerum2AudioProcessor::SummonerXSerum2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    settingsComponent(*this),
    serumInterface(*this)
{
}

SummonerXSerum2AudioProcessor::~SummonerXSerum2AudioProcessor()
{
}

const juce::String SummonerXSerum2AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SummonerXSerum2AudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SummonerXSerum2AudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SummonerXSerum2AudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SummonerXSerum2AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SummonerXSerum2AudioProcessor::getNumPrograms()
{
    return 1;
}

int SummonerXSerum2AudioProcessor::getCurrentProgram()
{
    return 0;
}

void SummonerXSerum2AudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SummonerXSerum2AudioProcessor::getProgramName(int index)
{
    return {};
}

void SummonerXSerum2AudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void SummonerXSerum2AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    DBG("prepareToPlay called: SampleRate = " << sampleRate << ", BlockSize = " << samplesPerBlock);
    if (serumInterface.getSerumInstance() == nullptr)
    {
        serumInterface.loadSerum(juce::File(serumPluginPath));
    }
    if (serumInterface.getSerumInstance() != nullptr)
    {
        DBG("Serum loaded successfully from stored path!");
        serumInterface.prepareToPlay(sampleRate, samplesPerBlock);
    }
    else
    {
        DBG("Failed to load Serum from stored path.");
    }
    enumerateParameters();
}

void SummonerXSerum2AudioProcessor::enumerateParameters()
{
    auto* serum = getSerumInstance();
    if (!serum)
    {
        DBG("Serum instance not available for parameter enumeration.");
        return;
    }

    // Clear the existing parameter map
    parameterMap.clear();

    // Get the list of parameters using the non-deprecated method
    const auto& parameters = serum->getParameters();
    int paramIndex = 0;
    for (auto* param : parameters)
    {
        if (param != nullptr)
        {
            juce::String paramName = param->getName(128); // 128 is a reasonable max length for parameter names
            parameterMap[paramName.toStdString()] = paramIndex;
            DBG("Parameter [" << paramIndex << "] " << paramName);
        }
        paramIndex++;
    }
    DBG("Enumerated " << parameters.size() << " parameters from Serum.");
}

void SummonerXSerum2AudioProcessor::setParameterByName(const std::pair<std::string, float>& paramData)
{
    const std::string& paramName = paramData.first;
    float value = paramData.second;
    auto* serum = getSerumInstance();
    if (!serum)
    {
        DBG("Serum instance not available for setting parameter.");
        return;
    }

    auto it = parameterMap.find(paramName);
    if (it != parameterMap.end())
    {
        int paramIndex = it->second;
        const auto& parameters = serum->getParameters();
        if (paramIndex >= 0 && paramIndex < parameters.size())
        {
            auto* param = parameters[paramIndex];
            if (param != nullptr)
            {
                param->setValueNotifyingHost(std::clamp(value, 0.0f, 1.0f));
                DBG("Set " << paramName << " to " << value);
            }
            else
            {
                DBG("Parameter at index " << paramIndex << " is null.");
            }
        }
        else
        {
            DBG("Invalid parameter index: " << paramIndex);
        }
    }
    else
    {
        DBG("Parameter " << paramName << " not found in parameter map.");
    }
}

float SummonerXSerum2AudioProcessor::parseValue(const std::string& value)
{
    std::regex numberPattern(R"([-+]?\d*\.?\d+)");
    std::smatch match;
    if (std::regex_search(value, match, numberPattern))
    {
        float numericValue = std::stof(match.str());
        if (value.find("%") != std::string::npos)
            return numericValue / 100.0f;
        return numericValue;
    }
    return 0.0f;
}

void SummonerXSerum2AudioProcessor::applyPresetToSerum(const std::map<std::string, std::string>& ChatResponse)
{
    auto* serum = getSerumInstance();
    if (!serum) return;
    for (const auto& param : ChatResponse)
    {
        const std::string& paramName = param.first;
        const std::string& textValue = param.second;
        setParameterByName(normalizeValue(paramName, textValue));
    }
    if (onPresetApplied)
    {
        onPresetApplied();
    }
}

void SummonerXSerum2AudioProcessor::setSerumPath(const juce::String& newPath)
{
    if (newPath != serumPluginPath)
    {
        DBG("Updating Serum Path: " << newPath);
        serumPluginPath = newPath;
        serumInterface.loadSerum(juce::File(serumPluginPath));
        serumInterface.prepareToPlay(getSampleRate(), getBlockSize());
        enumerateParameters(); // Add this to refresh parameter list
    }
}

void SummonerXSerum2AudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SummonerXSerum2AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    DBG("Checking bus layouts...");
    DBG("Main Output Channels: " << layouts.getMainOutputChannelSet().getDescription());
    DBG("Main Input Channels: " << layouts.getMainInputChannelSet().getDescription());

#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    DBG("Plugin is a MIDI Effect.");
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    {
        DBG("Unsupported output layout.");
        return false;
    }
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    {
        DBG("Input and output layouts do not match.");
        return false;
    }
#endif
    DBG("Bus layout supported.");
    return true;
#endif
}
#endif

juce::AudioPluginInstance* SummonerXSerum2AudioProcessor::getSerumInstance()
{
    auto* instance = serumInterface.getSerumInstance();
    DBG("getSerumInstance: " << (instance ? instance->getName() : "nullptr"));
    return instance;
}

void SummonerXSerum2AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    serumInterface.processMidiAndAudio(buffer, midiMessages, getSampleRate());
}

bool SummonerXSerum2AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SummonerXSerum2AudioProcessor::createEditor()
{
    return new SummonerXSerum2AudioProcessorEditor(*this);
}

void SummonerXSerum2AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void SummonerXSerum2AudioProcessor::setResponses(const std::vector<std::map<std::string, std::string>>& newResponses)
{
    juce::ScopedLock lock(responseLock);
    responses = newResponses;
    currentResponseIndex = 0;
    if (!responses.empty())
        applyPresetToSerum(responses[currentResponseIndex]);
}

void SummonerXSerum2AudioProcessor::applyResponseAtIndex(int index)
{
    juce::ScopedLock lock(responseLock);
    if (index >= 0 && index < responses.size())
    {
        currentResponseIndex = index;
        applyPresetToSerum(responses[currentResponseIndex]);
    }
}

void SummonerXSerum2AudioProcessor::nextResponse()
{
    juce::ScopedLock lock(responseLock);
    if (currentResponseIndex < responses.size() - 1)
    {
        currentResponseIndex++;
        applyPresetToSerum(responses[currentResponseIndex]);
    }
}

void SummonerXSerum2AudioProcessor::previousResponse()
{
    juce::ScopedLock lock(responseLock);
    if (currentResponseIndex > 0)
    {
        currentResponseIndex--;
        applyPresetToSerum(responses[currentResponseIndex]);
    }
}

void SummonerXSerum2AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SummonerXSerum2AudioProcessor();
}

void SummonerXSerum2AudioProcessor::listSerumParameters()
{
    auto* serum = getSerumInstance();
    if (serum == nullptr)
    {
        DBG("Serum instance not loaded.");
        return;
    }

    const auto& parameters = serum->getParameters();
    DBG("Listing Serum Parameters (" << parameters.size() << " total):");
    int paramIndex = 0;
    for (auto* param : parameters)
    {
        if (param != nullptr)
        {
            juce::String paramName = param->getName(128);
            float paramValue = param->getValue();
            DBG("[" << paramIndex << "] " << paramName << " = " << paramValue);
        }
        paramIndex++;
    }
}