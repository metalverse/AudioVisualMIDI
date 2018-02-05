#include "Midi_Project.h"
#include "WavFileReader.h"
#include <iostream>
#include <cstdio>
#include <cmath>
#include "string.h"
#include "WavFileWritter.h"
//#include "mem.h"


WavFileReader::WavFileReader(const char* fileName, const char* fileToSave)
{
	FILE *fin = fopen(fileName, "rb");

	//Read WAV header
	wav_header_t header;
	fread(&header, sizeof(header), 1, fin);

	//Print WAV header
	//printf("WAV File Header read:\n");
	//printf("File Type: %s\n", header.chunkID);
	//printf("File Size: %ld\n", header.chunkSize);
	//printf("WAV Marker: %s\n", header.format);
	//printf("Format Name: %s\n", header.subchunk1ID);
	//printf("Format Length: %ld\n", header.subchunk1Size);
	//printf("Format Type: %hd\n", header.audioFormat);
	//printf("Number of Channels: %hd\n", header.numChannels);
	//printf("Sample Rate: %ld\n", header.sampleRate);
	//printf("Sample Rate * Bits/Sample * Channels / 8: %ld\n", header.byteRate);
	//printf("Bits per Sample * Channels / 8.1: %hd\n", header.blockAlign);
	//printf("Bits per Sample: %hd\n", header.bitsPerSample);

	bitsPerSample = (int)header.bitsPerSample;
	maxAmplitude = (int)(FMath::Pow(2.0f, bitsPerSample - 1));

	//skip wExtraFormatBytes & extra format bytes
	//fseek(f, header.chunkSize - 16, SEEK_CUR);

	//Reading file
	chunk_t chunk;
	//printf("id\t" "size\n");
	//go to data chunk
	while (true)
	{
		fread(&chunk, sizeof(chunk), 1, fin);
		//printf("%c%c%c%c\t" "%li\n", chunk.ID[0], chunk.ID[1], chunk.ID[2], chunk.ID[3], chunk.size);
		if (*(unsigned int *)&chunk.ID == 0x61746164)
			break;
		//skip chunk data bytes
		fseek(fin, chunk.size, SEEK_CUR);
	}

	//Number of samples
	int sample_size = header.bitsPerSample / 8;
	samplesCount = chunk.size * 8 / header.bitsPerSample;
	//printf("Samples count = %i\n", samples_count);

	short int *value = new short int[samplesCount];
	memset(value, 0, sizeof(short int) * samplesCount);

	dataBuff = new float[samplesCount];

	//Reading data
	for (int i = 0; i < samplesCount; i++)
	{
		fread(&value[i], sample_size, 1, fin);
		dataBuff[i] = (float)(value[i]) / maxAmplitude;
	}
	WavFileWritter writter(std::string(fileName) + "2");
	writter.writeHeader(header.sampleRate, header.numChannels);
	writter.writeData(dataBuff, samplesCount);
	writter.closeFile();
	//Write data into the file
	//FILE *fout = fopen(fileToSave, "w");
	//for (int i = 0; i < samples_count; i++)
	//{
	//	fprintf(fout, "%f\n", ((float)value[i])/maxAmplitude);
	//}
	//fclose(fin);
	//fclose(fout);
}


WavFileReader::~WavFileReader()
{
}

void WavFileReader::getData(float** out, int& buffSize) {
	*out = dataBuff;
	buffSize = samplesCount;
}