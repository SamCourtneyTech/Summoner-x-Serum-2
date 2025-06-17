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
    
    // Setup ChatBar callbacks
    chatBar.onLoadingStateChanged = [this](bool show) {
        showLoadingScreen(show);
    };
    chatBar.onRefreshTokenRequested = [this]() {
        refreshAccessToken();
    };
    chatBar.onCreditsUpdated = [this](int newCredits) {
        settings.setCredits(newCredits);
        appProps.getUserSettings()->setValue("credits", newCredits);
        appProps.getUserSettings()->save();
    };
    
    // Set up tab change callback
    tabs.onTabChanged = [this]() {
        updateChatLoginOverlay();
    };
    
    // Setup welcome and logged out screens
    setupWelcomeScreen();
    setupLoggedOutScreen();
    setupChatLoginOverlay();
    
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
        settings.setCredits(credits);
        
        // Start token refresh timer after successful login
        startTimer(900000); // 15 minutes = 900000 ms (frequent credit refresh)
        
        // Mark as not first time user
        appProps.getUserSettings()->setValue("hasLoggedInBefore", true);
        appProps.getUserSettings()->save();
        
        updateUIState();
        
        // If login was initiated from settings, redirect to chatbar tab
        if (loginInitiatedFromSettings) {
            tabs.setCurrentTabIndex(0); // 0 is the ChatGPT tab
            loginInitiatedFromSettings = false; // Reset the flag
            DBG("Login initiated from settings - redirecting to chatbar tab");
        }
        
        DBG("Login successful - UI updated to LoggedIn state");
        
        // Bring plugin window to front after successful login
        bringPluginToFront();
        
        repaint();
        resized(); // Ensure layout updates
        };

    login.onCancel = [this]() {
        handleLoginCancel();
        };

    settings.onLogout = [this]() {
        handleLogout();
        };
    
    settings.onLogin = [this]() {
        loginInitiatedFromSettings = true;
        startLoginProcess();
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
        settings.setCredits(credits);
        
        // Validate token and fetch current credits on plugin instantiation if logged in
        refreshAccessToken();
        
        // Start token refresh timer (900000 ms = 15 minutes)
        startTimer(900000); // 15 minutes = 900000 ms (frequent credit refresh)
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
    welcomeLoginButton.setLookAndFeel(nullptr);
    loggedOutLoginButton.setLookAndFeel(nullptr);
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
    // chatLoginOverlay bounds are handled by updateChatLoginOverlay()
    
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
    
    // Tab changes are now handled by the onTabChanged callback
    // Update chat overlay positioning after resize
    updateChatLoginOverlay();
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
    appProps.getUserSettings()->saveIfNeeded(); // Ensure settings are flushed

    chatBar.setCredits(0);
    settings.setCredits(0);
    
    // Set to LoggedOut state to show overlay on ChatGPT tab only
    currentUIState = UIState::LoggedOut;
    
    // Switch to ChatGPT tab
    tabs.setCurrentTabIndex(0);
    
    // Make sure tabs are visible and overlay is shown on ChatGPT tab
    tabs.setVisible(true);
    updateChatLoginOverlay();
    
    // Update settings button text
    settings.updateLoginState(false);
    
    DBG("User logged out - showing ChatGPT tab with overlay, other tabs remain accessible");
    repaint();
    resized();
}

void SummonerXSerum2AudioProcessorEditor::handleLoginCancel()
{
    // Return to the previous UI state
    currentUIState = previousUIState;
    updateUIState();
    
    DBG("Login cancelled - returned to previous state: " + juce::String((int)currentUIState));
}

void SummonerXSerum2AudioProcessorEditor::timerCallback()
{
    // Called every 15 minutes to validate token and update credits
    DBG("Timer callback triggered - validating token and updating credits");
    refreshAccessToken();
}


void SummonerXSerum2AudioProcessorEditor::refreshAccessToken()
{
    // Check if we're logged in and have an access token
    bool isLoggedIn = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
    juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
    int currentCredits = appProps.getUserSettings()->getIntValue("credits", 0);
    
    DBG("refreshAccessToken() called - isLoggedIn: " << (isLoggedIn ? "true" : "false") 
        << ", stored credits: " << currentCredits << ", token length: " << accessToken.length());
    
    if (!isLoggedIn || accessToken.isEmpty()) {
        DBG("Not logged in or no access token available - skipping credit refresh");
        // Don't force logout here - user might still be in the process of logging in
        // or there might be a temporary state issue
        return;
    }
    
    DBG("Valid login state detected, starting background credit refresh");
    
    // Check if credit fetch is already in progress
    if (creditsFetchInProgress.exchange(true)) {
        DBG("Credit fetch already in progress, skipping duplicate request");
        return;
    }
    
    // Fetch current credits to verify token is still valid
    std::thread([this, accessToken]() {
        DBG("Background thread started for credit refresh");
        fetchAndUpdateCredits(accessToken);
        creditsFetchInProgress = false;
        DBG("Background thread completed for credit refresh");
    }).detach();
}

void SummonerXSerum2AudioProcessorEditor::fetchAndUpdateCredits(const juce::String& accessToken)
{
    // Temporarily disable credits fetching to avoid component lifecycle issues
    DBG("Credits fetching disabled to prevent component exceptions");
    return;
    if (accessToken.isEmpty()) {
        DBG("No access token available for credits fetch");
        return;
    }
    
    DBG("Making request to get-credits endpoint");
    
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
            auto* obj = result.getDynamicObject();
            
            // Check if this is an error response with "detail" field
            if (obj->hasProperty("detail"))
            {
                juce::String detail = obj->getProperty("detail").toString();
                DBG("Credits fetch error: " + detail);
                
                if (detail == "Invalid token")
                {
                    DBG("Token is invalid - logging out user");
                    juce::MessageManager::callAsync([this]() {
                        handleLogout();
                    });
                }
                else
                {
                    DBG("Other API error during credits fetch: " + detail);
                    // Don't log out for other errors, just skip this update
                }
            }
            else if (obj->hasProperty("credits"))
            {
                // Successful response with credits
                int newCredits = obj->getProperty("credits").toString().getIntValue();
                
                // Update stored credits and display
                if (juce::MessageManager::getInstance()->isThisTheMessageThread())
                {
                    // We're already on the message thread, update directly
                    chatBar.setCredits(newCredits);
                    settings.setCredits(newCredits);
                    
                    // Save to settings safely
                    if (auto* userSettings = appProps.getUserSettings()) {
                        userSettings->setValue("credits", newCredits);
                        userSettings->saveIfNeeded();
                    }
                    
                    DBG("Credits updated successfully: " + juce::String(newCredits));
                }
                else
                {
                    // We're on a background thread, defer to message thread
                    juce::MessageManager::callAsync([this, newCredits]() {
                        chatBar.setCredits(newCredits);
                        settings.setCredits(newCredits);
                        
                        // Save to settings safely
                        if (auto* userSettings = appProps.getUserSettings()) {
                            userSettings->setValue("credits", newCredits);
                            userSettings->saveIfNeeded();
                        }
                        
                        DBG("Credits updated successfully: " + juce::String(newCredits));
                    });
                }
            }
            else
            {
                DBG("Credits fetch failed: No credits field in response");
                // Unknown response format - don't log out, just skip
            }
        }
        else
        {
            DBG("Credits fetch failed: Invalid response format - likely network issue, not logging out");
            // Don't log out for parsing errors - could be temporary network/server issues
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
    welcomeLoginButton.setLookAndFeel(&customLoginButtonLookAndFeel);
    welcomeLoginButton.onClick = [this]() { startLoginProcess(); };
    addChildComponent(welcomeLoginButton);
}

void SummonerXSerum2AudioProcessorEditor::setupLoggedOutScreen()
{
    // Logged out title - only used for chat overlay now
    loggedOutTitle.setText("Summoner x Serum 2", juce::dontSendNotification);
    loggedOutTitle.setFont(juce::Font("Press Start 2P", 28.0f, juce::Font::plain));
    loggedOutTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    loggedOutTitle.setJustificationType(juce::Justification::centred);
    // Note: These are now added to chatLoginOverlay in setupChatLoginOverlay()
    
    // Logged out message - only used for chat overlay now
    loggedOutMessage.setText("You're currently logged out", juce::dontSendNotification);
    loggedOutMessage.setFont(juce::Font("Press Start 2P", 14.0f, juce::Font::plain));
    loggedOutMessage.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    loggedOutMessage.setJustificationType(juce::Justification::centred);
    // Note: These are now added to chatLoginOverlay in setupChatLoginOverlay()
    
    // Logged out login button - only used for chat overlay now
    loggedOutLoginButton.setButtonText("Login");
    loggedOutLoginButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
    loggedOutLoginButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    loggedOutLoginButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loggedOutLoginButton.setLookAndFeel(&customLoginButtonLookAndFeel);
    loggedOutLoginButton.onClick = [this]() { startLoginProcess(); };
    // Note: These are now added to chatLoginOverlay in setupChatLoginOverlay()
}

void SummonerXSerum2AudioProcessorEditor::startLoginProcess()
{
    previousUIState = currentUIState;
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
    chatLoginOverlay.setVisible(false);
    
    // Always hide logged out components - they're only shown via chatLoginOverlay
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
            tabs.setVisible(true);
            updateChatLoginOverlay();
            break;
            
        case UIState::LoggingIn:
            login.setVisible(true);
            break;
            
        case UIState::LoggedIn:
            tabs.setVisible(true);
            updateChatLoginOverlay(); // Ensure overlay is hidden when logged in
            break;
    }
    
    // Update settings button text
    settings.updateLoginState(currentUIState == UIState::LoggedIn);
    
    DBG("UI State updated to: " + juce::String((int)currentUIState));
}

void SummonerXSerum2AudioProcessorEditor::setupChatLoginOverlay()
{
    chatLoginOverlay.addChildComponent(loggedOutTitle);
    chatLoginOverlay.addChildComponent(loggedOutMessage);
    chatLoginOverlay.addChildComponent(loggedOutLoginButton);
    addChildComponent(chatLoginOverlay);
}

void SummonerXSerum2AudioProcessorEditor::updateChatLoginOverlay()
{
    if (currentUIState == UIState::LoggedOut && tabs.getCurrentTabIndex() == 0)
    {
        chatLoginOverlay.setVisible(true);
        loggedOutTitle.setVisible(true);
        loggedOutMessage.setVisible(true);
        loggedOutLoginButton.setVisible(true);
        
        // Position overlay over the chat tab content
        auto tabContentArea = tabs.getTabContentComponent(0)->getBounds();
        chatLoginOverlay.setBounds(tabContentArea);
        
        // Center the login elements within the chat tab
        auto overlayBounds = chatLoginOverlay.getLocalBounds().reduced(50);
        auto centerX = overlayBounds.getCentreX();
        auto centerY = overlayBounds.getCentreY();
        
        loggedOutTitle.setBounds(overlayBounds.withHeight(60).withCentre(juce::Point<int>(centerX, centerY - 40)));
        loggedOutMessage.setBounds(overlayBounds.withHeight(40).withCentre(juce::Point<int>(centerX, centerY + 10)));
        loggedOutLoginButton.setBounds(overlayBounds.withHeight(40).withWidth(200).withCentre(juce::Point<int>(centerX, centerY + 60)));
    }
    else
    {
        chatLoginOverlay.setVisible(false);
    }
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
