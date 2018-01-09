// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Voice.h"
#include "OnlineSubsystemUtils.h"
#include <cmath>
#include <vector>
#include "CoreMisc.h"
#include "SimplePitchTracker.h"
#include "SimplePitch.h"
#include "MicrophoneInput.generated.h"




UCLASS(Blueprintable)
class MIDI_PROJECT_API AMicrophoneInput : public AActor
{
	GENERATED_BODY()
	

public:
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") float volumedB;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") float volumeAmplitude;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") bool isSilence;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") int fundamental_frequency;
	UPROPERTY(BlueprintReadOnly, Category = "SoundParameters") FString currentPitch;	
	UPROPERTY(BlueprintReadWrite, Category = "SoundParameters") int vampBlockSize = 2048;
	UPROPERTY(BlueprintReadWrite, Category = "SoundParameters") int vampStepSize = 1024;
	UPROPERTY(BlueprintReadWrite, Category = "SoundParameters") float silenceTreshold = 75.f * 75.f;
	UPROPERTY(BlueprintReadWrite, Category = "SoundParameters") TArray<int> bufferedMidiNotes;
	UPROPERTY(BlueprintReadWrite, Category = "SoundParameters") float maxSoundValue = 0;


	TSharedPtr<class IVoiceCapture> voiceCapture;
	TSharedPtr<class VampPluginHost> vampHost;
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category = "SoundParameters") USimplePitchTracker* tracker;

	AMicrophoneInput(const FObjectInitializer& ObjectInitializer);
	~AMicrophoneInput();
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION(BlueprintCallable, Category = "MicrophoneInput")
		void StartRecordingRecognizedFrequencies();

	UFUNCTION(BlueprintCallable, Category = "MicrophoneInput")
		void StopRecordingRecognizedFrequencies();

	UFUNCTION(BlueprintCallable, Category = "MicrophoneInput")
		void ResetRecognizedFrequenciesBuffer();

	UFUNCTION(BlueprintCallable, Category = "MicrophoneInput")
		void SaveStringTextToFile(
			FString SaveDirectory,
			FString FileName,
			FString SaveText
		);

	UFUNCTION(BlueprintCallable, Category = "MicrophoneInput")
		void SaveRecognizedFrequenciesToFile(
			FString SaveDirectory,
			FString FileName
		);

private:
	template<typename T>
	bool NormalizeDataAndCheckForSilence(T* inBuff, uint8* inBuff8, int32 buffSize, float* outBuf, uint32 outBuffSize, float& volumedB, float& volumeAmplitude);
	static const unsigned int N = 4096;
	const unsigned int sampleRate = 44100.0f;
	bool isRecordingRecognizedFrequencies = false;
	std::vector<float> recognizedFrequenciesToSave;

};
