 // Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <ctime>

TIME12AudioProcessor::TIME12AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
         .withInput("Input", juce::AudioChannelSet::stereo(), true)
         .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
     )
    , settings{}
    , params(*this, &undoManager, "PARAMETERS", {
        std::make_unique<juce::AudioParameterInt>("pattern", "Pattern", 1, 12, 1),
        std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterChoice>("patsync", "Pattern Sync", StringArray { "Off", "1/4 Beat", "1/2 Beat", "1 Beat", "2 Beats", "4 Beats"}, 0),
        std::make_unique<juce::AudioParameterChoice>("trigger", "Trigger", StringArray { "Sync", "MIDI", "Audio" }, 0),
        std::make_unique<juce::AudioParameterChoice>("sync", "Sync", StringArray { "Rate Hz", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "1/16t", "1/8t", "1/4t", "1/2t", "1/1t", "1/16.", "1/8.", "1/4.", "1/2.", "1/1." }, 5),
        std::make_unique<juce::AudioParameterFloat>("rate", "Rate Hz", juce::NormalisableRange<float>(0.1f, 5000.0f, 0.01f, 0.2f), 1.0f),
        std::make_unique<juce::AudioParameterFloat>("phase", "Phase", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("min", "Min", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("max", "Max", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterFloat>("smooth", "Smooth", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("attack", "Attack", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("release", "Release", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tension", "Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tensionatk", "Attack Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tensionrel", "Release Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterBool>("snap", "Snap", false),
        std::make_unique<juce::AudioParameterInt>("grid", "Grid", 0, (int)std::size(GRID_SIZES)-1, 2),
        std::make_unique<juce::AudioParameterChoice>("anoise", "Anti-Noise", StringArray { "Off", "Low", "Medium", "High"}, 2),
        std::make_unique<juce::AudioParameterInt>("seqstep", "Sequencer Step", 0, (int)std::size(GRID_SIZES)-1, 2),
        // audio trigger params
        std::make_unique<juce::AudioParameterChoice>("algo", "Audio Algorithm", StringArray { "Simple", "Drums" }, 0),
        std::make_unique<juce::AudioParameterFloat>("threshold", "Audio Threshold", NormalisableRange<float>(0.0f, 1.0f), 0.5f),
        std::make_unique<juce::AudioParameterFloat>("sense", "Audio Sensitivity", 0.0f, 1.0f, 0.5f),
        std::make_unique<juce::AudioParameterFloat>("lowcut", "Audio LowCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20.f),
        std::make_unique<juce::AudioParameterFloat>("highcut", "Audio HighCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20000.f),
        std::make_unique<juce::AudioParameterFloat>("offset", "Audio Offset", -1.0f, 1.0f, 0.0f)
    })
#endif
{
    srand(static_cast<unsigned int>(time(nullptr))); // seed random generator
    juce::PropertiesFile::Options options{};
    options.applicationName = ProjectInfo::projectName;
    options.filenameSuffix = ".settings";
#if defined(JUCE_LINUX) || defined(JUCE_BSD)
    options.folderName = "~/.config/TIME12";
#endif
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = PropertiesFile::storeAsXML;
    settings.setStorageParameters(options);

    for (auto* param : getParameters()) {
        param->addListener(this);
    }

    params.addParameterListener("pattern", this);

    // init patterns
    for (int i = 0; i < 12; ++i) {
        patterns[i] = new Pattern(i);
        patterns[i]->insertPoint(0.0, 0.0, 0, 0);
        patterns[i]->insertPoint(1.0, 0.0, 0, 0);
        patterns[i]->buildSegments();
    }

    // init paintMode Patterns
    for (int i = 0; i < PAINT_PATS; ++i) {
        paintPatterns[i] = new Pattern(i + PAINT_PATS_IDX);
        if (i < 8) {
            auto preset = Presets::getPaintPreset(i);
            for (auto& point : preset) {
                paintPatterns[i]->insertPoint(point.x, point.y, point.tension, point.type);
            }
        }
        else {
            paintPatterns[i]->insertPoint(0.0, 0.0, 0.0, 1);
            paintPatterns[i]->insertPoint(1.0, 1.0, 0.0, 1);
        }
        paintPatterns[i]->buildSegments();
    }

    sequencer = new Sequencer(*this);
    pattern = patterns[0];
    viewPattern = pattern;
    preSamples.resize(MAX_PLUG_WIDTH, 0); // samples array size must be >= viewport width
    postSamples.resize(MAX_PLUG_WIDTH, 0);
    monSamples.resize(MAX_PLUG_WIDTH, 0); // samples array size must be >= audio monitor width
    value = new RCSmoother();

    loadSettings();
}

TIME12AudioProcessor::~TIME12AudioProcessor()
{
    params.removeParameterListener("pattern", this);
}

void TIME12AudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "pattern") {
        int pat = (int)newValue;
        if (pat != pattern->index + 1 && pat != queuedPattern) {
            queuePattern(pat);
        }
    }
}

void TIME12AudioProcessor::parameterValueChanged (int parameterIndex, float newValue)
{
    (void)newValue;
    (void)parameterIndex;
    paramChanged = true;
}

void TIME12AudioProcessor::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    (void)parameterIndex;
    (void)gestureIsStarting;
}

void TIME12AudioProcessor::loadSettings ()
{
    settings.closeFiles(); // FIX files changed by other plugin instances not loading
    if (auto* file = settings.getUserSettings()) {
        scale = (float)file->getDoubleValue("scale", 1.0f);
        plugWidth = file->getIntValue("width", PLUG_WIDTH);
        plugHeight = file->getIntValue("height", PLUG_HEIGHT);
        auto tensionparam = (double)params.getRawParameterValue("tension")->load();
        auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
        auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();

        for (int i = 0; i < PAINT_PATS; ++i) {
            auto str = file->getValue("paintpat" + String(i),"").toStdString();
            if (!str.empty()) {
                paintPatterns[i]->clear();
                paintPatterns[i]->clearUndo();
                double x, y, tension;
                int type;
                std::istringstream iss(str);
                while (iss >> x >> y >> tension >> type) {
                    paintPatterns[i]->insertPoint(x,y,tension,type, false);
                }
                paintPatterns[i]->setTension(tensionparam, tensionatk, tensionrel, dualTension);
                paintPatterns[i]->buildSegments();
            }
        }
    }
}

void TIME12AudioProcessor::saveSettings ()
{
    settings.closeFiles(); // FIX files changed by other plugin instances not loading
    if (auto* file = settings.getUserSettings()) {
        file->setValue("scale", scale);
        file->setValue("width", plugWidth);
        file->setValue("height", plugHeight);
        for (int i = 0; i < PAINT_PATS; ++i) {
            std::ostringstream oss;
            auto points = paintPatterns[i]->points;
            for (const auto& point : points) {
                oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
            }
            file->setValue("paintpat"+juce::String(i), var(oss.str()));
        }
    }
    settings.saveIfNeeded();
}

void TIME12AudioProcessor::setScale(float s)
{
    scale = s;
    saveSettings();
}

void TIME12AudioProcessor::resizeDelays(double srate, bool clear)
{
    auto sync = (int)params.getRawParameterValue("sync")->load();
    const int size = sync == 0
        ? (int)(srate * 10)
        : (int)(syncQN * srate * 60 / tempo);

    delayL.resize(size, clear);
    delayR.resize(size, clear);

    if (sync == 0) {
        auto ratehz = (double)params.getRawParameterValue("rate")->load();
        delayL.resize((int)(srate / ratehz), clear);
        delayR.resize((int)(srate / ratehz), clear);
    }
}

int TIME12AudioProcessor::getCurrentGrid()
{
    auto gridIndex = (int)params.getRawParameterValue("grid")->load();
    return GRID_SIZES[gridIndex];
}

int TIME12AudioProcessor::getCurrentSeqStep()
{
    auto gridIndex = (int)params.getRawParameterValue("seqstep")->load();
    return GRID_SIZES[gridIndex];
}

void TIME12AudioProcessor::createUndoPoint(int patindex)
{
    if (patindex == -1) {
        viewPattern->createUndo();
    }
    else {
        if (patindex < 12) {
            patterns[patindex]->createUndo();
        }
        else {
            paintPatterns[patindex - 100]->createUndo();
        }
    }
    sendChangeMessage(); // UI repaint
}

/*
    Used to create an undo point from a previously saved state
    Assigns the snapshot points to the pattern temporarily
    Creates an undo point and finally replaces back the points
*/
void TIME12AudioProcessor::createUndoPointFromSnapshot(std::vector<PPoint> snapshot)
{
    if (!Pattern::comparePoints(snapshot, viewPattern->points)) {
        auto points = viewPattern->points;
        viewPattern->points = snapshot;
        createUndoPoint();
        viewPattern->points = points;
    }
}

void TIME12AudioProcessor::setUIMode(UIMode mode)
{
    MessageManager::callAsync([this, mode]() {
        if ((mode != Seq && mode != PaintEdit) && sequencer->isOpen) {
            sequencer->close();
        }

        if (mode == UIMode::Normal) {
            viewPattern = pattern;
            showSequencer = false;
            showPaintWidget = false;
        }
        else if (mode == UIMode::Paint) {
            viewPattern = pattern;
            showPaintWidget = true;
            showSequencer = false;
        }
        else if (mode == UIMode::PaintEdit) {
            viewPattern = paintPatterns[paintTool];
            showPaintWidget = true;
            showSequencer = false;
        }
        else if (mode == UIMode::Seq) {
            if (sequencer->isOpen) {
                sequencer->close(); // just in case its changing from PaintEdit back to sequencer
            }
            sequencer->open();
            viewPattern = pattern;
            showPaintWidget = sequencer->selectedShape == CellShape::SPTool;
            showSequencer = true;
        }
        luimode = uimode;
        uimode = mode;
        sendChangeMessage();
    });
}

void TIME12AudioProcessor::togglePaintMode()
{
    setUIMode(uimode == UIMode::Paint
        ? UIMode::Normal
        : UIMode::Paint
    );
}

void TIME12AudioProcessor::togglePaintEditMode()
{
    setUIMode(uimode == UIMode::PaintEdit
        ? luimode
        : UIMode::PaintEdit
    );
}

void TIME12AudioProcessor::toggleSequencerMode()
{
    setUIMode(uimode == UIMode::Seq
        ? UIMode::Normal
        : UIMode::Seq
    );
}

Pattern* TIME12AudioProcessor::getPaintPatern(int index)
{
    return paintPatterns[index];
}

void TIME12AudioProcessor::setViewPattern(int index)
{
    if (index >= 0 && index < 12) {
        viewPattern = patterns[index];
    }
    else if (index >= PAINT_PATS && index < PAINT_PATS_IDX + PAINT_PATS) {
        viewPattern = paintPatterns[index - PAINT_PATS_IDX];
    }
    sendChangeMessage();
}

void TIME12AudioProcessor::setPaintTool(int index)
{
    paintTool = index;
    if (uimode == UIMode::PaintEdit) {
        viewPattern = paintPatterns[index];
        sendChangeMessage();
    }
}

void TIME12AudioProcessor::restorePaintPatterns()
{
    for (int i = 0; i < 8; ++i) {
        paintPatterns[i]->clear();
        paintPatterns[i]->clearUndo();
        auto preset = Presets::getPaintPreset(i);
        for (auto& point : preset) {
            paintPatterns[i]->insertPoint(point.x, point.y, point.tension, point.type);
        }
        paintPatterns[i]->buildSegments();
    }
    sendChangeMessage();
}

void TIME12AudioProcessor::toggleShowKnobs()
{
    showKnobs = !showKnobs;
    if (showAudioKnobs && !showKnobs) {
        showAudioKnobs = false;
    }
    MessageManager::callAsync([this]() { sendChangeMessage(); });
}

void TIME12AudioProcessor::startMidiTrigger()
{
    double phase = (double)params.getRawParameterValue("phase")->load();
    clearDrawBuffers();
    midiTrigger = !alwaysPlaying;
    trigpos = 0.0;
    trigphase = phase;
    restartEnv(true);
}

TensionParameters TIME12AudioProcessor::getTensionParameters()
{

    return TensionParameters((double)params.getRawParameterValue("tension")->load(),
                             (double)params.getRawParameterValue("tensionatk")->load(),
                             (double)params.getRawParameterValue("tensionrel")->load(), dualTension);
}

//==============================================================================
const juce::String TIME12AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TIME12AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TIME12AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TIME12AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TIME12AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TIME12AudioProcessor::getNumPrograms()
{
    return 79;
}

int TIME12AudioProcessor::getCurrentProgram()
{
    return currentProgram == -1 ? 0 : currentProgram;
}

void TIME12AudioProcessor::setCurrentProgram (int index)
{
    if (currentProgram == index) return;
    loadProgram(index);
}

void TIME12AudioProcessor::loadProgram (int index)
{
    if (sequencer->isOpen)
        sequencer->close();

    currentProgram = index;
    auto loadPreset = [](Pattern& pat, int idx) {
        auto preset = Presets::getPreset(idx);
        pat.clear();
        for (auto p = preset.begin(); p < preset.end(); ++p) {
            pat.insertPoint(p->x, p->y, p->tension, p->type);
        }
        pat.buildSegments();
        pat.clearUndo();
    };

    if (index == 0) { // Init
        for (int i = 0; i < 12; ++i) {
            patterns[i]->clear();
            patterns[i]->insertPoint(0.0, 0.0, 0, 0);
            patterns[i]->insertPoint(1.0, 0.0, 0, 0);
            patterns[i]->buildSegments();
            patterns[i]->clearUndo();
        }
    }
    else if (index % 13 == 1) {
        for (int i = 0; i < 12; ++i) {
            loadPreset(*patterns[i], index + i);
        }
    }
    else {
        loadPreset(*pattern, index - 1);
    }

    setUIMode(UIMode::Normal);
    sendChangeMessage(); // UI Repaint
}

const juce::String TIME12AudioProcessor::getProgramName (int index)
{
    static const std::array<juce::String, 79> progNames = {
        "Init",
        "Stutter 1 Load All","Empty","Stutter 1","Stutter 2","Stutter 3","Stutter 4","Stutter 5","Stutter 6","Stutter 7","Stutter 8","Stutter 9","Stutter 10","Stutter 11",
        "Stutter 2 Load All","Stutter 12","Stutter 13","Stutter 14","Stutter 15","Stairs 1","Stairs 2","Stairs 3","Stairs 4","Stairs 5","Stairs 6","Stairs 7","Empty",
        "Stutter 3 Load All","Gated 1","Gated 2","Shuffle 1","Shuffle 2","Shuffle 3","Shuffle 4","Empty","Empty","Empty","Empty","Empty","Empty",
        "Basic Load All","Empty","Basic 2","Basic 3","Basic 4","Basic 5","Basic 6","Basic 7","Basic 8","Basic 9","Basic 10","Basic 11","Basic 12",
        "Complex Load All","Complex 1","Complex 2","Complex 3","Complex 4","Complex 5","Complex 6","Complex 7","Complex 8","Complex 9","Complex 10","Complex 11","Complex 12",
        "Chaos Load All","Chaos 1","Chaos 2","Chaos 3","Chaos 4","Chaos 5","Chaos 6","Chaos 7","Chaos 8","Chaos 9","Chaos 10","Chaos 11","Chaos 12",
    };
    return progNames.at(index);
}

void TIME12AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    (void)index;
    (void)newName;
}

//==============================================================================
void TIME12AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    (void)samplesPerBlock;
    updateLatency(sampleRate);
    lpFilterL.clear(0.0);
    lpFilterR.clear(0.0);
    hpFilterL.clear(0.0);
    hpFilterR.clear(0.0);
    transDetectorL.clear(sampleRate);
    transDetectorR.clear(sampleRate);
    resizeDelays(sampleRate, true);
    setAntiNoise(anoise);
    onSlider();
}

void TIME12AudioProcessor::setAntiNoise(ANoise mode)
{
    auto srate = getSampleRate();
    anoise = mode;
    ansamps = anoise == ANOff ? 0
        : anoise == ANLinear ? (int)(ANOISE_LIN_MILLIS / 1000.0 * srate)
        : anoise == ANLow ? (int)(ANOISE_LOW_MILLIS / 1000.0 * srate)
        : (int)(ANOISE_HIGH_MILLIS / 1000.0 * srate);
}

void TIME12AudioProcessor::updateLatency(double sampleRate)
{
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    int audioMillis = trigger == Trigger::Audio ? AUDIO_LATENCY_MILLIS : 0;
    setLatencySamples((int)(audioMillis / 1000.0 * sampleRate));
    clearLatencyBuffers();
}

void TIME12AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TIME12AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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

void TIME12AudioProcessor::onSlider()
{
    setSmooth();
    auto srate = getSampleRate();

    int trigger = (int)params.getRawParameterValue("trigger")->load();
    if (trigger != ltrigger) {
        updateLatency(srate);
        ltrigger = trigger;
    }
    if (trigger == Trigger::Sync && alwaysPlaying)
        alwaysPlaying = false; // force alwaysPlaying off when trigger is not MIDI or Audio

    if (trigger != Trigger::MIDI && midiTrigger)
        midiTrigger = false;

    if (trigger != Trigger::Audio && audioTrigger)
        audioTrigger = false;

    auto tension = (double)params.getRawParameterValue("tension")->load();
    auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
    auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
    if (tension != ltension || tensionatk != ltensionatk || tensionrel != ltensionrel) {
        onTensionChange();
        ltensionatk = tensionatk;
        ltensionrel = tensionrel;
        ltension = tension;
    }

    auto sync = (int)params.getRawParameterValue("sync")->load();
    if (sync == 0) syncQN = 1.; // not used
    else if (sync == 1) syncQN = 1./4.; // 1/16
    else if (sync == 2) syncQN = 1./2.; // 1/8
    else if (sync == 3) syncQN = 1./1.; // 1/4
    else if (sync == 4) syncQN = 1.*2.; // 1/2
    else if (sync == 5) syncQN = 1.*4.; // 1bar
    else if (sync == 6) syncQN = 1.*8.; // 2bar
    else if (sync == 7) syncQN = 1.*16.; // 4bar
    else if (sync == 8) syncQN = 1./6.; // 1/16t
    else if (sync == 9) syncQN = 1./3.; // 1/8t
    else if (sync == 10) syncQN = 2./3.; // 1/4t
    else if (sync == 11) syncQN = 4./3.; // 1/2t
    else if (sync == 12) syncQN = 8./3.; // 1/1t
    else if (sync == 13) syncQN = 1./4.*1.5; // 1/16.
    else if (sync == 14) syncQN = 1./2.*1.5; // 1/8.
    else if (sync == 15) syncQN = 1./1.*1.5; // 1/4.
    else if (sync == 16) syncQN = 2./1.*1.5; // 1/2.
    else if (sync == 17) syncQN = 4./1.*1.5; // 1/1.
    if (sync != lsync) {
        resizeDelays(srate, true);
        if ((sync == 0 && !showKnobs) || (sync > 0 && showKnobs && !showAudioKnobs)) {
            toggleShowKnobs();
        }
    }
    lsync = sync;

    auto highcut = (double)params.getRawParameterValue("highcut")->load();
    auto lowcut = (double)params.getRawParameterValue("lowcut")->load();
    lpFilterL.lp(srate, highcut, 0.707);
    lpFilterR.lp(srate, highcut, 0.707);
    hpFilterL.hp(srate, lowcut, 0.707);
    hpFilterR.hp(srate, lowcut, 0.707);
}

void TIME12AudioProcessor::onTensionChange()
{
    auto tension = (double)params.getRawParameterValue("tension")->load();
    auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
    auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
    pattern->setTension(tension, tensionatk, tensionrel, dualTension);
    pattern->buildSegments();
    for (int i = 0; i < PAINT_PATS; ++i) {
        paintPatterns[i]->setTension(tension, tensionatk, tensionrel, dualTension);
        paintPatterns[i]->buildSegments();
    }
}

void TIME12AudioProcessor::onPlay()
{
    clearDrawBuffers();
    clearLatencyBuffers();
    delayL.clear();
    delayR.clear();
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    double ratehz = (double)params.getRawParameterValue("rate")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();

    midiTrigger = false;
    audioTrigger = false;

    beatPos = ppqPosition;
    ratePos = beatPos * secondsPerBeat * ratehz;
    trigpos = 0.0;
    trigposSinceHit = 1.0;
    trigphase = phase;

    audioTriggerCountdown = -1;
    double srate = getSampleRate();
    transDetectorL.clear(srate);
    transDetectorR.clear(srate);

    if (trigger == 0 || alwaysPlaying) {
        restartEnv(false);
    }
}

void TIME12AudioProcessor::restartEnv(bool fromZero)
{
    int sync = (int)params.getRawParameterValue("sync")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();

    if (fromZero) { // restart from phase
        xpos = phase;
    }
    else { // restart from beat pos
        xpos = sync > 0
            ? beatPos / syncQN + phase
            : ratePos + phase;
        xpos -= std::floor(xpos);

        value->reset(getY(xpos, min, max)); // reset smooth
    }
}

void TIME12AudioProcessor::onStop()
{
    delayL.clear();
    delayR.clear();
    clearLatencyBuffers();
    if (showLatencyWarning) {
        showLatencyWarning = false;
        MessageManager::callAsync([this]() { sendChangeMessage(); });
    }
}

void TIME12AudioProcessor::clearDrawBuffers()
{
    std::fill(preSamples.begin(), preSamples.end(), 0.0);
    std::fill(postSamples.begin(), postSamples.end(), 0.0);
}

void TIME12AudioProcessor::clearLatencyBuffers()
{
    if (getLatencySamples() != latency && playing) {
        showLatencyWarning = true;
        MessageManager::callAsync([this]() { sendChangeMessage(); });
    }
    latency = getLatencySamples();
    latBufferL.resize(latency, 0.0);
    latBufferR.resize(latency, 0.0);
    latMonitorBufferL.resize(latency, 0.0);
    latMonitorBufferR.resize(latency, 0.0);
    std::fill(monSamples.begin(), monSamples.end(), 0.0);
    writepos = 0;
}

void TIME12AudioProcessor::toggleUseSidechain()
{
    useSidechain = !useSidechain;
    hpFilterL.clear(0.0);
    hpFilterR.clear(0.0);
    lpFilterL.clear(0.0);
    lpFilterR.clear(0.0);
}

void TIME12AudioProcessor::toggleMonitorSidechain()
{
    useMonitor = !useMonitor;
    hpFilterL.clear(0.0);
    hpFilterR.clear(0.0);
    lpFilterL.clear(0.0);
    lpFilterR.clear(0.0);
}

double inline TIME12AudioProcessor::getY(double x, double min, double max)
{
    return min + (max - min) * pattern->get_y_at(x);
}

void TIME12AudioProcessor::setSmooth()
{
    if (dualSmooth) {
        float attack = params.getRawParameterValue("attack")->load();
        float release = params.getRawParameterValue("release")->load();
        attack *= attack;
        release *= release;
        value->setup(attack * 0.25, release * 0.25, getSampleRate());
    }
    else {
        float lfosmooth = params.getRawParameterValue("smooth")->load();
        lfosmooth *= lfosmooth;
        value->setup(lfosmooth * 0.25, lfosmooth * 0.25, getSampleRate());
    }
}

void TIME12AudioProcessor::queuePattern(int patidx)
{
    queuedPattern = patidx;
    queuedPatternCountdown = 0;
    int patsync = (int)params.getRawParameterValue("patsync")->load();

    if (playing && patsync != PatSync::Off) {
        int interval = samplesPerBeat;
        if (patsync == PatSync::QuarterBeat)
            interval = interval / 4;
        else if (patsync == PatSync::HalfBeat)
            interval = interval / 2;
        else if (patsync == PatSync::Beat_x2)
            interval = interval * 2;
        else if (patsync == PatSync::Beat_x4)
            interval = interval * 4;
        queuedPatternCountdown = (interval - timeInSamples % interval) % interval;
    }
}

bool TIME12AudioProcessor::supportsDoublePrecisionProcessing() const
{
    return true;
}

void TIME12AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockByType(buffer, midiMessages);
}
void TIME12AudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockByType(buffer, midiMessages);
}
template <typename FloatType>
void TIME12AudioProcessor::processBlockByType (AudioBuffer<FloatType>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals disableDenormals;
    double srate = getSampleRate();
    int sblock = getBlockSize();
    bool looping = false;
    double loopStart = 0.0;
    double loopEnd = 0.0;
    if (latency != getLatencySamples()) {
        clearLatencyBuffers();
    }

    // Get playhead info
    if (auto* phead = getPlayHead()) {
        if (auto pos = phead->getPosition()) {
            if (auto tempo_ = pos->getBpm()) {
                beatsPerSecond = *tempo_ / 60.0;
                beatsPerSample = *tempo_ / (60.0 * srate);
                samplesPerBeat = (int)((60.0 / *tempo_) * srate);
                secondsPerBeat = 60.0 / *tempo_;
                tempo = *tempo_;

                if (ltempo != -1.0 && ltempo != tempo) {
                    delayL.reserve((int)srate * 10); // tempo is changing, allocate memory so resizes become cheap
                    delayR.reserve((int)srate * 10);
                    resizeDelays(srate, false);
                }
                else if (tempo != ltempo) {
                    resizeDelays(srate, false); // FIX - initial tempo only set after plugin starts
                }

                ltempo = tempo;
            }
            if (auto ppq = pos->getPpqPosition()) {
                ppqPosition = *ppq;
            }
            looping = pos->getIsLooping();
            if (auto loopPoints = pos->getLoopPoints()) {
                loopStart = loopPoints->ppqStart;
                loopEnd = loopPoints->ppqEnd;
            }
            auto play = pos->getIsPlaying();
            bool playToggle = !playing && play;
            bool stopToggle = playing && !play;
            playing = play;

            if (playToggle)
                onPlay();
            else if (stopToggle)
                onStop();

            if (playing) {
                if (auto samples = pos->getTimeInSamples()) {
                    timeInSamples = *samples;
                }
            }
        }
    }

    int inputBusCount = getBusCount(true);
    int audioOutputs = getTotalNumOutputChannels();
    int audioInputs = inputBusCount > 0 ? getChannelCountOfBus(true, 0) : 0;
    int sideInputs = inputBusCount > 1 ? getChannelCountOfBus(true, 1) : 0;

    if (!audioInputs || !audioOutputs)
        return;

    double mix = (double)params.getRawParameterValue("mix")->load();
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    int sync = (int)params.getRawParameterValue("sync")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();
    double ratehz = (double)params.getRawParameterValue("rate")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();
    double lowcut = (double)params.getRawParameterValue("lowcut")->load();
    double highcut = (double)params.getRawParameterValue("highcut")->load();
    int algo = (int)params.getRawParameterValue("algo")->load();
    double threshold = (double)params.getRawParameterValue("threshold")->load();
    double sense = 1.0 - (double)params.getRawParameterValue("sense")->load();
    sense = std::pow(sense, 2); // make sensitivity more responsive
    int numSamples = buffer.getNumSamples();

    // processes draw wave samples
    auto processDisplaySample = [&](int sampidx, double pos, double prelsamp, double prersamp) {
        auto preamp = std::max(std::fabs(prelsamp), std::fabs(prersamp));
        auto postlsamp = (double)buffer.getSample(0, sampidx);
        auto postrsamp = audioInputs > 1 ? (double)buffer.getSample(1, sampidx) : postlsamp;
        auto postamp = std::max(std::fabs(postlsamp), std::fabs(postrsamp));
        winpos = (int)std::floor(pos * viewW);
        if (lwinpos != winpos) {
            preSamples[winpos] = 0.0;
            postSamples[winpos] = 0.0;
        }
        lwinpos = winpos;
        if (preSamples[winpos] < preamp)
            preSamples[winpos] = preamp;
        if (postSamples[winpos] < postamp)
            postSamples[winpos] = postamp;
    };

    auto processEnv = [&](int sampidx, double env, double lsamp, double rsamp) {
        delayL.write(lsamp);
        delayR.write(rsamp);
        double outL, outR;

        if (lypos == ypos) {
            outL = delayL.read(1 + ypos * delayL.size);
            outR = delayR.read(1 + ypos * delayR.size);
        }
        else {
            // interpolate delay only when ypos is changing
            outL = delayL.read3(1 + ypos * delayL.size);
            outR = delayR.read3(1 + ypos * delayR.size);
        }

        // when y value jumps activate cross fade / anti-click
        if (std::fabs(ypos - lypos) > 1e-3) {
            xfade = ansamps;
            xfadepos = 1 + lypos * delayL.size;
        }

        if (xfade > 0) {
            if (anoise == ANLinear) {
                outL = outL * (ansamps - xfade) / ansamps + delayL.read3(xfadepos + ansamps - xfade) * xfade / ansamps;
                outR = outR * (ansamps - xfade) / ansamps + delayR.read3(xfadepos + ansamps - xfade) * xfade / ansamps;
            }
            else {
                double fadeOut = 0.5 * (1.0 + std::cos(MathConstants<double>::pi * xfade / ansamps));
                double fadeIn = 1.0 - fadeOut;

                outL = outL * fadeOut + delayL.read3(xfadepos + ansamps - xfade) * fadeIn;
                outR = outR * fadeOut + delayR.read3(xfadepos + ansamps - xfade) * fadeIn;
            }
            xfade -= 1;
        }

        for (int channel = 0; channel < audioOutputs; ++channel) {
            auto wet = channel == 0 ? outL : outR;
            auto dry = (double)buffer.getSample(channel, sampidx);
            if (outputCV)
                buffer.setSample(channel, sampidx, static_cast<FloatType>(env));
            else
                buffer.setSample(channel, sampidx, static_cast<FloatType>(wet * mix + dry * (1.0 - mix)));
        }

        lypos = ypos;
    };

    double monIncrementPerSample = 1.0 / ((srate * 2) / monW); // 2 seconds of audio displayed on monitor
    auto processMonitorSample = [&](double lsamp, double rsamp, bool hit) {
        double indexd = monpos.load();
        indexd += monIncrementPerSample;

        if (indexd >= monW)
            indexd -= monW;

        int index = (int)indexd;
        if (lmonpos != index)
            monSamples[index] = 0.0;
        lmonpos = index;

        double maxamp = std::max(std::fabs(lsamp), std::fabs(rsamp));
        if (hit || monSamples[index] >= 10.0)
            maxamp = std::max(maxamp + 10.0, hitamp + 10.0); // encode hits by adding +10 to amp

        monSamples[index] = std::max(monSamples[index], maxamp);
        monpos.store(indexd);
    };

    // applies envelope to a sample index
    auto applyGain = [&](int sampIdx, double env, double lsample, double rsample) {
        for (int channel = 0; channel < audioOutputs; ++channel) {
            auto wet = (channel == 0 ? lsample : rsample) * env;
            auto dry = (double)buffer.getSample(channel, sampIdx);
            if (outputCV) {
                buffer.setSample(channel, sampIdx, static_cast<FloatType>(env));
            }
            else {
                buffer.setSample(channel, sampIdx, static_cast<FloatType>(wet * mix + dry * (1.0 - mix)));
            }
        }
    };

    if (paramChanged) {
        onSlider();
        paramChanged = false;
    }

    // Process new MIDI messages
    for (const auto metadata : midiMessages) {
        juce::MidiMessage message = metadata.getMessage();
        if (message.isNoteOn() || message.isNoteOff()) {
            midiIn.push_back({ // queue midi message
                metadata.samplePosition,
                message.isNoteOn(),
                message.getNoteNumber(),
                message.getVelocity(),
                message.getChannel() - 1
            });
        }
    }

    // Process midi out queue
    for (auto it = midiOut.begin(); it != midiOut.end();) {
        auto& [msg, offset] = *it;

        if (offset < sblock) {
            midiMessages.addEvent(msg, offset);
            it = midiOut.erase(it);
        }
        else {
            offset -= sblock;
            ++it;
        }
    }

    // remove midi in messages that have been processed
    midiIn.erase(std::remove_if(midiIn.begin(), midiIn.end(), [](const MidiInMsg& msg) {
        return msg.offset < 0;
    }), midiIn.end());

    // update outputs with last envelope value at the start of the block
    if (outputCC > 0) {
        auto val = (int)std::round(ypos*127.0);
        if (bipolarCC) val -= 64;
        auto cc = MidiMessage::controllerEvent(outputCCChan + 1, outputCC-1, val);
        midiMessages.addEvent(cc, 0);
    }

    // keep beatPos in sync with playhead so plugin can be bypassed and return to its sync pos
    if (playing) {
        beatPos = ppqPosition;
        ratePos = beatPos * secondsPerBeat * ratehz;
    }

    for (int sample = 0; sample < numSamples; ++sample) {
        if (playing && looping && beatPos >= loopEnd) {
            beatPos = loopStart + (beatPos - loopEnd);
            ratePos = beatPos * secondsPerBeat * ratehz;
        }

        // process midi in queue
        for (auto& msg : midiIn) {
            if (msg.offset == 0) {
                if (msg.isNoteon) {
                    if (msg.channel == triggerChn || triggerChn == 16) {
                        auto patidx = msg.note % 12;
                        queuePattern(patidx + 1);
                    }
                    if (trigger == Trigger::MIDI && (msg.channel == midiTriggerChn || midiTriggerChn == 16)) {
                        if (queuedPattern) {
                            queuedMidiTrigger = true;
                        }
                        else {
                            startMidiTrigger();
                        }
                    }
                }
            }
            msg.offset -= 1;
        }

        // process queued pattern
        if (queuedPattern) {
            if (!playing || queuedPatternCountdown == 0) {
                if (sequencer->isOpen) {
                    sequencer->close();
                    setUIMode(UIMode::Normal);
                }
                pattern = patterns[queuedPattern - 1];
                viewPattern = pattern;
                auto tension = (double)params.getRawParameterValue("tension")->load();
                auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
                auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
                pattern->setTension(tension, tensionatk, tensionrel, dualTension);
                pattern->buildSegments();
                MessageManager::callAsync([this]() {
                    sendChangeMessage();
                });
                queuedPattern = 0;
                if (queuedMidiTrigger) {
                    queuedMidiTrigger = false;
                    startMidiTrigger();
                }
            }
            if (queuedPatternCountdown > 0) {
                queuedPatternCountdown -= 1;
            }
        }

        // Sync mode
        if (trigger == Trigger::Sync) {
            xpos = sync > 0
                ? beatPos / syncQN + phase
                : ratePos + phase;
            xpos -= std::floor(xpos);

            double newypos = getY(xpos, min, max);
            ypos = value->process(newypos, newypos > ypos);

            double lsample = (double)buffer.getSample(0, sample);
            double rsample = (double)buffer.getSample(audioInputs > 1 ? 1 : 0, sample);
            processEnv(sample, ypos, lsample, rsample);
            processDisplaySample(sample, xpos, lsample, rsample);
        }

        // MIDI mode
        else if (trigger == Trigger::MIDI) {
            auto inc = sync > 0
                ? beatsPerSample / syncQN
                : 1 / srate * ratehz;
            xpos += inc;
            trigpos += inc;
            xpos -= std::floor(xpos);

            if (!alwaysPlaying) {
                if (midiTrigger) {
                    if (trigpos >= 1.0) { // envelope finished, stop midiTrigger
                        midiTrigger = false;
                        xpos = phase ? phase : 1.0;
                    }
                }
                else {
                    xpos = phase ? phase : 1.0; // midiTrigger is stopped, hold last position
                }
            }

            double newypos = getY(xpos, min, max);
            ypos = value->process(newypos, newypos > ypos);

            double lsample = (double)buffer.getSample(0, sample);
            double rsample = (double)buffer.getSample(audioInputs > 1 ? 1 : 0, sample);
            double viewpos = (alwaysPlaying || midiTrigger) ? xpos
                : (trigpos + trigphase) - std::floor(trigpos + trigphase);

            processEnv(sample, ypos, lsample, rsample);
            processDisplaySample(sample, viewpos, lsample, rsample);
        }

        // Audio mode
        else if (trigger == Trigger::Audio) {
            // process latency buffers
            latBufferL[writepos] = (double)buffer.getSample(0, sample);
            latBufferR[writepos] = (double)buffer.getSample(audioInputs > 1 ? 1 : 0, sample);
            readpos = latency == 0 ? latency : (writepos + 1) % latency;
            double lsample = latBufferL[readpos]; // delayed sample
            double rsample = latBufferR[readpos]; // delayed sample

            // read sidechain samples
            double lrawsample = (double)buffer.getSample(0, sample);
            double rrawsample = (double)buffer.getSample(audioInputs > 1 ? 1 : 0, sample);
            if (useSidechain && sideInputs) {
                lrawsample = (double)buffer.getSample(audioInputs, sample);
                rrawsample = (double)buffer.getSample(sideInputs > 1 ? audioInputs + 1 : audioInputs, sample);
            }

            // Detect audio transients
            auto monSampleL = lrawsample;
            auto monSampleR = rrawsample;
            if (lowcut > 20.0) {
                monSampleL = hpFilterL.df1(monSampleL);
                monSampleR = hpFilterR.df1(monSampleR);
            }
            if (highcut < 20000.0) {
                monSampleL = lpFilterL.df1(monSampleL);
                monSampleR = lpFilterR.df1(monSampleR);
            }
            latMonitorBufferL[writepos] = monSampleL;
            latMonitorBufferR[writepos] = monSampleR;

            if (transDetectorL.detect(algo, monSampleL, threshold, sense) ||
                transDetectorR.detect(algo, monSampleR, threshold, sense))
            {
                transDetectorL.startCooldown();
                transDetectorR.startCooldown();
                int offset = (int)(params.getRawParameterValue("offset")->load() * AUDIO_LATENCY_MILLIS / 1000.f * srate);
                audioTriggerCountdown = std::max(0, latency + offset);
                hitamp = transDetectorL.hit ? std::fabs(monSampleL) : std::fabs(monSampleR);
            }
            auto hit = audioTriggerCountdown == 0; // there was an audio transient trigger in this sample

            // read the monitor sample 'latency' samples ago
            monSampleL = latMonitorBufferL[readpos];
            monSampleR = latMonitorBufferR[readpos];
            processMonitorSample(monSampleL, monSampleR, hit);

            // envelope processing
            auto inc = sync > 0
                ? beatsPerSample / syncQN
                : 1 / srate * ratehz;
            xpos += inc;
            trigpos += inc;
            trigposSinceHit += inc;
            xpos -= std::floor(xpos);

            // send output midi notes on audio trigger hit
            if (hit && outputATMIDI > 0) {
                auto noteOn = MidiMessage::noteOn(1, outputATMIDI - 1, (float)hitamp);
                midiMessages.addEvent(noteOn, sample);

                auto offnoteDelay = static_cast<int>(srate * AUDIO_NOTE_LENGTH_MILLIS / 1000.0);
                int noteOffSample = sample + offnoteDelay;
                auto noteOff = MidiMessage::noteOff(1, outputATMIDI - 1);

                if (noteOffSample < sblock) {
                    midiMessages.addEvent(noteOff, noteOffSample);
                }
                else {
                    int offset = noteOffSample - sblock;
                    midiOut.push_back({ noteOff, offset });
                }
            }

            if (hit && (alwaysPlaying || !audioIgnoreHitsWhilePlaying || trigposSinceHit > 0.98)) {
                clearDrawBuffers();
                audioTrigger = !alwaysPlaying;
                trigpos = 0.0;
                trigphase = phase;
                trigposSinceHit = 0.0;
                restartEnv(true);
            }

            if (!alwaysPlaying) {
                if (audioTrigger) {
                    if (trigpos >= 1.0) { // envelope finished, stop trigger
                        audioTrigger = false;
                        xpos = phase ? phase : 1.0;
                    }
                }
                else {
                    xpos = phase ? phase : 1.0; // audioTrigger is stopped, hold last position
                }
            }

            double newypos = getY(xpos, min, max);
            ypos = value->process(newypos, newypos > ypos);

            if (useMonitor) {
                for (int channel = 0; channel < audioOutputs; ++channel) {
                    buffer.setSample(channel, sample, static_cast<FloatType>(channel == 0 ? monSampleL : monSampleR));
                }
            }
            else {
                processEnv(sample, ypos, lsample, rsample);
            }

            auto viewpos = (alwaysPlaying || audioTrigger) ? xpos
                : (trigpos + trigphase) - std::floor(trigpos + trigphase);
            processDisplaySample(sample, viewpos, lsample, rsample);

            if (audioTriggerCountdown > -1)
                audioTriggerCountdown -= 1;

            writepos = latency == 0 ? 0 : (writepos + 1) % latency;
        }

        xenv.store(xpos);
        yenv.store(1.0-ypos);
        beatPos += beatsPerSample;
        ratePos += 1 / srate * ratehz;
        if (playing)
            timeInSamples += 1;
    }
    drawSeek.store(playing && (trigger == Trigger::Sync || midiTrigger || audioTrigger));
}

//==============================================================================
bool TIME12AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TIME12AudioProcessor::createEditor()
{
    return new TIME12AudioProcessorEditor (*this);
}

//==============================================================================
void TIME12AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = ValueTree("PluginState");
    state.appendChild(params.copyState(), nullptr);
    state.setProperty("version", PROJECT_VERSION, nullptr);
    state.setProperty("currentProgram", currentProgram, nullptr);
    state.setProperty("alwaysPlaying",alwaysPlaying, nullptr);
    state.setProperty("dualSmooth",dualSmooth, nullptr);
    state.setProperty("dualTension",dualTension, nullptr);
    state.setProperty("triggerChn",triggerChn, nullptr);
    state.setProperty("useMonitor",useMonitor, nullptr);
    state.setProperty("useSidechain",useSidechain, nullptr);
    state.setProperty("outputCC", outputCC, nullptr);
    state.setProperty("outputCCChan", outputCCChan, nullptr);
    state.setProperty("outputCV", outputCV, nullptr);
    state.setProperty("outputATMIDI", outputATMIDI, nullptr);
    state.setProperty("bipolarCC", bipolarCC, nullptr);
    state.setProperty("paintTool", paintTool, nullptr);
    state.setProperty("paintPage", paintPage, nullptr);
    state.setProperty("pointMode", pointMode, nullptr);
    state.setProperty("anoise", anoise, nullptr);
    state.setProperty("audioIgnoreHitsWhilePlaying", audioIgnoreHitsWhilePlaying, nullptr);
    state.setProperty("linkSeqToGrid", linkSeqToGrid, nullptr);
    state.setProperty("currpattern", pattern->index + 1, nullptr);
    state.setProperty("midiTriggerChn", midiTriggerChn, nullptr);


    for (int i = 0; i < 12; ++i) {
        std::ostringstream oss;
        auto points = patterns[i]->points;

        if (sequencer->isOpen && i == sequencer->patternIdx) {
            points = sequencer->backup;
        }

        for (const auto& point : points) {
            oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
        }
        state.setProperty("pattern" + juce::String(i), var(oss.str()), nullptr);
    }

    // serialize sequencer cells
    std::ostringstream oss;
    for (const auto& cell : sequencer->cells) {
        oss << cell.shape << ' '
            << cell.lshape << ' '
            << cell.ptool << ' '
            << cell.invertx << ' '
            << cell.minx << ' '
            << cell.maxx << ' '
            << cell.miny << ' '
            << cell.maxy << ' '
            << cell.tenatt << ' '
            << cell.tenrel << ' '
            << cell.skew << '\n';
    }
    state.setProperty("seqcells", var(oss.str()), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TIME12AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (sequencer->isOpen)
        sequencer->close();

    std::unique_ptr<juce::XmlElement>xmlState (getXmlFromBinary (data, sizeInBytes));
    if (!xmlState) return;
    auto state = ValueTree::fromXml (*xmlState);
    if (!state.isValid()) return;

    params.replaceState(state.getChild(0));
    if (state.hasProperty("version")) {
        currentProgram = (int)state.getProperty("currentProgram");
        alwaysPlaying = (bool)state.getProperty("alwaysPlaying");
        dualSmooth = (bool)state.getProperty("dualSmooth");
        dualTension = (bool)state.getProperty("dualTension");
        triggerChn = (int)state.getProperty("triggerChn");
        useMonitor = (bool)state.getProperty("useMonitor");
        useSidechain = (bool)state.getProperty("useSidechain");
        outputCC = (int)state.getProperty("outputCC");
        outputCCChan = (int)state.getProperty("outputCCChan");
        bipolarCC = (bool)state.getProperty("bipolarCC");
        outputCV = (bool)state.getProperty("outputCV");
        outputATMIDI = (int)state.getProperty("outputATMIDI");
        paintTool = (int)state.getProperty("paintTool");
        paintPage = (int)state.getProperty("paintPage");
        pointMode = state.hasProperty("pointMode") ? (int)state.getProperty("pointMode") : 0;
        audioIgnoreHitsWhilePlaying = (bool)state.getProperty("audioIgnoreHitsWhilePlaying");
        anoise = state.hasProperty("anoise") ? (ANoise)(int)state.getProperty("anoise") : anoise;
        linkSeqToGrid = state.hasProperty("linkSeqToGrid") ? (bool)state.getProperty("linkSeqToGrid") : true;
        midiTriggerChn = (int)state.getProperty("midiTriggerChn");

        for (int i = 0; i < 12; ++i) {
            patterns[i]->clear();
            patterns[i]->clearUndo();

            auto str = state.getProperty("pattern" + String(i)).toString().toStdString();
            if (!str.empty()) {
                double x, y, tension;
                int type;
                std::istringstream iss(str);
                while (iss >> x >> y >> tension >> type) {
                    patterns[i]->insertPoint(x,y,tension,type, false);
                }
            }

            auto tension = (double)params.getRawParameterValue("tension")->load();
            auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
            auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
            patterns[i]->setTension(tension, tensionatk, tensionrel, dualTension);
            patterns[i]->buildSegments();
        }

        if (state.hasProperty("seqcells")) {
            auto str = state.getProperty("seqcells").toString().toStdString();
            sequencer->cells.clear();
            std::istringstream iss(str);
            Cell cell;
            int shape, lshape;
            while (iss >> shape >> lshape >> cell.ptool >> cell.invertx
                >> cell.minx >> cell.maxx >> cell.miny >> cell.maxy >> cell.tenatt
                >> cell.skew >> cell.tenrel)
            {
                cell.shape = static_cast<CellShape>(shape);
                cell.lshape = static_cast<CellShape>(lshape);
                sequencer->cells.push_back(cell);
            }
        }

        int currpattern = 1;
        if (!state.hasProperty("currpattern")) {
            currpattern = (int)params.getRawParameterValue("pattern")->load();
        }
        else {
            currpattern = state.getProperty("currpattern");
        }
        queuePattern(currpattern);
        auto param = params.getParameter("pattern");
        param->setValueNotifyingHost(param->convertTo0to1((float)currpattern));
    }

    setAntiNoise(anoise);
    setUIMode(UIMode::Normal);
}

void TIME12AudioProcessor::importPatterns() 
{
    if (sequencer->isOpen)
        sequencer->close();
    patternManager.importPatterns(patterns,getTensionParameters());
    setUIMode(UIMode::Normal);
}

void TIME12AudioProcessor::exportPatterns()
{
    if (sequencer->isOpen)
        sequencer->close();
    patternManager.exportPatterns(patterns);
    setUIMode(UIMode::Normal);
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TIME12AudioProcessor();
}
