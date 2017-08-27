// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "Object.h"
#include "SimplePitch.generated.h"

using namespace std;

UCLASS(BlueprintType)
class MIDI_PROJECT_API USimplePitch : public UObject
{
	GENERATED_BODY()

public:
	USimplePitch();
	~USimplePitch();
	UFUNCTION(BlueprintCallable, Category = "SoundParameters") FString getName() { return name; };
	UFUNCTION(BlueprintCallable, Category = "SoundParameters") float getFrequency() { return frequency; };
	UFUNCTION(BlueprintCallable, Category = "SoundParameters") int getOctave() { return octave; };
	UFUNCTION(BlueprintCallable, Category = "SoundParameters") int getTime() { return time; };
	UFUNCTION(BlueprintCallable, Category = "SoundParameters") int getPitchId() { return pitchId; };
	void setParams(FString name, float freq, int octave, int time, int pitchId);
	void incrementTime(int value) { time += value; };

private:
	FString name;
	int octave;
	float frequency;
	int time;
	int pitchId;
};
