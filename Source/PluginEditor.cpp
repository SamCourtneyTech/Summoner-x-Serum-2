#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "LoadingComponent.h"

#if JUCE_WINDOWS
    #include <windows.h>
#endif

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

    // Setup tabs but don't make visible yet
    tabs.addTab("ChatGPT", juce::Colours::transparentBlack, &chatBar, false);
    tabs.addTab("Serum", juce::Colours::transparentBlack, &audioProcessor.getSerumInterface(), false);
    tabs.addTab("Settings", juce::Colours::transparentBlack, &settings, false);
    addAndMakeVisible(tabs);
    tabs.setVisible(false);
    
    // Setup welcome and logged out screens
    setupWelcomeScreen();
    setupLoggedOutScreen();
    
    // Don't add login component to visible yet - we'll manage state properly
    addChildComponent(login);
    DBG("Setting onLoginSuccess callback for LoginComponent");
    login.onLoginSuccess = [this](juce::String token, int credits) {
        DBG("onLoginSuccess called with Token=" + token.substring(0, 10) + "...[truncated], Credits=" + juce::String(credits));
        
        // Save login state with explicit save
        appProps.getUserSettings()->setValue("isLoggedIn", true);
        appProps.getUserSettings()->setValue("accessToken", token);
        appProps.getUserSettings()->setValue("credits", credits);
        appProps.getUserSettings()->saveIfNeeded();
        appProps.getUserSettings()->save();  // Force save
        
        // Validate that the token was saved correctly
        juce::String savedToken = appProps.getUserSettings()->getValue("accessToken", "");
        bool savedLoginState = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
        int savedCredits = appProps.getUserSettings()->getIntValue("credits", 0);
        
        DBG("Validation after save - saved token length: " << savedToken.length() 
            << ", isLoggedIn: " << (savedLoginState ? "true" : "false") 
            << ", credits: " << savedCredits);
            
        if (savedToken.isEmpty() || !savedLoginState) {
            DBG("ERROR: Token was not saved properly!");
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Login Error",
                "Failed to save login credentials. Please try logging in again.");
            return;
        }
        
        currentUIState = UIState::LoggedIn;
        chatBar.setCredits(credits);
        
        // Start hourly token refresh timer after successful login
        startTimer(3600000); // 1 hour = 3600000 ms
        
        // Mark as not first time user
        appProps.getUserSettings()->setValue("hasLoggedInBefore", true);
        appProps.getUserSettings()->save();
        
        updateUIState();
        
        DBG("Login successful - UI updated to LoggedIn state");
        
        // Bring plugin window to front after successful login
        bringPluginToFront();
        
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

    // Determine initial UI state
    bool loggedIn = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
    juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
    int credits = appProps.getUserSettings()->getIntValue("credits", 0);
    
    if (loggedIn && !accessToken.isEmpty()) {
        currentUIState = UIState::LoggedIn;
        chatBar.setCredits(credits);
        
        // Validate token and fetch current credits on plugin instantiation if logged in
        refreshAccessToken();
        
        // Start hourly token refresh timer (3600000 ms = 1 hour)
        startTimer(3600000);
    } else if (isFirstTimeUser()) {
        currentUIState = UIState::FirstTime;
    } else {
        currentUIState = UIState::LoggedOut;
    }
    
    updateUIState();
}

SummonerXSerum2AudioProcessorEditor::~SummonerXSerum2AudioProcessorEditor()
{
    stopTimer();
}

void SummonerXSerum2AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    
    if (isLoading)
    {
        //DBG("Painting with isLoading=true, calling showLoadingScreen");
        loadingManager->showLoadingScreen(true);
        repaint();
    }
}

void SummonerXSerum2AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Set bounds for all components
    login.setBounds(bounds);
    tabs.setBounds(bounds);
    
    // Welcome screen layout
    auto welcomeArea = bounds.reduced(50);
    auto centerX = welcomeArea.getCentreX();
    auto centerY = welcomeArea.getCentreY();
    
    welcomeTitle.setBounds(welcomeArea.withHeight(60).withCentre(juce::Point<int>(centerX, centerY - 60)));
    welcomeMessage.setBounds(welcomeArea.withHeight(40).withCentre(juce::Point<int>(centerX, centerY - 10)));
    welcomeLoginButton.setBounds(welcomeArea.withHeight(40).withWidth(200).withCentre(juce::Point<int>(centerX, centerY + 40)));
    
    // Logged out screen layout  
    loggedOutTitle.setBounds(welcomeArea.withHeight(60).withCentre(juce::Point<int>(centerX, centerY - 40)));
    loggedOutMessage.setBounds(welcomeArea.withHeight(40).withCentre(juce::Point<int>(centerX, centerY + 10)));
    loggedOutLoginButton.setBounds(welcomeArea.withHeight(40).withWidth(200).withCentre(juce::Point<int>(centerX, centerY + 60)));
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
    // Stop token refresh timer
    stopTimer();
    
    appProps.getUserSettings()->setValue("isLoggedIn", false);
    appProps.getUserSettings()->setValue("accessToken", "");
    appProps.getUserSettings()->setValue("idToken", "");
    appProps.getUserSettings()->setValue("credits", 0);
    appProps.getUserSettings()->save();

    chatBar.setCredits(0);
    currentUIState = UIState::LoggedOut;
    updateUIState();
    
    DBG("User logged out - UI updated to LoggedOut state");
    repaint();
    resized();
}

void SummonerXSerum2AudioProcessorEditor::timerCallback()
{
    // Called every hour to validate token and update credits
    DBG("Timer callback triggered - validating token and updating credits");
    refreshAccessToken();
}

void SummonerXSerum2AudioProcessorEditor::refreshAccessToken()
{
    // Check if we're logged in and have an access token
    bool isLoggedIn = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
    juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
    
    if (!isLoggedIn || accessToken.isEmpty()) {
        DBG("Not logged in or no access token available - redirecting to login");
        
        // Clear login state and redirect to login
        juce::MessageManager::callAsync([this]() {
            handleLogout();
        });
        return;
    }
    
    DBG("Validating current access token and fetching latest credits");
    
    // Fetch current credits to verify token is still valid
    std::thread([this, accessToken]() {
        fetchAndUpdateCredits(accessToken);
    }).detach();
}

void SummonerXSerum2AudioProcessorEditor::fetchAndUpdateCredits(const juce::String& accessToken)
{
    if (accessToken.isEmpty()) {
        DBG("No access token available for credits fetch");
        return;
    }
    
    juce::URL creditsUrl("https://ydr97n8vxe.execute-api.us-east-2.amazonaws.com/prod/get-credits");
    
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withExtraHeaders("Authorization: Bearer " + accessToken)
        .withConnectionTimeoutMs(30000);
    
    std::unique_ptr<juce::InputStream> stream(creditsUrl.createInputStream(options));
    
    if (stream != nullptr)
    {
        juce::String response = stream->readEntireStreamAsString();
        DBG("Credits fetch response: " + response);
        
        juce::var result = juce::JSON::parse(response);
        
        if (result.isObject())
        {
            int newCredits = result["credits"].toString().getIntValue();
            
            // Update stored credits and display
            juce::MessageManager::callAsync([this, newCredits]() {
                appProps.getUserSettings()->setValue("credits", newCredits);
                appProps.getUserSettings()->save();
                
                // Update credits display
                chatBar.setCredits(newCredits);
                
                DBG("Credits updated successfully: " + juce::String(newCredits));
            });
        }
        else
        {
            DBG("Credits fetch failed: Invalid response format");
            // If credits fetch fails, token might be invalid
            juce::MessageManager::callAsync([this]() {
                DBG("Credits fetch failed - token may be invalid, logging out");
                handleLogout();
            });
        }
    }
    else
    {
        DBG("Credits fetch failed: Unable to connect to credits endpoint");
        // Network error - don't log out, just skip this update
    }
}

bool SummonerXSerum2AudioProcessorEditor::isFirstTimeUser()
{
    return !appProps.getUserSettings()->getBoolValue("hasLoggedInBefore", false);
}

void SummonerXSerum2AudioProcessorEditor::setupWelcomeScreen()
{
    // Welcome title
    welcomeTitle.setText("Welcome to Summoner x Serum 2", juce::dontSendNotification);
    welcomeTitle.setFont(juce::Font("Press Start 2P", 24.0f, juce::Font::plain));
    welcomeTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    welcomeTitle.setJustificationType(juce::Justification::centred);
    addChildComponent(welcomeTitle);
    
    // Welcome message
    welcomeMessage.setText("Get ready to create amazing sounds!", juce::dontSendNotification);
    welcomeMessage.setFont(juce::Font("Press Start 2P", 14.0f, juce::Font::plain));
    welcomeMessage.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    welcomeMessage.setJustificationType(juce::Justification::centred);
    addChildComponent(welcomeMessage);
    
    // Welcome login button
    welcomeLoginButton.setButtonText("Login");
    welcomeLoginButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
    welcomeLoginButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    welcomeLoginButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    welcomeLoginButton.onClick = [this]() { startLoginProcess(); };
    addChildComponent(welcomeLoginButton);
}

void SummonerXSerum2AudioProcessorEditor::setupLoggedOutScreen()
{
    // Logged out title
    loggedOutTitle.setText("Summoner x Serum 2", juce::dontSendNotification);
    loggedOutTitle.setFont(juce::Font("Press Start 2P", 28.0f, juce::Font::plain));
    loggedOutTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    loggedOutTitle.setJustificationType(juce::Justification::centred);
    addChildComponent(loggedOutTitle);
    
    // Logged out message
    loggedOutMessage.setText("You're currently logged out", juce::dontSendNotification);
    loggedOutMessage.setFont(juce::Font("Press Start 2P", 14.0f, juce::Font::plain));
    loggedOutMessage.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    loggedOutMessage.setJustificationType(juce::Justification::centred);
    addChildComponent(loggedOutMessage);
    
    // Logged out login button
    loggedOutLoginButton.setButtonText("Login");
    loggedOutLoginButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
    loggedOutLoginButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    loggedOutLoginButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loggedOutLoginButton.onClick = [this]() { startLoginProcess(); };
    addChildComponent(loggedOutLoginButton);
}

void SummonerXSerum2AudioProcessorEditor::startLoginProcess()
{
    currentUIState = UIState::LoggingIn;
    updateUIState();
    
    // Explicitly start the login flow
    login.startLoginFlow();
    DBG("Login process initiated by user click - browser should open");
}

void SummonerXSerum2AudioProcessorEditor::updateUIState()
{
    // Hide all components first
    tabs.setVisible(false);
    login.setVisible(false);
    welcomeTitle.setVisible(false);
    welcomeMessage.setVisible(false);
    welcomeLoginButton.setVisible(false);
    loggedOutTitle.setVisible(false);
    loggedOutMessage.setVisible(false);
    loggedOutLoginButton.setVisible(false);
    
    // Show appropriate components based on state
    switch (currentUIState)
    {
        case UIState::FirstTime:
            welcomeTitle.setVisible(true);
            welcomeMessage.setVisible(true);
            welcomeLoginButton.setVisible(true);
            break;
            
        case UIState::LoggedOut:
            loggedOutTitle.setVisible(true);
            loggedOutMessage.setVisible(true);
            loggedOutLoginButton.setVisible(true);
            break;
            
        case UIState::LoggingIn:
            login.setVisible(true);
            break;
            
        case UIState::LoggedIn:
            tabs.setVisible(true);
            break;
    }
    
    DBG("UI State updated to: " + juce::String((int)currentUIState));
}

void SummonerXSerum2AudioProcessorEditor::bringPluginToFront()
{
    DBG("Attempting to bring plugin window to front");
    
    // Use a small delay to ensure login processing is complete
    juce::Timer::callAfterDelay(500, [this]()
    {
        // Get the top-level component (plugin window)
        auto* topLevel = getTopLevelComponent();
        if (topLevel != nullptr)
        {
            DBG("Found top-level component, bringing to front");
            
            // Bring window to front
            topLevel->toFront(true);
            
            // Make sure it's visible
            topLevel->setVisible(true);
            
            // Try to grab keyboard focus
            topLevel->grabKeyboardFocus();
            
            // Also try to grab focus for this editor component specifically
            grabKeyboardFocus();
            
            // Force a repaint to ensure visibility
            topLevel->repaint();
            repaint();
            
#if JUCE_MAC
            // On macOS, try additional methods to bring window to front
            if (auto* peer = topLevel->getPeer())
            {
                peer->toFront(true);
            }
            
            // Try to activate the application
            juce::Process::makeForegroundProcess();
#endif

#if JUCE_WINDOWS
            // On Windows, try to restore and activate the window
            if (auto* peer = topLevel->getPeer())
            {
                peer->toFront(true);
                // Try to set foreground window
                auto windowHandle = peer->getNativeHandle();
                if (windowHandle != nullptr)
                {
                    SetForegroundWindow((HWND)windowHandle);
                    SetActiveWindow((HWND)windowHandle);
                    BringWindowToTop((HWND)windowHandle);
                }
            }
#endif
            
            DBG("Plugin window brought to front");
        }
        else
        {
            DBG("Could not find top-level component");
        }
    });
}
