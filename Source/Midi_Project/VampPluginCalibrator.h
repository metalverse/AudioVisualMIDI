// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "VampPluginHost.h"
#include "VampPluginCalibrator.generated.h"

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

UCLASS(Blueprintable)
class MIDI_PROJECT_API AVampPluginCalibrator : public AActor
{
	GENERATED_BODY()
	
public:	
	AVampPluginCalibrator();
	//AVampPluginCalibrator(float* audioBuffer, const int samples, const int sampleRate = 44100);
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;

private:
	VampPluginHost *host;
	float* audioBuffer = nullptr;
	int samples = 0;
	int sampleRate = 44100;
	
};
