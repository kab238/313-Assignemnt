//g++ task2.1.cpp -o task2.1 -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio
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

const long  OSAMP = 64;
const PaSampleFormat SAMPLE_FORMAT = paFloat32;


PaStreamParameters inputParameters, outputParameters;
PaStream *stream = NULL;
PaError err;

float pitchShift = 2;

 const bool PRINTDEFAULTDEVICEONLY = true;
// void printDeviceInfo (const PaDeviceInfo* deviceInfo, const char* deviceType, bool printDefaultDevice) {
//     if (printDefaultDevice == true) {
//         std::cout << "Default: " << deviceType << " Device: "<< deviceInfo->name << std::endl;

//     }
//     else{
//         std::cout << deviceType << " Device: "<< deviceInfo->name << std::endl;
//     }
//         std::cout << "   Input Channels: " << deviceInfo->maxInputChannels << std::endl;
//         std::cout << "   Input Channels: " << deviceInfo->maxOutputChannels << std::endl;
//         std::cout << "   Input Channels: " << deviceInfo->defaultSampleRate << std::endl;
//         std::cout << std::endl;
    
// }

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
/*
    std::cout << "Available audio devices:" << std::endl;
    for (int i = 0; i < Pa_GetDeviceCount(); ++i) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

        if (PRINTDEFAULTDEVICEONLY == true) {
            if (i == defaultInputDevice) {
                printDeviceInfo(deviceInfo, "Input", true);
            }

            if (i == defaultOutputDevice) {
                printDeviceInfo(deviceInfo,"Output",true);
            }
        }
        else {
            printDeviceInfo(deviceInfo,"Input", false);
        }
    }
*/
    err = Pa_OpenDefaultStream(&stream, INPUT_CHANNEL_NO, OUTPUT_CHANNEL_NO, SAMPLE_FORMAT, SAMPLE_RATE,FRAMES_PER_BUFFER,nullptr,nullptr);
    checkErr(err);

    /* -- start stream -- */
    err = Pa_StartStream(stream);
    checkErr(err);

    float inputBuffer[BUFFER_SIZE];
    float outputBuffer[BUFFER_SIZE];

    std::cout << "running the audio process" << std::endl;

    for( int i = 0; i<(NUM_SECONDS*SAMPLE_RATE)/BUFFER_SIZE; i++) {
        
        err = Pa_ReadStream(stream,inputBuffer,BUFFER_SIZE);
        checkErr(err);

        //add pitchshift
        smbPitchShift(pitchShift,BUFFER_SIZE,FRAMES_PER_BUFFER,OSAMP,SAMPLE_RATE,inputBuffer,outputBuffer);

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