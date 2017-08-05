// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "PitchTracker.h"
#include "CoreMisc.h"


using namespace std;

PitchTracker::PitchTracker()
{
	initPitchTable();
}

PitchTracker::~PitchTracker()
{
}

bool PitchTracker::trackNewNote(float freq)
{
	if (freq > pitchTable[notesToRecognize - 1].getFrequency() || freq < pitchTable[0].getFrequency()) {
		return false;
	} else {
		int left = 0;
		int right = notesToRecognize - 1;
		float freqLeft = freq - freq * (freqMultiplier / 2);
		float freqRight = freq + freq * (freqMultiplier / 2);
		int noteIndex = findPitchByFrequency(left, right, freqLeft, freqRight);
		if (noteIndex != -1) {
			FString trackedNote = FString(pitchTable[noteIndex].getName().c_str());
			UE_LOG(LogTemp, Log, TEXT("TRACKED NOTE: %s"), *trackedNote);
			return true;
		}
		return false;
	}
}

void PitchTracker::initPitchTable()
{
	double noteFreq = 16.3516;
	string toneNames[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","H"};
	for (int i = 0; i < notesToRecognize; i++) {
		for (int octave = 0; octave < octavesToRecognize; octave++) {
			string name = toneNames[i % 12] + to_string(octave);
			pitchTable[i] = Pitch(name, noteFreq, octave);
			noteFreq *= freqMultiplier;
			FString test = FString(name.c_str());
			float test2 = noteFreq;
			UE_LOG(LogTemp, Log, TEXT("My note: %s"), *test);
			UE_LOG(LogTemp, Log, TEXT("My note: %f"), test2);
		}
	}
}

int PitchTracker::findPitchByFrequency(int left, int right, int freqLeft, int freqRight)
{
	if (left > right) {
		return -1;
	}

	int middle = (left + right) / 2;

	if (freqLeft < pitchTable[middle].getFrequency() && freqRight > pitchTable[middle].getFrequency()) {
		return middle;
	}
	if (freqLeft < pitchTable[middle].getFrequency()) {
		return findPitchByFrequency(left, middle - 1, freqLeft, freqRight);
	}
	else {
		return findPitchByFrequency(middle + 1, right, freqLeft, freqRight);
	}
	return -1;
}
