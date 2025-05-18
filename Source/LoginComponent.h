#pragma once
#include <JuceHeader.h>

class LoginComponent : public juce::Component, public juce::Thread
{
public:
    std::function<void(juce::String, int)> onLoginSuccess;

    LoginComponent() : juce::Thread("LoginComponent Server Thread")
    {
        addAndMakeVisible(loginLabel);
        loginLabel.setText("Logging you in... Please complete the login in your browser.", juce::dontSendNotification);

        juce::PropertiesFile::Options options;
        options.applicationName = "SummonerXSerum2";
        options.filenameSuffix = ".settings";
        options.folderName = "SummonerXSerum2App";
        options.osxLibrarySubFolder = "Application Support";
        appProps.setStorageParameters(options);

        bool loggedIn = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
        juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
        int credits = appProps.getUserSettings()->getIntValue("credits", 0);

        DBG("LoginComponent constructor - loggedIn: " + juce::String(loggedIn ? "true" : "false") + ", accessToken: " + accessToken + ", credits: " + juce::String(credits));
        if (loggedIn && accessToken.isNotEmpty())
        {
            DBG("User already logged in - triggering onLoginSuccess");
            juce::MessageManager::callAsync([this, accessToken, credits]() {
                if (onLoginSuccess)
                {
                    DBG("Calling onLoginSuccess from already logged in state");
                    onLoginSuccess(accessToken, credits);
                    setVisible(false);
                    if (getParentComponent())
                    {
                        getParentComponent()->repaint();
                        getParentComponent()->resized();
                    }
                }
                });
        }
        else
        {
            static bool hasLaunchedBrowser = false;
            if (!hasLaunchedBrowser)
            {
                DBG("Opening Cognito login URL in browser");
                cognitoUrl = juce::URL(
                    "https://us-east-2pvccmvarp.auth.us-east-2.amazoncognito.com/login?"
                    "client_id=77ottlt6s5ntp1jup4av1r62m3&"
                    "redirect_uri=http://localhost:8000/callback&"
                    "response_type=code&"
                    "scope=email+openid+phone"
                );
                cognitoUrl.launchInDefaultBrowser();
                hasLaunchedBrowser = true;
            }

            startThread();
        }
    }

    ~LoginComponent() override
    {
        signalThreadShouldExit();
        if (serverSocket != nullptr)
        {
            serverSocket->close();
        }
        stopThread(100);
    }

    void resized() override
    {
        loginLabel.setBounds(getLocalBounds().reduced(100));
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
                char buffer[1024];
                int bytesRead = clientSocket->read(buffer, sizeof(buffer), false);
                if (bytesRead > 0)
                {
                    request = juce::String(buffer, bytesRead);
                    DBG("Received request: " + request);
                }
                else
                {
                    DBG("No data received from client");
                }

                if (request.contains("GET /callback?code="))
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
                else
                {
                    DBG("Invalid request received: " + request);
                    juce::String response = "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n"
                        "<html><body><h1>Invalid Request</h1></body></html>";
                    clientSocket->write(response.toRawUTF8(), response.length());
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
    juce::Label loginLabel;
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
            DBG("Access token extracted: " + accessToken);

            if (accessToken.isNotEmpty())
            {
                int credits = fetchUserCredits(accessToken);
                DBG("Credits fetched: " + juce::String(credits));
                appProps.getUserSettings()->setValue("isLoggedIn", true);
                appProps.getUserSettings()->setValue("accessToken", accessToken);
                appProps.getUserSettings()->setValue("credits", credits);
                appProps.saveIfNeeded();
                if (onLoginSuccess)
                {
                    DBG("Triggering onLoginSuccess");
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
                DBG("Authentication failed: No access token in response");
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Login Failed",
                    "No access token received from server. Please check the server logs.");
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
            int credits = result["credits"].toString().getIntValue();
            DBG("Parsed credits: " + juce::String(credits));
            return credits;
        }
        DBG("Failed to fetch credits: No response from server");
        return 0;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoginComponent)
};