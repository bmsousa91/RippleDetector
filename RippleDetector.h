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
    void finishCalibration();
    void detectRipples(std::vector<double>& rmsValuesArr, std::vector<int>& rmsSamplesArr);
    double calculateRms(const float* rawBufferData, int initIndex, int endIndexOpen);

	//Time related variables
	float sampleRate;
	std::chrono::milliseconds refractoryTimeStart;
	std::chrono::milliseconds timeNow;

	//Interface corresponding parameters
	int uiOutputChannel;				//Output channel
	int uiInputChannel;				//Input channel
	double uiSdsThreshold;			//Number of standard deviations above average
	double uiTimeThreshold;			//Time threshold in milliseconds
	//int numRmsSamplesTimeThreshold;	//Number of RMS samples corresponding to time threshold according to the sample rate
	int numSamplesTimeThreshold;	//Number of samples corresponding to time threshold according to the sample rate
	int rmsSamplesCount;			//Counting of RMS samples corresponding to time threshold
	int samplesCount;			//Counting of RMS samples corresponding to time threshold
	unsigned int uiRefractoryTime;	//Refractory time in milliseconds
	int uiRmsSamples;				//Number of samples in each buffer subdivision for calculating the RMS
	int bufferSize;					//Number of pre-allocated samples in each buffer
	int realNumberOfSamples;		//Real number of samples in the buffer (usually less than buffer size, at least for File Reader tests)
	int rmsEndIdx;				//The end index for RMS calculation window

    //RMS statistics
    std::vector<double> rmsValuesArray;
	std::vector<int> rmsSamplesArray;
    std::vector<double> calibrationRmsValues;
    double rmsMean;
    double rmsStd;
	double threshold;				//Final threshold calculation

    //Buffer control
	int pointsProcessed;
	int calibrationPoints;

    //Event flags
	bool isCalibrating { true };
	bool isPluginEnabled { true };
	bool onRefractoryTime { false };
	bool refractoryTimeStartFlag { false };
	bool detectionOnProgress{ false };			// Indicates if the detection process started (there are points above the amplitude threshold)
	bool detectionEnabled { true };				// Indicates if a new detection is enabled
    bool movementDetected { false };

    //TTL event channel
    EventChannel *pTtlEventChannel;

    //Editor
    RippleDetectorEditor *pRippleDetectorEditor;

	//-------------File to save data buffer ---------
	bool writeToFile = false;
	std::fstream file;
	//-----------------------------------------------

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetector);
};

#endif