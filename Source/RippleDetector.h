// v060422
#ifndef __RIPPLE_DETECTOR_H
#define __RIPPLE_DETECTOR_H

#include <ProcessorHeaders.h>
#include <stdio.h>			 
#include <iostream>
#include <string>			  
#include <vector>
#include <chrono>

class RippleDetectorEditor;

class RippleDetectorSettings
{
public:

    /** Constructor -- sets default values **/
    RippleDetectorSettings();
    
	/** Destructor **/
    ~RippleDetectorSettings() {}
    
	/** Creates an event associated with ripple detection */
    TTLEventPtr createEvent(int64 outputLine, int64 sample_number, bool state);
    
    // Time-related variables
    std::chrono::milliseconds refractoryTimeStart;
    std::chrono::milliseconds timeNow;

    // Interface corresponding parameters
    int rippleInputChannel;                			    //Input channel
    int rippleOutputChannel;            			    //Output TTL channel
    int movementInputChannel;                			//Movement detection channel
    int movementOutputChannel;            				//Movement detection output TTL channel
    double rippleSds;                					//Number of standard deviations above average to compose the amplitude threshold
    int timeThreshold;            					    //Time threshold (milliseconds)
    unsigned int refractoryTime;    					//Refractory time in milliseconds
    int rmsSamples;                					    //Number of samples in each buffer subdivision for calculating the RMS
    String movSwitch;            					    //Movement detection switch (on/off)
    double movSds;        							    //Number of standard deviations above average for EMG/ACC
    int minTimeWoMov;        							//Minimum time that EMG RMS must be below threshold to enable detection
    int minTimeWMov;        							//Minimum time that EMG RMS must be above threshold to disable detection

    // Internal auxiliary variables
    int numSamplesTimeThreshold;    					//Number of samples corresponding to time threshold according to the sample rate
    int minMovSamplesBelowThresh;    					//Number of samples corresponding to EMG/ACC minimum time below threshold to enable detection
    int minMovSamplesAboveThresh;        				//Number of samples corresponding to EMG minimum time above threshold to disable detection
    unsigned int counterAboveThresh;    				//Accumulate the number of samples when RMS values are above threshold
    int bufferSize;                        				//Number of pre-allocated samples in each buffer
    int realNumberOfSamples;        					//Real number of samples in the buffer (usually lesser than buffer size, at least for File Reader tests)
    int pointsProcessed;            					//Total number of points processed during the calibration step
    int calibrationPoints;                    			//Total number of points that defines the calibration duration
    unsigned int counterMovUpThresh;        			//Accumulate the number of samples when RMS values for EMG/ACC are above threshold
    unsigned int counterMovDownThresh;        			//Accumulate the number of samples when RMS values for EMG/ACC are below threshold
    std::vector<int> auxChannelIndices;        			//Contains the indices of aux channels. Useful for movement detector when "ACCEL" is selected

    // RMS variables
    int rmsEndIdx;            							//The end index for RMS calculation windows
    double rmsMean;            							//Baseline RMS mean for ripple detection
    double rmsStdDev;            						//Baseline RMS standard deviation for ripple detection
    double movRmsMean;        							//Baseline RMS mean for EMG/ACC signal
    double movRmsStdDev;        						//Baseline RMS standard deviation for EMG signal
    double threshold;        							//Final threshold for ripple detection
    double movThreshold;    							//Final EMG/ACC threshold for movement detector

    // Event flags
    bool isCalibrating { true };        				//Is in the calibration step
    bool pluginEnabled{ true };            				//Plugin is enabled to send TTL events
    bool onRefractoryTime { false };    				//On refractory time (ripples cannot be detected during the refractory time)
    bool rippleDetected{ false };        				//Indicates that a ripple was detected
    bool flagTimeThreshold{ false };    				//Indicates that the time threshold was achieved
    bool movSwitchEnabled{ false };        				//EMG switching mechanism is enabled
    bool movChannChanged{ false };        				//User selected new EMG/ACC channel
    bool calibrate{ false };            				//Indicates the need to recalibrate
    bool flagMovMinTimeUp{ false };        				//Indicates that the minimum time of EMG/ACC above threshold was achieved
    bool flagMovMinTimeDown{ false };        			//Indicates that the minimum time of EMG/ACC below threshold was achieved
    bool accelerometerSelected{ false };    			//True when accelerometer is selected to movement detector

    // TTL event channel
    EventChannel* eventChannel;
    
};

class RippleDetector : public GenericProcessor
{
public:
    
    /** Constructor */
    RippleDetector();
    
    /** Destructor */
    ~RippleDetector() {}
    
    /** Creates the custom editor for this plugin */
    AudioProcessorEditor* createEditor() override;
    
    /** Emits an event whenever a ripple is detected */
    void process (AudioBuffer<float>& buffer) override;
    
    /** Called when a processor needs to update its settings */
    void updateSettings() override;

    /** Called when a parameter is updated */
    void parameterValueChanged(Parameter* param) override;

    std::atomic<bool> shouldCalibrate { true };

private:

    RippleDetectorEditor* ed;
    
    StreamSettings<RippleDetectorSettings> settings;

    std::map<uint64, std::vector<double>> rmsValuesArray;
    std::map<uint64, std::vector<int>> rmsNumSamplesArray;
    std::map<uint64, std::vector<double>> movRmsValuesArray;
    std::map<uint64, std::vector<int>> movRmsNumSamplesArray;
    std::map<uint64, std::vector<double>> calibrationRmsValues;
    std::map<uint64, std::vector<double>> calibrationMovRmsValues;
    
    // Ripple-specific functions
    void finishCalibration(uint64 streamId);
    void detectRipples(uint64 streamId);
    void evalMovement(uint64 streamId);
    
    double calculateRms(const float* data, int initIndex, int endIndexOpen);
    double calculateRms(std::vector<float> data, int initIndex, int endIndex);
    std::vector<float> calculateAccelMod(const float* axis[3], int numberOfSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetector);
};

#endif
