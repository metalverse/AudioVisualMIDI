// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "tools/kiss_fftnd.h"
#include "MicrophoneInput.h"

#define SAMPLE_RATE (44100);




// Sets default values
AMicrophoneInput::AMicrophoneInput()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.bCanEverTick = true;
	voiceCapture = FVoiceModule::Get().CreateVoiceCapture();
	voiceCapture->Init(44000, 1);
	voiceCapture->Start();
	spectrum.Init(0, N / 2);
	
}

// Called when the game starts or when spawned
void AMicrophoneInput::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AMicrophoneInput::Tick( float DeltaTime )
{
	Super::Tick(DeltaTime);
	
	int maxBytes = 0;
	uint32 bytesAvailable = 0;
	EVoiceCaptureState::Type captureState = voiceCapture->GetCaptureState(bytesAvailable);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, EVoiceCaptureState::ToString(captureState));
	if (captureState == EVoiceCaptureState::Ok && bytesAvailable >= 0)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(bytesAvailable).Append(" bytesAvailable"));
		maxBytes = bytesAvailable;
		uint8* buf = new uint8[maxBytes];
		memset(buf, 0, maxBytes);
		uint32 readBytes = 0;

		voiceCapture->GetVoiceData(buf, maxBytes, readBytes);

		uint32 samples = 0;
		samples = readBytes / 2;

		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(samples).Append(" samples"));

		float* sampleBuf = new float[samples];
		kiss_fft_cpx in[N], out[N];
		float tmp = 0;
		int16_t sample;
		for (uint32 i = 0; i < samples; i++)
		{
			sample = (buf[i * 2 + 1] << 8) | buf[i * 2];
			sampleBuf[i] = float(sample) / 32768.0f;
			if (i < N) {
				in[i].r = sampleBuf[i], in[i].i = 0;
				out[i].r = 0, out[i].i = 0;

			}
			if (float(sample) / 32768.0f > 0) tmp += sampleBuf[i] * sampleBuf[i];
			//if (i % 250 == 0) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(sample));
		}
		tmp = tmp / samples * 10;
		//tmp = FMath::Sqrt(tmp);
		Volume = 20 * log10f(tmp) + 100;

		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::SanitizeFloat(Volume).Append(" glosnosci |||| ").Append(FString::SanitizeFloat(samples).Append(" samples")));
		

		// Do fun stuff here
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::FromInt(fundamental_frequency).Append("Hz (Base FQ)"));
		kiss_fft_cfg mycfg;
		if ((mycfg = kiss_fft_alloc(N, 0, NULL, NULL)) != NULL) {
			kiss_fft(mycfg, in, out);
			free(mycfg);
		}
		int peak = 0, peak_idx = 0;
		//spectrum[0] = 0;
		//out[0].r = 0;
		for (int i = 0; i < N / 2; i++)
		{
			spectrum[i] = sqrt(abs(out[i].r + out[i].i));
			if (spectrum[i] > peak) peak_idx = i, peak = spectrum[i];
		}

		fundamental_frequency = (peak_idx * 44000.0f / (1.0f * N));
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, FString::SanitizeFloat(fundamental_frequency).Append(" HZ (fundamental frequency) "));
		if (tmpCounter == 5) {
			//FString textToSave = "";
			//FString textToSave2 = "";
			//FString textToSave3 = "";
			for (uint32 i = 0; i < N / 2; i++)
			{
				spectrum[i] = sqrt(abs(out[i].r + out[i].i));
				//textToSave += FString::SanitizeFloat(out[i].r).Append(", ").Append(FString::SanitizeFloat(out[i].i)).Append("; ");
				////textToSave2 += FString::SanitizeFloat(out[i].r).Append("; ");
				//textToSave3 += FString::SanitizeFloat(spectrum[i]).Append("; ");

			}

			//AMircophoneInputController::SaveStringTextToFile("D:", "samples.txt", textToSave);
			////AMicrophoneInput::SaveStringTextToFile("D:", "samples2.txt", textToSave2);
			//AMircophoneInputController::SaveStringTextToFile("D:", "samples3.txt", textToSave3);

		}
		tmpCounter++;




		delete[] sampleBuf; ////////////////////////////
	}
	else {
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, "Missing!");
		//Volume = 0;
	}
	
}

//IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModule, portaudio, "portaudio");

/*
void AMircophoneInputController::SaveStringTextToFile(
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
*/
