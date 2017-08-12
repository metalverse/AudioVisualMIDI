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
	string getName() { return name; };
	float getFrequency() { return frequency; };
	int getOctave() { return octave; };
	int getTime() { return time; };
	void setParams(string name, float freq, int octave, int time);
	void incrementTime(int value) { time += value; };

private:
	string name;
	int octave;
	float frequency;
	int time;
};
