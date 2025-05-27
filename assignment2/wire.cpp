/*gcc wire.cpp -o main -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio
g++ wire.cpp -o wire -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio


 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portaudio.h"

#define SAMPLE_RATE       (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS       (10)
#define DITHER_FLAG       (0)

#define PA_SAMPLE_TYPE  paFloat32
#define SAMPLE_SIZE     (4)
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"

int main(void) {
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream = NULL;
    PaError err;
    const PaDeviceInfo* inputInfo;
    const PaDeviceInfo* outputInfo;
    char *sampleBlock = NULL;
    int i;
    int numBytes;
    int numChannels;

    printf("patest_read_write_wire.c\n"); fflush(stdout);
    printf("sizeof(int) = %lu\n", sizeof(int)); fflush(stdout);
    printf("sizeof(long) = %lu\n", sizeof(long)); fflush(stdout);

    err = Pa_Initialize();
    if (err != paNoError) goto error2;

    inputParameters.device = Pa_GetDefaultInputDevice();
    printf("Input device # %d.\n", inputParameters.device);
    inputInfo = Pa_GetDeviceInfo(inputParameters.device);
    printf("    Name: %s\n", inputInfo->name);
    printf("      LL: %g s\n", inputInfo->defaultLowInputLatency);
    printf("      HL: %g s\n", inputInfo->defaultHighInputLatency);

    outputParameters.device = Pa_GetDefaultOutputDevice();
    printf("Output device # %d.\n", outputParameters.device);
    outputInfo = Pa_GetDeviceInfo(outputParameters.device);
    printf("   Name: %s\n", outputInfo->name);
    printf("     LL: %g s\n", outputInfo->defaultLowOutputLatency);
    printf("     HL: %g s\n", outputInfo->defaultHighOutputLatency);

    numChannels = inputInfo->maxInputChannels < outputInfo->maxOutputChannels
        ? inputInfo->maxInputChannels : outputInfo->maxOutputChannels;
    printf("Num channels = %d.\n", numChannels);

    inputParameters.channelCount = numChannels;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.channelCount = numChannels;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = outputInfo->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        NULL,
        NULL
    );
    if (err != paNoError) goto error2;

    numBytes = FRAMES_PER_BUFFER * numChannels * SAMPLE_SIZE;
    sampleBlock = (char *) malloc(numBytes);
    if (sampleBlock == NULL) {
        printf("Could not allocate record array.\n");
        goto error1;
    }
    memset(sampleBlock, SAMPLE_SILENCE, numBytes);

    err = Pa_StartStream(stream);
    if (err != paNoError) goto error1;
    printf("Wire on. Will run %d seconds.\n", NUM_SECONDS); fflush(stdout);

    for (i = 0; i < (NUM_SECONDS * SAMPLE_RATE) / FRAMES_PER_BUFFER; ++i) {
        err = Pa_WriteStream(stream, sampleBlock, FRAMES_PER_BUFFER);
        if (err) goto xrun;
        err = Pa_ReadStream(stream, sampleBlock, FRAMES_PER_BUFFER);
        if (err) goto xrun;
    }

    printf("Wire off.\n"); fflush(stdout);

    err = Pa_StopStream(stream);
    if (err != paNoError) goto error1;

    free(sampleBlock);
    Pa_Terminate();
    return 0;

xrun:
    printf("err = %d\n", err); fflush(stdout);
    if (stream) {
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
    }
    free(sampleBlock);
    Pa_Terminate();
    if (err & paInputOverflow)
        fprintf(stderr, "Input Overflow.\n");
    if (err & paOutputUnderflow)
        fprintf(stderr, "Output Underflow.\n");
    return -2;

error1:
    free(sampleBlock);

error2:
    if (stream) {
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
    fprintf(stderr, "An error occurred while using the PortAudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return -1;
}
