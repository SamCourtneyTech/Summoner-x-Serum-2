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
#if JUCE_WINDOWS
    defaultPath = "C:/Program Files/Common Files/VST3/Serum2.vst3";
#elif JUCE_MAC
    defaultPath = "/Library/Audio/Plug-Ins/VST3/Serum2.vst3";
#else
    defaultPath = "Serum2.vst3";
#endif
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

    logoutButton.setLookAndFeel(&customSettingsButtons);
    logoutButton.setButtonText("Logout");
    logoutButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    logoutButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    logoutButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    logoutButton.onClick = [this]() {
        if (logoutButton.getButtonText() == "Logout" && onLogout) {
            onLogout();
        } else if (logoutButton.getButtonText() == "Login" && onLogin) {
            onLogin();
        }
        };
    addAndMakeVisible(logoutButton);

    // Credits label
    creditsLabel.setText("Credits: 0", juce::dontSendNotification);
    creditsLabel.setFont(juce::Font("Press Start 2P", 12.0f, juce::Font::plain));
    creditsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(creditsLabel);

    // Purchase credits button
    purchaseCreditsButton.setLookAndFeel(&customSettingsButtons);
    purchaseCreditsButton.setButtonText("Purchase Credits");
    purchaseCreditsButton.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    purchaseCreditsButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    purchaseCreditsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    purchaseCreditsButton.onClick = [this]() {
        // Placeholder for future implementation
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Purchase Credits",
            "Credit purchasing functionality will be implemented soon!");
    };
    addAndMakeVisible(purchaseCreditsButton);
}

SettingsComponent::~SettingsComponent()
{
    browseButton.setLookAndFeel(nullptr);
    resetButton.setLookAndFeel(nullptr);
    logoutButton.setLookAndFeel(nullptr);
    purchaseCreditsButton.setLookAndFeel(nullptr);
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
        "Select Serum2.vst3 or its Containing Folder",
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
                    auto serumFile = selectedFileOrFolder.getChildFile("Serum2.vst3");
                    if (serumFile.existsAsFile())
                    {
                        DBG("Found Serum2.vst3 in directory: " << serumFile.getFullPathName());
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
                            "The selected folder does not contain Serum2.vst3. Please select the correct folder or file."
                        );
                    }
                }
                else if (selectedFileOrFolder.getFileName() == "Serum2.vst3" && (selectedFileOrFolder.existsAsFile() || selectedFileOrFolder.isDirectory()))
                {
                    DBG("Selected Serum2.vst3: " << selectedFileOrFolder.getFullPathName());
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
                        "Please select the Serum2.vst3 file or its containing folder."
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

    // Top button row: Browse + Reset
    auto topButtonArea = bounds.removeFromTop(buttonHeight);
    browseButton.setBounds(topButtonArea.getX(), topButtonArea.getY(), buttonWidth, buttonHeight);
    resetButton.setBounds(topButtonArea.getX() + buttonWidth + buttonSpacing, topButtonArea.getY(), buttonWidth, buttonHeight);

    // Spacer between path section and credits section
    bounds.removeFromTop(buttonSpacing * 5);

    // Credits section
    creditsLabel.setBounds(bounds.getX(), bounds.getY(), buttonWidth * 2, buttonHeight);
    bounds.removeFromTop(buttonHeight + buttonSpacing);

    // Purchase credits button (original size)
    purchaseCreditsButton.setBounds(bounds.getX(), bounds.getY(), buttonWidth * 2, buttonHeight);
    bounds.removeFromTop(buttonHeight + buttonSpacing * 5);

    // Logout button
    logoutButton.setBounds(bounds.getX(), bounds.getY(), buttonWidth, buttonHeight);
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
    juce::String path = userSettings->getValue("pluginPath", defaultPath);
    DBG("Loaded saved path: " << path);
    return path;
}

void SettingsComponent::updateLoginState(bool isLoggedIn)
{
    if (isLoggedIn) {
        logoutButton.setButtonText("Logout");
        logoutButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    } else {
        logoutButton.setButtonText("Login");
        logoutButton.setColour(juce::TextButton::buttonColourId, juce::Colours::blue);
    }
}

void SettingsComponent::setCredits(int credits)
{
    currentCredits = credits;
    creditsLabel.setText("Credits: " + juce::String(credits), juce::dontSendNotification);
    
    // Set color to red if credits are 0, otherwise white
    if (credits == 0) {
        creditsLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    } else {
        creditsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
}

int SettingsComponent::getCredits() const
{
    return currentCredits;
}
