// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SimplePitchTracker.h"


using namespace std;

USimplePitchTracker::USimplePitchTracker(const FObjectInitializer& ObjectInitializer)
{
	//trackedPitches.Empty();
	
	lastTrackedNote = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, TEXT("lastTrackedPitch"));
	currentNote = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, TEXT("currentPitch"));
	pitchTable.Init(currentNote, notesToRecognize);
	//trackedPitches.Add(lastTrackedNote);
	//initPitchTable(ObjectInitializer);
	float noteFreq = 16.3516;
	FString toneNames[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","H" };
	for (int octave = 0; octave < octavesToRecognize; octave++) {
		for (int note = 0; note < 12; note++) {
			FString name = toneNames[note] + FString::FromInt(octave);
			USimplePitch* pitch = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, FName(*name));
			pitch->setParams(name, noteFreq, octave, 0);
			pitchTable[octave * 12 + note] = pitch;//.Add(pitch);
			float test2 = noteFreq;
			UE_LOG(LogTemp, Log, TEXT("My note: %s"), *name);
			UE_LOG(LogTemp, Log, TEXT("My note: %f"), noteFreq);
			noteFreq *= freqHalfToneMultiplier;
		}
	}
}

USimplePitchTracker::~USimplePitchTracker()
{
	/*//trackedPitches.Empty();auto& Unit : AllUnits_
	for (auto& pitch : trackedPitches) {
		pitch->DestroyNonNativeProperties();// ->ConditionalBeginDestroy();
		pitch = NULL;
	}
	UE_LOG(LogTemp, Log, TEXT("CLEARED!!!!!!!!!!!!!!!!"));*/
	//trackedPitches.Empty();
}

/*void USimplePitchTracker::initPitchTable(const FObjectInitializer& ObjectInitializer)
{
	
}*/


bool USimplePitchTracker::trackNewNote(float freq)
{
	UE_LOG(LogTemp, Log, TEXT("trackNewNote. freq: %f"), freq);
	UE_LOG(LogTemp, Log, TEXT("[0] freq: %f"), pitchTable[0]->getFrequency());
	UE_LOG(LogTemp, Log, TEXT("[notesToRecognize - 1] freq: %f"), pitchTable[notesToRecognize - 1]->getFrequency());
	if (freq > pitchTable[notesToRecognize - 1]->getFrequency() || freq < pitchTable[notesToRecognize - 1]->getFrequency()) {
		UE_LOG(LogTemp, Log, TEXT("Note out of tracked range."));
		return false;
	}
	else {
		int left = 0;
		int right = notesToRecognize - 1;

		UE_LOG(LogTemp, Log, TEXT("FREQ: %f"), freq);

		int noteIndex = findPitchByFrequency(left, right, freq);
		UE_LOG(LogTemp, Log, TEXT("index: %d"), noteIndex);
		if (noteIndex != -1) {
			FString noteName = FString(pitchTable[noteIndex]->getName());
			lastTrackedNote = currentNote;
			currentNote = pitchTable[noteIndex];
			/*if (lastTrackedNote == currentNote) {
				trackedPitches.Last()->incrementTime(1);
			}
			else {*/
			UE_LOG(LogTemp, Log, TEXT("Creating new PITCH"));
				USimplePitch* newPitch = NewObject<USimplePitch>(this);
				newPitch->setParams(currentNote->getName(), currentNote->getFrequency(), currentNote->getOctave(), 1);
				UE_LOG(LogTemp, Log, TEXT("CREATED. Adding..."));
				trackedPitches.Add(newPitch);
			//}
			UE_LOG(LogTemp, Log, TEXT("TRACKED NOTE: %s"), *noteName);
			UE_LOG(LogTemp, Log, TEXT("FREQ: %f"), pitchTable[noteIndex]->getFrequency());
			return true;
		}
		return false;
	}
}

int USimplePitchTracker::findPitchByFrequency(int left, int right, int freq)
{
	UE_LOG(LogTemp, Log, TEXT("findPitchByFreq"));
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
