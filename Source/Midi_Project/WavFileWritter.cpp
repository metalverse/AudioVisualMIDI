// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "WavFileWritter.h"

namespace little_endian_io
{
	template <typename Word>
	std::ostream& write_word(std::ostream& outs, Word value, unsigned size = sizeof(Word))
	{
		for (; size; --size, value >>= 8)
			outs.put(static_cast <char> (value & 0xFF));
		return outs;
	}
}
using namespace little_endian_io;

WavFileWritter::WavFileWritter(string filename) : f(filename, ios::binary){}

WavFileWritter::~WavFileWritter()
{
	f.clear();
	f.close();
}

void WavFileWritter::writeHeader(int sampleRate, int channels) {
	// Write the file headers
	UE_LOG(LogTemp, Log, TEXT("Max amplitude: %d"), maxAmplitude);
	f << "RIFF----WAVEfmt ";     // (chunk size to be filled in later)
	write_word(f, 16, 4);  // no extension data
	write_word(f, 1, 2);  // 1: WAVE_FORMAT_PCM - integer samples; 3: WAVE_FORMAT_IEEE_FLOAT - float 32 bit
	write_word(f, channels, 2);  // 1 channel for mono and two channels for stereo files
	write_word(f, sampleRate, 4);  // samples per second (Hz)
	write_word(f, (sampleRate * bitsPerSample * channels)/8, 4);  // (Sample Rate * BitsPerSample * Channels) / 8
	write_word(f, channels * 2, 2);  // data block size (size of two integer samples, one for each channel, in bytes)
	write_word(f, bitsPerSample, 2);  // number of bits per sample (use a multiple of 8)

						   // Write the data chunk header
	data_chunk_pos = f.tellp();
	f << "data----";  // (chunk size to be filled in later)
}

void WavFileWritter::writeData(float* buff, const int samples) {
	//Writes data from normalized float samples to int values in range <0, 32768>
	for (int i = 0; i < samples; ++i) {
		int value = (int)(buff[i] * maxAmplitude);
		write_word(f, value, 2);
	}
}

void WavFileWritter::closeFile() {
	// (We'll need the final file size to fix the chunk sizes above)
	size_t file_length = f.tellp();

	// Fix the data chunk header to contain the data size
	f.seekp(data_chunk_pos + 4);
	write_word(f, file_length - data_chunk_pos + 8);

	// Fix the file header to contain the proper RIFF chunk size, which is (file size - 8) bytes
	f.seekp(0 + 4);
	write_word(f, file_length - 8, 4);
	f.close();
}