// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "VampPluginHost.h"
#include <iostream>


std::vector<std::pair<int, float>> VampPluginHost::getExtractedFeatures() {
	return extractedFeatures;
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
	params.pOverlapSize = params.pBlockSize - params.pStepSize;
	params.pBlockSize = bSize;
	params.pStepSize = sSize;
	UE_LOG(LogTemp, Log, TEXT("Block size: %d"), params.pBlockSize);
	UE_LOG(LogTemp, Log, TEXT("Step size: %d"), params.pStepSize);

	for(auto &parameter : pluginToInit->getParameterDescriptors()) {
		FString paramId = parameter.identifier.c_str();
		UE_LOG(LogTemp, Log, TEXT("Parameter ID: %s ||| Default value: %f ||| Min: %f ||| Max: %f"), *paramId, parameter.defaultValue, parameter.minValue, parameter.maxValue);
	}

	return true;
}



VampPluginHost::VampPluginHost(float sR, int bSize, int sSize)
{
	sampleRate = sR;

	vector<string> path = PluginHostAdapter::getPluginPath();
	FString searchDirectory = path[0].c_str();
	UE_LOG(LogTemp, Log, TEXT("Plugin search directory: %s"), *searchDirectory);

	loader = PluginLoader::getInstance();

	initPlugin(pluginPyin, "pyin", "yin", pyinParams, bSize, sSize);
	initPlugin(pluginOnsetDetector, "vamp-example-plugins", "percussiononsets", onsetDetectorParams, bSize, sSize);

	pluginOnsetDetector->setParameter("treshold", 2.8f);
	pluginOnsetDetector->setParameter("sensitivity", 40.0f);

	UE_LOG(LogTemp, Log, TEXT("Parameter ID: percussiononsets ||| Current treshold: %f ||| Current sensitivity: %f"), pluginOnsetDetector->getParameter("treshold"), pluginOnsetDetector->getParameter("sensitivity"));

}

VampPluginHost::~VampPluginHost(){}

/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

using namespace std;

int VampPluginHost::runPlugin(string soname, string id, float *inputBuffer, int inputSize)
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
	int channels = 1;
	
	int finalStepsRemaining = max(1, (inputSize / params.pStepSize));
	int currentStep = 0;
	
	int blockLeft = inputSize;

	if (!runningPlugin->initialise(channels, params.pStepSize, params.pBlockSize)) {
		UE_LOG(LogTemp, Log, TEXT("Error initializing plugin!"));
		return -1;
	}
	extractedFeatures.clear();
	int	leftRange = 0;
	int rightRange = 0;
	do {
		int count = 0;
		if ((currentStep == 0)) {
			leftRange = 0;
			if (blockLeft > params.pBlockSize) {
				rightRange = params.pBlockSize;
				blockLeft -= params.pBlockSize;
			} else {
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
				leftRange += blockLeft;
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
			while (j < count) {
				plugbuf[c][j] = inputBuffer[(j + leftRange) * channels + c];// 0;//filebuf[j * sfinfo.channels + c];
				//UE_LOG(LogTemp, Log, TEXT("inputBuffer: %f"), plugbuf[c][j]);
				++j;
			}
			while (j < params.pBlockSize) {
				plugbuf[c][j] = 0.0f;
				++j;
			}
		}
		Plugin::OutputList outputs = runningPlugin->getOutputDescriptors();
		Plugin::OutputDescriptor od;

		RealTime rt;

		if (outputs.empty()) {
			UE_LOG(LogTemp, Log, TEXT("Error, plugin has no outputs!"));
		}
		
		rt = RealTime::frame2RealTime(currentStep * params.pStepSize, sampleRate);

		UE_LOG(LogTemp, Log, TEXT("Processing. Current step: %d"), currentStep);
		features = runningPlugin->process(plugbuf, rt);

		od = outputs[0]; //outputsNo = 0

		if (id == "percussiononsets")  {
			UE_LOG(LogTemp, Log, TEXT("RealTime: %s"), rt.toString().c_str());
			if (!(features.find(0) == features.end())) {
				UE_LOG(LogTemp, Log, TEXT("Output 0 has %zu features, values: %zu, RealTime: "), features.at(0).size(), features.at(0).at(0).values.size(), rt.toText().c_str());
			} else if (!(features.find(1) == features.end())) {
				UE_LOG(LogTemp, Log, TEXT("Output 1 has %zu features"), features.at(1).size());
			}
		}
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
	return 0;
}

double VampPluginHost::toSeconds(const RealTime &time)
{
	return time.sec + double(time.nsec + 1) / 1000000000.0;
}

void VampPluginHost::printPluginPath(bool verbose)
{
	if (verbose) {
		cout << "\nVamp plugin search path: ";
	}

	vector<string> path = PluginHostAdapter::getPluginPath();
	for (size_t i = 0; i < path.size(); ++i) {
		if (verbose) {
			cout << "[" << path[i] << "]";
		}
		else {
			cout << path[i] << endl;
		}
	}

	if (verbose) cout << endl;
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
