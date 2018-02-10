// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "VampPluginHost.h"
#include "vamp-sdk/FFT.h"
#include <iostream>


VampPluginHost::VampPluginHost(float sR) : sampleRate(sR) {
	vector<string> path = PluginHostAdapter::getPluginPath();
	FString searchDirectory = path[0].c_str();
	UE_LOG(LogTemp, Log, TEXT("Plugin search directory: %s"), *searchDirectory);

	loadVampPlugin(pluginPyin, "pyin", "yin");
	loadVampPlugin(pluginOnsetDetector, "vamp-example-plugins", "percussiononsets");
};

VampPluginHost::~VampPluginHost(){
	delete[] overlapBufferOnsets;
	delete debugWavFile;
	delete pluginPyin;
	delete pluginOnsetDetector;
}

/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

using namespace std;

int VampPluginHost::runPlugin(string soname, string id, float *inputBuffer, int inputSize, bool runInOverlapMode, int startFrame)
{
	Plugin* runningPlugin;
	pluginParams params;
	if (soname == "pyin" && id == "yin") {
		runningPlugin = pluginPyin;
		params = pyinParams;
	}
	else if (soname == "vamp-example-plugins" && id == "percussiononsets") {
		runningPlugin = pluginOnsetDetector;
		params = onsetDetectorParams;
	}
	else {
		return 1;
	}

	UE_LOG(LogTemp, Log, TEXT("In plugin, size: %d"), inputSize);
	float* processBuffer = nullptr;
	const int channels = 1;
	int samplesLeft = 0;
	int currentStep = 0;

	//Initialize plugin again when no continuous stream from overlap mode provided
	if (!runInOverlapMode) {
		if (!runningPlugin->initialise(channels, params.pStepSize, params.pBlockSize)) {
			UE_LOG(LogTemp, Log, TEXT("Error initializing X plugin!"));
		}
	}
	extractedFeatures.clear();
	UE_LOG(LogTemp, Log, TEXT("Current yinThreshold: %f ||| Current outputunvoiced: %f"), pluginPyin->getParameter("yinThreshold"), pluginPyin->getParameter("outputunvoiced"));


	if (runInOverlapMode) {
		if (id == "yin" && overlapBufferYin != nullptr) {
			UE_LOG(LogTemp, Log, TEXT("Calibration: ENTRY1"));
			processBuffer = new float[inputSize + overlapBufferSizeYin];
			memcpy(processBuffer, overlapBufferYin, sizeof(float) * overlapBufferSizeYin);
			memcpy(processBuffer + overlapBufferSizeYin, inputBuffer, sizeof(float) * inputSize);
			samplesLeft = inputSize + overlapBufferSizeYin;
		} else if (id == "percussiononsets" && overlapBufferOnsets != nullptr) {
			processBuffer = new float[inputSize + overlapBufferSizeOnsets];
			memcpy(processBuffer, overlapBufferOnsets, sizeof(float) * overlapBufferSizeOnsets);
			memcpy(processBuffer + overlapBufferSizeOnsets, inputBuffer, sizeof(float) * inputSize);
			samplesLeft = inputSize + overlapBufferSizeOnsets;
		}
	} else {
		UE_LOG(LogTemp, Log, TEXT("Calibration: ENTRY2"));
		samplesLeft = inputSize;
		processBuffer = inputBuffer;
	}
	int finalStepsRemaining = max(1, (samplesLeft / params.pStepSize));

	if (startFrame > 0) {
		startFrame += 1;
		string filename = to_string(startFrame) + "-" + to_string(startFrame + inputSize - 1) + ".wav";
		debugWavFile = new WavFileWritter("D:\\Studia\\Praca Magisterska\\WavOutput\\Debug\\" + filename);
		debugWavFile->writeHeader(44100, 1);
		debugWavFile->writeData(processBuffer, inputSize + overlapBufferSizeOnsets);
		debugWavFile->closeFile();
	}

	int	leftRange = 0;
	int rightRange = 0;
	int overlapFromSamplesLeft = 0;
	do {
		int count = 0;
		if (currentStep == 0) {
			if (samplesLeft > params.pBlockSize) {
				rightRange = params.pBlockSize;
				samplesLeft -= params.pBlockSize;
			} else if (samplesLeft == params.pBlockSize) {
				rightRange = samplesLeft;
				samplesLeft = 0;
				finalStepsRemaining = 0;
			} else {
				overlapFromSamplesLeft += samplesLeft;
				break;
			}
		} else {
			if (samplesLeft > params.pStepSize) {
				leftRange += params.pStepSize;
				rightRange += params.pStepSize;
				samplesLeft -= params.pStepSize;
			} else if (samplesLeft == params.pStepSize) {
				leftRange += params.pStepSize; //samplesLeft?
				rightRange += samplesLeft;
				samplesLeft = 0;
				finalStepsRemaining = 0;
			} else {
				overlapFromSamplesLeft += samplesLeft;
				break;
			}
		}
		count = rightRange - leftRange;
		
		float **plugbuf = new float*[channels];
		for (int c = 0; c < channels; ++c) {
			plugbuf[c] = new float[params.pBlockSize + 2];
		}
		for (int c = 0; c < channels; ++c) {
			int j = 0;
			int overlapOffset = 0;
			while (j < count) {
				plugbuf[c][j] = processBuffer[(j + leftRange) * channels + c];// 0;//filebuf[j * sfinfo.channels + c];
				++j;
			}
			while (j < params.pBlockSize) {
				UE_LOG(LogTemp, Log, TEXT("Uzupelniam zerami!"));
				plugbuf[c][j] = 0.0f;
				++j;
			}
			//UE_LOG(LogTemp, Log, TEXT("Plugbuf with value: %d, zeroed: %d, input: %d, stepsRemaining: %d"), count, params.pBlockSize-count, inputSize, finalStepsRemaining);
		}
		Plugin::OutputList outputs = runningPlugin->getOutputDescriptors();
		Plugin::OutputDescriptor od;
		RealTime rt;

		if (outputs.empty()) {
			UE_LOG(LogTemp, Log, TEXT("Error, plugin has no outputs!"));
		}
		od = outputs[0]; //outputsNo = 0
		rt = RealTime::frame2RealTime(currentStep * params.pStepSize, sampleRate);

		// Providing data in frequency domain for PercussionOnsets plugin. Time domain for others.
		if (soname == "vamp-example-plugins" && id == "percussiononsets") {
			float** plugbufFft = new float*[channels];
			plugbufFft[0] = new float[params.pBlockSize * 2];

			forwardFft(params.pBlockSize, plugbuf[0], plugbufFft[0]);
			features = runningPlugin->process(plugbufFft, rt);
			delete[] plugbufFft;
		} else {
			features = runningPlugin->process(plugbuf, rt);
		}

		//Checking for features and obtaining timestamps.
		checkForFeatures(od, rt, id);

		delete plugbuf;
		++currentStep;

	} while (finalStepsRemaining > 0);

	//Cleaning and preparing overlap buffers.
	if (runInOverlapMode) {
		delete[] processBuffer;
	}
	if (id == "percussiononsets") {
		if (overlapBufferOnsets != nullptr) {
			delete[] overlapBufferOnsets;
		}
		overlapBufferSizeOnsets = onsetDetectorParams.pStepSize + overlapFromSamplesLeft;
		if (inputSize >= overlapBufferSizeOnsets) {
			overlapBufferOnsets = new float[overlapBufferSizeOnsets];
			memcpy(overlapBufferOnsets, inputBuffer + (inputSize - overlapBufferSizeOnsets), sizeof(float) * overlapBufferSizeOnsets);
		}
		else {
			overlapBufferOnsets = nullptr;
		}
	} else if (id == "yin") {
		if (overlapBufferYin != nullptr) {
			delete[] overlapBufferYin;
		}
		overlapBufferSizeYin = pyinParams.pStepSize + overlapFromSamplesLeft;
		if (inputSize >= overlapBufferSizeYin) {
			overlapBufferYin = new float[overlapBufferSizeYin];
			memcpy(overlapBufferYin, inputBuffer + (inputSize - overlapBufferSizeYin), sizeof(float) * overlapBufferSizeYin);
		} else {
			overlapBufferYin = nullptr;
		}
	}
	return 0;
}

bool VampPluginHost::loadVampPlugin(Plugin* &pluginToInit, const std::string &libName, const std::string &plugName) {

	loader = PluginLoader::getInstance();
	PluginLoader::PluginKey key = loader->composePluginKey(libName, plugName);
	pluginToInit = loader->loadPlugin(key, sampleRate, PluginLoader::ADAPT_ALL_SAFE);
	if (!pluginToInit) {
		UE_LOG(LogTemp, Log, TEXT("Error loading plugin!"));
		return false;
	}
	else {
		FString id = pluginToInit->getIdentifier().c_str();
		UE_LOG(LogTemp, Log, TEXT("Plugin loaded! (%s)"), *id);
	}
	return true;
}

bool VampPluginHost::initializeVampPlugin(const std::string &plugName, const int bSize, const int sSize, TMap<FString, float> params, const int channels = 1) {
	Plugin* pluginToInit = nullptr;
	if (plugName == "yin") {
		pluginToInit = pluginPyin;
		pyinParams.pBlockSize = bSize;
		pyinParams.pStepSize = sSize;
		pyinParams.pOverlapSize = bSize - sSize;
		overlapBufferSizeYin = 2 * sSize;

	} else if(plugName == "percussiononsets") {
		pluginToInit = pluginOnsetDetector;
		onsetDetectorParams.pBlockSize = bSize;
		onsetDetectorParams.pStepSize = sSize;
		onsetDetectorParams.pOverlapSize = bSize - sSize;
		overlapBufferSizeOnsets = 2 * sSize;
	} else {
		UE_LOG(LogTemp, Log, TEXT("Unknown plugin name!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("Preferred block size: %d | Preferred step size: %d"), pluginToInit->getPreferredBlockSize(), pluginToInit->getPreferredStepSize());
	for (const auto& param : params) {
		UE_LOG(LogTemp, Log, TEXT("Changing parameter %s with value %f for plugin %s"), *(param.Key), param.Value, plugName.c_str());
		pluginToInit->setParameter(std::string(TCHAR_TO_UTF8(*param.Key)), param.Value);
	}
	//UE_LOG(LogTemp, Log, TEXT("Parameter ID: percussiononsets ||| Current threshold: %f ||| Current sensitivity: %f"), pluginOnsetDetector->getParameter("threshold"), pluginOnsetDetector->getParameter("sensitivity"));
	if (plugName == "percussiononsets") {
		if (!pluginToInit->initialise(channels, sSize*2, bSize*2)) {
			UE_LOG(LogTemp, Log, TEXT("Error initializing %s plugin!"), plugName.c_str());
			return false;
		}
	} else {
		if (!pluginToInit->initialise(channels, sSize, bSize)) {
			UE_LOG(LogTemp, Log, TEXT("Error initializing %s plugin!"), plugName.c_str());
			return false;
		}
	}
	return true;
}

void VampPluginHost::checkForFeatures(Plugin::OutputDescriptor& od, RealTime& rt, std::string& id) {
	int featureCount = -1;
	if (!(features.find(0) == features.end())) {

		for (size_t i = 0; i < features.at(0).size(); ++i) {
			const Plugin::Feature &f = features.at(0).at(i);
			bool haveRt = false;
			if (od.sampleType == Plugin::OutputDescriptor::VariableSampleRate) {
				rt = f.timestamp;
				haveRt = true;
			}
			else if (od.sampleType == Plugin::OutputDescriptor::FixedSampleRate) {
				int n = featureCount + 1;
				if (f.hasTimestamp) {
					n = int(round(toSeconds(f.timestamp) * od.sampleRate));
				}
				rt = RealTime::fromSeconds(double(n) / od.sampleRate);
				haveRt = true;
				featureCount = n;
			}
			double sec = toSeconds(rt);
			int frame = int(round(sec * sampleRate));
			if (id == "percussiononsets" && overlapBufferOnsets != nullptr) {
				frame -= overlapBufferSizeOnsets;
			}
			else if (id == "yin" && overlapBufferYin != nullptr) {
				frame -= overlapBufferSizeYin;
			}
			if (f.values.size() == 0) {
				extractedFeatures.emplace_back(frame, 0.0f);
				UE_LOG(LogTemp, Log, TEXT("FRAME: %d"), frame);
			}
			for (size_t j = 0; j < f.values.size(); ++j) {
				float outputValue = f.values[j];
				extractedFeatures.emplace_back(frame, outputValue);
				UE_LOG(LogTemp, Log, TEXT("Value: %f"), outputValue);
			}
		}
	}
}

std::vector<std::pair<int, float>> VampPluginHost::getExtractedFeatures() {
	return extractedFeatures;
}

double VampPluginHost::toSeconds(const RealTime &time)
{
	return time.sec + double(time.nsec + 1) / 1000000000.0;
}

//static string header(string text, int level)
//{
//	string out = '\n' + text + '\n';
//	for (size_t i = 0; i < text.length(); ++i) {
//		out += (level == 1 ? '=' : level == 2 ? '-' : '~');
//	}
//	out += '\n';
//	return out;
//}

void VampPluginHost::forwardFft(int n, const float *realInput, float *complexOutput)
{
	kiss_fft_cfg c = kiss_fft_alloc(n, false, NULL, NULL);
	kiss_fft_cpx *in = new kiss_fft_cpx[n];
	kiss_fft_cpx *out = new kiss_fft_cpx[n];
	for (int i = 0; i < n; ++i) {
		out[i].r = 0;
		out[i].i = 0;
		in[i].r = realInput[i];
		in[i].i = 0;
	}
	kiss_fft(c, in, out);
	for (int i = 0; i < n ; ++i) {
		complexOutput[2 * i] = out[i].r;
		complexOutput[2 * i + 1] = out[i].i;
	}
	kiss_fft_free(c);
	delete[] in;
	delete[] out;
}