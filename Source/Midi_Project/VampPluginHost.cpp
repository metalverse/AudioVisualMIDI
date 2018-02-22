// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "VampPluginHost.h"
#include "vamp-sdk/FFT.h"
#include <iostream>

using namespace std;

VampPluginHost::VampPluginHost(float sampleRate) : sampleRate(sampleRate) {
	vector<string> path = PluginHostAdapter::getPluginPath();
	FString searchDirectory = path[0].c_str();
	UE_LOG(LogTemp, Log, TEXT("Plugin search directory: %s"), *searchDirectory);

	loadVampPlugin("pyin", "yin");
	loadVampPlugin("vamp-example-plugins", "percussiononsets");
};

VampPluginHost::~VampPluginHost(){
	for (const auto plugin : plugins) {
		delete plugin.second;
	}
	for (const auto buffer : overlapBuffers) {
		delete[] buffer.second;
	}
	//delete[] overlapBufferOnsets;
	//delete debugWavFile;
	//delete pluginPyin;
	//delete pluginOnsetDetector;
}

/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

bool VampPluginHost::loadVampPlugin(const std::string &libName, const std::string &plugName) {

	Plugin* newPlugin;
	loader = PluginLoader::getInstance();
	PluginLoader::PluginKey key = loader->composePluginKey(libName, plugName);
	newPlugin = loader->loadPlugin(key, sampleRate, PluginLoader::ADAPT_ALL_SAFE);
	if (!newPlugin) {
		UE_LOG(LogTemp, Log, TEXT("Error loading plugin!"));
		return false;
	}
	else {
		FString id = newPlugin->getIdentifier().c_str();
		UE_LOG(LogTemp, Log, TEXT("Plugin loaded! (%s)"), *id);
	}
	plugins.insert(std::make_pair(plugName, newPlugin));
	for (auto desc : newPlugin->getParameterDescriptors()) {
		FString paramDesc = desc.description.c_str();
		float defVal = desc.defaultValue;
		UE_LOG(LogTemp, Log, TEXT("Param for plugin: (%s): defVal: %f"), *paramDesc, defVal);
	}
	return true;
}

bool VampPluginHost::initializeVampPlugin(const std::string &plugName, const int blockSize, const int stepSize, 
										  TMap<FString, float>& params, const int channels = 1) {
	Plugin* pluginToInit = nullptr;
	if (plugins.find(plugName) == plugins.end()) {
		UE_LOG(LogTemp, Log, TEXT("Unknown plugin name!"));
		return false;
	}
	pluginToInit = plugins.at(plugName);
	pluginsConfig[plugName].pBlockSize = blockSize;
	pluginsConfig[plugName].pStepSize = stepSize;
	pluginsConfig[plugName].pBufferOverlapSize = stepSize;
	overlapBuffers[plugName] = nullptr;

	UE_LOG(LogTemp, Log, TEXT("Preferred block size: %d | Preferred step size: %d"), pluginToInit->getPreferredBlockSize(), pluginToInit->getPreferredStepSize());
	for (const auto& param : params) {
		UE_LOG(LogTemp, Log, TEXT("Changing parameter %s with value %f for plugin %s"), *(param.Key), param.Value, plugName.c_str());
		pluginToInit->setParameter(std::string(TCHAR_TO_UTF8(*param.Key)), param.Value);
	}
	//UE_LOG(LogTemp, Log, TEXT("Parameter ID: percussiononsets ||| Current threshold: %f ||| Current sensitivity: %f"), pluginOnsetDetector->getParameter("threshold"), pluginOnsetDetector->getParameter("sensitivity"));
	if (plugName == "percussiononsets") {
		if (!pluginToInit->initialise(channels, stepSize * 2, blockSize * 2)) {
			UE_LOG(LogTemp, Log, TEXT("Error initializing %s plugin!"), plugName.c_str());
			return false;
		}
	}
	else {
		if (!pluginToInit->initialise(channels, stepSize, blockSize)) {
			UE_LOG(LogTemp, Log, TEXT("Error initializing %s plugin!"), plugName.c_str());
			return false;
		}
	}
	return true;
}

int VampPluginHost::runPlugin(const std::string& id, float *inputBuffer, int inputSize, bool runInOverlapMode, int startFrame)
{
	Plugin* runningPlugin;
	PluginConfig config;
	FString pluginId = id.c_str();
	//try {
		runningPlugin = plugins.at(id);
		config = pluginsConfig.at(id);
	/*} catch (const std::out_of_range e) {
		UE_LOG(LogTemp, Log, TEXT("No plugin loaded with ID: %s"), *pluginId);
		return -1;
	}*/

	UE_LOG(LogTemp, Log, TEXT("In plugin, size: %d"), inputSize);
	float* processBuffer = nullptr;
	const int channels = 1;
	int samplesLeft = 0;
	int currentStep = 0;

	//Initialize plugin again when no continuous stream from overlap mode provided
	if (!runInOverlapMode) {
		if (!runningPlugin->initialise(channels, config.pStepSize, config.pBlockSize)) {
			UE_LOG(LogTemp, Log, TEXT("Error initializing % plugin!"), *pluginId);
		}
	}
	//Clear features from last runPlugin invocation
	extractedFeatures.clear();

	if (runInOverlapMode) {
		if (overlapBuffers.at(id) != nullptr) {
			processBuffer = new float[inputSize + config.pBufferOverlapSize];
			memcpy(processBuffer, overlapBuffers.at(id), sizeof(float) * config.pBufferOverlapSize);
			memcpy(processBuffer + config.pBufferOverlapSize, inputBuffer, sizeof(float) * inputSize);
			samplesLeft = inputSize + config.pBufferOverlapSize;
		}
	} else {
		samplesLeft = inputSize;
		processBuffer = inputBuffer;
	}
	int finalStepsRemaining = max(1, (samplesLeft / config.pStepSize));

	//if (startFrame > 0) {
	//	startFrame += 1;
	//	string filename = to_string(startFrame) + "-" + to_string(startFrame + inputSize - 1) + ".wav";
	//	debugWavFile = new WavFileWritter("D:\\Studia\\Praca Magisterska\\WavOutput\\Debug\\" + filename);
	//	debugWavFile->writeHeader(44100, 1);
	//	debugWavFile->writeData(processBuffer, inputSize + overlapBufferSizeOnsets);
	//	debugWavFile->closeFile();
	//}

	int	leftRange = 0;
	int rightRange = 0;
	int overlapFromSamplesLeft = 0;
	do {
		int count = 0;
		if (currentStep == 0) {
			if (samplesLeft > config.pBlockSize) {
				rightRange = config.pBlockSize;
				samplesLeft -= config.pBlockSize;
			} else if (samplesLeft == config.pBlockSize) {
				rightRange = samplesLeft;
				samplesLeft = 0;
				finalStepsRemaining = 0;
			} else {
				overlapFromSamplesLeft += samplesLeft;
				break;
			}
		} else {
			if (samplesLeft > config.pStepSize) {
				leftRange += config.pStepSize;
				rightRange += config.pStepSize;
				samplesLeft -= config.pStepSize;
			} else if (samplesLeft == config.pStepSize) {
				leftRange += config.pStepSize; //samplesLeft?
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
			plugbuf[c] = new float[config.pBlockSize + 2];
		}
		for (int c = 0; c < channels; ++c) {
			int j = 0;
			int overlapOffset = 0;
			while (j < count) {
				plugbuf[c][j] = processBuffer[(j + leftRange) * channels + c];// 0;//filebuf[j * sfinfo.channels + c];
				++j;
			}
			while (j < config.pBlockSize) {
				UE_LOG(LogTemp, Log, TEXT("Uzupelniam zerami!"));
				plugbuf[c][j] = 0.0f;
				++j;
			}
			//UE_LOG(LogTemp, Log, TEXT("Plugbuf with value: %d, zeroed: %d, input: %d, stepsRemaining: %d"), count, config.pBlockSize-count, inputSize, finalStepsRemaining);
		}
		Plugin::OutputList outputs = runningPlugin->getOutputDescriptors();
		Plugin::OutputDescriptor od;
		RealTime rt;

		if (outputs.empty()) {
			UE_LOG(LogTemp, Log, TEXT("Error, plugin has no outputs!"));
		}
		od = outputs[0]; //outputsNo = 0
		rt = RealTime::frame2RealTime(currentStep * config.pStepSize, sampleRate);

		// Providing data in frequency domain for PercussionOnsets plugin. Time domain for others.
		if (id == "percussiononsets") {
			float** plugbufFft = new float*[channels];
			plugbufFft[0] = new float[config.pBlockSize * 2];

			forwardFft(config.pBlockSize, plugbuf[0], plugbufFft[0]);
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
		if (overlapBuffers.at(id) != nullptr) {
			delete[] overlapBuffers.at(id);
		}
		config.pBufferOverlapSize = config.pBlockSize - config.pStepSize + overlapFromSamplesLeft;
		if (inputSize >= config.pBufferOverlapSize) {
			overlapBuffers.at(id) = new float[config.pBufferOverlapSize];
			memcpy(overlapBuffers.at(id), inputBuffer + (inputSize - config.pBufferOverlapSize), sizeof(float) * config.pBufferOverlapSize);
		}
		else {
			overlapBuffers.at(id) = nullptr;
		}
	} else if (id == "yin") {
		if (overlapBuffers.at(id) != nullptr) {
			delete[] overlapBuffers.at(id);
		}
		config.pBufferOverlapSize = config.pBlockSize - config.pStepSize + overlapFromSamplesLeft;
		if (inputSize >= config.pBufferOverlapSize) {
			overlapBuffers.at(id) = new float[config.pBufferOverlapSize];
			memcpy(overlapBuffers.at(id), inputBuffer + (inputSize - config.pBufferOverlapSize), sizeof(float) * config.pBufferOverlapSize);
		} else {
			overlapBuffers.at(id) = nullptr;
		}
	}
	return 0;
}

void VampPluginHost::checkForFeatures(Plugin::OutputDescriptor& od, RealTime& rt, const std::string& id) {
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
			if (id == "percussiononsets" && overlapBuffers.at(id) != nullptr) {
				frame -= pluginsConfig.at(id).pBufferOverlapSize;
			}
			else if (id == "yin" && overlapBuffers.at(id) != nullptr) {
				frame -= pluginsConfig.at(id).pBufferOverlapSize;
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