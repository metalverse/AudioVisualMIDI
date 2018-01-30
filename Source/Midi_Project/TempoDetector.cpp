// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "TempoDetector.h"



TempoDetector::TempoDetector(int sampleRate, int maxFrameDiff, int maxTrackingFrameDiff)
	: sampleRate(sampleRate), maxFrameDifference(maxFrameDiff), maxTrackingFrameDifference(maxTrackingFrameDiff)
{
	totalTrackedSamples = 0;
	firstOnsetFrame = 0;
	lastOnsetFrame = 0;
}

TempoDetector::~TempoDetector()
{
}

void TempoDetector::update(bool newValue, int deltaSamples, int frame)
{
	if ((deltaSamples + totalTrackedSamples) - lastOnsetFrame > maxFrameDifference) {
		resetBuffer();
	}
	while (totalTrackedSamples - firstOnsetFrame > maxTrackingFrameDifference && trackedOnsets.size() > 1) {
		firstOnsetFrame += trackedOnsets.front();
		trackedOnsets.pop_front();
	}
	if (newValue) {
		if (trackedOnsets.empty()) {
			trackedOnsets.push_back(0);
			totalTrackedSamples += (deltaSamples - frame);
		} else if (trackedOnsets.size() == 1 && trackedOnsets.back() == 0) {
			trackedOnsets.pop_front();
			trackedOnsets.push_back(totalTrackedSamples + frame - lastOnsetFrame);
			lastOnsetFrame = totalTrackedSamples + frame;
		} else {
			trackedOnsets.push_back(totalTrackedSamples + frame - lastOnsetFrame);
			lastOnsetFrame = totalTrackedSamples + frame;
			totalTrackedSamples += deltaSamples;
		}
	} else if (trackedOnsets.size() > 0) {
		totalTrackedSamples += deltaSamples;
	}
}

/* 
 * Get estimated tempo in BPM from onset frames.
 *
 * return        estimated tempo 
 * retval -1.f   if not enough data provided
 * retval -2.f   if calculated coefficient of variation exeeds permissible level
 */
float TempoDetector::calculateTempo()
{
	int size = trackedOnsets.size();
	if (trackedOnsets.size() < 4) {
		return -1.f;
	}
	float sum = 0;
	for (auto frameNumber : trackedOnsets) {
		sum += frameNumber;
	}
	float avg = sum / size;
	float variance = 0;
	for (auto frameNumber : trackedOnsets) {
		//UE_LOG(LogTemp, Log, TEXT("TempoDetector. sample: %d"), frameNumber);
		variance += (frameNumber - avg) * (frameNumber - avg);
	}
	variance /= size;
	float cv = sqrt(variance)/avg; // coefficient of variation
	if (cv > permissibleCoefficientOfVariation) {
		resetBuffer();
		return -2.f;
	}
	UE_LOG(LogTemp, Log, TEXT("TempoDetector. avg: %f, st dev: %f, cv: %f"), avg, sqrt(variance), cv);
	return (60.f / (avg / sampleRate));
}

void TempoDetector::resetBuffer()
{
	trackedOnsets.clear();
	totalTrackedSamples = 0;
	lastOnsetFrame = 0;
}