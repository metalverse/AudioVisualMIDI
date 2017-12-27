// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "VampPluginHost.h"
#include <iostream>


std::vector<float> VampPluginHost::getExtractedFeatures() {
	return extractedFeatures;
}


VampPluginHost::VampPluginHost(float sR, int bSize, int sSize)
{
	sampleRate = sR;
	loader2 = PluginLoader::getInstance();
	//PluginLoader::PluginKey key = loader2->composePluginKey("pyin", "yin");
	PluginLoader::PluginKey key = loader2->composePluginKey("pyin", "yin");
	plugin2 = loader2->loadPlugin(key, sR, PluginLoader::ADAPT_ALL_SAFE);
	vector<string> path = PluginHostAdapter::getPluginPath();
	//if(path.size() > 0) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, path[0].c_str());
	if (!plugin2) {
		UE_LOG(LogTemp, Log, TEXT("Error loading plugin!"));
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, "BLAD WCZYTYWANIA WTYCZKI");
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Wtyczka wczytana!"));
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, plugin2->getIdentifier().c_str());
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, "Wtyczka wczytana!");
	}
	
	blockSize = plugin2->getPreferredBlockSize();
	stepSize = plugin2->getPreferredStepSize();
	UE_LOG(LogTemp, Log, TEXT("Preferred block size: %d"), blockSize);
	UE_LOG(LogTemp, Log, TEXT("Preferred step size: %d"), stepSize);
	
	if (blockSize == 0) {
		blockSize = bSize;
	}
	if (stepSize == 0) {
		if (plugin2->getInputDomain() == Plugin::FrequencyDomain) {
			stepSize = blockSize / 2;
		}
		else {
			stepSize = blockSize;
		}
	}
	else if (stepSize > blockSize) {
		//cerr << "WARNING: stepSize " << stepSize << " > blockSize " << blockSize << ", resetting blockSize to ";
		if (plugin2->getInputDomain() == Plugin::FrequencyDomain) {
			blockSize = stepSize * 2;
		}
		else {
			blockSize = stepSize;
		}
		//cerr << blockSize << endl;
	}
	overlapSize = blockSize - stepSize;
	stepSize = sSize;
	UE_LOG(LogTemp, Log, TEXT("Block size: %d"), blockSize);
	UE_LOG(LogTemp, Log, TEXT("Step size: %d"), stepSize);
}

VampPluginHost::~VampPluginHost(){}

/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

using namespace std;

int VampPluginHost::runPlugin(string soname, string id, float *inputBuffer, int inputSize)
{
	UE_LOG(LogTemp, Log, TEXT("In plugin, size: %d"), inputSize);
	int channels = 1;
	
	int finalStepsRemaining = max(1, (inputSize / stepSize));
	int currentStep = 0;
	
	//float *filebuf = new float[blockSize * channels];
	int blockLeft = inputSize;

	if (!plugin2->initialise(channels, stepSize, blockSize)) {
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
			if (blockLeft > blockSize) {
				rightRange = blockSize;
				blockLeft -= blockSize;
			} else {
				rightRange = blockLeft;
				blockLeft = 0;
				finalStepsRemaining = 0;
			}
		} else {
			if (blockLeft > stepSize) {
				leftRange += stepSize;
				rightRange += stepSize;
				blockLeft -= stepSize;
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
			plugbuf[c] = new float[blockSize + 2];
		}
		for (int c = 0; c < channels; ++c) {
			int j = 0;
			while (j < count) {
				plugbuf[c][j] = inputBuffer[(j + leftRange) * channels + c];// 0;//filebuf[j * sfinfo.channels + c];
				//UE_LOG(LogTemp, Log, TEXT("inputBuffer: %f"), plugbuf[c][j]);
				++j;
			}
			while (j < blockSize) {
				plugbuf[c][j] = 0.0f;
				++j;
			}
		}
		
				//Plugin::OutputList outputs = plugin->getOutputDescriptors();
				//Plugin::OutputDescriptor od;
		

		RealTime rt;

		rt = RealTime::frame2RealTime(currentStep * stepSize, sampleRate);
		UE_LOG(LogTemp, Log, TEXT("Processing. Current step: %d"), currentStep);
		features = plugin2->process(plugbuf, rt);

		if (!(features.find(0) == features.end())) {
			for (size_t i = 0; i < features.at(0).size(); ++i) {
				const Plugin::Feature &f = features.at(0).at(i);
				for (size_t j = 0; j < f.values.size(); ++j) {
					float pluginFreq = f.values[j];
					if (pluginFreq > 0) {
						extractedFeatures.push_back(pluginFreq);
					}
					UE_LOG(LogTemp, Log, TEXT("Value: %f"), pluginFreq);
				}
			}
		}
		//UE_LOG(LogTemp, Log, TEXT("After process"));
		//extractedFeatures
		/*if (!(features.find(0) == features.end())) {
			UE_LOG(LogTemp, Log, TEXT("Features!"));
			for (size_t i = 0; i < features.at(0).size(); ++i) {
				const Plugin::Feature &f = features.at(0).at(i);
				for (size_t j = 0; j < f.values.size(); ++j) {
					UE_LOG(LogTemp, Log, TEXT("Value: %f"), f.values[j]);
				}
			}
		} else {
			UE_LOG(LogTemp, Log, TEXT("Features empty"));
		}*/
		//const Plugin::Feature &f = features.at(0).at(0);

		/*for (unsigned int i = 0; i < f.values.size(); ++i) {
			string sth = std::to_string(f.values[i]);
			UE_LOG(LogTemp, Log, TEXT("Value: %f"), f.values[i]);
		}*/
		delete plugbuf;
		
		++currentStep;
	} while (finalStepsRemaining > 0);
	UE_LOG(LogTemp, Log, TEXT("Plugin loaded!"));
	return 0;
}

static double toSeconds(const RealTime &time)
{
	return time.sec + double(time.nsec + 1) / 1000000000.0;
}

void VampPluginHost::printFeatures(int frame, int sr,
	const Plugin::OutputDescriptor &output, int outputNo,
	const Plugin::FeatureSet &features, ofstream *out, bool useFrames)
{
	static int featureCount = -1;

	if (features.find(outputNo) == features.end()) return;

	for (size_t i = 0; i < features.at(outputNo).size(); ++i) {

		const Plugin::Feature &f = features.at(outputNo).at(i);

		bool haveRt = false;
		RealTime rt;

		if (output.sampleType == Plugin::OutputDescriptor::VariableSampleRate) {
			rt = f.timestamp;
			haveRt = true;
		}
		else if (output.sampleType == Plugin::OutputDescriptor::FixedSampleRate) {
			int n = featureCount + 1;
			if (f.hasTimestamp) {
				n = int(round(toSeconds(f.timestamp) * output.sampleRate));
			}
			rt = RealTime::fromSeconds(double(n) / output.sampleRate);
			haveRt = true;
			featureCount = n;
		}

		if (useFrames) {

			int displayFrame = frame;

			if (haveRt) {
				displayFrame = RealTime::realTime2Frame(rt, sr);
			}

			(out ? *out : cout) << displayFrame;

			if (f.hasDuration) {
				displayFrame = RealTime::realTime2Frame(f.duration, sr);
				(out ? *out : cout) << "," << displayFrame;
			}

			(out ? *out : cout) << ":";

		}
		else {

			if (!haveRt) {
				rt = RealTime::frame2RealTime(frame, sr);
			}

			(out ? *out : cout) << rt.toString();

			if (f.hasDuration) {
				rt = f.duration;
				(out ? *out : cout) << "," << rt.toString();
			}

			(out ? *out : cout) << ":";
		}

		for (unsigned int j = 0; j < f.values.size(); ++j) {
			(out ? *out : cout) << " " << f.values[j];
		}
		(out ? *out : cout) << " " << f.label;

		(out ? *out : cout) << endl;
	}
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

void VampPluginHost::enumeratePlugins(Verbosity verbosity)
{
	PluginLoader *loader = PluginLoader::getInstance();

	if (verbosity == PluginInformation) {
		cout << "\nVamp plugin libraries found in search path:" << endl;
	}

	vector<PluginLoader::PluginKey> plugins = loader->listPlugins();
	typedef multimap<string, PluginLoader::PluginKey>
		LibraryMap;
	LibraryMap libraryMap;

	for (size_t i = 0; i < plugins.size(); ++i) {
		string path = loader->getLibraryPathForPlugin(plugins[i]);
		libraryMap.insert(LibraryMap::value_type(path, plugins[i]));
	}

	string prevPath = "";
	int index = 0;

	for (LibraryMap::iterator i = libraryMap.begin();
		i != libraryMap.end(); ++i) {

		string path = i->first;
		PluginLoader::PluginKey key = i->second;

		if (path != prevPath) {
			prevPath = path;
			index = 0;
			if (verbosity == PluginInformation) {
				cout << "\n  " << path << ":" << endl;
			}
			else if (verbosity == PluginInformationDetailed) {
				string::size_type ki = i->second.find(':');
				string text = "Library \"" + i->second.substr(0, ki) + "\"";
				cout << "\n" << header(text, 1);
			}
		}

		Plugin *plugin = loader->loadPlugin(key, 48000);
		if (plugin) {

			char c = char('A' + index);
			if (c > 'Z') c = char('a' + (index - 26));

			PluginLoader::PluginCategoryHierarchy category =
				loader->getPluginCategory(key);
			string catstr;
			if (!category.empty()) {
				for (size_t ci = 0; ci < category.size(); ++ci) {
					if (ci > 0) catstr += " > ";
					catstr += category[ci];
				}
			}

			if (verbosity == PluginInformation) {

				cout << "    [" << c << "] [v"
					<< plugin->getVampApiVersion() << "] "
					<< plugin->getName() << ", \""
					<< plugin->getIdentifier() << "\"" << " ["
					<< plugin->getMaker() << "]" << endl;

				if (catstr != "") {
					cout << "       > " << catstr << endl;
				}

				if (plugin->getDescription() != "") {
					cout << "        - " << plugin->getDescription() << endl;
				}

			}
			else if (verbosity == PluginInformationDetailed) {

				cout << header(plugin->getName(), 2);
				cout << " - Identifier:         "
					<< key << endl;
				cout << " - Plugin Version:     "
					<< plugin->getPluginVersion() << endl;
				cout << " - Vamp API Version:   "
					<< plugin->getVampApiVersion() << endl;
				cout << " - Maker:              \""
					<< plugin->getMaker() << "\"" << endl;
				cout << " - Copyright:          \""
					<< plugin->getCopyright() << "\"" << endl;
				cout << " - Description:        \""
					<< plugin->getDescription() << "\"" << endl;
				cout << " - Input Domain:       "
					<< (plugin->getInputDomain() == Vamp::Plugin::TimeDomain ?
						"Time Domain" : "Frequency Domain") << endl;
				cout << " - Default Step Size:  "
					<< plugin->getPreferredStepSize() << endl;
				cout << " - Default Block Size: "
					<< plugin->getPreferredBlockSize() << endl;
				cout << " - Minimum Channels:   "
					<< plugin->getMinChannelCount() << endl;
				cout << " - Maximum Channels:   "
					<< plugin->getMaxChannelCount() << endl;

			}
			else if (verbosity == PluginIds) {
				cout << "vamp:" << key << endl;
			}

			Plugin::OutputList outputs =
				plugin->getOutputDescriptors();

			if (verbosity == PluginInformationDetailed) {

				Plugin::ParameterList params = plugin->getParameterDescriptors();
				for (size_t j = 0; j < params.size(); ++j) {
					Plugin::ParameterDescriptor &pd(params[j]);
					cout << "\nParameter " << j + 1 << ": \"" << pd.name << "\"" << endl;
					cout << " - Identifier:         " << pd.identifier << endl;
					cout << " - Description:        \"" << pd.description << "\"" << endl;
					if (pd.unit != "") {
						cout << " - Unit:               " << pd.unit << endl;
					}
					cout << " - Range:              ";
					cout << pd.minValue << " -> " << pd.maxValue << endl;
					cout << " - Default:            ";
					cout << pd.defaultValue << endl;
					if (pd.isQuantized) {
						cout << " - Quantize Step:      "
							<< pd.quantizeStep << endl;
					}
					if (!pd.valueNames.empty()) {
						cout << " - Value Names:        ";
						for (size_t k = 0; k < pd.valueNames.size(); ++k) {
							if (k > 0) cout << ", ";
							cout << "\"" << pd.valueNames[k] << "\"";
						}
						cout << endl;
					}
				}

				if (outputs.empty()) {
					cout << "\n** Note: This plugin reports no outputs!" << endl;
				}
				for (size_t j = 0; j < outputs.size(); ++j) {
					Plugin::OutputDescriptor &od(outputs[j]);
					cout << "\nOutput " << j + 1 << ": \"" << od.name << "\"" << endl;
					cout << " - Identifier:         " << od.identifier << endl;
					cout << " - Description:        \"" << od.description << "\"" << endl;
					if (od.unit != "") {
						cout << " - Unit:               " << od.unit << endl;
					}
					if (od.hasFixedBinCount) {
						cout << " - Default Bin Count:  " << od.binCount << endl;
					}
					if (!od.binNames.empty()) {
						bool have = false;
						for (size_t k = 0; k < od.binNames.size(); ++k) {
							if (od.binNames[k] != "") {
								have = true; break;
							}
						}
						if (have) {
							cout << " - Bin Names:          ";
							for (size_t k = 0; k < od.binNames.size(); ++k) {
								if (k > 0) cout << ", ";
								cout << "\"" << od.binNames[k] << "\"";
							}
							cout << endl;
						}
					}
					if (od.hasKnownExtents) {
						cout << " - Default Extents:    ";
						cout << od.minValue << " -> " << od.maxValue << endl;
					}
					if (od.isQuantized) {
						cout << " - Quantize Step:      "
							<< od.quantizeStep << endl;
					}
					cout << " - Sample Type:        "
						<< (od.sampleType ==
							Plugin::OutputDescriptor::OneSamplePerStep ?
							"One Sample Per Step" :
							od.sampleType ==
							Plugin::OutputDescriptor::FixedSampleRate ?
							"Fixed Sample Rate" :
							"Variable Sample Rate") << endl;
					if (od.sampleType !=
						Plugin::OutputDescriptor::OneSamplePerStep) {
						cout << " - Default Rate:       "
							<< od.sampleRate << endl;
					}
					cout << " - Has Duration:       "
						<< (od.hasDuration ? "Yes" : "No") << endl;
				}
			}

			if (outputs.size() > 1 || verbosity == PluginOutputIds) {
				for (size_t j = 0; j < outputs.size(); ++j) {
					if (verbosity == PluginInformation) {
						cout << "         (" << j << ") "
							<< outputs[j].name << ", \""
							<< outputs[j].identifier << "\"" << endl;
						if (outputs[j].description != "") {
							cout << "             - "
								<< outputs[j].description << endl;
						}
					}
					else if (verbosity == PluginOutputIds) {
						cout << "vamp:" << key << ":" << outputs[j].identifier << endl;
					}
				}
			}

			++index;

			delete plugin;
		}
	}

	if (verbosity == PluginInformation ||
		verbosity == PluginInformationDetailed) {
		cout << endl;
	}
}

void VampPluginHost::printPluginCategoryList()
{
	PluginLoader *loader = PluginLoader::getInstance();

	vector<PluginLoader::PluginKey> plugins = loader->listPlugins();

	set<string> printedcats;

	for (size_t i = 0; i < plugins.size(); ++i) {

		PluginLoader::PluginKey key = plugins[i];

		PluginLoader::PluginCategoryHierarchy category =
			loader->getPluginCategory(key);

		Plugin *plugin = loader->loadPlugin(key, 48000);
		if (!plugin) continue;

		string catstr = "";

		if (category.empty()) catstr = '|';
		else {
			for (size_t j = 0; j < category.size(); ++j) {
				catstr += category[j];
				catstr += '|';
				if (printedcats.find(catstr) == printedcats.end()) {
					std::cout << catstr << std::endl;
					printedcats.insert(catstr);
				}
			}
		}

		std::cout << catstr << key << ":::" << plugin->getName() << ":::" << plugin->getMaker() << ":::" << plugin->getDescription() << std::endl;
	}
}

