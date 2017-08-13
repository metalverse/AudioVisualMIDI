// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SimplePitchTracker.h"


using namespace std;

USimplePitchTracker::USimplePitchTracker(const FObjectInitializer& ObjectInitializer)
{
	trackedPitches.Empty();
	//pitchTable.Init(ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, TEXT("NewPitch")), notesToRecognize);
	initPitchTable(ObjectInitializer);
	currentNote = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, TEXT("currentPitch"));
	currentNote->setParams("None", 0, 0, 0);
	/*for (int i = 0; i < notesToRecognize; i++) {
		UE_LOG(LogTemp, Log, TEXT("My note FREQ: %f"), pitchTable[i]->getFrequency());
	}*/
}

USimplePitchTracker::~USimplePitchTracker()
{
	/*for (int i = 0; i < notesToRecognize; i++) {
		deleteSimplePitchObject(trackedPitches.Pop());
	}*/
}

void USimplePitchTracker::initPitchTable(const FObjectInitializer& ObjectInitializer)
{
	float noteFreq = 16.3516;
	FString toneNames[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","H" };
	for (int octave = 0; octave < octavesToRecognize; octave++) {
		for (int note = 0; note < 12; note++) {
			FString name = toneNames[note] + FString::FromInt(octave);
			USimplePitch* pitch = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, FName(*name));
			pitch->setParams(name, noteFreq, octave, 0);
			pitchTable.Add(pitch);
			float test2 = noteFreq;
			UE_LOG(LogTemp, Log, TEXT("My note: %s"), *name);
			UE_LOG(LogTemp, Log, TEXT("My note: %f"), noteFreq);
			noteFreq *= freqHalfToneMultiplier;
		}
	}
}


bool USimplePitchTracker::trackNewNote(float freq)
{
	for (int i = 0; i < notesToRecognize; i++) {
		UE_LOG(LogTemp, Log, TEXT("My note FREQ+++: %f"), pitchTable[i]->getFrequency());
	}
	if (freq > pitchTable[notesToRecognize - 1]->getFrequency() || freq < pitchTable[0]->getFrequency()) {
		UE_LOG(LogTemp, Log, TEXT("STH WENT WRONG"));
		UE_LOG(LogTemp, Log, TEXT("MAX: %f "), pitchTable[notesToRecognize - 1]->getFrequency());
		UE_LOG(LogTemp, Log, TEXT("MIN: %f "), pitchTable[0]->getFrequency());
		UE_LOG(LogTemp, Log, TEXT("FREQ: %f "), freq);
		return false;
	}
	else {
		int left = 0;
		int right = notesToRecognize - 1;

		UE_LOG(LogTemp, Log, TEXT("FREQ: %f"), freq);

		int noteIndex = findPitchByFrequency(left, right, freq);
		if (noteIndex != -1) {
			FString noteName = FString(pitchTable[noteIndex]->getName());
			lastTrackedNote = currentNote;
			currentNote = pitchTable[noteIndex];
			if (currentNote == lastTrackedNote) {
				//trackedPitches.Last()->incrementTime(1);
			} else {
				USimplePitch* pitch = NewObject<USimplePitch>(this);
				pitch->setParams(currentNote->getName(), currentNote->getFrequency(), currentNote->getOctave(), 1);
				trackedPitches.Add(pitch);
			}
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

void USimplePitchTracker::deleteSimplePitchObject(USimplePitch* MyObject)
{
	if (!MyObject) return;
	if (!MyObject->IsValidLowLevel()) return;

	MyObject->ConditionalBeginDestroy(); //instantly clears UObject out of memory
	MyObject = nullptr;
}