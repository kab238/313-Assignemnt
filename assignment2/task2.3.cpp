//g++ task2.3.cpp smbPitchShift.cpp -o task2.3 -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portaudio.h"
#include "smbPitchShift.h"
#include <iostream>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>
using namespace std;


#define SAMPLE_RATE       (44100)
#define FRAMES_PER_BUFFER   (1024)
#define BUFFER_SIZE        (1024)
#define INPUT_CHANNEL_NO 1
#define OUTPUT_CHANNEL_NO 1

const long  OSAMP = 64;
const PaSampleFormat SAMPLE_FORMAT = paFloat32;
float pitchShift = 1;

PaStreamParameters inputParameters, outputParameters;
PaStream *stream = NULL;
PaError err;

mutex mutex_thread,mutex_pitch;
float outputBuffer[BUFFER_SIZE];

// initialises queue of audio buffers (each vector is a vector of float samples)
std::queue<std::vector<float>> audioQueue; 
std::condition_variable cond,pitch_enable;

std::atomic<bool> stop_requested(false);
std::atomic<bool> pitch_enabled(false);

static void checkErr(PaError err) {
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        exit(EXIT_FAILURE);
    }
}

void ReadAudioThread(PaStream *stream) {

    while (!stop_requested) {

        //create temporary input vector of float samples
        std::vector<float> inputBuffer(BUFFER_SIZE);

        //reads stream to input vector
        err = Pa_ReadStream(stream,inputBuffer.data(),BUFFER_SIZE);
        checkErr(err);
    {
        // locks the mutex so PitchShiftThread doesn't have access & move input data to queue
        std::lock_guard<std::mutex> lock(mutex_thread);
        audioQueue.push(std::move(inputBuffer));
    }
    //notify pitchshift thread that one frame has been read to the shared vector
        cond.notify_one();
    }
}

void PitchShiftThread(){

    while (1) {
   
        // gives access to the lock shared by both threads
        std::unique_lock<mutex> lock(mutex_thread);

        //waits to be notified for data to be read or if quit comand ha sbeen entered and the audio queue is empty
        cond.wait(lock, [](){return (!audioQueue.empty() || stop_requested);}); // spurious wake up condiiton

        // exit condition from thread
        if (stop_requested && audioQueue.empty()) {break;}

        //move data from queue to the input vector
        std::vector<float> input = std::move(audioQueue.front());
        audioQueue.pop(); // clear audio queue
        lock.unlock();

    if (pitch_enabled) {
        smbPitchShift(pitchShift,BUFFER_SIZE,FRAMES_PER_BUFFER,OSAMP,SAMPLE_RATE,input.data(),outputBuffer);\
    } else { // fills output buffer with zeros to prevent underrun
        std::fill(outputBuffer, outputBuffer + BUFFER_SIZE, 0.0f);
    }
        err = Pa_WriteStream(stream, outputBuffer, BUFFER_SIZE);
        checkErr(err);
    }
    return;
}

void HotKeyThread() {
    
    char input;
    int passthrough_flag = 0;
    
    while(1) {
        std::cin >> input;

        //exits the program if 'q' is input
        if (input == 'q') {
        // stop_requested is an atomic flag, so when it is changed it notifies the whole program at the same time   
            stop_requested = true; 
            cond.notify_all();
            break;
        }
        // if 's' is input the pitch shifter is enbaled in the pitchshift thread
        if (input == 's') {
            pitch_enabled = true;
        }
        // if 'p' is input, pass through mode is enabled
        if (input == 'p') {
            pitchShift = 1;
            passthrough_flag = 1;  
        }
        //if 'u' is input and the program isn't in passthrough mode add 0.5 units to the pitch shifter
        if ((input == 'u') && (passthrough_flag == 1)) {
           pitchShift += 0.5;
        }
        //if 'd' is input and the program isn't in passthrough mode decrease 0.5 units from the pitch shifter
        if ((input == 'd') && (passthrough_flag == 1)) {
            pitchShift -= 0.5;
        }
}}

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
    
    err = Pa_StartStream(stream);
    checkErr(err);

    std::thread adjustemnt(HotKeyThread);
    std::thread reader(ReadAudioThread,stream);
    std::thread processor(PitchShiftThread);
  
    adjustemnt.join();
    reader.join();
    processor.join();

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