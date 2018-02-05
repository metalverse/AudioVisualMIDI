// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

#include <iostream>
#include <fstream>
#include <set>

#include "tools/kiss_fftnd.h"
//#include <vamp-hostsdk/sndfile.h>

#include <cstring>
#include <cstdlib>

#include "vamp-hostsdk/system.h"

#include <cmath>

#include "WavFileWritter.h"

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

enum Verbosity {
	PluginIds,
	PluginOutputIds,
	PluginInformation,
	PluginInformationDetailed
};

struct pluginParams {
	int pBlockSize;
	int pStepSize;
	int pOverlapSize;
};

class MIDI_PROJECT_API VampPluginHost
{
private:
	float sampleRate = 44100.f;
	pluginParams pyinParams;
	pluginParams onsetDetectorParams;
	PluginLoader *loader;
	Plugin *pluginPyin;
	Plugin *pluginOnsetDetector;
	Plugin *pluginOnsetDetector1;
	//Plugin *pluginOnsetDetector2;
	double toSeconds(const RealTime &time);
	bool initPlugin(Plugin* &pluginToInit, const std::string &libName, const std::string &plugName, pluginParams &params, int bSize, int sSize);
	float* overlapBuffer = nullptr;
	int overlapBufferSize;
	int samplesStored = 0;
	WavFileWritter* debugWavFile = nullptr;

public:
	Plugin::FeatureSet features;
	std::vector<std::pair<int, float>> extractedFeatures;
	VampPluginHost(float, int, int, float, float);
	~VampPluginHost();
	void forwardFft(int n, const float *realInput, float *complexOutput);
	int runPlugin(string soname, string id, float *inputBuffer, int inputSize, bool runInOverlapMode, int startFrame);
	std::vector<std::pair<int, float>> getExtractedFeatures();
};
