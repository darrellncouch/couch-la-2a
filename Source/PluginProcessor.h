#pragma once
#include <JuceHeader.h>
#include "Dsp/OptoCompressor.h"

class CouchLA2AProcessor final : public juce::AudioProcessor
{
public:
    CouchLA2AProcessor();
    ~CouchLA2AProcessor() override = default;

    // ── AudioProcessor interface ──────────────────────────────────────────
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    using juce::AudioProcessor::processBlock;   // suppress hidden-override warning

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Couch LA-2A"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms() override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ── Meter accessors (UI thread) ───────────────────────────────────────
    float getGainReductionDb() const noexcept { return compressor.getGainReductionDb(); }
    float getOutputLevelDb()   const noexcept { return compressor.getOutputLevelDb();   }

    // ── Parameter tree ────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    OptoCompressor compressor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CouchLA2AProcessor)
};
