
#include "PluginProcessor.h"
#include "PluginEditor.h"

SolarInfernoAudioProcessor::SolarInfernoAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    ),
    parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
SolarInfernoAudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<Float>(
        juce::ParameterID{"sunDrive", 1}, "SUN Drive",
        Range(0.0f, 24.0f), 6.0f));

    layout.add(std::make_unique<Float>(
        juce::ParameterID{"planet", 1}, "PLANET",
        Range(0.0f, 12.0f), 3.0f));

    layout.add(std::make_unique<Float>(
        juce::ParameterID{"rocket", 1}, "ROCKET",
        Range(0.0f, 100.0f), 25.0f));

    layout.add(std::make_unique<Float>(
        juce::ParameterID{"heat", 1}, "HEAT",
        Range(0.0f, 100.0f), 20.0f));

    layout.add(std::make_unique<Float>(
        juce::ParameterID{"mix", 1}, "MIX",
        Range(0.0f, 100.0f), 100.0f));

    layout.add(std::make_unique<Float>(
        juce::ParameterID{"output", 1}, "OUTPUT",
        Range(-24.0f, 6.0f), 0.0f));

    return layout;
}

void SolarInfernoAudioProcessor::prepareToPlay(
    double, int)
{
}

void SolarInfernoAudioProcessor::releaseResources()
{
}

bool SolarInfernoAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()
        == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet()
        == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet()
        == juce::AudioChannelSet::stereo());
}

void SolarInfernoAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const float sun = parameters.getRawParameterValue("sunDrive")->load();
    const float planet = parameters.getRawParameterValue("planet")->load();
    const float rocket = parameters.getRawParameterValue("rocket")->load();
    const float heat = parameters.getRawParameterValue("heat")->load();
    const float mixAmount = parameters.getRawParameterValue("mix")->load() / 100.0f;
    const float output = parameters.getRawParameterValue("output")->load();

    const float drive = juce::Decibels::decibelsToGain(sun);
    const float planetGain = juce::Decibels::decibelsToGain(planet * 0.5f);
    const float rocketAmount = rocket / 100.0f;
    const float heatAmount = heat / 100.0f;
    const float outputGain = juce::Decibels::decibelsToGain(output);

    const float normaliser = 1.0f / std::tanh(drive);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* data = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float dry = data[sample];

            float wet = dry * drive * planetGain;
            wet = std::tanh(wet * (1.0f + heatAmount * 2.0f));
            wet *= normaliser;

            wet = wet * (1.0f - rocketAmount * 0.5f)
                + std::tanh(wet * 2.0f) * (rocketAmount * 0.5f);

            const float result = dry * (1.0f - mixAmount)
                + wet * mixAmount;

            data[sample] = result * outputGain;
        }
    }
}

juce::AudioProcessorEditor*
SolarInfernoAudioProcessor::createEditor()
{
    return new SolarInfernoAudioProcessorEditor(*this);
}

void SolarInfernoAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SolarInfernoAudioProcessor::setStateInformation(
    const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr)
    {
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(
                juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SolarInfernoAudioProcessor();
}
