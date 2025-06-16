#pragma once
#include <JuceHeader.h>

class LoginComponent : public juce::Component, public juce::Thread, public juce::Timer
{
public:
    std::function<void(juce::String, int)> onLoginSuccess;
    std::function<void()> onCancel;
    void startLoginFlow();

    LoginComponent() : juce::Thread("LoginComponent Server Thread")
    {
        addAndMakeVisible(loginLabel);
        loginLabel.setText("Keep this page open. Sign-in in progress... ", juce::dontSendNotification);
        loginLabel.setFont(juce::Font("Press Start 2P", 16.0f, juce::Font::plain));
        loginLabel.setJustificationType(juce::Justification::centred);
        loginLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        cancelButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        cancelButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        cancelButton.setLookAndFeel(&customCancelButtonLookAndFeel);
        cancelButton.onClick = [this]() {
            if (onCancel) {
                onCancel();
            }
        };

        juce::PropertiesFile::Options options;
        options.applicationName = "SummonerXSerum2";
        options.filenameSuffix = ".settings";
        options.folderName = "SummonerXSerum2App";
        options.osxLibrarySubFolder = "Application Support";
        appProps.setStorageParameters(options);

        // Don't automatically start login process anymore - wait for explicit call
        DBG("LoginComponent constructed - waiting for explicit login start");

        // Initialize mystical floating boxes effect
        floatingBoxes.reserve(40); // Reserve space for up to 40 boxes
        startTimer(50); // 50ms timer for smooth animation (20 FPS)
    }

    ~LoginComponent() override
    {
        stopTimer();
        cancelButton.setLookAndFeel(nullptr);
        signalThreadShouldExit();
        if (serverSocket != nullptr)
        {
            serverSocket->close();
        }
        stopThread(100);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto buttonHeight = 40;
        auto buttonWidth = 120;
        auto spacing = 20;
        
        // Calculate the center position
        auto centerX = bounds.getWidth() / 2;
        auto centerY = bounds.getHeight() / 2;
        
        // Position the login label above center
        auto labelHeight = 60;
        loginLabel.setBounds(centerX - bounds.getWidth() / 2 + 20, 
                            centerY - labelHeight / 2 - spacing / 2, 
                            bounds.getWidth() - 40, 
                            labelHeight);
        
        // Position the cancel button right below the text
        cancelButton.setBounds(centerX - buttonWidth / 2, 
                              centerY + spacing / 2, 
                              buttonWidth, buttonHeight);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        
        // Draw mystical floating boxes
        for (const auto& box : floatingBoxes)
        {
            if (box.alpha > 0.0f)
            {
                g.setColour(box.color.withAlpha(box.alpha));
                g.fillRect(box.x, box.y, box.size, box.size);
            }
        }
    }

    void run() override
    {
        serverSocket = std::make_unique<juce::StreamingSocket>();
        DBG("Attempting to create listener on port 8000");
        if (!serverSocket->createListener(8000))
        {
            DBG("Failed to create listener on port 8000");
            juce::MessageManager::callAsync([this]() {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Login Failed",
                    "Could not start local server on port 8000. Ensure the port is not blocked and try again.");
                });
            return;
        }
        DBG("Successfully created listener on port 8000");

        while (!threadShouldExit())
        {
            DBG("Waiting for connection on port 8000");
            auto* clientSocket = serverSocket->waitForNextConnection();
            if (clientSocket != nullptr)
            {
                DBG("Received connection on port 8000");
                juce::String request;
                
                // Read the HTTP request with proper timeout and retry logic
                const int maxAttempts = 50; // 5 seconds total (50 * 100ms)
                int attempts = 0;
                bool dataReceived = false;
                
                while (attempts < maxAttempts && !dataReceived && !threadShouldExit())
                {
                    char buffer[2048];
                    int bytesRead = clientSocket->read(buffer, sizeof(buffer) - 1, false);
                    
                    if (bytesRead > 0)
                    {
                        buffer[bytesRead] = '\0'; // Null terminate
                        request += juce::String(buffer, bytesRead);
                        
                        // Check if we have a complete HTTP request (ends with double CRLF)
                        if (request.contains("\r\n\r\n") || request.contains("\n\n"))
                        {
                            dataReceived = true;
                            DBG("Received complete request: " + request.substring(0, 200) + "...");
                        }
                    }
                    else if (bytesRead == 0)
                    {
                        // No data available yet, wait a bit
                        juce::Thread::sleep(100);
                        attempts++;
                    }
                    else
                    {
                        // Error occurred
                        DBG("Socket read error");
                        break;
                    }
                }
                
                if (!dataReceived)
                {
                    DBG("No complete HTTP request received after timeout");
                }

                if (dataReceived && request.contains("GET /callback?code="))
                {
                    auto code = request.fromFirstOccurrenceOf("code=", false, false)
                        .upToFirstOccurrenceOf(" ", false, false);
                    DBG("Extracted auth code: " + code);

                    juce::String response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n"
                        "<html><body>"
                        "<h1>Login Successful</h1>"
                        "<p>You have been logged in to SummonerXSerum2. Please close this window and return to the plugin.</p>"
                        "<script>try { window.close(); } catch (e) { console.log('Could not close window:', e); }</script>"
                        "</body></html>";
                    clientSocket->write(response.toRawUTF8(), response.length());
                    DBG("Sent HTTP response to browser");

                    juce::MessageManager::callAsync([this, code]() {
                        handleAuthCode(code);
                        });
                }
                else if (dataReceived)
                {
                    DBG("Invalid request received: " + request.substring(0, 100) + "...");
                    juce::String response = "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n"
                        "<html><body><h1>Invalid Request</h1></body></html>";
                    clientSocket->write(response.toRawUTF8(), response.length());
                }
                else
                {
                    DBG("Connection closed without receiving complete request");
                }

                delete clientSocket;
            }
            else
            {
                DBG("No connection received; continuing to poll");
            }

            juce::Thread::sleep(10);
        }

        DBG("Thread exiting, closing socket");
        serverSocket->close();
        serverSocket.reset();
    }

private:
    class CancelButtonLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawButtonText(juce::Graphics& g, juce::TextButton& button,
            bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
        {
            auto font = juce::Font("Press Start 2P", 12.0f, juce::Font::plain);
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
            juce::Colour fillColour = isButtonDown ? juce::Colours::darkred
                : isMouseOverButton ? juce::Colours::indianred
                : backgroundColour;
            g.setColour(fillColour);
            g.fillRect(bounds);
        }
    };

    // Mystical floating boxes effect
    struct FloatingBox
    {
        float x, y;
        float size;
        float alpha;
        float targetAlpha;
        float fadeSpeed;
        juce::Colour color;
        int lifeTime;
        int maxLifeTime;
    };
    
    std::vector<FloatingBox> floatingBoxes;
    std::array<int, 4> quadrantCounts = {0, 0, 0, 0}; // Track boxes per quadrant
    juce::Random random;

    juce::Label loginLabel;
    juce::TextButton cancelButton;
    CancelButtonLookAndFeel customCancelButtonLookAndFeel;
    juce::URL cognitoUrl;
    std::unique_ptr<juce::StreamingSocket> serverSocket;
    juce::ApplicationProperties appProps;

    juce::String urlEncode(const juce::String& input)
    {
        juce::String result;
        for (auto c : input)
        {
            if (juce::CharacterFunctions::isLetterOrDigit(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                result += c;
            }
            else
            {
                int charValue = static_cast<int>(c);
                result += "%" + juce::String::toHexString(charValue).toUpperCase().paddedLeft('0', 2);
            }
        }
        return result;
    }

    void handleAuthCode(const juce::String& code)
    {
        DBG("Handling auth code: " + code);
        juce::URL loginUrl("https://ydr97n8vxe.execute-api.us-east-2.amazonaws.com/prod/login");

        // Create JSON body
        juce::DynamicObject::Ptr jsonObject = new juce::DynamicObject();
        jsonObject->setProperty("code", code);
        jsonObject->setProperty("redirect_uri", "http://localhost:8000/callback");
        juce::String postData = juce::JSON::toString(jsonObject.get());
        DBG("Sending POST request to /login with JSON data: " + postData);

        // Set POST data and headers directly on the URL
        juce::URL urlWithPostData = loginUrl.withPOSTData(postData);

        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders("Content-Type: application/json")
            .withConnectionTimeoutMs(50000);

        std::unique_ptr<juce::InputStream> stream(urlWithPostData.createInputStream(options));

        if (stream != nullptr)
        {
            juce::String response = stream->readEntireStreamAsString();
            DBG("Raw response from /login endpoint: " + response);
            juce::var result = juce::JSON::parse(response);
            DBG("Parsed JSON response: " + juce::JSON::toString(result));
            juce::String accessToken = result["access_token"].toString();
            juce::String idToken = result["id_token"].toString();
            DBG("Access token extracted: " + accessToken);
            DBG("ID token extracted: " + idToken.substring(0, 10) + "...[truncated]");

            if (accessToken.isNotEmpty() && idToken.isNotEmpty())
            {
                int credits = fetchUserCredits(accessToken);
                DBG("Credits fetched: " + juce::String(credits));
                
                // Save with explicit save operations
                appProps.getUserSettings()->setValue("isLoggedIn", true);
                appProps.getUserSettings()->setValue("accessToken", accessToken);
                appProps.getUserSettings()->setValue("idToken", idToken);
                appProps.getUserSettings()->setValue("credits", credits);
                appProps.getUserSettings()->saveIfNeeded();
                appProps.getUserSettings()->save();  // Force save
                
                // Validate save was successful
                juce::String savedToken = appProps.getUserSettings()->getValue("accessToken", "");
                juce::String savedIdToken = appProps.getUserSettings()->getValue("idToken", "");
                bool savedLoginState = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
                DBG("LoginComponent validation - saved access token length: " << savedToken.length()
                    << ", saved ID token length: " << savedIdToken.length()
                    << ", isLoggedIn: " << (savedLoginState ? "true" : "false"));
                
                if (onLoginSuccess)
                {
                    DBG("Triggering onLoginSuccess with token length: " << accessToken.length());
                    onLoginSuccess(accessToken, credits);
                    setVisible(false);
                    if (getParentComponent())
                    {
                        getParentComponent()->repaint();
                        getParentComponent()->resized();
                    }
                }
            }
            else
            {
                DBG("Authentication failed: Missing tokens in response");
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Login Failed",
                    "Missing access or ID token from server. Please check the server logs.");
            }
        }
        else
        {
            DBG("Authentication failed: Failed to connect to server");
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Login Failed",
                "Failed to connect to server. Please check your API Gateway URL and network connectivity.");
        }
    }

    int fetchUserCredits(const juce::String& accessToken)
    {
        DBG("Fetching user credits with access token: " + accessToken);
        juce::URL url("https://ydr97n8vxe.execute-api.us-east-2.amazonaws.com/prod/get-credits");

        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
            .withExtraHeaders("Authorization: Bearer " + accessToken)
            .withConnectionTimeoutMs(5000);

        std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));

        if (stream != nullptr)
        {
            juce::String response = stream->readEntireStreamAsString();
            DBG("Received response from /get-credits endpoint: " + response);
            juce::var result = juce::JSON::parse(response);
            
            if (result.isObject())
            {
                auto* obj = result.getDynamicObject();
                
                // Check if this is an error response with "detail" field
                if (obj->hasProperty("detail"))
                {
                    juce::String detail = obj->getProperty("detail").toString();
                    DBG("Credits fetch error in Login: " + detail);
                    return 0;
                }
                else if (obj->hasProperty("credits"))
                {
                    // Successful response with credits
                    int credits = obj->getProperty("credits").toString().getIntValue();
                    DBG("Parsed credits: " + juce::String(credits));
                    return credits;
                }
                else
                {
                    DBG("No credits field found in response");
                    return 0;
                }
            }
            else
            {
                DBG("Invalid JSON response format");
                return 0;
            }
        }
        DBG("Failed to fetch credits: No response from server");
        return 0;
    }

    void timerCallback() override
    {
        updateFloatingBoxes();
        
        // Randomly create new boxes
        if (random.nextFloat() < 0.12f && floatingBoxes.size() < 35) // 12% chance per frame, max 35 boxes
        {
            createRandomBox();
        }
        
        repaint();
    }

    void updateFloatingBoxes()
    {
        for (auto it = floatingBoxes.begin(); it != floatingBoxes.end();)
        {
            auto& box = *it;
            
            // Update lifetime
            box.lifeTime++;
            
            // Update alpha towards target
            float alphaDiff = box.targetAlpha - box.alpha;
            box.alpha += alphaDiff * box.fadeSpeed;
            
            // Change target alpha based on lifecycle
            if (box.lifeTime < box.maxLifeTime * 0.3f)
            {
                // Fade in phase
                box.targetAlpha = 0.6f;
            }
            else if (box.lifeTime > box.maxLifeTime * 0.7f)
            {
                // Fade out phase
                box.targetAlpha = 0.0f;
            }
            else
            {
                // Stable phase with gentle pulsing
                box.targetAlpha = 0.4f + 0.2f * std::sin(box.lifeTime * 0.1f);
            }
            
            // Remove boxes that have faded out and exceeded their lifetime
            if (box.alpha < 0.01f && box.lifeTime > box.maxLifeTime)
            {
                // Update quadrant count before removing
                int quadrant = getQuadrant(box.x, box.y);
                quadrantCounts[quadrant]--;
                it = floatingBoxes.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void createRandomBox()
    {
        FloatingBox box;
        
        // Find the least populated quadrant for balanced distribution
        int minQuadrant = 0;
        int minCount = quadrantCounts[0];
        for (int i = 1; i < 4; ++i)
        {
            if (quadrantCounts[i] < minCount)
            {
                minCount = quadrantCounts[i];
                minQuadrant = i;
            }
        }
        
        // Generate position in the chosen quadrant
        float halfWidth = getWidth() * 0.5f;
        float halfHeight = getHeight() * 0.5f;
        
        switch (minQuadrant)
        {
            case 0: // Top-left
                box.x = random.nextFloat() * halfWidth;
                box.y = random.nextFloat() * halfHeight;
                break;
            case 1: // Top-right
                box.x = halfWidth + random.nextFloat() * halfWidth;
                box.y = random.nextFloat() * halfHeight;
                break;
            case 2: // Bottom-left
                box.x = random.nextFloat() * halfWidth;
                box.y = halfHeight + random.nextFloat() * halfHeight;
                break;
            case 3: // Bottom-right
                box.x = halfWidth + random.nextFloat() * halfWidth;
                box.y = halfHeight + random.nextFloat() * halfHeight;
                break;
        }
        
        // Random size between 3-12 pixels
        box.size = 3.0f + random.nextFloat() * 9.0f;
        
        // Start invisible
        box.alpha = 0.0f;
        box.targetAlpha = 0.0f;
        
        // Random fade speed
        box.fadeSpeed = 0.02f + random.nextFloat() * 0.08f;
        
        // Random mystical colors (blues, purples, whites)
        float colorChoice = random.nextFloat();
        if (colorChoice < 0.4f)
        {
            // Blue tones
            box.color = juce::Colour::fromRGB(100 + random.nextInt(100), 150 + random.nextInt(100), 255);
        }
        else if (colorChoice < 0.7f)
        {
            // Purple tones
            box.color = juce::Colour::fromRGB(150 + random.nextInt(100), 100 + random.nextInt(100), 255);
        }
        else
        {
            // White/silver tones
            int brightness = 200 + random.nextInt(55);
            box.color = juce::Colour::fromRGB(brightness, brightness, brightness);
        }
        
        // Random lifetime (2-8 seconds at 20 FPS)
        box.maxLifeTime = 40 + random.nextInt(120); // 40-160 frames
        box.lifeTime = 0;
        
        // Update quadrant count
        int quadrant = getQuadrant(box.x, box.y);
        quadrantCounts[quadrant]++;
        
        floatingBoxes.push_back(box);
    }

    int getQuadrant(float x, float y)
    {
        float halfWidth = getWidth() * 0.5f;
        float halfHeight = getHeight() * 0.5f;
        
        if (x < halfWidth && y < halfHeight)
            return 0; // Top-left
        else if (x >= halfWidth && y < halfHeight)
            return 1; // Top-right
        else if (x < halfWidth && y >= halfHeight)
            return 2; // Bottom-left
        else
            return 3; // Bottom-right
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoginComponent)
};
