/*
  ==============================================================================

    LoginComponent.cpp
    Created: 16 Apr 2025 2:47:59pm
    Author:  User

  ==============================================================================
*/

#include "LoginComponent.h"

void LoginComponent::startLoginFlow()
{
    DBG("Starting login flow explicitly");
    
    bool loggedIn = appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
    juce::String accessToken = appProps.getUserSettings()->getValue("accessToken", "");
    int credits = appProps.getUserSettings()->getIntValue("credits", 0);

    DBG("LoginFlow check - loggedIn: " + juce::String(loggedIn ? "true" : "false") + ", accessToken: " + accessToken + ", credits: " + juce::String(credits));
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
        DBG("Showing email/password login form");
        showEmailPasswordLogin();
    }
}

void LoginComponent::showEmailPasswordLogin()
{
    auto alertWindow = std::make_unique<juce::AlertWindow>("Login", "Enter your credentials:", juce::AlertWindow::NoIcon);
    alertWindow->addTextEditor("email", "", "Email:");
    alertWindow->addTextEditor("password", "", "Password:");
    alertWindow->getTextEditor("password")->setPasswordCharacter('*');
    alertWindow->addButton("Login", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    
    alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this](int result) {
        if (result == 1) // Login button pressed
        {
            auto email = static_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent())->getTextEditorContents("email");
            auto password = static_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent())->getTextEditorContents("password");
            
            if (email.isNotEmpty() && password.isNotEmpty())
            {
                authenticateWithEmailPassword(email, password);
            }
        }
    }), true);
}

void LoginComponent::authenticateWithEmailPassword(const juce::String& email, const juce::String& password)
{
    DBG("Authenticating with email: " + email);
    juce::URL loginUrl("https://ydr97n8vxe.execute-api.us-east-2.amazonaws.com/prod/login");

    // Create JSON body for email/password auth
    juce::DynamicObject::Ptr jsonObject = new juce::DynamicObject();
    jsonObject->setProperty("email", email);
    jsonObject->setProperty("password", password);
    juce::String postData = juce::JSON::toString(jsonObject.get());
    DBG("Sending POST request to /login with email/password: " + postData);

    // Set POST data and headers
    juce::URL urlWithPostData = loginUrl.withPOSTData(postData);
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders("Content-Type: application/json")
        .withConnectionTimeoutMs(30000);

    std::unique_ptr<juce::InputStream> stream(urlWithPostData.createInputStream(options));

    if (stream != nullptr)
    {
        juce::String response = stream->readEntireStreamAsString();
        DBG("Raw response from /login endpoint: " + response);
        juce::var result = juce::JSON::parse(response);
        
        if (result.hasProperty("access_token") && result.hasProperty("id_token"))
        {
            juce::String accessToken = result["access_token"].toString();
            juce::String idToken = result["id_token"].toString();
            
            // Store tokens and mark as logged in
            appProps.getUserSettings()->setValue("accessToken", accessToken);
            appProps.getUserSettings()->setValue("idToken", idToken);
            appProps.getUserSettings()->setValue("isLoggedIn", true);
            appProps.getUserSettings()->saveIfNeeded();
            
            // Get credits and trigger success
            fetchCreditsAndComplete(accessToken);
        }
        else
        {
            DBG("Authentication failed: " + response);
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Login Failed",
                "Invalid email or password. Please try again."
            );
        }
    }
    else
    {
        DBG("Failed to connect to server");
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Connection Failed", 
            "Could not connect to server. Please check your internet connection."
        );
    }
}

void LoginComponent::fetchCreditsAndComplete(const juce::String& accessToken)
{
    DBG("Fetching user credits with access token");
    juce::URL url("https://ydr97n8vxe.execute-api.us-east-2.amazonaws.com/prod/get-credits");

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withExtraHeaders("Authorization: Bearer " + accessToken)
        .withConnectionTimeoutMs(5000);

    std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));
    int credits = 0;

    if (stream != nullptr)
    {
        juce::String response = stream->readEntireStreamAsString();
        DBG("Credits response: " + response);
        juce::var result = juce::JSON::parse(response);
        if (result.hasProperty("credits"))
        {
            credits = result["credits"].toString().getIntValue();
        }
    }

    // Store credits and trigger success callback
    appProps.getUserSettings()->setValue("credits", credits);
    appProps.getUserSettings()->saveIfNeeded();

    if (onLoginSuccess)
    {
        DBG("Login successful - triggering onLoginSuccess callback");
        onLoginSuccess(accessToken, credits);
        setVisible(false);
        if (getParentComponent())
        {
            getParentComponent()->repaint();
            getParentComponent()->resized();
        }
    }
}
