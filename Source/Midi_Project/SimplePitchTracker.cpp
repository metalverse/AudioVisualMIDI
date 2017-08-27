// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SimplePitchTracker.h"


using namespace std;

USimplePitchTracker::USimplePitchTracker(const FObjectInitializer& ObjectInitializer)
{
	//pitchTable.Init(ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, TEXT("NewPitch")), notesToRecognize);
	initPitchTable(ObjectInitializer);
	currentNote = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, TEXT("currentPitch"));
	currentNote->setParams("None", 0, 0, 0, 0);
	USimplePitch* pitch = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, TEXT("tmpPitch"));
	pitch->setParams("None", 0, 0, 0, 0);
	trackedPitches.Add(pitch);
	numberOfTrackingPitches++;
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
	int id = 0;
	FString toneNames[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","H" };
	for (int octave = 0; octave < octavesToRecognize; octave++) {
		for (int note = 0; note < 12; note++) {
			FString name = toneNames[note] + FString::FromInt(octave);
			USimplePitch* pitch = ObjectInitializer.CreateDefaultSubobject<USimplePitch>(this, FName(*name));
			pitch->setParams(name, noteFreq, octave, 0, id++);
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
	/*for (int i = 0; i < notesToRecognize; i++) {
		UE_LOG(LogTemp, Log, TEXT("My note FREQ+++: %f"), pitchTable[i]->getFrequency());
	}*/
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
		UE_LOG(LogTemp, Log, TEXT("Index: %d "), noteIndex);
		if (noteIndex != -1) {
			FString noteName = FString(pitchTable[noteIndex]->getName());
			UE_LOG(LogTemp, Log, TEXT("Name: %s "), *noteName);
			lastTrackedNote = currentNote;
			currentNote = pitchTable[noteIndex];
			UE_LOG(LogTemp, Log, TEXT("Obtain current note "));
			if (currentNote == lastTrackedNote) {
				UE_LOG(LogTemp, Log, TEXT("Incrementing time of last note "));
				trackedPitches.Last()->incrementTime(1);
			}
			else if (trackedPitches.Last()->getTime() < 2) {
				UE_LOG(LogTemp, Log, TEXT("+Removing last note and adding new one "));
				trackedPitches.Pop();
				--numberOfTrackingPitches;
				addNewPitchToTrackedList(currentNote);
			}
			else {
				UE_LOG(LogTemp, Log, TEXT("+Adding new note "));
				addNewPitchToTrackedList(currentNote);
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

void USimplePitchTracker::addNewPitchToTrackedList(USimplePitch* myNote)
{
	if (!myNote) return;
	if (!myNote->IsValidLowLevel()) return;
	USimplePitch* pitch = NewObject<USimplePitch>(this);
	pitch->setParams(myNote->getName(), myNote->getFrequency(), myNote->getOctave(), 1, myNote->getPitchId());
	trackedPitches.Add(pitch);
	++numberOfTrackingPitches;
	OnSoundRecordedDelegate.Broadcast(pitch->getPitchId());
}