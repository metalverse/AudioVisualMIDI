// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
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

AMicrophoneInput::AMicrophoneInput(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	voiceCapture = FVoiceModule::Get().CreateVoiceCapture();
	voiceCapture->Init(sampleRate, channels); //supported range: 8000 - 48000 Hz
	voiceCapture->Start();
	host = new VampPluginHost(sampleRate, vampBlockSize, vampStepSize, onsetParamThreshold, onsetParamSensitivity);
	tracker = ObjectInitializer.CreateDefaultSubobject<USimplePitchTracker>(this, TEXT("MyPitchTracker"));
	isSilence = true;
	isOnsetDetected = false;
}

AMicrophoneInput::~AMicrophoneInput()
{
	delete wavFile;
	//delete tracker;
}


// Called when the game starts or when spawned
void AMicrophoneInput::BeginPlay()
{
	Super::BeginPlay();
}

template<typename T>
bool AMicrophoneInput::NormalizeDataAndCheckForSilence(T* inBuff, uint8* inBuff8, int32 buffSize, float* outBuf, uint32 samples, float& vol, float &vol2)
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

	int16_t sample;
	float totalSquare = 0;
	maxSoundValue = 0;
	for (uint32 i = 0; i < samples; i++)
	{
		sample = (inBuff8[i * 2 + 1] << 8) | inBuff8[i * 2];
		outBuf[i] = float(sample) / 32768.0f;
		if (outBuf[i] > maxSoundValue) maxSoundValue = outBuf[i];
		totalSquare += sample * sample;
	}
	
	float meanSquare = 2 * totalSquare / samples;
	float rms = FMath::Sqrt(meanSquare);
	float amplitude = rms / 32768.0f;
	vol = 20 * log10f(amplitude) + 85;
	vol2 = amplitude * 100;
	UE_LOG(LogTemp, Log, TEXT("Max audio value: %f, avarage mean square: %f, normalized ams: %f"), maxSoundValue, AverageMeanSquare, (meanSquare/samples));
	return AverageMeanSquare < silenceTreshold;
}

// Called every frame
void AMicrophoneInput::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	int maxBytes = 0;
	uint32 bytesAvailable = 0;
	EVoiceCaptureState::Type captureState = voiceCapture->GetCaptureState(bytesAvailable);
	FString bufforStatus = EVoiceCaptureState::ToString(captureState);
	UE_LOG(LogTemp, Log, TEXT("Bytes available: %d"), (int)bytesAvailable);

	if (captureState == EVoiceCaptureState::Ok && bytesAvailable >= 0)
	{
		maxBytes = bytesAvailable;
		uint8* buf = new uint8[maxBytes];
		memset(buf, 0, maxBytes);
		uint32 readBytes = 0;
		voiceCapture->GetVoiceData(buf, maxBytes, readBytes);
		uint32 samples = readBytes / 2;

		UE_LOG(LogTemp, Log, TEXT("ONSET new samples: %d"), samples);
		float* sampleBuf = new float[samples];
		isSilence = NormalizeDataAndCheckForSilence((int16*)buf, buf, readBytes, sampleBuf, samples, volumedB, volumeAmplitude);

		if (!isSilence && samples >= (unsigned)vampStepSize) {
			/////// FREQS /////////
			TrackFundamentalFrequency(sampleBuf, samples);
		} else {
			fundamental_frequency = 0;
		}
		/////// ONSETS /////////
		TrackPercussionOnsets(sampleBuf, samples, numberOfSamplesTracked);
		if(isSavingAudioInput) {
			UE_LOG(LogTemp, Log, TEXT("ONSET Number of samples tracked from: %d to %d "), numberOfSamplesTracked, numberOfSamplesTracked+samples);
			numberOfSamplesTracked += samples;
			wavFile->writeData(sampleBuf, samples);
		}
		delete[] sampleBuf;
		//delete buf;
	}
}

void AMicrophoneInput::TrackFundamentalFrequency(float* &sampleBuf, int samples) {

	//////////////// PYIN /////////////////////
	if (host->runPlugin("pyin", "yin", sampleBuf, samples, false, -1) != 0) {
		UE_LOG(LogTemp, Log, TEXT("Failed to run yin plugin!"));
	}
	auto features = host->getExtractedFeatures();
	///////////////////////////////////////////

	if (features.size() > 0) {
		std::sort(features.begin(), features.end(), [](auto &left, auto &right) {
			return left.second < right.second;
		});
		for (auto feature : features) {
			UE_LOG(LogTemp, Log, TEXT("My value: %f"), feature.second);
		}
		const auto median_it = features.begin() + features.size() / 2;
		fundamental_frequency = (*median_it).second;
		if (isRecordingRecognizedFeatures) {
			recognizedFrequenciesToSave.push_back(fundamental_frequency);
		}

		if (fundamental_frequency > 0) {
			if (!tracker->trackNewNote(fundamental_frequency)) {
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::SanitizeFloat(fundamental_frequency).Append(" Hz. Note unrecognized!"));
			}
			else {
				const int trackedSoundToMidiNote = tracker->currentNote->getMidiNoteId();
				if (bufferedMidiNotes.Num() == 0) {
					bufferedMidiNotes.Add(trackedSoundToMidiNote);
				}
				else if (bufferedMidiNotes.Last() == trackedSoundToMidiNote) {
					// Handle incrementing
				}
				else {
					bufferedMidiNotes.Add(trackedSoundToMidiNote);
				}
				currentPitch = tracker->currentNote->getName();
			}
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("Unrecognized frequency"));
		}
	}
	else {
		fundamental_frequency = 0;
		UE_LOG(LogTemp, Log, TEXT("Features empty"));
	}
}

void AMicrophoneInput::TrackPercussionOnsets(float* &sampleBuf, int samples, int numberOfSamplesTracked) {
	//////////////// ONSET DETECTOR /////////////////////
	if (host->runPlugin("vamp-example-plugins", "percussiononsets", sampleBuf, samples, true, numberOfSamplesTracked - 1) != 0) {
		UE_LOG(LogTemp, Log, TEXT("Failed to run percussiononsets plugin!"));
	}
	auto onsetFeatures = host->getExtractedFeatures();

	if (onsetFeatures.size() > 0) {
		for (auto onsetFeature : onsetFeatures) {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Silver, FString::FromInt(numberOfSamplesTracked + onsetFeature.first).Append(" SAMPLE IS ONSET"));
			UE_LOG(LogTemp, Log, TEXT("ONSET Detected onset sample: %d, sound volume: %f"), numberOfSamplesTracked + onsetFeature.first, maxSoundValue);
			if (isRecordingRecognizedFeatures) {
				float onsetFrame = 1.0f * numberOfSamplesTracked + onsetFeature.first;
				recognizedOnsetsToSave.push_back(onsetFrame);
			}
		}
		isOnsetDetected = true;
	}
	else {
		isOnsetDetected = false;
	}
}

void AMicrophoneInput::StartRecordingRecognizedFeatures()
{
	isRecordingRecognizedFeatures = true;
}

void AMicrophoneInput::StopRecordingRecognizedFeatures()
{
	isRecordingRecognizedFeatures = false;
}

void AMicrophoneInput::ResetRecognizedFeaturesBuffers()
{
	recognizedOnsetsToSave.clear();
	recognizedFrequenciesToSave.clear();
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

void AMicrophoneInput::SaveRecognizedFeaturesToFiles(
	FString SaveDirectory,
	FString FileName) {

	bool AllowOverwriting = true;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	FString frequenciesTextToSave = "";
	for (float const& value : recognizedFrequenciesToSave) {
		frequenciesTextToSave.Append(FString::SanitizeFloat(value)).Append(LINE_TERMINATOR);
	}

	FString onsetsTextToSave = "";
	for (float const& value : recognizedOnsetsToSave) {
		onsetsTextToSave.Append(FString::SanitizeFloat(value)).Append(LINE_TERMINATOR);
	}

	if (PlatformFile.CreateDirectoryTree(*SaveDirectory))
	{
		// Get absolute file path
		FString freqAbsoluteFilePath = SaveDirectory + "/" + FileName + "-freq.txt";
		FString onsetAbsoluteFilePath = SaveDirectory + "/" + FileName + "-onset.txt";

		// Allow overwriting or file doesn't already exist
		if (AllowOverwriting || !PlatformFile.FileExists(*freqAbsoluteFilePath))
		{
			FFileHelper::SaveStringToFile(frequenciesTextToSave, *freqAbsoluteFilePath);
		}

		if (AllowOverwriting || !PlatformFile.FileExists(*onsetAbsoluteFilePath))
		{
			FFileHelper::SaveStringToFile(onsetsTextToSave, *onsetAbsoluteFilePath);
		}
	}	
}

void AMicrophoneInput::StartSavingAudioInput(FString SaveDirectory, FString FileName) {
	numberOfSamplesTracked = 0;
	isSavingAudioInput = true;
	string(TCHAR_TO_UTF8(*SaveDirectory));
	FString AbsoluteFilePath = SaveDirectory + "/" + FileName;
	wavFile = new WavFileWritter(string(TCHAR_TO_UTF8(*AbsoluteFilePath)));
	wavFile->writeHeader(sampleRate, channels);
}

void AMicrophoneInput::StopSavingAudioInput() {
	isSavingAudioInput = false;
	wavFile->closeFile();
	wavFile = nullptr;
}
