// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

#include <iostream>
#include <fstream>
#include <set>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include "tools/kiss_fftnd.h"
//#include <vamp-hostsdk/sndfile.h>
#include "vamp-hostsdk/system.h"

//#include "WavFileWritter.h"

using namespace std;

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

#define HOST_VERSION "1.5"

//#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//#endif  /* NOMINMAX */

//enum Verbosity {
//	PluginIds,
//	PluginOutputIds,
//	PluginInformation,
//	PluginInformationDetailed
//};

struct PluginConfig {
	int pBlockSize;
	int pStepSize;
	int pBufferOverlapSize;
};

class MIDI_PROJECT_API VampPluginHost
{
private:
	Plugin::FeatureSet features;
	PluginLoader *loader;
	std::map<std::string, Plugin*> plugins;
	std::map<std::string, PluginConfig> pluginsConfig;
	std::map<std::string, float*> overlapBuffers;
	float sampleRate = 44100.f;
	//WavFileWritter* debugWavFile = nullptr;
	std::vector<std::pair<int, float>> extractedFeatures;
	bool loadVampPlugin(const std::string &libName, const std::string &plugName);
	void checkForFeatures(Plugin::OutputDescriptor& od, RealTime& rt, const std::string& pluginId);
	double toSeconds(const RealTime &time);

public:
	VampPluginHost(const float sampleRate);
	~VampPluginHost();
	bool initializeVampPlugin(const std::string &pluginId, const int blockSize, const int stepSize, TMap<FString, float>& params, const int channels);
	int runPlugin(const std::string& pluginId, float *inputBuffer, const int inputSize, bool runInOverlapMode, int startFrame = -1);
	void forwardFft(const int n, const float *realInput, float *complexOutput);
	std::vector<std::pair<int, float>> getExtractedFeatures();
};
