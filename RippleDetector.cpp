#include "RippleDetector.h"

#define MINIMUM_RMS_BLOCK_SIZE 1
#define MINIMUM_THRESHOLD_AMP 0
#define MINIMUM_THRESHOLD_TIME 0
#define MINIMUM_REFRACTORY_TIME 0
#define CALIBRATION_DURATION_SECONDS 20

RippleDetector::RippleDetector()
    : GenericProcessor("RippleDetector")
{
	//Init variables
	pointsProcessed = 0;
    rmsMean = 0.0;
    rmsStd = 0.0;
    threshold = 0.0;
    uiSdsThreshold = 1.0f;
	uiTimeThreshold = 5.0f;
	sampleRate = CoreServices::getGlobalSampleRate();
	calibrationPoints = sampleRate*CALIBRATION_DURATION_SECONDS;

    setProcessorType(PROCESSOR_TYPE_FILTER);
    createEventChannels();

	if (writeToFile) {
		file.open("data.csv", std::ios::app);
		file.precision(12);
	}

}

RippleDetector::~RippleDetector()
{
	if(writeToFile) file.close();
}

bool RippleDetector::enable()
{
    return true;
}

void RippleDetector::createEventChannels()
{
    const DataChannel *in = getDataChannel(0);
	sampleRate = (in) ? in->getSampleRate() : CoreServices::getGlobalSampleRate();
    pTtlEventChannel = new EventChannel(EventChannel::TTL, 8, 1, sampleRate, this);
    MetaDataDescriptor md(MetaDataDescriptor::CHAR, 34, "High frequency detection type", "Description of the frequency", "channelInfo.extra");
    MetaDataValue mv(md);
    pTtlEventChannel->addMetaData(md, mv);


    if (in)
    {
        md = MetaDataDescriptor(MetaDataDescriptor::UINT16,
                                3,
                                "Detection module",
                                "Index at its source, Source processor ID and Sub Processor index of the channel that triggers this event",
                                "source.channel.identifier.full");
        mv = MetaDataValue(md);
        uint16 source_info[3];
        source_info[0] = in->getSourceIndex();
        source_info[1] = in->getSourceNodeID();
        source_info[2] = in->getSubProcessorIdx();
        mv.setValue(static_cast<const uint16 *>(source_info));
        pTtlEventChannel->addMetaData(md, mv);
    }

    eventChannelArray.add(pTtlEventChannel);
}

void RippleDetector::updateSettings()
{
    const DataChannel *in = getDataChannel(0);
	sampleRate = (in) ? in->getSampleRate() : CoreServices::getGlobalSampleRate();
    pTtlEventChannel = new EventChannel(EventChannel::TTL, 8, 1, sampleRate, this);
    MetaDataDescriptor md(MetaDataDescriptor::CHAR, 34, "High frequency detection type", "Description of the frequency", "channelInfo.extra");
    MetaDataValue mv(md);
    pTtlEventChannel->addMetaData(md, mv);

    if (in)
    {
        md = MetaDataDescriptor(MetaDataDescriptor::UINT16,
                                3,
                                "Detection module",
                                "Index at its source, Source processor ID and Sub Processor index of the channel that triggers this event",
                                "source.channel.identifier.full");
        mv = MetaDataValue(md);
        uint16 source_info[3];
        source_info[0] = in->getSourceIndex();
        source_info[1] = in->getSourceNodeID();
        source_info[2] = in->getSubProcessorIdx();
        mv.setValue(static_cast<const uint16 *>(source_info));
        pTtlEventChannel->addMetaData(md, mv);
    }

    eventChannelArray.add(pTtlEventChannel);
}

AudioProcessorEditor *RippleDetector::createEditor()
{
    pRippleDetectorEditor = new RippleDetectorEditor(this, true);
    editor = pRippleDetectorEditor;
    return editor;
}

void RippleDetector::process(AudioSampleBuffer &rInBuffer)
{
	//Update parameters according to UI
	const DataChannel *in = getDataChannel(0);

	uiOutputChannel = pRippleDetectorEditor->pluginUi._outChannel - 1;
	uiInputChannel = pRippleDetectorEditor->pluginUi._inChannel - 1;
	uiSdsThreshold = pRippleDetectorEditor->pluginUi._thresholdSds;
	uiTimeThreshold = pRippleDetectorEditor->pluginUi._thresholdTime;
	uiRefractoryTime = pRippleDetectorEditor->pluginUi._refractoryTime;
	uiRmsSamples = pRippleDetectorEditor->pluginUi._rmsSamples;
	bufferSize = rInBuffer.getNumSamples();
	realNumberOfSamples = GenericProcessor::getNumSamples(uiInputChannel);

	// Sometimes it occurs that the number of samples is 0
	// In this case, finishes the process
	if (realNumberOfSamples == 0) return;
	
	// Check recalibration
	if (pRippleDetectorEditor->pluginUi._calibrate == true)
	{
		printf("Calibrating...\n");
		pRippleDetectorEditor->pluginUi._calibrate = false;
		isCalibrating = true;
		pointsProcessed = 0;

		//Reset calibration RMS array
		calibrationRmsValues.clear();
	}

	// Check parameters range and adjust if necessary,
	// including updates in the plugin interface
	if (uiRefractoryTime < MINIMUM_REFRACTORY_TIME)
		uiRefractoryTime = MINIMUM_REFRACTORY_TIME;

	if (uiRmsSamples < MINIMUM_RMS_BLOCK_SIZE)
		uiRmsSamples = MINIMUM_RMS_BLOCK_SIZE;
	if (uiRmsSamples > realNumberOfSamples)
		uiRmsSamples = realNumberOfSamples;

	if (uiSdsThreshold < MINIMUM_THRESHOLD_AMP)
		uiSdsThreshold = MINIMUM_THRESHOLD_AMP;

	if (uiTimeThreshold < MINIMUM_THRESHOLD_TIME)
		uiTimeThreshold = MINIMUM_THRESHOLD_TIME;

	// Calculate final threshold and numSamplesTimeThreshold (number of 
	// samples equivalent to the time threshold)
	threshold = rmsMean + uiSdsThreshold * rmsStd;
	numSamplesTimeThreshold = ceil(sampleRate * uiTimeThreshold / 1000);

	// Get raw data
	const float *rawSamples = rInBuffer.getReadPointer(uiInputChannel);

	// Guarante that the array with RMS values will be clean
	rmsValuesArray.clear();
	rmsSamplesArray.clear();

	//-------------- Raw data ---------------------
	//String completeString;
	//for (int i = 0; i < bufferSize; i++)
	//{
	//	completeString += String(rawSamples[i]);
	//	completeString += " ";
	//}
	//---------------------------------------------

	// Calculate RMS for each subblock of "uiRmsSamples" samples inside buffer
	for (int rmsStartIdx = 0; rmsStartIdx < realNumberOfSamples; rmsStartIdx += uiRmsSamples)
	{
		// Get rid of analysing 0-valued samples in the end of the buffer
		if (rmsStartIdx + uiRmsSamples > realNumberOfSamples)
			rmsEndIdx = realNumberOfSamples;
		else
			rmsEndIdx = rmsStartIdx + uiRmsSamples;
		
		// RMS calculation (includes rmsStartIdx but excludes rmsEndIdx)
		double rms = calculateRms(rawSamples, rmsStartIdx, rmsEndIdx);

		// Calculate the RMS average to further determine a baseline for the threshold
		if (isCalibrating)
		{
			// Variables to be used as a calibration basis
			calibrationRmsValues.push_back(rms);
			rmsMean += rms;
		}
		else
		{
			// Array of RMS values
			rmsValuesArray.push_back(rms);
			rmsSamplesArray.push_back(rmsEndIdx - rmsStartIdx);
		}
	}

	// Check plugin's current state: calibration or detection
	if (isCalibrating)
	{
		pointsProcessed += realNumberOfSamples;

		// Check if calibration is finished and calculate its statistics for threshold estimation
		if (pointsProcessed >= calibrationPoints)
		{
			finishCalibration();
		}
	}
	else
	{
		detectRipples(rmsValuesArray, rmsSamplesArray);
	}
}

// Calculate the RMS of rInBuffer data from position initIndex (included) to endIndex (not included)
double RippleDetector::calculateRms(const float *rawBufferData, int initIndex, int endIndex)
{
	double sum = 0.0;
	for (int idx = initIndex; idx < endIndex; idx++)
	{
		sum += pow(rawBufferData[idx], 2.0);
		// Write RMS values to a file, if enabled
		if(writeToFile) file << rawBufferData[idx] << "\n";
	}

	return sqrt(sum / (endIndex - initIndex));
}

void RippleDetector::finishCalibration()
{
		printf("Calibration finished!\n");

		// Set flag to false to end the calibration period
		isCalibrating = false;

		// Calculate RMS mean
		rmsMean = rmsMean / (double)calibrationRmsValues.size();

		// Calculate standard deviation
		for (unsigned int idx = 0; idx < calibrationRmsValues.size(); idx++)
		{
			rmsStd += pow(calibrationRmsValues[idx] - rmsMean, 2.0);
		}
		rmsStd = sqrt(rmsStd / ((double)calibrationRmsValues.size() - 1.0));

		threshold = rmsMean + uiSdsThreshold * rmsStd;

		// Print calculated statistics
		printf("RMS Mean: %f\n"
			"RMS Std: %f\n"
			"Threshold amplifier: %f\n"
			"Final RMS Threshold: %f\n",
			rmsMean, rmsStd, uiSdsThreshold, threshold);
	
}

void RippleDetector::detectRipples(std::vector<double>& rmsValuesArr, std::vector<int>& rmsSamplesArr)
{

	// Iterate over RMS blocks inside buffer
    for (unsigned int rmsIdx = 0; rmsIdx < rmsValuesArr.size(); rmsIdx++)
    {
        double rms = rmsValuesArr[rmsIdx];
		int samples = rmsSamplesArr[rmsIdx];
		samplesCount += samples;


		// Detect the first time the RMS is above the amplitude threshold
		if (detectionEnabled && rms > threshold)
		{
			detectionOnProgress = true;
			detectionEnabled = false;
			// Start counting the samples equivalent to the time threshold
			samplesCount = 0;
		}

		// If RMS drops below threshold, reset the algorithm
		if (detectionOnProgress && rms <= threshold) {
			detectionOnProgress = false;
			detectionEnabled = true;
			continue;
		}

		//printf("det %d, detOnProg %d, refrac %d, rms %f, thresh %f, samp %d\n", detectionEnabled, detectionOnProgress, onRefractoryTime, rms, threshold, samplesCount);

		// Send TTL to 0 again when refractory time starts
		if (refractoryTimeStartFlag)
		{
			sendTtlEvent(rmsIdx, 0);
			refractoryTimeStartFlag = false;
		}

		// If RMS detection flag persists for more than "uiTimeThreshold" milliseconds
		// (converted to numSamplesTimeThreshold), send a TTL event and start refractory time
		if (detectionOnProgress && rms > threshold && samplesCount >= numSamplesTimeThreshold)
		{
			printf("Ripple detected!\n");
			sendTtlEvent(rmsIdx, 1);

			// Start refractory time
			onRefractoryTime = true;
			refractoryTimeStartFlag = true;
			refractoryTimeStart = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

			// Reset detection flag and time threshold window
			detectionOnProgress = false;
			samplesCount = 0;
		}

		// Check the refractory time to enable detection again
		timeNow = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());
		if (!refractoryTimeStartFlag && onRefractoryTime && (timeNow.count() - refractoryTimeStart.count() >= uiRefractoryTime))
		{
			onRefractoryTime = false;
			detectionEnabled = true;
			refractoryTimeStart = timeNow;
		}
    }
}

void RippleDetector::sendTtlEvent(int rmsIndex, int val)
{
	// Send event only when the animal is not moving
	if (!isPluginEnabled)
		return;

	// Timestamp for this sample
	uint64 time_stamp = getTimestamp(uiInputChannel) + rmsIndex;

	uint8 ttlData;
	uint8 output_event_channel = uiOutputChannel;
	ttlData = val << uiOutputChannel;
	TTLEventPtr ttl = TTLEvent::createTTLEvent(pTtlEventChannel, time_stamp, &ttlData, sizeof(uint8), output_event_channel);
	addEvent(pTtlEventChannel, ttl, rmsIndex);
}

void RippleDetector::handleEvent(const EventChannel *rInEventInfo, const MidiMessage &rInEvent, int samplePosition)
{
}