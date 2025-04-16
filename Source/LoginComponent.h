#pragma once
#include <JuceHeader.h>

class LoginComponent : public juce::Component, public juce::Button::Listener
{
public:
    std::function<void()> onLoginSuccess;

    LoginComponent()
    {
        addAndMakeVisible(usernameLabel);
        addAndMakeVisible(username);
        addAndMakeVisible(passwordLabel);
        addAndMakeVisible(password);
        addAndMakeVisible(loginButton);

        usernameLabel.setText("Username:", juce::dontSendNotification);
        passwordLabel.setText("Password:", juce::dontSendNotification);

        username.setTextToShowWhenEmpty("Enter username", juce::Colours::grey);
        password.setTextToShowWhenEmpty("Enter password", juce::Colours::grey);
        password.setPasswordCharacter('*');

        loginButton.setButtonText("Login");
        loginButton.addListener(this);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(100);
        usernameLabel.setBounds(area.removeFromTop(20));
        username.setBounds(area.removeFromTop(40));
        area.removeFromTop(20);
        passwordLabel.setBounds(area.removeFromTop(20));
        password.setBounds(area.removeFromTop(40));
        area.removeFromTop(20);
        loginButton.setBounds(area.removeFromTop(40));
    }

    void buttonClicked(juce::Button* b) override
    {
        if (b == &loginButton)
        {
            if (username.getText() == "test" && password.getText() == "test123")
            {
                if (onLoginSuccess) onLoginSuccess();
                setVisible(false);
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Login Failed",
                    "Incorrect username or password.");
            }
        }
    }

private:
    juce::Label usernameLabel, passwordLabel;
    juce::TextEditor username, password;
    juce::TextButton loginButton;
};
