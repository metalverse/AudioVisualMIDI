// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SimplePitchTracker.h"


using namespace std;

USimplePitchTracker::USimplePitchTracker()
{
	initPitchTable();
	lastTrackedNote = new USimplePitch();
}

USimplePitchTracker::~USimplePitchTracker()
{
	delete lastTrackedNote;
	delete [] pitchTable;
}

void USimplePitchTracker::initPitchTable()
{
	pitchTable = new USimplePitch*[notesToRecognize];
	float noteFreq = 16.3516;
	string toneNames[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","H" };
	for (int octave = 0; octave < octavesToRecognize; octave++) {
		for (int note = 0; note < 12; note++) {
			string name = toneNames[note] + to_string(octave);
			pitchTable[octave * 12 + note] = new USimplePitch(name, noteFreq, octave);
			FString test = FString(name.c_str());
			float test2 = noteFreq;
			UE_LOG(LogTemp, Log, TEXT("My note: %s"), *test);
			UE_LOG(LogTemp, Log, TEXT("My note: %f"), noteFreq);
			noteFreq *= freqHalfToneMultiplier;
		}
	}
}

bool USimplePitchTracker::trackNewNote(float freq)
{
	if (freq > pitchTable[notesToRecognize - 1]->getFrequency() || freq < pitchTable[0]->getFrequency()) {
		return false;
	}
	else {
		int left = 0;
		int right = notesToRecognize - 1;

		UE_LOG(LogTemp, Log, TEXT("FREQ: %f"), freq);

		int noteIndex = findPitchByFrequency(left, right, freq);
		if (noteIndex != -1) {
			FString noteName = FString(pitchTable[noteIndex]->getName().c_str());
			lastTrackedNote = pitchTable[noteIndex];
			UE_LOG(LogTemp, Log, TEXT("TRACKED NOTE: %s"), *noteName);
			UE_LOG(LogTemp, Log, TEXT("FREQ: %f"), pitchTable[noteIndex]->getFrequency());
			return true;
		}
		return false;
	}
}

int USimplePitchTracker::findPitchByFrequency(int left, int right, int freq)
{
	if (left > right) {
		return -1;
	}
	int middle = (left + right) / 2;
	float tmpFreq = pitchTable[middle]->getFrequency();
	float freqLeft = (tmpFreq + tmpFreq * (1.0 / freqHalfToneMultiplier)) / 2;
	float freqRight = (tmpFreq + tmpFreq * freqHalfToneMultiplier) / 2;

	if (freq > freqLeft && freq < freqRight) {
		UE_LOG(LogTemp, Log, TEXT("FREQ L: %f"), freqLeft);
		UE_LOG(LogTemp, Log, TEXT("FREQ R: %f"), freqRight);
		return middle;
	}
	if (freq < tmpFreq) {
		return findPitchByFrequency(left, middle - 1, freq);
	}
	else {
		return findPitchByFrequency(middle + 1, right, freq);
	}
	return -1;
}


