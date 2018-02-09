// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "VampPluginCalibrator.h"
#include "WavFileReader.h"


// Sets default values
AVampPluginCalibrator::AVampPluginCalibrator() {
	PrimaryActorTick.bCanEverTick = true;
	host = new VampPluginHost(sampleRate);
}

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

void AVampPluginCalibrator::RunOnsetDetectorTests() {
	for (const auto& file : testFiles) {
		LoadAudioDataFromWavFile(file);
		if (audioBuffer == nullptr || samples == 0) {
			UE_LOG(LogTemp, Log, TEXT("Calibration: NO DATA!"));
			return;
		}
		testParamsToRecognizedOnsets.clear();
		for (const auto& threshold : onsetThresholdValues) {
			TMap<FString, float> params;
			params.Add("threshold", threshold);
			for (const auto& sensitivity : onsetSensitivityValues) {
				params.Add("sensitivity", sensitivity);
				host->initializeVampPlugin("percussiononsets", 1024, 512, params, 1);
				UE_LOG(LogTemp, Log, TEXT("Calibration: Running test with %d samples"), samples);
				if (host->runPlugin("vamp-example-plugins", "percussiononsets", audioBuffer, samples, true, -1) != 0) {
					UE_LOG(LogTemp, Log, TEXT("Calibration: Failed to run percussiononsets plugin!"));
				}
				else {
					std::vector<float> recognizedOnsets;
					auto onsetFeatures = host->getExtractedFeatures();
					if (onsetFeatures.size() > 0) {
						for (auto onsetFeature : onsetFeatures) {
							float onsetFrame = onsetFeature.first;
							recognizedOnsets.push_back(onsetFrame);
						}
					}
					std::string testDescription = std::to_string(threshold) + ";" + std::to_string(sensitivity) + ";" + std::to_string(recognizedOnsets.size()) + ";";
					//std::string testDescription = "threshold-" + std::to_string(threshold) + "-sensitivity-" + std::to_string(sensitivity) + "-found-" + std::to_string(recognizedOnsets.size());
					UE_LOG(LogTemp, Log, TEXT("Calibration: step finished: %s"), *(FString(testDescription.c_str())));
					testParamsToRecognizedOnsets.insert(std::make_pair(testDescription, recognizedOnsets));
				}
			}
		}
		saveDataToFile(file, testParamsToRecognizedOnsets);
	}
}

void AVampPluginCalibrator::RunYinTests() {

}

void AVampPluginCalibrator::LoadAudioDataFromWavFile(FString wavFilename) {
	std::string wavFilePath = "D:\\Studia\\Praca Magisterska\\Plugin tests\\TestFiles\\" + std::string(TCHAR_TO_UTF8(*wavFilename));
	WavFileReader reader(wavFilePath.c_str(), "D:\\Studia\\Praca Magisterska\\Plugin tests\\test.txt");
	reader.getData(&audioBuffer, samples);
}

void AVampPluginCalibrator::saveDataToFile(FString testId, std::map<std::string, std::vector<float>> testParamsToRecognizedFeatures) {
	bool AllowOverwriting = true;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString onsetsTextToSave = "";
	FString shortTestResult = "";
	for (auto const& test : testParamsToRecognizedFeatures) {
		shortTestResult.Append(UTF8_TO_TCHAR(test.first.c_str())).Append(LINE_TERMINATOR);
		/*onsetsTextToSave.Append(UTF8_TO_TCHAR(test.first.c_str())).Append(LINE_TERMINATOR);
		for (auto const& value : test.second) {
			onsetsTextToSave.Append(FString::SanitizeFloat(value)).Append(LINE_TERMINATOR);
		}
		onsetsTextToSave.Append(LINE_TERMINATOR);*/
	}
	TCHAR* directory = TEXT("D:\\Studia\\Praca Magisterska\\Plugin tests\\DetectedFeatures\\");
	if (PlatformFile.CreateDirectoryTree(directory))
	{
		// Get absolute file path
		FString onsetAbsoluteFilePath = directory + testId + FString("-features.txt");
		FString shortTestResultFilePath = directory + testId + FString("-short-result.txt");

		//if (AllowOverwriting || !PlatformFile.FileExists(*onsetAbsoluteFilePath))
		//{
		//	FFileHelper::SaveStringToFile(onsetsTextToSave, *onsetAbsoluteFilePath);
		//}
		if (AllowOverwriting || !PlatformFile.FileExists(*shortTestResultFilePath))
		{
			FFileHelper::SaveStringToFile(shortTestResult, *shortTestResultFilePath);
		}
	}
}
