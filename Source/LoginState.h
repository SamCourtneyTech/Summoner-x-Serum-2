#pragma once
#include <JuceHeader.h>

class LoginState
{
public:
    static bool isLoggedIn()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SummonerXSerum2";
        options.filenameSuffix = ".settings";
        options.folderName = "SummonerXSerum2App";
        options.osxLibrarySubFolder = "Application Support";
        juce::ApplicationProperties appProps;
        appProps.setStorageParameters(options);
        return appProps.getUserSettings()->getBoolValue("isLoggedIn", false);
    }

    static juce::String getAccessToken()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SummonerXSerum2";
        options.filenameSuffix = ".settings";
        options.folderName = "SummonerXSerum2App";
        options.osxLibrarySubFolder = "Application Support";
        juce::ApplicationProperties appProps;
        appProps.setStorageParameters(options);
        return appProps.getUserSettings()->getValue("accessToken", "");
    }

    static int getCredits()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SummonerXSerum2";
        options.filenameSuffix = ".settings";
        options.folderName = "SummonerXSerum2App";
        options.osxLibrarySubFolder = "Application Support";
        juce::ApplicationProperties appProps;
        appProps.setStorageParameters(options);
        return appProps.getUserSettings()->getIntValue("credits", 0);
    }

    static void setLoginState(const juce::String& accessToken, int credits, bool loggedIn)
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SummonerXSerum2";
        options.filenameSuffix = ".settings";
        options.folderName = "SummonerXSerum2App";
        options.osxLibrarySubFolder = "Application Support";
        juce::ApplicationProperties appProps;
        appProps.setStorageParameters(options);
        appProps.getUserSettings()->setValue("isLoggedIn", loggedIn);
        appProps.getUserSettings()->setValue("accessToken", accessToken);
        appProps.getUserSettings()->setValue("credits", credits);
        appProps.saveIfNeeded();
    }

    static void clearLoginState()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SummonerXSerum2";
        options.filenameSuffix = ".settings";
        options.folderName = "SummonerXSerum2App";
        options.osxLibrarySubFolder = "Application Support";
        juce::ApplicationProperties appProps;
        appProps.setStorageParameters(options);
        appProps.getUserSettings()->setValue("isLoggedIn", false);
        appProps.getUserSettings()->setValue("accessToken", "");
        appProps.getUserSettings()->setValue("credits", 0);
        appProps.saveIfNeeded();
    }
};