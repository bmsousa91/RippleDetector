#ifndef __RIPPLE_DETECTOR_H
#define __RIPPLE_DETECTOR_H

#include <ProcessorHeaders.h>
#include <stdio.h>			 
#include <iostream>
#include <fstream>
#include <string>			  
#include <vector>
#include <chrono>

#include "RippleDetectorEditor.h" 

class RippleDetector : public GenericProcessor
{
public:
    RippleDetector();
    ~RippleDetector();
    void process(AudioSampleBuffer &rInContinuousBuffer) override;
    bool enable() override;
    void handleEvent(const EventChannel *rInChannelInfo, const MidiMessage &rInEvent, int sampleNum) override;
    AudioProcessorEditor *createEditor() override;
    void createEventChannels() override;
    void updateSettings() override;
    void sendTtlEvent(int rmsIndex, int val);

    //Ripple-specific functions
    void calibrate();
    void detectRipples(std::vector<double> &rInRmsBuffer);
    double calculateRms(const float *rInBuffer, int initIndex, int endIndexOpen);

    //Time related variables
    float sampleRate;
    std::chrono::milliseconds refractoryTimeStart;
    std::chrono::milliseconds timeNow;

    //Interface corresponding parameters
    int outputChannel;				//Output channel
    int inputChannel;				//Input channel
    double thresholdSds;			//Number of standard deviations above average
    double thresholdTime;			//Time threshold in milliseconds
    int numRmsSamplesThresholdTime;	//Number of RMS samples corresponding to time threshold according to the sample rate
    int rmsSamplesCount;			//Counting of RMS samples corresponding to time threshold
    unsigned int refractoryTime;	//Refractory time in milliseconds
    int rmsBlockSize;				//Number of samples in each buffer subdivision for calculating the RMS
    int bufferSize;					//Number of pre-allocated samples in each buffer
    int realNumberOfSamples;		//Real number of samples in the buffer (usually less than buffer size, at least for File Reader tests)
    int rmsEndIndex;				//The end index for RMS calculation window

    //RMS statistics
    std::vector<double> localRms;
    std::vector<double> calibrationRms;
    double rmsMean;
    double rmsStandardDeviation;
    double threshold;				//Final threshold calculation

    //Buffer control
    int currentBuffer;
    int calibrationBuffers;

    //Event flags
    bool isCalibrating;
    bool isPluginEnabled;
    bool onRefractoryTime;
    bool refractoryTimeStartFlag { false };
    bool detected;
    bool detectionEnabled;
    bool movementDetected;

    //TTL event channel
    EventChannel *pTtlEventChannel;

    //Editor
    RippleDetectorEditor *pRippleDetectorEditor;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetector);
};

#endif
