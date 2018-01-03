// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

#include <iostream>
#include <fstream>
#include <set>
//#include <vamp-hostsdk/sndfile.h>

#include <cstring>
#include <cstdlib>

#include "vamp-hostsdk/system.h"

#include <cmath>

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

class MIDI_PROJECT_API VampPluginHost
{
private:
	float sampleRate = 44100.f;
	int blockSize;
	int stepSize;
	int overlapSize;
	PluginLoader *loaderPyin;
	Plugin *pluginPyin;
public:
	Plugin::FeatureSet features;
	std::vector<float> extractedFeatures;
	VampPluginHost(float, int, int);
	~VampPluginHost();
	void printFeatures(int, int,
		const Plugin::OutputDescriptor &, int,
		const Plugin::FeatureSet &, ofstream *, bool frames);
	void transformInput(float *, size_t);
	void fft(unsigned int, bool, double *, double *, double *, double *);
	void printPluginPath(bool verbose);
	void printPluginCategoryList();
	void enumeratePlugins(Verbosity);
	void listPluginsInLibrary(string soname);
	int runPlugin(string soname, string id, float *inputBuffer, int inputSize);
	std::vector<float> getExtractedFeatures();
};
