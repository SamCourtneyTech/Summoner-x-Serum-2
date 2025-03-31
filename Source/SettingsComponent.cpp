#include "SettingsComponent.h"

class SettingsButtonLookAndFeel : public juce::LookAndFeel_V4
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

SettingsComponent::SettingsComponent(SummonerXSerum2AudioProcessor& processor)
{
    static SettingsButtonLookAndFeel customSettingsButtons;
    juce::PropertiesFile::Options options;
    options.applicationName = "SummonerXSerum2";
    options.filenameSuffix = ".settings";
    options.folderName = "SummonerXSerum2App";
    options.osxLibrarySubFolder = "Application Support";
    applicationProperties.setStorageParameters(options);
    defaultPath = "C:/Program Files/Common Files/VST3/Serum2.vst3";
    pathLabel.setText("Serum Path:", juce::dontSendNotification);
    pathLabel.setColour(juce::Label::textColourId, juce::Colours::indianred);
    pathLabel.setFont(juce::Font("Press Start 2P", 12.0f, juce::Font::italic));
    addAndMakeVisible(pathLabel);
    pathDisplay.setText(loadSavedPath(), juce::dontSendNotification);
    pathDisplay.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    pathDisplay.setColour(juce::Label::textColourId, juce::Colours::white);
    pathDisplay.setFont(juce::Font("Press Start 2P", 12.0f, juce::Font::plain));
    pathDisplay.setJustificationType(juce::Justification::centredLeft);
    pathDisplay.setBorderSize(juce::BorderSize<int>(2));
    addAndMakeVisible(pathDisplay);
    browseButton.setLookAndFeel(&customSettingsButtons);
    addAndMakeVisible(browseButton);
    browseButton.setButtonText("Browse");
    browseButton.setColour(juce::TextButton::buttonColourId, juce::Colours::whitesmoke);
    browseButton.setColour(juce::TextButton::textColourOnId, juce::Colours::darkgoldenrod);
    browseButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    browseButton.onClick = [this]() { browseForPath(); };
    addAndMakeVisible(resetButton);
    resetButton.setButtonText("Reset Path");
    resetButton.setLookAndFeel(&customSettingsButtons);
    resetButton.onClick = [this]() { resetSavedPath(); };
    resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::whitesmoke);
    resetButton.setColour(juce::TextButton::textColourOnId, juce::Colours::darkgoldenrod);
    resetButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    DBG("SettingsComponent constructed with path: " << loadSavedPath());
}

SettingsComponent::~SettingsComponent()
{
    browseButton.setLookAndFeel(nullptr);
}

void SettingsComponent::resetSavedPath()
{
    auto* userSettings = applicationProperties.getUserSettings();
    if (userSettings != nullptr)
    {
        userSettings->removeValue("pluginPath");
        userSettings->saveIfNeeded();
        pathDisplay.setText(defaultPath, juce::dontSendNotification);
        if (onPathChanged)
        {
            onPathChanged(defaultPath);
        }
        DBG("Path reset to default: " << defaultPath);
    }
    else
    {
        DBG("User settings not initialized.");
    }
}

void SettingsComponent::browseForPath()
{
    DBG("Browse button clicked!");
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Serum.vst3 or its Containing Folder",
        juce::File(defaultPath),
        "*.vst3"
    );
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fileChooser)
        {
            DBG("FileChooser callback triggered.");
            auto selectedFileOrFolder = fileChooser.getResult();
            if (selectedFileOrFolder.exists())
            {
                if (selectedFileOrFolder.isDirectory())
                {
                    DBG("Selected directory: " << selectedFileOrFolder.getFullPathName());
                    auto serumFile = selectedFileOrFolder.getChildFile("Serum.vst3");
                    if (serumFile.existsAsFile())
                    {
                        DBG("Found Serum.vst3 in directory: " << serumFile.getFullPathName());
                        pathDisplay.setText(serumFile.getFullPathName(), juce::dontSendNotification);
                        savePath(serumFile.getFullPathName());
                        if (onPathChanged)
                            onPathChanged(serumFile.getFullPathName());
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            "Invalid Folder",
                            "The selected folder does not contain Serum.vst3. Please select the correct folder or file."
                        );
                    }
                }
                else if (selectedFileOrFolder.getFileName() == "Serum.vst3" && selectedFileOrFolder.existsAsFile())
                {
                    DBG("Selected Serum.vst3 file: " << selectedFileOrFolder.getFullPathName());
                    pathDisplay.setText(selectedFileOrFolder.getFullPathName(), juce::dontSendNotification);
                    savePath(selectedFileOrFolder.getFullPathName());
                    if (onPathChanged)
                        onPathChanged(selectedFileOrFolder.getFullPathName());
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Invalid Selection",
                        "Please select the Serum.vst3 file or its containing folder."
                    );
                }
            }
            else
            {
                DBG("No valid file or folder selected, or dialog canceled.");
            }
        });
}

juce::String SettingsComponent::getPluginPath() const
{
    return pluginPathLabel.getText();
}

void SettingsComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void SettingsComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    pathLabel.setBounds(bounds.removeFromTop(20));
    pathDisplay.setBounds(bounds.removeFromTop(30));
    auto buttonWidth = 120;
    auto buttonHeight = 30;
    auto buttonSpacing = 10;
    auto buttonArea = bounds.removeFromTop(buttonHeight * 2 + buttonSpacing);
    browseButton.setBounds(buttonArea.getX(), buttonArea.getY(), buttonWidth, buttonHeight);
    resetButton.setBounds(buttonArea.getX() + buttonWidth + buttonSpacing, buttonArea.getY(), buttonWidth, buttonHeight);
}

void SettingsComponent::savePath(const juce::String& path)
{
    auto* userSettings = applicationProperties.getUserSettings();
    userSettings->setValue("pluginPath", path);
    userSettings->saveIfNeeded();
}

juce::String SettingsComponent::loadSavedPath()
{
    auto* userSettings = applicationProperties.getUserSettings();
    return userSettings->getValue("pluginPath", defaultPath);
}