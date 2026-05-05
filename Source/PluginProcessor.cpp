/*
  ==============================================================================

    PLUGINPROCESSOR.CPP

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "Utils.h"



namespace IDs{ // IDs utilitzades per la GUI
    static juce::Identifier oscilloscope { "oscilloscope" }; // Oscil·loscopi
    static juce::Identifier fft {"FFT"}; // Analitzador FFT
}


//==============================================================================
KobolVCOAudioProcessor::KobolVCOAudioProcessor() // Constructor

: foleys::MagicProcessor (juce::AudioProcessor::BusesProperties() // Inicialització (bus de sortida estèreo, àudio)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
apvts (*this, nullptr, "PARAMETERS", createParameterLayout())

{

    FOLEYS_SET_SOURCE_PATH (__FILE__);

    castParameter(apvts, ParameterID::outputLevel, outputLevelParam); // Enllaça outputLevel amb outputLevelParam
    castParameter(apvts, ParameterID::waveF, waveFParam); // Enllaça waveF amb waveFParam
    castParameter(apvts, ParameterID::attackParam,  attackParam);
    castParameter(apvts, ParameterID::decayParam,   decayParam);
    castParameter(apvts, ParameterID::sustainParam, sustainParam);
    castParameter(apvts, ParameterID::decayOff, decayOffParam);

    castParameter(apvts, ParameterID::vcfInputLevel, vcfInputLevelParam);
    castParameter(apvts, ParameterID::cutoffParam, cutoffParam);
    castParameter(apvts, ParameterID::resonanceParam, resonanceParam);
    castParameter(apvts, ParameterID::filterBypass, filterBypassParam);
    
    apvts.state.addListener(this);
    
    
    // MAGIC GUI: register an oscilloscope to display in the GUI. keep a pointer to push samples into in processBlock(). Only interested in channel 0
    oscilloscope = magicState.createAndAddObject<foleys::MagicOscilloscope>(IDs::oscilloscope, 0); // Oscil·loscopi (canal esquerre)
    analyser = magicState.createAndAddObject<foleys::MagicAnalyser>(IDs::fft, 1); // Analitzador FFT


    magicState.setGuiValueTree (BinaryData::magic_xml, BinaryData::magic_xmlSize); //Loads magic_XML config
}


//==============================================================================

KobolVCOAudioProcessor::~KobolVCOAudioProcessor(){ // Destructor
    
    apvts.state.removeListener(this);
}



//==============================================================================
//PrePareToPlay and releasesResources:
//==============================================================================

void KobolVCOAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // pre-playback initialisation
    synth.allocateResources(sampleRate, samplesPerBlock);
    parametersChanged.store(true);
    analyser->prepareToPlay(sampleRate, samplesPerBlock);
    magicState.prepareToPlay (sampleRate, samplesPerBlock);
    reset();
}

//==============================================================================

void KobolVCOAudioProcessor::releaseResources()
{
    // When playback stops, free up any spare memory, etc.
    synth.deallocateResources();
}
//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool KobolVCOAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // Check if the layout is supported. Thisc code only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


//==============================================================================
//HandleMIDI
//==============================================================================
void KobolVCOAudioProcessor::handleMIDI (uint8_t data0, uint8_t data1, uint8_t data2){
 
    synth.midiMessage(data0, data1, data2);
}


//==============================================================================
//RENDER AUDIO
//==============================================================================
void KobolVCOAudioProcessor::render(juce::AudioBuffer<float>& buffer, int sampleCount, int bufferOffset){ // Genera àudio per un fragment del buffer
    
    float* outputBuffers[2]={nullptr, nullptr}; // Array de punters als canals de sortida
    outputBuffers[0]=buffer.getWritePointer(0)+bufferOffset; // Canal esquerre amb desplaçament
    if(getTotalNumOutputChannels() > 1){
        outputBuffers[1]=buffer.getWritePointer(1); // Canal dret (si existeix)
    }
    synth.render(outputBuffers, sampleCount); //RENDER AUDIO // Genera àudio (synth)
}


//==============================================================================
//SplitBufferByEvents (Here Audio is Rendered)
//==============================================================================
void KobolVCOAudioProcessor:: splitBufferByEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages){
    
    int bufferOffset=0;
    for (const auto metadata : midiMessages){ // Per cada missatge MIDI...
        //render audio that happens before the event
        int samplesThisSegment = metadata.samplePosition - bufferOffset; // Calcula nombre de mostres abans de l'esdeveniment
        if(samplesThisSegment > 0){
            render(buffer, samplesThisSegment, bufferOffset); // Renderitza l'àudio fins a l'esdeveniment
            bufferOffset += samplesThisSegment;
        }
        if (metadata.numBytes <=3){
            uint8_t data1 = (metadata.numBytes >= 2) ? metadata.data[1] : 0;
            uint8_t data2 = (metadata.numBytes ==3) ? metadata.data[2] :0 ;
            handleMIDI(metadata.data[0], data1, data2); // Processa l'esdeveniment MIDI exactament a la seva mostra
        }
    }
    int samplesLastSegment=buffer.getNumSamples() - bufferOffset;
    if(samplesLastSegment > 0){
        render (buffer, samplesLastSegment, bufferOffset);
        
    }
    midiMessages.clear(); // Neteja el buffer MIDI després de processar-lo
}


//==============================================================================
//Reset
//==============================================================================

void KobolVCOAudioProcessor::reset(){
    
    synth.reset();
    synth.outputLevelSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->get()));
}

//==============================================================================
//PROCESS BLOCK
//==============================================================================

void KobolVCOAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) // Funció principal cridada pel DAW
{
    juce::ScopedNoDenormals noDenormals;
    

    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    magicState.processMidiBuffer(midiMessages, buffer.getNumSamples() ); //Allows PGM keyboard to be used

    // Clear any output channels that don't contain input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }
    
    bool expected=true;
    if(isNonRealtime() || parametersChanged.compare_exchange_strong(expected, false)){
        update();
    }
    
    splitBufferByEvents(buffer, midiMessages); //RENDER here
    
    oscilloscope->pushSamples (buffer);
    analyser->pushSamples (buffer);
}

//==============================================================================
//UPDATE: update Parameters values: outputLevel and waveForm
//==============================================================================
void KobolVCOAudioProcessor::update(){
    
    float waveF=waveFParam->get();
    synth.waveForm=waveF; // Assigna la forma d'ona
    
    synth.outputLevelSmoother.setTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->get())); // Nivell de sortida
    synth.attackTime   = Synth::attackCurve(attackParam->get());
    bool decayOff = decayOffParam->get() > 0.5f;
    if (decayOff) synth.decayTime = Synth::decayCurve(0.0f);
    else synth.decayTime = Synth::decayCurve(decayParam->get());
    synth.sustainLevel = Synth::sustainCurve(sustainParam->get());

    synth.vcfInputLevel = vcfInputLevelParam->get() / 5.0f;
    synth.filterCutoff    = Synth::cutoffCurve(cutoffParam->get());
    synth.filterResonance = resonanceParam->get();
    synth.filterBypass = filterBypassParam->get() > 0.5f;
}


//==============================================================================
// PARAMETERS:APVTS
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout KobolVCOAudioProcessor::createParameterLayout(){
    
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    

 
    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::outputLevel,
            "Output Level",
            juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::waveF,
            "Wave Form",
            juce::NormalisableRange<float>(0.0f, 1.1f, 0.02f), //
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("VCO")));

    // Sliders d'attack, sustain i decay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::attackParam, "Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("V")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::decayParam, "Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("V")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::sustainParam, "Sustain",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f,
            juce::AudioParameterFloatAttributes().withLabel("V")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::decayOff, "Decay Off",
            juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::vcfInputLevel, "VCF Input Level",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f,
            juce::AudioParameterFloatAttributes().withLabel("V")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::cutoffParam, "VCF Cutoff",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 7.0f,
            juce::AudioParameterFloatAttributes().withLabel("V")));
 
    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::resonanceParam, "VCF Resonance",
            juce::NormalisableRange<float>(0.0f, 1.225f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID::filterBypass, "Filter Bypass",
            juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

    return layout;

}


//==============================================================================
//PARAMETERS: Saving and loading parameters state
//==============================================================================
void KobolVCOAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Method to store your parameters in the memory block.
    
    magicState.getStateInformation(destData);
    
}


//==============================================================================

void KobolVCOAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore your parameters from this memory block created by the getStateInformation() call.
        magicState.setStateInformation(data, sizeInBytes, getActiveEditor());
}


//==============================================================================
// This creates new instances of the plugin..

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KobolVCOAudioProcessor();
}
//==============================================================================



