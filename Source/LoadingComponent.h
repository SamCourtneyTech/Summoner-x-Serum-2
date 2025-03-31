#pragma once
#include <JuceHeader.h>
class LoadingScreenComponent : public juce::Component
{
public:
    LoadingScreenComponent()
    {
        setAlwaysOnTop(true);
        customFont = juce::Font("Press Start 2P", 15.0f, juce::Font::plain);
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.7f));
        g.setColour(juce::Colours::white);
        g.setFont(customFont);
        juce::String loadingText = "Processing request... (This can take up to a minute)";
        auto textBounds = getLocalBounds();
        int verticalOffset = 0;
        textBounds.translate(0, verticalOffset);
        g.drawText(loadingText, textBounds, juce::Justification::centred);
        float centerX = getWidth() / 2.0f;
        float centerY = getHeight() / 2.0f - 100.0f;
        float time = static_cast<float>(juce::Time::getMillisecondCounter()) / 500.0f;
        for (int i = 0; i < 4; ++i)
        {
            float angle = time + i * juce::MathConstants<float>::pi * 0.5f;
            float alpha = 0.2f + 0.8f * std::abs(std::sin(angle));
            g.setColour(juce::Colours::white.withAlpha(alpha));
            float squareSize = 12.0f;
            float radius = 30.0f;
            float x = centerX + radius * std::cos(angle) - squareSize * 0.5f;
            float y = centerY + radius * std::sin(angle) - squareSize * 0.5f;
            x = std::round(x);
            y = std::round(y);
            g.fillRect(x, y, squareSize, squareSize);
        }
        repaint();
    }
private:
    juce::Font customFont;
};
class LoadingScreenManager
{
public:
    LoadingScreenManager(juce::Component* parent)
        : parentComponent(parent)
    {
    }
    void showLoadingScreen(bool show)
    {
        if (show && loadingScreen == nullptr)
        {
            loadingScreen = std::make_unique<LoadingScreenComponent>();
            parentComponent->addAndMakeVisible(loadingScreen.get());
            loadingScreen->setBounds(parentComponent->getLocalBounds());
            loadingScreen->setVisible(true);
            loadingScreen->toFront(true);
        }
        else if (!show && loadingScreen != nullptr)
        {
            loadingScreen->setVisible(false);
            loadingScreen.reset();
        }
    }
private:
    juce::Component* parentComponent;
    std::unique_ptr<LoadingScreenComponent> loadingScreen;
};