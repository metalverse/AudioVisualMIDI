// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Voice.h"
#include "OnlineSubsystemUtils.h"
#include <cmath>
#include "CoreMisc.h"
#include "SimplePitchTracker.h"
#include "SimplePitch.h"
#include "MicrophoneInput.generated.h"




UCLASS(Blueprintable)
class MIDI_PROJECT_API AMicrophoneInput : public AActor
{
	GENERATED_BODY()
	

public:
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") float volume;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") int fundamental_frequency;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") FString currentPitch;	
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") TArray<float> spectrum;

	TSharedPtr<class IVoiceCapture> voiceCapture;
	TSharedPtr<class VampPluginHost> vampHost;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") USimplePitchTracker* tracker;

	AMicrophoneInput(const FObjectInitializer& ObjectInitializer);
	~AMicrophoneInput();
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION(BlueprintCallable, Category = "MicrophoneInput")
		void SaveStringTextToFile(
			FString SaveDirectory,
			FString FileName,
			FString SaveText
		);

private:
	int tmpCounter = 0;
	//double pi;
	float* vector;
	float const silenceTreshold = 65.f*65.f;
	bool finished = true;
	static const unsigned int N = 4096;
};
