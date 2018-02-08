// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "VampPluginCalibrator.h"


// Sets default values

AVampPluginCalibrator::AVampPluginCalibrator() {
	PrimaryActorTick.bCanEverTick = true;
}
//AVampPluginCalibrator::AVampPluginCalibrator(float* audioBuffer, const int samples, const int sampleRate) : audioBuffer(audioBuffer), samples(samples), sampleRate(sampleRate) {
//	host = new VampPluginHost(sampleRate);
//
//	PrimaryActorTick.bCanEverTick = true;
//
//}

// Called when the game starts or when spawned
void AVampPluginCalibrator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVampPluginCalibrator::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

