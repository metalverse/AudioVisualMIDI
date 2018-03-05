// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "MicrophoneInput.h"
#include "VampPluginHost.h"
#include "TempoDetector.h"

#include <algorithm>

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
	if (!(voiceCapture.IsValid())) {
		UE_LOG(LogTemp, Log, TEXT("VoiceCapture not valid!"));
		voiceCapture.Reset();
		voiceCapture = FVoiceModule::Get().CreateVoiceCapture();
		if (!(voiceCapture.IsValid())) {
			UE_LOG(LogTemp, Log, TEXT("VoiceCapture still not valid!"));
		}
	}
	else {
		if (!(voiceCapture->Init(sampleRate, channels))) { 		 //supported range: 8000 - 48000 Hz
			UE_LOG(LogTemp, Log, TEXT("Failed to init VoiceCapture!"));
		}
		else {
			voiceCapture->Start();
			UE_LOG(LogTemp, Log, TEXT("VoiceCapture started."));
		}
	}

	initWeighteningCurveValues();

	tracker = ObjectInitializer.CreateDefaultSubobject<USimplePitchTracker>(this, TEXT("MyPitchTracker"));
	tempoDetector = MakeShareable(new TempoDetector(sampleRate, sampleRate * 3, sampleRate * 8));

	isSilence = true;
	isWaitingForData = true;
	isOnsetDetected = false;
	isInCallibrationMode = false;
}

AMicrophoneInput::~AMicrophoneInput()
{
	delete wavFile;
	if (voiceCapture.IsValid()) {
		voiceCapture->Stop();
		voiceCapture->Shutdown();
	}
	voiceCapture.Reset();
	UE_LOG(LogTemp, Log, TEXT("Closing VoiceCapture."));
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
	vol = 20 * log10f(amplitude) - noiseLevel;
	vol2 = amplitude * 100;
	UE_LOG(LogTemp, Log, TEXT("Max audio value: %f, avarage mean square: %f, normalized ams: %f"), maxSoundValue, AverageMeanSquare, (meanSquare/samples));
	if (isInCallibrationMode) {
		callibratedAMSSum += AverageMeanSquare;
		callibratedVolumeSum += vol;
		++numberOfCheckedBuffers;
	}
	return AverageMeanSquare < silenceTreshold;
}

// Called every frame
void AMicrophoneInput::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	int maxBytes = 0;
	uint32 bytesAvailable = 0;
	EVoiceCaptureState::Type captureState = EVoiceCaptureState::NotCapturing;
	if (voiceCapture.IsValid()) {
		captureState = voiceCapture->GetCaptureState(bytesAvailable);
	}
	FString bufforStatus = EVoiceCaptureState::ToString(captureState);
	//UE_LOG(LogTemp, Log, TEXT("Bytes available: %d"), (int)bytesAvailable);

	if (captureState == EVoiceCaptureState::Ok && bytesAvailable >= 0)
	{
		isWaitingForData = false;
		maxBytes = bytesAvailable;
		uint8* buf = new uint8[maxBytes];
		memset(buf, 0, maxBytes);
		uint32 readBytes = 0;
		voiceCapture->GetVoiceData(buf, maxBytes, readBytes);
		uint32 samples = readBytes / 2;

		UE_LOG(LogTemp, Log, TEXT("New samples: %d"), samples);
		float* sampleBuf = new float[samples];
		isSilence = NormalizeDataAndCheckForSilence((int16*)buf, buf, readBytes, sampleBuf, samples, volumedB, volumeAmplitude);

		if (!isInCallibrationMode) {
			currentNoiseFactor = 0;
			if (!isSilence && samples >= (unsigned)vampStepSize) {
				/////// FREQS /////////
				TrackFundamentalFrequency(sampleBuf, samples);
				lastBufferWasSilence = false;
			} else {
				fundamental_frequency = 0;
				lastBufferWasSilence = true;
			}
			/////// ONSETS /////////
			TrackPercussionOnsets(sampleBuf, samples, isSilence);
			if (isSavingAudioInput) {
				UE_LOG(LogTemp, Log, TEXT("ONSET Number of samples tracked from: %d to %d "), numberOfSamplesTracked, numberOfSamplesTracked + samples);
				numberOfSamplesTracked += samples;
				wavFile->writeData(sampleBuf, samples);
			}
		}
		delete[] sampleBuf;
		delete[] buf;
	}
	else {
		isWaitingForData = true;
	}
}

void AMicrophoneInput::TrackFundamentalFrequency(float* &sampleBuf, int samples) {

	//////////////// YIN /////////////////////
	if (host->runPlugin("yin", sampleBuf, samples, (!lastBufferWasSilence)) != 0) {
		UE_LOG(LogTemp, Log, TEXT("Failed to run yin plugin!"));
	}
	auto features = host->getExtractedFeatures();
	///////////////////////////////////////////
	if (features.size() > 0) {
		if (isRecordingRecognizedFeatures) {
			for (auto feature : features) {
				recognizedFrequenciesToSave.push_back(feature.second);
			}
		}
		std::sort(features.begin(), features.end(), [](auto &left, auto &right) {
			return left.second < right.second;
		});
		const auto median_it = features.begin() + features.size() / 2;
		int median_freq = (*median_it).second;
		if (median_freq > 0) {
			for (auto feature : features) {

				UE_LOG(LogTemp, Log, TEXT("My value: %f"), feature.second);
				fundamental_frequency = feature.second;
				int trackedSoundToMidiNote = 0;
				// If sound not recognized as a music tone
				if (!(fundamental_frequency > 0)) {
					trackedSoundToMidiNote = -1;
					UE_LOG(LogTemp, Log, TEXT("Unrecognized frequency"));
					currentPitch = "?";
				}
				else if (!tracker->trackNewNote(fundamental_frequency)) {
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::SanitizeFloat(fundamental_frequency).Append(" Hz. Note unrecognized!"));
					trackedSoundToMidiNote = -1;
				}
				// If recognized properly
				else {
					trackedSoundToMidiNote = tracker->currentNote->getMidiNoteId();
					currentPitch = tracker->currentNote->getName();
				}
				// Adding tracked note to buffer
				if (bufferedMidiNotes.Num() > 0 && bufferedMidiNotes.Contains(trackedSoundToMidiNote)) {
					int idx = 0;
					bufferedMidiNotes.Find(trackedSoundToMidiNote, idx);
					bufferedMidiNotesOccurences[idx]++;
				}
				else {
					bufferedMidiNotes.Add(trackedSoundToMidiNote);
					bufferedMidiNotesOccurences.Add(1);
				}
			}
		} else {
			float noiseOccurences = 0;
			for (const auto& feature : features) {
				if (feature.second < 0) {
					noiseOccurences += 1;
				}
			}
			currentNoiseFactor = noiseOccurences / features.size();
		}
	}
	if(fundamental_frequency > 0) correctVolumeByRecognizedFrequency(fundamental_frequency);
	else {
		fundamental_frequency = 0;
		UE_LOG(LogTemp, Log, TEXT("Features empty"));
	}
}

void AMicrophoneInput::TrackPercussionOnsets(float* &sampleBuf, int samples, bool isSilence) {
	//////////////// ONSET DETECTOR /////////////////////
	if (host->runPlugin("percussiononsets", sampleBuf, samples, true, numberOfSamplesTracked - 1) != 0) {
		UE_LOG(LogTemp, Log, TEXT("Failed to run percussiononsets plugin!"));
	}
	newTempoTracked = false;
	auto onsetFeatures = host->getExtractedFeatures();
	if (onsetFeatures.size() > 0 && !isSilence) {

		for (auto onsetFeature : onsetFeatures) {
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Silver, FString::FromInt(numberOfSamplesTracked + samples).Append(" SAMPLES"));
//			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Silver, FString::FromInt(numberOfSamplesTracked + onsetFeature.first).Append(" SAMPLE IS ONSET"));
			UE_LOG(LogTemp, Log, TEXT("ONSET Detected onset sample: %d, sound volume: %f"), numberOfSamplesTracked + onsetFeature.first, maxSoundValue);
			if (!newTempoTracked) {
				if (tempoDetector->update(true, samples, onsetFeature.first)) {
					float tmpTempo = tempoDetector->calculateTempo();
					tempoDetector->setCurrentMidiTempo(tmpTempo);
					if (tmpTempo > 0) {
						trackedTempoInBpm = tmpTempo;
						newTempoTracked = true;
					}
				}
			}
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Silver, FString::SanitizeFloat(tmpTempo).Append(" BPM"));
			if (isRecordingRecognizedFeatures) {
				float onsetFrame = 1.0f * numberOfSamplesTracked + onsetFeature.first;
				recognizedOnsetsToSave.push_back(onsetFrame);
			}
		}
		isOnsetDetected = true;
	}
	else {
		tempoDetector->update(false, samples);
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

void AMicrophoneInput::StartCallibration() {
	callibratedAMSSum = 0;
	numberOfCheckedBuffers = 0;
	isInCallibrationMode = true;
}

void AMicrophoneInput::StopCallibration() {
	isInCallibrationMode = false;
}

float AMicrophoneInput::GetSilenceThresholdByCallibration() {
	return (callibratedAMSSum / numberOfCheckedBuffers) * 1.5f;
}

float AMicrophoneInput::GetNoiseLevelByCallibration() {
	return callibratedVolumeSum / numberOfCheckedBuffers;
}

void AMicrophoneInput::SetDetectorMidiTempo(float tempo) {
	tempoDetector->setCurrentMidiTempo(tempo);
}

void AMicrophoneInput::InitVampPlugins() {
	host = new VampPluginHost(sampleRate);// , vampBlockSize, vampStepSize, onsetParamThreshold, onsetParamSensitivity);
	TMap<FString, float> yinParams;
	yinParams.Add("yinThreshold", 0.15f);
	yinParams.Add("outputunvoiced", 1.f);
	host->initializeVampPlugin("yin", 2048, 512, yinParams, (int)channels);
	TMap<FString, float> onsetParams;
	onsetParams.Add("threshold", onsetParamThreshold);
	onsetParams.Add("sensitivity", onsetParamSensitivity);
	host->initializeVampPlugin("percussiononsets", 1024, 512, onsetParams, (int)channels);
}

void AMicrophoneInput::initWeighteningCurveValues() {
	const int freqs[] = { 40,50,63,80,100,125,160,200,250,315,400,500,650,800,1000,1250,1600,2000,2500,3150,4000 };
	const float values[] = { 34.6,30.2,26.2,22.5,19.1,16.1,13.4,10.9,8.6,6.6,4.8,3.2,1.9,0.8,0,-0.6,-1,-1.2,-1.3,-1.2,-1 };
	for (int i = 0; i < 21; ++i) {
		aWeighteningCurveValues.push_back(std::make_pair(freqs[i], values[i]));
	}
}

void AMicrophoneInput::correctVolumeByRecognizedFrequency(float frequency) {
	std::pair<int, float> previous;
	previous = std::make_pair(31, 39.3f);
	for (const auto& point : aWeighteningCurveValues) {
		if (frequency < point.first && frequency > previous.first) {
			float value = ((frequency - previous.first) / (point.first - previous.first) * (point.second - previous.second) + previous.second);
			UE_LOG(LogTemp, Log, TEXT("Freq: %f, increasing volume: %f"), frequency, value);
			volumedB += value;
			return;
		}
		previous = point;
	}
}
