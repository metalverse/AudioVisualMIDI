// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SimplePitch.h"

USimplePitch::USimplePitch()
{
	name = "Undefined";
	frequency = 0;
	octave = -1;
	time = 0;
}

USimplePitch::~USimplePitch(){}

void USimplePitch::setParams(FString n, float freq, int oct, int t)
{
	name = n;
	frequency = freq;
	octave = oct;
	time = t;
}