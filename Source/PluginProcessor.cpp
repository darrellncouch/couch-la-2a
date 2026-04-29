#include "PluginProcessor.h"
#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Parameter layout
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout CouchLA2AProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // GAIN  0–100 (maps to 0–40 dB output gain)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "gain", "Gain",
        juce::NormalisableRange<float> (0.f, 100.f, 0.1f), 50.f));

    // PEAK REDUCTION  0–100 (maps to threshold 0 to −40 dBFS)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "peakReduction", "Peak Reduction",
        juce::NormalisableRange<float> (0.f, 100.f, 0.1f), 0.f));

    // MODE  0 = Compress, 1 = Limit
    layout.add (std::make_unique<juce::AudioParameterInt> (
        "mode", "Mode", 0, 1, 0));

    return layout;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

CouchLA2AProcessor::CouchLA2AProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CouchLA2A", createParameterLayout())
{
}

// ─────────────────────────────────────────────────────────────────────────────
// prepareToPlay
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    compressor.prepare (sampleRate);

    compressor.setGain          (apvts.getRawParameterValue ("gain")->load());
    compressor.setPeakReduction (apvts.getRawParameterValue ("peakReduction")->load());
    compressor.setMode          (static_cast<int> (apvts.getRawParameterValue ("mode")->load()));
}

// ─────────────────────────────────────────────────────────────────────────────
// isBusesLayoutSupported
// ─────────────────────────────────────────────────────────────────────────────

bool CouchLA2AProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// processBlock
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    // ── Pull parameter values ──────────────────────────────────────────────
    compressor.setGain          (apvts.getRawParameterValue ("gain")->load());
    compressor.setPeakReduction (apvts.getRawParameterValue ("peakReduction")->load());
    compressor.setMode          (static_cast<int> (apvts.getRawParameterValue ("mode")->load()));

    // ── Process ───────────────────────────────────────────────────────────
    const int numSamples = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;

    compressor.processBlock (L, R, numSamples);
}

// ─────────────────────────────────────────────────────────────────────────────
// State persistence
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CouchLA2AProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ─────────────────────────────────────────────────────────────────────────────
// Editor
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessorEditor* CouchLA2AProcessor::createEditor()
{
    return new CouchLA2AEditor (*this);
}

// ─────────────────────────────────────────────────────────────────────────────
// JUCE plugin entry point
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CouchLA2AProcessor();
}
