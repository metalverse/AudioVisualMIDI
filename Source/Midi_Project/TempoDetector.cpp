// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "TempoDetector.h"



TempoDetector::TempoDetector(int sampleRate, int maxFrameDiff, int maxTrackingFrameDiff)
	: sampleRate(sampleRate), maxFrameDifference(maxFrameDiff), maxTrackingFrameDifference(maxTrackingFrameDiff)
{
	totalTrackedSamples = 0;
	firstOnsetFrame = 0;
	lastOnsetFrame = 0;
	currentMidiTempo = 86;
	maxOnsetsTracked = 5;
	mode = TempoMode::NORMAL;
}

TempoDetector::~TempoDetector()
{
}

bool TempoDetector::update(bool newValue, int deltaSamples, int frame)
{
	bool status = true;
	if ((deltaSamples + totalTrackedSamples) - lastOnsetFrame > maxFrameDifference) {
		resetBuffer();
		if (mode == TempoMode::REALTIME) {
			mode = TempoMode::NORMAL;
		}
		return false;
	}
	int tmpOffset = totalTrackedSamples + frame - lastOnsetFrame;
	if (newValue && (tmpOffset < (trackedOnsets.back() / 2.2f) || tmpOffset < minFrameDifference)) {
		UE_LOG(LogTemp, Log, TEXT("TempoDetector. Action. Too short gap: %d"), tmpOffset);
		totalTrackedSamples += deltaSamples;
		return false;
	}
	while (totalTrackedSamples - firstOnsetFrame > maxTrackingFrameDifference 
		&& trackedOnsets.size() > 1 && trackedOnsets.size() > maxOnsetsTracked) 
	{
		firstOnsetFrame += trackedOnsets.front();
		trackedOnsets.pop_front();
	}
	if (newValue) {
		if (mode == TempoMode::REALTIME) {
			int newFrameDifference = totalTrackedSamples + frame - lastOnsetFrame;
			UE_LOG(LogTemp, Log, TEXT("TempoDetector. New frame difference %d, metronom: %d"), newFrameDifference, metronom);
			metronom = metronom % 4;
			metronom++;
			if (abs(newFrameDifference - trackedOnsets.back()) > 2.0f * trackedOnsets.back()) {
				UE_LOG(LogTemp, Log, TEXT("TempoDetector. Action. Too long gap: %d, switching to NORMAL mode"), abs(newFrameDifference - trackedOnsets.back()));
				mode = TempoMode::NORMAL;
				resetBuffer();
				status = false;
			} else if ((newFrameDifference - trackedOnsets.back()) > 0.8 * trackedOnsets.back()) {
				UE_LOG(LogTemp, Log, TEXT("TempoDetector. Action. Deviding too long gap: %d"), newFrameDifference);
				newFrameDifference /= 2;
			}
			trackedOnsets.push_back(newFrameDifference);
			lastOnsetFrame = totalTrackedSamples + frame;
			totalTrackedSamples += deltaSamples;
		} else {
			if (trackedOnsets.empty()) {
				trackedOnsets.push_back(0);
				totalTrackedSamples += (deltaSamples - frame);
			}
			else if (trackedOnsets.size() == 1 && trackedOnsets.back() == 0) {
				trackedOnsets.pop_front();
				trackedOnsets.push_back(totalTrackedSamples - lastOnsetFrame + frame);
				lastOnsetFrame = totalTrackedSamples + frame;
				totalTrackedSamples += deltaSamples;
			}
			else {
				trackedOnsets.push_back(totalTrackedSamples - lastOnsetFrame + frame);
				lastOnsetFrame = totalTrackedSamples + frame;
				totalTrackedSamples += deltaSamples;
			}
		}
	} else if (trackedOnsets.size() > 0) {
		totalTrackedSamples += deltaSamples;
	}
	return status;
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
	if (mode == TempoMode::REALTIME) {
		float tempo = (60.f / (1.0f * trackedOnsets.back() / sampleRate));
		UE_LOG(LogTemp, Log, TEXT("TempoDetector. Action. Returning tempo: %f"), tempo);
		//float avgLast = (float)(trackedOnsets.back() + *std::prev(trackedOnsets.end(), 2)) / 2;
		return tempo;
	} else {
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
		//UE_LOG(LogTemp, Log, TEXT("TempoDetector. avg: %f, st dev: %f, cv: %f"), avg, sqrt(variance), cv);
		float tempo = (60.f / (avg / sampleRate));
		if (abs(tempo - currentMidiTempo) < .08f * currentMidiTempo) {
			mode = TempoMode::REALTIME;
			UE_LOG(LogTemp, Log, TEXT("TempoDetector. Real time mode ON!"));
		}
		return tempo;
	}
}

void TempoDetector::resetBuffer()
{
	UE_LOG(LogTemp, Log, TEXT("TempoDetector. RESET"));
	trackedOnsets.clear();
	totalTrackedSamples = 0;
	lastOnsetFrame = 0;
	mode = TempoMode::NORMAL;
}