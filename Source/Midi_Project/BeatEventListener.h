// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
class MIDI_PROJECT_API BeatEventListener
{
public:
	virtual void onEventBeat2(int mode, int value) {};
};
