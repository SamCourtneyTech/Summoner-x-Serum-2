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
        DBG("Opening Cognito login URL in browser");
        cognitoUrl = juce::URL(
            "https://us-east-2_pvCcMvaRP.auth.us-east-2.amazoncognito.com/login?"
            "client_id=77ottlt6s5ntp1jup4av1r62m3&"
            "redirect_uri=http://localhost:8000/callback&"
            "response_type=code&"
            "scope=email+openid+phone"
        );
        cognitoUrl.launchInDefaultBrowser();
        startThread();
    }
}
