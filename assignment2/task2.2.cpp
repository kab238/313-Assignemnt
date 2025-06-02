//g++ task2.2.cpp smbPitchShift.cpp -o task2.2 -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio

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
#define NUM_SECONDS          (10)
#define BUFFER_SIZE        (1024)
#define INPUT_CHANNEL_NO 1
#define OUTPUT_CHANNEL_NO 1

const long  OSAMP = 32;
const PaSampleFormat SAMPLE_FORMAT = paFloat32;
float pitchShift = 2;
bool finishedReading = false;

PaStreamParameters inputParameters, outputParameters;
PaStream *stream = NULL;
PaError err;

mutex mutex_thread;
float outputBuffer[BUFFER_SIZE];
// initialises queue of audio buffers (each vector is a vector of float samples)
std::queue<std::vector<float>> audioQueue; 
std::condition_variable cond;


static void checkErr(PaError err) {
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        exit(EXIT_FAILURE);
    }
}

void ReadAudioThread(PaStream *stream) {

    //*read the input
    for( int i = 0; i<(NUM_SECONDS*SAMPLE_RATE)/BUFFER_SIZE; i++) {
        
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

    // All samples have finsihed being read, toggle finish flag and notify other thread
    {std::lock_guard<std::mutex> lock(mutex_thread);
    finishedReading = true;
    }
    cond.notify_one();
    return;
}

void PitchShiftThread(){

    while (1) {
        // gives access to the lock shared by both threads
        std::unique_lock<mutex> lock(mutex_thread);

        //waits to be notified for data to be read
        cond.wait(lock, [](){return (!audioQueue.empty() || finishedReading);}); // spurious wake up condiiton

        // condiiton for when the duration of time is over
        if (audioQueue.empty() && finishedReading) {
            break;
        }
        std::vector<float> input = std::move(audioQueue.front());
        audioQueue.pop();
        lock.unlock();
        
        // output buffer thread safe as it's only modified in one thread
        smbPitchShift(pitchShift,BUFFER_SIZE,FRAMES_PER_BUFFER,OSAMP,SAMPLE_RATE,input.data(),outputBuffer);
        err = Pa_WriteStream(stream, outputBuffer, BUFFER_SIZE);
        checkErr(err);
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
    
    err = Pa_StartStream(stream);
    checkErr(err);

  std::thread reader(ReadAudioThread,stream);
  std::thread processor(PitchShiftThread);
    
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