// Fill out your copyright notice in the Description page of Project Settings.
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

#pragma once

/**
 * 
 */
class MIDI_PROJECT_API WavFileWritter
{
public:
	WavFileWritter(string filename);
	~WavFileWritter();
	void writeHeader(int freq, int channels);
	void writeData(float* buff, const int samples);
	void closeFile();

private:
	ofstream f;
	size_t data_chunk_pos = 0;
	const int bitsPerSample = 16;
	const int maxAmplitude = (int)(FMath::Pow(2.0f, bitsPerSample-1));
};
