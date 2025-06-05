#include "SettingsButton.h"
#include "../PluginProcessor.h"
#include "../Globals.h"

void SettingsButton::paint(Graphics& g) 
{
	auto r = 1.5f;
	auto bounds = getLocalBounds().expanded(-2,-4).toFloat();
	g.setColour(Colour(globals::COLOR_ACTIVE));
	g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), r * 2, 2.f);
	g.fillRoundedRectangle(bounds.getX(), bounds.getCentreY() - r, bounds.getWidth(), r*2, 2.f);
	g.fillRoundedRectangle(bounds.getX(), bounds.getBottom() - r*2, bounds.getWidth(), r*2, 2.f);
};

void SettingsButton::mouseDown(const juce::MouseEvent& e)
{
	(void)e;

	PopupMenu uiScale;
	uiScale.addItem(1, "100%", true, audioProcessor.scale == 1.0f);
	uiScale.addItem(2, "125%", true, audioProcessor.scale == 1.25f);
	uiScale.addItem(3, "150%", true, audioProcessor.scale == 1.5f);
	uiScale.addItem(4, "175%", true, audioProcessor.scale == 1.75f);
	uiScale.addItem(5, "200%", true, audioProcessor.scale == 2.0f);

	PopupMenu midiTriggerChn;
	midiTriggerChn.addItem(3010, "Off", true, audioProcessor.midiTriggerChn == -1);
	for (int i = 0; i < 16; i++) {
		midiTriggerChn.addItem(3010 + i + 1, String(i + 1), true, audioProcessor.midiTriggerChn == i);
	}
	midiTriggerChn.addItem(3027, "Any", true, audioProcessor.midiTriggerChn == 16);

	PopupMenu triggerChn;
	triggerChn.addItem(10, "Off", true, audioProcessor.triggerChn == -1);
	for (int i = 0; i < 16; i++) {
		triggerChn.addItem(10 + i + 1, String(i + 1), true, audioProcessor.triggerChn == i);
	}
	triggerChn.addItem(27, "Any", true, audioProcessor.triggerChn == 16);

	PopupMenu audioTrigger;
	audioTrigger.addItem(32, "Ignore hits while playing", true, audioProcessor.audioIgnoreHitsWhilePlaying);

	PopupMenu CC;
	CC.addItem(300, "Off", true, audioProcessor.outputCC == 0);
	CC.addSeparator();
	for (int i = 1; i <= 128; ++i) {
		CC.addItem(300 + i,  String(i - 1), true, audioProcessor.outputCC == i);
	}

	PopupMenu CCChan;
	for (int i = 0; i < 16; ++i) {
		CCChan.addItem(450 + i, String(i+1), true, audioProcessor.outputCCChan == i);
	}

	auto midiNoteToName = [](int noteNumber) -> std::string {
		const std::string noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
		int octave = (noteNumber / 12) - 1;
		std::string noteName = noteNames[noteNumber % 12];
		return std::to_string(noteNumber) + " " + noteName + std::to_string(octave);
	};

	PopupMenu audioOutputMIDI;
	audioOutputMIDI.addItem(500, "Off", true, audioProcessor.outputATMIDI == 0);
	audioOutputMIDI.addSeparator();
	for (int i = 1; i < 129; ++i) {
		audioOutputMIDI.addItem(500+i, midiNoteToName(i-1), true, audioProcessor.outputATMIDI == i);
	}

	PopupMenu output;
	output.addItem(700, "CV", true, audioProcessor.outputCV);
	output.addSubMenu("CC", CC);
	output.addSubMenu("CC Channel", CCChan);
	output.addSubMenu("Audio Trig. MIDI", audioOutputMIDI);
	output.addSeparator();
	output.addItem(701, "Bipolar CC", true, audioProcessor.bipolarCC);

	PopupMenu antiNoise;
	antiNoise.addItem(710, "Off", true, audioProcessor.anoise == ANoise::ANOff);
	antiNoise.addItem(713, "Linear", true, audioProcessor.anoise == ANoise::ANLinear);
	antiNoise.addItem(711, "Normal", true, audioProcessor.anoise == ANoise::ANLow);
	antiNoise.addItem(712, "High", true, audioProcessor.anoise == ANoise::ANHigh);

	PopupMenu options;
	options.addSubMenu("Anti-noise", antiNoise);
	options.addSubMenu("Output", output);
	options.addSubMenu("MIDI trigger chn", midiTriggerChn);
	options.addSubMenu("Pattern select chn", triggerChn);
	options.addSubMenu("Audio trigger", audioTrigger);
	options.addSeparator();
	options.addItem(30, "Dual smooth", true, audioProcessor.dualSmooth);
	options.addItem(31, "Dual tension", true, audioProcessor.dualTension);


	PopupMenu load;
	load.addItem(100, "Sine", audioProcessor.uimode != UIMode::Seq);
	load.addItem(101, "Triangle", audioProcessor.uimode != UIMode::Seq);
	load.addItem(102, "Random", audioProcessor.uimode != UIMode::Seq);
	load.addSeparator();
	load.addItem(109, "Init");

	PopupMenu stutter1;
	PopupMenu stutter2;
	PopupMenu stutter3;
	PopupMenu pattern;
	PopupMenu complex;
	PopupMenu chaos;

	stutter1.addItem(2000, "Load All");
	stutter1.addSeparator();
	stutter1.addItem(2001, "Empty");
	stutter1.addItem(2002, "Stutter 1");
	stutter1.addItem(2003, "Stutter 2");
	stutter1.addItem(2004, "Stutter 3");
	stutter1.addItem(2005, "Stutter 4");
	stutter1.addItem(2006, "Stutter 5");
	stutter1.addItem(2007, "Stutter 6");
	stutter1.addItem(2008, "Stutter 7");
	stutter1.addItem(2009, "Stutter 8");
	stutter1.addItem(2010, "Stutter 9");
	stutter1.addItem(2011, "Stutter 10");
	stutter1.addItem(2012, "Stutter 11");

	stutter2.addItem(2013, "Load All");
	stutter2.addSeparator();
	stutter2.addItem(2014, "Stutter 12");
	stutter2.addItem(2015, "Stutter 13");
	stutter2.addItem(2016, "Stutter 14");
	stutter2.addItem(2017, "Stutter 15");
	stutter2.addItem(2018, "Stairs 1");
	stutter2.addItem(2019, "Stairs 2");
	stutter2.addItem(2020, "Stairs 3");
	stutter2.addItem(2021, "Stairs 4");
	stutter2.addItem(2022, "Stairs 5");
	stutter2.addItem(2023, "Stairs 6");
	stutter2.addItem(2024, "Stairs 7");
	stutter2.addItem(2025, "Empty");

	stutter3.addItem(2026, "Load All");
	stutter3.addSeparator();
	stutter3.addItem(2027, "Gated 1");
	stutter3.addItem(2028, "Gated 2");
	stutter3.addItem(2029, "Shuffle 1");
	stutter3.addItem(2030, "Shuffle 2");
	stutter3.addItem(2031, "Shuffle 3");
	stutter3.addItem(2032, "Shuffle 4");
	stutter3.addItem(2033, "Empty");
	stutter3.addItem(2034, "Empty");
	stutter3.addItem(2035, "Empty");
	stutter3.addItem(2036, "Empty");
	stutter3.addItem(2037, "Empty");
	stutter3.addItem(2038, "Empty");

	pattern.addItem(2039, "Load All");
	pattern.addSeparator();
	pattern.addItem(2040, "Basic 1");
	pattern.addItem(2041, "Basic 2");
	pattern.addItem(2042, "Basic 3");
	pattern.addItem(2043, "Basic 4");
	pattern.addItem(2044, "Basic 5");
	pattern.addItem(2045, "Basic 6");
	pattern.addItem(2046, "Basic 7");
	pattern.addItem(2047, "Basic 8");
	pattern.addItem(2048, "Basic 9");
	pattern.addItem(2049, "Basic 10");
	pattern.addItem(2050, "Basic 11");
	pattern.addItem(2051, "Basic 12");

	complex.addItem(2052, "Load All");
	complex.addSeparator();
	complex.addItem(2053, "Complex 1");
	complex.addItem(2054, "Complex 2");
	complex.addItem(2055, "Complex 3");
	complex.addItem(2056, "Complex 4");
	complex.addItem(2057, "Complex 5");
	complex.addItem(2058, "Complex 6");
	complex.addItem(2059, "Complex 7");
	complex.addItem(2060, "Complex 8");
	complex.addItem(2061, "Complex 9");
	complex.addItem(2062, "Complex 10");
	complex.addItem(2063, "Complex 11");
	complex.addItem(2064, "Complex 12");

	chaos.addItem(2065, "Load All");
	chaos.addSeparator();
	chaos.addItem(2066, "Chaos 1");
	chaos.addItem(2067, "Chaos 2");
	chaos.addItem(2068, "Chaos 3");
	chaos.addItem(2069, "Chaos 4");
	chaos.addItem(2070, "Chaos 5");
	chaos.addItem(2071, "Chaos 6");
	chaos.addItem(2072, "Chaos 7");
	chaos.addItem(2073, "Chaos 8");
	chaos.addItem(2074, "Chaos 9");
	chaos.addItem(2075, "Chaos 10");
	chaos.addItem(2076, "Chaos 11");
	chaos.addItem(2077, "Chaos 12");

	PopupMenu loadOther;
	loadOther.addItem(150, "Restore paint patterns");

	load.addSubMenu("Stutter 1", stutter1);
	load.addSubMenu("Stutter 2", stutter2);
	load.addSubMenu("Stutter 3", stutter3);
	load.addSubMenu("Pattern", pattern);
	load.addSubMenu("Complex", complex);
	load.addSubMenu("Chaos", chaos);
	load.addSeparator();
	load.addSubMenu("Other", loadOther);
	

	PopupMenu menu;
	auto menuPos = localPointToGlobal(getLocalBounds().getBottomRight());
	menu.addSubMenu("UI Scale", uiScale);
	menu.addSubMenu("Options", options);
	menu.addSeparator();
	menu.addItem(53, "Copy", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(54, "Paste", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(55, "Invert", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(56, "Reverse", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(57, "Double");
	menu.addItem(52, audioProcessor.uimode == UIMode::Seq ? "Reset" : "Clear");
	menu.addSeparator();
	menu.addSubMenu("Load", load);
	menu.addItem(1000, "About");
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetScreenArea({menuPos.getX() -110, menuPos.getY(), 1, 1}),
		[this](int result) {
			if (result == 0) return;
			else if (result >= 1 && result <= 5) { // UI Scale
				audioProcessor.setScale(result == 5 ? 2.0f : result == 4 ? 1.75f : result == 3 ? 1.5f : result == 2 ? 1.25f : 1.0f);
				onScaleChange();
			}
			else if (result >= 3010 && result <= 3027) {
				audioProcessor.midiTriggerChn = result - 3010 - 1;
			}
			else if (result >= 10 && result <= 27) { // Trigger channel
				audioProcessor.triggerChn = result - 10 - 1;
			}
			else if (result == 30) { // Dual smooth
				audioProcessor.dualSmooth = !audioProcessor.dualSmooth;
				toggleUIComponents();
			}
			else if (result == 31) { // Dual tension
				MessageManager::callAsync([this]() {
					audioProcessor.dualTension = !audioProcessor.dualTension;
					audioProcessor.onTensionChange();
					toggleUIComponents();
				});
			}
			else if (result == 32) {
				MessageManager::callAsync([this]() {
					audioProcessor.audioIgnoreHitsWhilePlaying = !audioProcessor.audioIgnoreHitsWhilePlaying;
				});
			}
			else if (result == 52) {
				if (audioProcessor.uimode == UIMode::Seq) {
					auto snap = audioProcessor.sequencer->cells;
					audioProcessor.sequencer->clear();
					audioProcessor.sequencer->createUndo(snap);
					audioProcessor.sequencer->build();
				}
				else {
					auto snapshot = audioProcessor.viewPattern->points;
					audioProcessor.viewPattern->clear();
					audioProcessor.viewPattern->buildSegments();
					audioProcessor.createUndoPointFromSnapshot(snapshot);
				}
			}
			else if (result == 53) {
				audioProcessor.viewPattern->copy();
			}
			else if (result == 54) {
				auto snapshot = audioProcessor.viewPattern->points;
				audioProcessor.viewPattern->paste();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			else if (result == 55) {
				auto snapshot = audioProcessor.viewPattern->points;
				audioProcessor.viewPattern->invert();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			else if (result == 56) {
				auto snapshot = audioProcessor.viewPattern->points;
				audioProcessor.viewPattern->reverse();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			else if (result == 57) {
				if (audioProcessor.uimode == UIMode::Seq) {
					audioProcessor.sequencer->doublePattern();
				}
				else {
					auto snapshot = audioProcessor.viewPattern->points;
					audioProcessor.viewPattern->doublePattern();
					audioProcessor.viewPattern->buildSegments();
					audioProcessor.createUndoPointFromSnapshot(snapshot);
				}
			}
			else if (result == 109) {
				audioProcessor.loadProgram(0);
			}
			else if (result >= 100 && result <= 200) { // load
				if (result == 100) { // load sine
					audioProcessor.viewPattern->loadSine();
					audioProcessor.viewPattern->buildSegments();
				}
				if (result == 101) { // load triangle
					audioProcessor.viewPattern->loadTriangle();
					audioProcessor.viewPattern->buildSegments();
				}
				if (result == 102) { // load random
					int grid = audioProcessor.getCurrentGrid();
					audioProcessor.viewPattern->loadRandom(grid);
					audioProcessor.viewPattern->buildSegments();
				}
				if (result == 150) {
					audioProcessor.restorePaintPatterns();
				}
			}
			else if (result >= 2000 && result < 2100) {
				MessageManager::callAsync([this, result]() {
					audioProcessor.loadProgram(result-2000+1);
				});
			}
			// output cc channel
			else if (result >= 300 && result <= 300 + 129) {
				audioProcessor.outputCC = result - 300;
			}
			else if (result >= 450 && result <= 450 + 16) {
				audioProcessor.outputCCChan = result - 450;
			}
			// output audio trigger midi note
			else if (result >= 500 && result <= 500 + 129) {
				audioProcessor.outputATMIDI = result - 500;
			}
			// output options
			else if (result == 700) { 
				audioProcessor.outputCV = !audioProcessor.outputCV;
			}
			else if (result == 701) {
				audioProcessor.bipolarCC = !audioProcessor.bipolarCC;
			}
			else if (result >= 710 && result <= 713) {
				auto anoise = (ANoise)(result - 710);
				MessageManager::callAsync([this, anoise]() {
					audioProcessor.setAntiNoise(anoise);
				});
			}
			else if (result == 1000) {
				toggleAbout();
			}
		}
	);
};

