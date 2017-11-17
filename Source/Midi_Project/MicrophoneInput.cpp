// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
//#include "tools/kiss_fftnd.h"
#include "MicrophoneInput.h"
#include "VampPluginHost.h"

#include <algorithm>

//#include "vamp/vamp.h"
//#include "vamp-sdk/PluginAdapter.h"

/*#include "vamp-plugins/SpectralCentroid.h"
#include "vamp-plugins/ZeroCrossing.h"
#include "vamp-plugins/PercussionOnsetDetector.h"
#include "vamp-plugins/FixedTempoEstimator.h"
#include "vamp-plugins/AmplitudeFollower.h"
#include "vamp-plugins/PowerSpectrum.h"*/

#include "vamp-hostsdk/PluginHostAdapter.h"
#include "vamp-hostsdk/PluginInputDomainAdapter.h"
#include "vamp-hostsdk/PluginLoader.h"

#include "EngineMinimal.h" 


#define SAMPLE_RATE (44100);

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

VampPluginHost *host;

// Sets default values
AMicrophoneInput::AMicrophoneInput(const FObjectInitializer& ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.bCanEverTick = true;
	voiceCapture = FVoiceModule::Get().CreateVoiceCapture();
	voiceCapture->Init(44000, 1); //wspierane 8000 - 48000Hz
	voiceCapture->Start();
	spectrum.Init(0, N / 2);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, key.c_str());
	host = new VampPluginHost(44000, 2048, 1024);
	//tracker = NewObject <USimplePitchTracker>(this, Namesss); //new USimplePitchTracker();
	tracker = ObjectInitializer.CreateDefaultSubobject<USimplePitchTracker>(this, TEXT("MyPitchTracker"));
}

AMicrophoneInput::~AMicrophoneInput()
{
	//delete tracker;
}


// Called when the game starts or when spawned
void AMicrophoneInput::BeginPlay()
{
	Super::BeginPlay();
}

template<typename T>
bool AMicrophoneInput::NormalizeDataAndCheckForSilence(T* inBuff, uint8* inBuff8, int32 buffSize, float* outBuf, uint32 samples, float& vol)
{
	if (buffSize == 0)
	{
		return 0;
	}
	const int32 IterSize = buffSize / sizeof(T);
	double Total = 0.0f;
	for (int32 i = 0; i < IterSize; i++)
	{
		Total += inBuff[i];
	}
	const double Average = Total / IterSize;
	double SumMeanSquare = 0.0f;
	double Diff = 0.0f;
	for (int32 i = 0; i < IterSize; i++)
	{
		Diff = (inBuff[i] - Average);
		SumMeanSquare += (Diff * Diff);
	}
	float AverageMeanSquare = SumMeanSquare / IterSize;
	static float Threshold = 75.0 * 75.0;

	int16_t sample;
	float totalVolume = 0;
	for (uint32 i = 0; i < samples; i++)
	{
		sample = (inBuff8[i * 2 + 1] << 8) | inBuff8[i * 2];
		outBuf[i] = float(sample) / 32768.0f;
		if (i < N) {
			totalVolume += outBuf[i] * outBuf[i];
		}
	}
	totalVolume = totalVolume / samples;
	totalVolume = FMath::Sqrt(totalVolume);
	vol = 20 * log10f(totalVolume) + 80;
	return AverageMeanSquare < Threshold;
}

// Called every frame
void AMicrophoneInput::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	int maxBytes = 0;
	uint32 bytesAvailable = 0;
	EVoiceCaptureState::Type captureState = voiceCapture->GetCaptureState(bytesAvailable);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(bytesAvailable).Append(" bytesAvailable"));
	FString bufforStatus = EVoiceCaptureState::ToString(captureState);
	UE_LOG(LogTemp, Log, TEXT("Buffor status: %s"), *bufforStatus);
	int bytAvailable = bytesAvailable;
	UE_LOG(LogTemp, Log, TEXT("Bytes available: %d"), bytAvailable);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, EVoiceCaptureState::ToString(captureState));
	if (captureState == EVoiceCaptureState::Ok && bytesAvailable >= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Bytes taken: %d"), bytesAvailable);
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(bytesAvailable).Append(" bytesAvailable"));
		maxBytes = bytesAvailable;
		uint8* buf = new uint8[maxBytes];
		memset(buf, 0, maxBytes);
		uint32 readBytes = 0;

		voiceCapture->GetVoiceData(buf, maxBytes, readBytes);
		uint32 samples = 0;
		samples = readBytes / 2;
		float* sampleBuf = new float[samples];
		bool isSilence = NormalizeDataAndCheckForSilence((int16*)buf, buf, readBytes, sampleBuf, samples, volume);
		
		if (!isSilence && samples >= 2048) {
			//fundamental_frequency = (peak_idx * 44000.0f / (1.0f * N));
			
			//////////////// HOST /////////////////////
			const int frequencyOutput = 0;
			host->runPlugin("pyin", "yin", sampleBuf, samples);

			auto features = host->getExtractedFeatures();
			if (features.size() > 0) {
				std::sort(features.begin(), features.end());
				for (auto feature : features) {
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Silver, FString::SanitizeFloat(feature).Append(" Hz"));
					UE_LOG(LogTemp, Log, TEXT("My value: %f"), feature);
				}
				const auto median_it = features.begin() + features.size() / 2;
				fundamental_frequency = (*median_it);
				if (!tracker->trackNewNote(fundamental_frequency)) {
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::SanitizeFloat(fundamental_frequency).Append(" Hz. Note unrecognized!"));
				}
				else {
					currentPitch = tracker->currentNote->getName();
				}
			}
			else {
				fundamental_frequency = 0;
				UE_LOG(LogTemp, Log, TEXT("Features empty"));
			}

		} else {
			fundamental_frequency = 0;
			/*for (int i = 0; i < N / 2; i++){
				spectrum[i] = 0;
			}*/
		}
		delete[] sampleBuf;
	}
	else {
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Turquoise, "No data");
	}

}

void AMicrophoneInput::SaveStringTextToFile(
	FString SaveDirectory,
	FString FileName,
	FString SaveText) {

	bool AllowOverwriting = true;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (PlatformFile.CreateDirectoryTree(*SaveDirectory))
	{
		// Get absolute file path
		FString AbsoluteFilePath = SaveDirectory + "/" + FileName;

		// Allow overwriting or file doesn't already exist
		if (AllowOverwriting || !PlatformFile.FileExists(*AbsoluteFilePath))
		{
			FFileHelper::SaveStringToFile(SaveText, *AbsoluteFilePath);
		}
	}
}

