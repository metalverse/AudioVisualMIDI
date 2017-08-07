#pragma once
#include "Pitch.h"
#include <string>
#include <vector>


class MIDI_PROJECT_API PitchTracker
{
public:
	PitchTracker();
	~PitchTracker();
	bool trackNewNote(float freq);
	Pitch lastTrackedNote;

private:
	static const int octavesToRecognize = 9;
	static const int notesToRecognize = octavesToRecognize * 12;
	const double freqHalfToneMultiplier = 1.05946309436;
	Pitch pitchTable[notesToRecognize];
	void initPitchTable();
	int findPitchByFrequency(int left, int right, int freq);
};
