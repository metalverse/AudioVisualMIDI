// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Voice.h"
#include "OnlineSubsystemUtils.h"
#include <cmath>
#include "CoreMisc.h"
#include "MicrophoneInput.generated.h"



UCLASS()
class MIDI_PROJECT_API AMicrophoneInput : public AActor
{
	GENERATED_BODY()
	

public:
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") float volume;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") int fundamental_frequency;
	int tmpCounter = 0;
	double pi;
	float* vector;
	float const silenceTreshold = 65.f*65.f;
	bool finished = true;
	static const int N = 4096;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") int NN = N;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") TArray<float> spectrum;

	TSharedPtr<class IVoiceCapture> voiceCapture;
	TSharedPtr<class VampPluginHost> vampHost;

	// Sets default values for this actor's properties
	AMicrophoneInput();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION(BlueprintCallable, Category = "MicrophoneInput")
		void SaveStringTextToFile(
			FString SaveDirectory,
			FString FileName,
			FString SaveText
		);
};
