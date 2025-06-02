//g++ task2.1.cpp smbPitchShift.cpp -o task2.1 -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portaudio.h"
#include "smbPitchShift.h"
#include <iostream>


#define SAMPLE_RATE       (44100)
#define FRAMES_PER_BUFFER   (1024)
#define NUM_SECONDS          (10)
#define BUFFER_SIZE        (1024)
#define INPUT_CHANNEL_NO 1
#define OUTPUT_CHANNEL_NO 1

//initialise paramters 
const long  OSAMP = 32;
const PaSampleFormat SAMPLE_FORMAT = paFloat32;

PaStreamParameters inputParameters, outputParameters;
PaStream *stream = NULL;
PaError err;

float pitchShift = 1;

static void checkErr(PaError err) {
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        exit(EXIT_FAILURE);
    }
}

int main() {
    
    //initialising portAudio,checking input and output devices
    err = Pa_Initialize();
    checkErr(err);
    int defaultInputDevice = Pa_GetDefaultInputDevice();
    int defaultOutputDevice = Pa_GetDefaultOutputDevice();

    if (defaultInputDevice == paNoDevice || defaultOutputDevice == paNoDevice) {
        std::cerr << "No default input or output device found. " << std::endl;
        Pa_Terminate();
        return -1;
    }

    err = Pa_OpenDefaultStream(&stream, INPUT_CHANNEL_NO, OUTPUT_CHANNEL_NO, SAMPLE_FORMAT, SAMPLE_RATE,FRAMES_PER_BUFFER,nullptr,nullptr);
    checkErr(err);

    /* -- start stream -- */

    err = Pa_StartStream(stream);
    checkErr(err);

    // initialisation of buffers
    float inputBuffer[BUFFER_SIZE];
    float outputBuffer[BUFFER_SIZE];

    std::cout << "running the audio process" << std::endl;

    for( int i = 0; i<(NUM_SECONDS*SAMPLE_RATE)/BUFFER_SIZE; i++) {
        
        //read stream into input buffer
        err = Pa_ReadStream(stream,inputBuffer,BUFFER_SIZE);
        checkErr(err);

        //add pitchshift
        smbPitchShift(pitchShift,BUFFER_SIZE,FRAMES_PER_BUFFER,OSAMP,SAMPLE_RATE,inputBuffer,outputBuffer);

        // write the contents of the output buffer to the stream
        err = Pa_WriteStream(stream, outputBuffer, BUFFER_SIZE);
        checkErr(err);

    }

    std::cout << "Loop stopped. " << std::endl;

    //Stop input and output streams
    err = Pa_StopStream(stream);
    checkErr(err);

    //Close input and output streams
    err = Pa_CloseStream(stream);
    checkErr(err);

    //Terminate PortAudio
    Pa_Terminate();

    return 0;
    
}