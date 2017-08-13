// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "tools/kiss_fftnd.h"
#include "MicrophoneInput.h"
#include "VampPluginHost.h"

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
	host = new VampPluginHost(44000, 1024);
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
	tracker->trackedPitches.Empty();
}

void NormalizeBufValues(uint8* inBuf, float* outBuf, int samples) {
	int16_t sample;
	for (uint32 i = 0; i < (uint32)samples; i++)
	{
		//if (i % 100 == 0) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::SanitizeFloat(buf[i]));
		sample = (inBuf[i * 2 + 1] << 8) | inBuf[i * 2];
		outBuf[i] = float(sample) / 32768.0f;
		//

		//}
		//if (float(sample) / 32768.0f > 0) tmp += sampleBuf[i] * sampleBuf[i];
		//if (i % 250 == 0) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(sample));
	}

}

template<typename T>
float IsSilence(T* Buffer, int32 BuffSize)
{
	if (BuffSize == 0)
	{
		return 0;
	}

	const int32 IterSize = BuffSize / sizeof(T);

	double Total = 0.0f;
	for (int32 i = 0; i < IterSize; i++)
	{
		Total += Buffer[i];
	}

	const double Average = Total / IterSize;

	double SumMeanSquare = 0.0f;
	double Diff = 0.0f;
	for (int32 i = 0; i < IterSize; i++)
	{
		Diff = (Buffer[i] - Average);
		SumMeanSquare += (Diff * Diff);
	}

	double AverageMeanSquare = SumMeanSquare / IterSize;

	static double Threshold = 75.0 * 75.0;
	return AverageMeanSquare;// < Threshold;
}

template<typename T>
float GetVolume(T* buf, int bufSize) {
	float total = 0;
	const int32 iterSize = bufSize / sizeof(T);

	for (int i = 0; i < bufSize; i++) {
		total += abs(buf[i]);
		//if (i % 100 == 0)GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::SanitizeFloat(buf[i]).Append(" Part"));
	}
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::SanitizeFloat(total).Append(" Total"));
	const float avg = total / bufSize;
	float sumMeanSquare = 0, diff = 0;
	for (int i = 0; i < bufSize; i++) {
		diff = (buf[i] - avg);
		sumMeanSquare += (diff * diff);
	}
	return (sumMeanSquare / (1.f * bufSize));
}


// Called every frame
void AMicrophoneInput::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	int maxBytes = 0;
	uint32 bytesAvailable = 0;
	EVoiceCaptureState::Type captureState = voiceCapture->GetCaptureState(bytesAvailable);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, EVoiceCaptureState::ToString(captureState));
	if (captureState == EVoiceCaptureState::Ok && bytesAvailable >= 0)
	{
		++tmpCounter;
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(bytesAvailable).Append(" bytesAvailable"));
		maxBytes = bytesAvailable;
		uint8* buf = new uint8[maxBytes];
		memset(buf, 0, maxBytes);
		uint32 readBytes = 0;

		voiceCapture->GetVoiceData(buf, maxBytes, readBytes);
		//if (volume >= silenceTreshold) {
		uint32 samples = 0;
		samples = readBytes / 2;

		float* sampleBuf = new float[samples];
		kiss_fft_cpx in[N], out[N];
		float tmp = 0;

		NormalizeBufValues(buf, sampleBuf, samples);
		volume = GetVolume(sampleBuf, samples);

		if (volume > 0.0000f) {
			for (uint32 i = 0; i < samples; i++) {
				if (i < N) {
					in[i].r = sampleBuf[i], in[i].i = 0;
					out[i].r = 0, out[i].i = 0;
					tmp += sampleBuf[i] * sampleBuf[i];
				}
			}
			//////// HOST /////////////////////
			host->runPlugin("vamp-example-plugins", "zerocrossing", sampleBuf, samples, sampleBuf);
			const Plugin::Feature &f = host->features.at(0).at(0);
			for (unsigned int i = 0; i < f.values.size(); ++i) {
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::SanitizeFloat(f.values[i]));
			}
			///////////////////////////////////
			tmp = tmp / samples;
			tmp = FMath::Sqrt(tmp);
			volume = 20 * log10f(tmp) + 80;
			if (volume > 18.f) {
				//Adding sampleBuf to mainBuf

				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::SanitizeFloat(volume).Append(" glosnosci |||| ").Append(FString::SanitizeFloat(samples).Append(" samples")));

				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(fundamental_frequency).Append("Hz (Base FQ)"));
				kiss_fft_cfg mycfg;

				// Get N samples from sampleBuf for FFT

				if ((mycfg = kiss_fft_alloc(N, 0, NULL, NULL)) != NULL) {
					kiss_fft(mycfg, in, out);
					free(mycfg);
				}
				int peak = 0, peak_idx = 0;

				for (int i = 0; i < N / 2; i++)
				{
					spectrum[i] = sqrt(abs(out[i].r + out[i].i));
					if (spectrum[i] > peak) peak_idx = i, peak = spectrum[i];
				}
				fundamental_frequency = (peak_idx * 44000.0f / (1.0f * N));
				UE_LOG(LogTemp, Log, TEXT("My value: %d"), fundamental_frequency);
				if (!tracker->trackNewNote(fundamental_frequency)) {
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::SanitizeFloat(fundamental_frequency).Append(" Hz. Note unrecognized!"));
				}
				else {
					currentPitch = tracker->currentNote->getName();
				}
			}
			else {
				fundamental_frequency = 0;
				for (int i = 0; i < N / 2; i++)
				{
					spectrum[i] = 0;
				}
			}

		delete[] sampleBuf;
		}



	}
	else {
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, "Missing!");
		//volume = 0;
	}

}

//IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModule, portaudio, "portaudio");


void AMicrophoneInput::SaveStringTextToFile(
FString SaveDirectory,
FString FileName,
FString SaveText
) {

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

