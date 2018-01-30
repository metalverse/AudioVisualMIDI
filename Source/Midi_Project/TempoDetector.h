// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <list>

/**
 * 
 */
class MIDI_PROJECT_API TempoDetector
{
public:
	TempoDetector(int sampleRate, int maxFrameDiff, int maxTrackingFrameDiff);
	~TempoDetector();
	void update(bool newValue, int deltaSamples, int frame = 0);
	float calculateTempo();
	void setMaxFrameDifference(int maxFrameDiff) {
		maxFrameDifference = maxFrameDiff;
	}
	void setMaxTrackingFrameDifference(int maxTrackFrameDiff) {
		maxTrackingFrameDifference = maxTrackFrameDiff;
	}

private:
	int totalTrackedSamples;
	int firstOnsetFrame;
	int lastOnsetFrame;
	std::list<int> trackedOnsets;
	int maxFrameDifference;
	int maxTrackingFrameDifference;
	int sampleRate;
	float permissibleCoefficientOfVariation = .35f;
	void resetBuffer();
};
