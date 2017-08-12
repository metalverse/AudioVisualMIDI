// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "Object.h"
//#include "SimplePitch.generated.h"

using namespace std;


class MIDI_PROJECT_API SimplePitch
{
	
public:
	SimplePitch();
	SimplePitch(float freq);
	SimplePitch(string name, float freq);
	SimplePitch(string name, float freq, int octave);
	SimplePitch(string name, float freq, int octave, int time);
	~SimplePitch();
	string getName() { return name; };
	float getFrequency() { return frequency; };
	int getOctave() { return octave; };
	int getTime() { return time; };
	void incrementTime(int value) { time += value; };

private:
	string name;
	int octave;
	float frequency;
	int time;
};
