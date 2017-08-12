// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Object.h"
#include "SimplePitch.h"
#include "SimplePitchTracker.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MIDI_PROJECT_API USimplePitchTracker : public UObject
{
	GENERATED_BODY()
	
public:
	USimplePitchTracker(const FObjectInitializer& ObjectInitializer);
	~USimplePitchTracker();
	bool trackNewNote(float freq);
	UPROPERTY(BlueprintReadWrite, Category = "SoundParameters") USimplePitch* lastTrackedNote;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") TArray<USimplePitch*> pitchTable;

private:
	static const int octavesToRecognize = 9;
	static const int notesToRecognize = octavesToRecognize * 12;
	const double freqHalfToneMultiplier = 1.05946309436;
	void initPitchTable(const FObjectInitializer& ObjectInitializer);
	int findPitchByFrequency(int left, int right, int freq);
	
	
};
