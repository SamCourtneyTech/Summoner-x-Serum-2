#include "ChatBarComponent.h"
#include <sstream>
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <string>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "PluginProcessor.h" 
#include "PluginEditor.h"

class ChatBarButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
        bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        auto font = juce::Font("Press Start 2P", 15.0f, juce::Font::plain);
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
        juce::Colour fillColour = isButtonDown ? juce::Colours::darkblue
            : isMouseOverButton ? juce::Colours::blue
            : backgroundColour;
        g.setColour(fillColour);
        g.fillRect(bounds);
    }
};

ChatBarComponent::ChatBarComponent(SummonerXSerum2AudioProcessor& p) : processor(p)
{
    static ChatBarButtonLookAndFeel customSummonButton;

    // Initialize appProps for accessing the access token
    juce::PropertiesFile::Options options;
    options.applicationName = "SummonerXSerum2";
    options.filenameSuffix = ".settings";
    options.folderName = "SummonerXSerum2App";
    options.osxLibrarySubFolder = "Application Support";
    appProps.setStorageParameters(options);

    addAndMakeVisible(chatInput);
    addAndMakeVisible(sendButton);
    chatInput.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    chatInput.setColour(juce::TextEditor::outlineColourId, juce::Colours::dimgrey);
    chatInput.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::whitesmoke);
    chatInput.setFont(juce::Font("Press Start 2P", 12.0f, juce::Font::plain));
    chatInput.setBorder(juce::BorderSize<int>(2));
    sendButton.setButtonText("Summon");
    sendButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    sendButton.setColour(juce::TextButton::textColourOnId, juce::Colours::whitesmoke);
    sendButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    sendButton.setLookAndFeel(&customSummonButton);

    addAndMakeVisible(creditsLabel);
    creditsLabel.setText("Credits: 0", juce::dontSendNotification);
    creditsLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    sendButton.onClick = [this]() {
        if (requestInProgress)
        {
            DBG("Request already in progress, ignoring new request");
            return;
        }
        juce::String userInput = chatInput.getText();
        if (userInput.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Input Error",
                "Please enter a prompt before summoning.");
            return;
        }

        if (auto* editor = dynamic_cast<SummonerXSerum2AudioProcessorEditor*>(getParentComponent()->getParentComponent()))
        {
            editor->showLoadingScreen(true);
        }

        sendPromptToGenerateParameters(userInput);
        };
}

ChatBarComponent::~ChatBarComponent()
{
    sendButton.setLookAndFeel(nullptr);
}

void ChatBarComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void ChatBarComponent::resized()
{
    auto area = getLocalBounds();
    auto chatBarHeight = 25;
    auto chatBarWidth = 600;
    auto buttonWidth = 100;
    auto yPosition = (getHeight() - chatBarHeight) / 2;
    chatInput.setBounds((getWidth() - chatBarWidth - buttonWidth - 10) / 2, yPosition + -50, chatBarWidth, chatBarHeight);
    sendButton.setBounds(chatInput.getRight() + 10, yPosition - 50, buttonWidth, chatBarHeight);
    creditsLabel.setBounds(area.removeFromTop(30).reduced(10));
}

void ChatBarComponent::sendPromptToGenerateParameters(const juce::String& userPrompt)
{
    requestInProgress = true;

    std::thread([this, userPrompt]() {
        juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
        if (accessToken.isEmpty())
        {
            juce::MessageManager::callAsync([this]() {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Authentication Error",
                    "No access token found. Please log in again.");
                if (auto* editor = dynamic_cast<SummonerXSerum2AudioProcessorEditor*>(getParentComponent()->getParentComponent()))
                {
                    editor->showLoadingScreen(false);
                }
                requestInProgress = false;
                });
            return;
        }

        juce::URL endpoint("https://ydr97n8vxe.execute-api.us-east-2.amazonaws.com/prod/generate-parameters");

        // Create JSON body
        juce::DynamicObject::Ptr jsonObject = new juce::DynamicObject();
        jsonObject->setProperty("input", userPrompt);
        juce::String postData = juce::JSON::toString(jsonObject.get());
        DBG("Sending POST request to /generate-parameters with JSON data: " + postData);

        juce::URL urlWithPostData = endpoint.withPOSTData(postData);

        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders("Content-Type: application/json\nAuthorization: Bearer " + accessToken)
            .withConnectionTimeoutMs(50000);

        std::unique_ptr<juce::InputStream> stream(urlWithPostData.createInputStream(options));

        if (stream != nullptr)
        {
            juce::String response = stream->readEntireStreamAsString();
            DBG("Raw response from /generate-parameters endpoint: " + response);
            juce::var result = juce::JSON::parse(response);

            if (result.isObject())
            {
                std::map<std::string, std::string> parameterMap;
                auto* obj = result.getDynamicObject();
                for (const auto& property : obj->getProperties())
                {
                    juce::String key = property.name.toString();
                    juce::String value = property.value.toString();
                    parameterMap[key.toStdString()] = value.toStdString();
                }

                juce::MessageManager::callAsync([this, parameterMap]() {
                    std::vector<std::map<std::string, std::string>> responses = { parameterMap };
                    processor.setResponses(responses);
                    if (auto* serumInterface = dynamic_cast<SerumInterfaceComponent*>(&processor.getSerumInterface()))
                    {
                        serumInterface->updateResponseCounter();
                    }

                    // Update credits after successful response
                    int credits = fetchUserCredits();
                    setCredits(credits);

                    if (auto* editor = dynamic_cast<SummonerXSerum2AudioProcessorEditor*>(getParentComponent()->getParentComponent()))
                    {
                        editor->showLoadingScreen(false);
                    }
                    requestInProgress = false;
                    });
            }
            else
            {
                juce::MessageManager::callAsync([this, response]() {
                    DBG("Failed to parse response from /generate-parameters: " + response);
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Error",
                        "Failed to generate parameters: " + response);
                    if (auto* editor = dynamic_cast<SummonerXSerum2AudioProcessorEditor*>(getParentComponent()->getParentComponent()))
                    {
                        editor->showLoadingScreen(false);
                    }
                    requestInProgress = false;
                    });
            }
        }
        else
        {
            juce::MessageManager::callAsync([this]() {
                DBG("Failed to connect to /generate-parameters endpoint");
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Connection Error",
                    "Failed to connect to the server. Please try again.");
                if (auto* editor = dynamic_cast<SummonerXSerum2AudioProcessorEditor*>(getParentComponent()->getParentComponent()))
                {
                    editor->showLoadingScreen(false);
                }
                requestInProgress = false;
                });
        }
        }).detach();
}

void ChatBarComponent::sendAIResponseToProcessor(const std::map<std::string, std::string>& aiResponse)
{
    processor.applyPresetToSerum(aiResponse);
}

void ChatBarComponent::setCredits(int credits)
{
    creditsLabel.setText("Credits: " + juce::String(credits), juce::dontSendNotification);
}

int ChatBarComponent::fetchUserCredits()
{
    juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
    if (accessToken.isEmpty())
    {
        DBG("No access token available to fetch credits");
        return 0;
    }

    juce::URL url("https://ydr97n8vxe.execute-api.us-east-2.amazonaws.com/prod/get-credits");

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withExtraHeaders("Authorization: Bearer " + accessToken)
        .withConnectionTimeoutMs(50000);

    std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));

    if (stream != nullptr)
    {
        juce::String response = stream->readEntireStreamAsString();
        DBG("Received response from /get-credits endpoint: " + response);
        juce::var result = juce::JSON::parse(response);
        int credits = result["credits"].toString().getIntValue();
        DBG("Parsed credits: " + juce::String(credits));
        return credits;
    }
    DBG("Failed to fetch credits: No response from server");
    return 0;
}