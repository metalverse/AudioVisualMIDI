// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <string>

using namespace std;

class MIDI_PROJECT_API Pitch
{
public:
	Pitch();
	Pitch(float freq);
	Pitch(string name, float freq);
	Pitch(string name, float freq, int octave);
	Pitch(string name, float freq, int octave, int time);
	~Pitch();
	string getName() { return name; };
	float getFrequency() { return frequency; };
	int getOctave() { return octave; };
	int getTime() { return time; };

private:
	string name;
	int octave;
	float frequency;
	int time;

	/*static enum Tone {
		C4 = 26163,
		Cis4 = 27718,
		D4 = 29366,
		Dis4 = 31113,
		E4 = 32963,
		F4 = 34923,
		Fis4 = 36999,
		G4 = 39200,
		Gis4 = 41530,
		A4 = 44000,
		Ais4 = 46616,
		H4 = 49388
	};*/

};

