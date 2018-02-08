// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "VampPluginHost.h"
#include "vamp-sdk/FFT.h"
#include <iostream>

std::vector<std::pair<int, float>> VampPluginHost::getExtractedFeatures() {
	return extractedFeatures;
}

VampPluginHost::VampPluginHost(float sR) : sampleRate(sR) {
	loadVampPlugin(pluginPyin, "pyin", "yin");
	loadVampPlugin(pluginOnsetDetector, "vamp-example-plugins", "percussiononsets");
};

VampPluginHost::VampPluginHost(float sR, int bSize, int sSize, float onsetThreshold, float onsetSensitivity)
{
	sampleRate = sR;
	vector<string> path = PluginHostAdapter::getPluginPath();
	FString searchDirectory = path[0].c_str();
	UE_LOG(LogTemp, Log, TEXT("Plugin search directory: %s"), *searchDirectory);

	loader = PluginLoader::getInstance();

	initPlugin(pluginPyin, "pyin", "yin", pyinParams, 2048, 512);
	initPlugin(pluginOnsetDetector, "vamp-example-plugins", "percussiononsets", onsetDetectorParams, 1024, 512);

	overlapBufferSize = 2 * 512;

	UE_LOG(LogTemp, Log, TEXT("Onset program size: %d"), pluginOnsetDetector->getPrograms().size());
	for (auto program : pluginOnsetDetector->getOutputDescriptors()) {
		UE_LOG(LogTemp, Log, TEXT("Onset output: %s"), program.description.c_str());
	}
	for (auto program : pluginPyin->getPrograms()) {
		UE_LOG(LogTemp, Log, TEXT("Yin program: %s"), program.c_str());
	}
	pluginOnsetDetector->setParameter("threshold", 5);
	pluginOnsetDetector->setParameter("sensitivity", 35);

	//pluginOnsetDetector1->setParameter("threshold", 20);
	//pluginOnsetDetector1->setParameter("sensitivity", 100);
	//if (!pluginOnsetDetector->initialise(1, pyinParams.pStepSize, pyinParams.pBlockSize)) {
	//		UE_LOG(LogTemp, Log, TEXT("Error initializing yin plugin!"));
	//}
	//if (!pluginOnsetDetector->initialise(1, 2*onsetDetectorParams.pStepSize, 2*onsetDetectorParams.pBlockSize)) {
	//	UE_LOG(LogTemp, Log, TEXT("Error initializing onset plugin!"));
	//}

	UE_LOG(LogTemp, Log, TEXT("Parameter ID: percussiononsets ||| Current threshold: %f ||| Current sensitivity: %f"), pluginOnsetDetector->getParameter("threshold"), pluginOnsetDetector->getParameter("sensitivity"));
}

VampPluginHost::~VampPluginHost(){
	delete[] overlapBuffer;
	delete debugWavFile;
	delete pluginPyin;
	delete pluginOnsetDetector;
}

bool VampPluginHost::initPlugin(Plugin* &pluginToInit, const std::string &libName, const std::string &plugName, pluginParams &params, int bSize, int sSize) {

	PluginLoader::PluginKey key = loader->composePluginKey(libName, plugName);

	pluginToInit = loader->loadPlugin(key, sampleRate, PluginLoader::ADAPT_ALL_SAFE);

	if (!pluginToInit) {
		UE_LOG(LogTemp, Log, TEXT("Error loading plugin!"));
		return false;
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Plugin loaded! (%s)"), pluginToInit->getIdentifier().c_str());
	}

	params.pBlockSize = pluginToInit->getPreferredBlockSize();
	params.pStepSize = pluginToInit->getPreferredStepSize();
	UE_LOG(LogTemp, Log, TEXT("Preferred block size: %d"), params.pBlockSize);
	UE_LOG(LogTemp, Log, TEXT("Preferred step size: %d"), params.pStepSize);

	if (params.pBlockSize == 0) {
		params.pBlockSize = bSize;
	}
	if (params.pStepSize == 0) {
		if (pluginToInit->getInputDomain() == Plugin::FrequencyDomain) {
			params.pStepSize = params.pBlockSize / 2;
		}
		else {
			params.pStepSize = params.pBlockSize;
		}
	}
	else if (params.pStepSize > params.pBlockSize) {
		if (pluginToInit->getInputDomain() == Plugin::FrequencyDomain) {
			params.pBlockSize = params.pStepSize * 2;
		}
		else {
			params.pBlockSize = params.pStepSize;
		}
	}
	//if (pluginToInit->getInputDomain() == Plugin::FrequencyDomain) {
	//	params.pBlockSize = params.pStepSize * 2;
	//}
	params.pOverlapSize = params.pBlockSize - params.pStepSize;
	params.pBlockSize = bSize;
	params.pStepSize = sSize;
	UE_LOG(LogTemp, Log, TEXT("Block size: %d"), params.pBlockSize);
	UE_LOG(LogTemp, Log, TEXT("Step size: %d"), params.pStepSize);

	for (auto &parameter : pluginToInit->getParameterDescriptors()) {
		FString paramId = parameter.identifier.c_str();
		UE_LOG(LogTemp, Log, TEXT("Parameter ID: %s ||| Default value: %f ||| Min: %f ||| Max: %f"), *paramId, parameter.defaultValue, parameter.minValue, parameter.maxValue);
	}

	return true;
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

	float* processBuffer = nullptr;
	int blockLeft = 0;

	if (runInOverlapMode && overlapBuffer != nullptr) {
		processBuffer = new float[inputSize + overlapBufferSize];
		memcpy(processBuffer, overlapBuffer, sizeof(float) * overlapBufferSize);
		memcpy(processBuffer + overlapBufferSize, inputBuffer, sizeof(float) * inputSize);
		blockLeft = inputSize + overlapBufferSize;
	}
	else {
		blockLeft = inputSize;
		processBuffer = inputBuffer;
	}

	//if (startFrame > 0) {
	//	startFrame += 1;
	//	string filename = to_string(startFrame) + "-" + to_string(startFrame + inputSize - 1) + ".wav";
	//	debugWavFile = new WavFileWritter("D:\\Studia\\Praca Magisterska\\WavOutput\\Debug\\" + filename);
	//	debugWavFile->writeHeader(44100, 1);
	//	debugWavFile->writeData(processBuffer, inputSize + overlapBufferSize);
	//	debugWavFile->closeFile();
	//}

	UE_LOG(LogTemp, Log, TEXT("In plugin, size: %d"), inputSize);
	int channels = 1;
	
	int finalStepsRemaining = max(1, (blockLeft / params.pStepSize));

	int currentStep = 0;

	if (!runningPlugin->initialise(1, params.pStepSize, params.pBlockSize)) {
		UE_LOG(LogTemp, Log, TEXT("Error initializing X plugin!"));
	}

	extractedFeatures.clear();
	int	leftRange = 0;
	int rightRange = 0;
	do {
		int count = 0;
		if (currentStep == 0) {
			leftRange = 0;
			if (blockLeft > params.pBlockSize) {
				rightRange = params.pBlockSize;
				blockLeft -= params.pBlockSize;
			}
			else {
				rightRange = blockLeft;
				blockLeft = 0;
				finalStepsRemaining = 0;
			}
		} else {
			if (blockLeft > params.pStepSize) {
				leftRange += params.pStepSize;
				rightRange += params.pStepSize;
				blockLeft -= params.pStepSize;
			} else {
				leftRange += params.pStepSize; //blockLeft?
				rightRange += blockLeft;
				blockLeft = 0;
				finalStepsRemaining = 0;
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
		
		rt = RealTime::frame2RealTime(currentStep * params.pStepSize, sampleRate);

		if (soname == "vamp-example-plugins" && id == "percussiononsets") {
			float** plugbufFft = new float*[channels];
			plugbufFft[0] = new float[params.pBlockSize * 2];
			/*for (int i = 0; i < (params.pBlockSize * 2); ++i) {
				plugbufFft[0][i] = 0;
			}*/
			forwardFft(params.pBlockSize, plugbuf[0], plugbufFft[0]);
			features = runningPlugin->process(plugbufFft, rt);
			delete[] plugbufFft;
		} else {
			features = runningPlugin->process(plugbuf, rt);
		}

		od = outputs[0]; //outputsNo = 0

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
		delete plugbuf;
		++currentStep;
	} while (finalStepsRemaining > 0);
	if (runInOverlapMode && overlapBuffer != nullptr) {
		delete[] processBuffer;
	}
	if (runInOverlapMode) {
		delete[] overlapBuffer;
		if (inputSize >= overlapBufferSize) {
			overlapBuffer = new float[overlapBufferSize];
			memcpy(overlapBuffer, inputBuffer + (inputSize - overlapBufferSize), sizeof(float) * overlapBufferSize);
		}
		else {
			overlapBuffer = nullptr;
		}
	}
	return 0;
}


bool VampPluginHost::loadVampPlugin(Plugin* &pluginToInit, const std::string &libName, const std::string &plugName) {

	PluginLoader::PluginKey key = loader->composePluginKey(libName, plugName);
	pluginToInit = loader->loadPlugin(key, sampleRate, PluginLoader::ADAPT_ALL_SAFE);
	if (!pluginToInit) {
		UE_LOG(LogTemp, Log, TEXT("Error loading plugin!"));
		return false;
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Plugin loaded! (%s)"), pluginToInit->getIdentifier().c_str());
	}
	return true;
}

bool VampPluginHost::initializeVampPlugin(const std::string &plugName, const int bSize, const int sSize, TMap<FString, float> params, const int channels = 1) {
	Plugin* pluginToInit = nullptr;
	UE_LOG(LogTemp, Log, TEXT("Preferred block size: %d | Preferred step size: %d"), pluginToInit->getPreferredBlockSize(), pluginToInit->getPreferredStepSize());
	if (plugName == "yin") {
		pluginToInit = pluginPyin;
		pyinParams.pBlockSize = bSize;
		pyinParams.pStepSize = sSize;
		pyinParams.pOverlapSize = bSize - sSize;
	} else if(plugName == "percussiononsets") {
		pluginToInit = pluginOnsetDetector;
		pyinParams.pBlockSize = bSize;
		pyinParams.pStepSize = sSize;
		pyinParams.pOverlapSize = bSize - sSize;
		overlapBufferSize = 2 * sSize;
	} else {
		UE_LOG(LogTemp, Log, TEXT("Unknown plugin name!"));
		return false;
	}
	for (const auto& param : params) {
		UE_LOG(LogTemp, Log, TEXT("Changing parameter %s with value %f for plugin %s"), *(param.Key), param.Value, plugName.c_str());
		pluginToInit->setParameter(std::string(TCHAR_TO_UTF8(*param.Key)), param.Value);
	}
	if (!pluginToInit->initialise(channels, sSize, bSize)) {
		UE_LOG(LogTemp, Log, TEXT("Error initializing %s plugin!"), plugName.c_str());
		return false;
	}
	return true;
}

double VampPluginHost::toSeconds(const RealTime &time)
{
	return time.sec + double(time.nsec + 1) / 1000000000.0;
}

static string header(string text, int level)
{
	string out = '\n' + text + '\n';
	for (size_t i = 0; i < text.length(); ++i) {
		out += (level == 1 ? '=' : level == 2 ? '-' : '~');
	}
	out += '\n';
	return out;
}

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