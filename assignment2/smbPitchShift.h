#ifndef SMB_PITCH_SHIFT_H
#define SMB_PITCH_SHIFT_H

void smbPitchShift(float pitchShift, 
        long numSampsToProcess, 
        long fftFrameSize, 
        long osamp, 
        float sampleRate, float *indata, float *outdata);

#endif