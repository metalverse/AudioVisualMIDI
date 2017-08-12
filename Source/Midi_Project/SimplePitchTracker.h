// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Object.h"
#include "SimplePitch.h"
#include "SimplePitchTracker.generated.h"

/**
 * 
 */
UCLASS()
class MIDI_PROJECT_API USimplePitchTracker : public UObject
{
	GENERATED_BODY()
	
public:
	USimplePitchTracker(const FObjectInitializer& ObjectInitializer);
	~USimplePitchTracker();
	bool trackNewNote(float freq);
	SimplePitch* lastTrackedNote;

private:
	static const int octavesToRecognize = 9;
	static const int notesToRecognize = octavesToRecognize * 12;
	const double freqHalfToneMultiplier = 1.05946309436;
	SimplePitch** pitchTable;
	void initPitchTable();
	int findPitchByFrequency(int left, int right, int freq);
	
	
};
