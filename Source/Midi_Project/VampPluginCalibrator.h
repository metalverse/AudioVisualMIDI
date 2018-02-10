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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration") TArray<float> onsetSensitivityValues;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration") TArray<float> onsetThresholdValues;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration") TArray<float> yinOutputunvoicedValues;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration") TArray<float> yinThresholdValues;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration") TArray<FString> onsetTestFiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration") TArray<FString> yinTestFiles;


	UFUNCTION(BlueprintCallable, Category = "Calibration")
		void RunOnsetDetectorTests();
	UFUNCTION(BlueprintCallable, Category = "Calibration")
		void RunYinTests();
	UFUNCTION(BlueprintCallable, Category = "Calibration")
		void LoadAudioDataFromWavFile(FString wavFilename);

	AVampPluginCalibrator();
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;
	
	

private:
	VampPluginHost *host;
	float* audioBuffer = nullptr;
	int samples = 0;
	int sampleRate = 44100;
	std::map<std::string, std::vector<float>> testParamsToRecognizedOnsets;
	std::map<std::string, std::vector<float>> testParamsToRecognizedFreqs;
	void saveOnsetDataToFile(FString testId, std::map<std::string, std::vector<float>> testParamsToRecognizedFeatures);
	void saveYinDataToFile(FString testId, std::map<std::string, std::vector<float>> testParamsToRecognizedFreqs);
};
