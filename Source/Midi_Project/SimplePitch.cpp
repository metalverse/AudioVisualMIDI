// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SimplePitch.h"

SimplePitch::SimplePitch() 
{
} 

SimplePitch::SimplePitch(float freq) : frequency(freq)
{
	name = "undefined";
	octave = -1;
	time = 0;
}

SimplePitch::SimplePitch(string name, float freq) : name(name), frequency(freq)
{
	octave = -1;
	time = 0;
}

SimplePitch::SimplePitch(string name, float freq, int octave) : name(name), frequency(freq), octave(octave)
{
	time = 0;
}

SimplePitch::SimplePitch(string name, float freq, int octave, int time) : name(name), frequency(freq), octave(octave), time(time)
{
}

SimplePitch::~SimplePitch()
{
}
