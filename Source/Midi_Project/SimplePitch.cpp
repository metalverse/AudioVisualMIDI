// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SimplePitch.h"

USimplePitch::USimplePitch()
{
}

USimplePitch::USimplePitch(float freq) : frequency(freq)
{
	name = "undefined";
	octave = -1;
	time = 0;
}

USimplePitch::USimplePitch(string name, float freq) : name(name), frequency(freq)
{
	octave = -1;
	time = 0;
}

USimplePitch::USimplePitch(string name, float freq, int octave) : name(name), frequency(freq), octave(octave)
{
	time = 0;
}

USimplePitch::USimplePitch(string name, float freq, int octave, int time) : name(name), frequency(freq), octave(octave), time(time)
{
}

USimplePitch::~USimplePitch()
{
}
