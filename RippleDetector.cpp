#include "RippleDetector.h"

#define MINIMUM_RMS_BLOCK_SIZE 1
#define MINIMUM_THRESHOLD_AMP 0
#define MINIMUM_THRESHOLD_TIME 0
#define MINIMUM_REFRACTORY_TIME 0
#define CALIBRATION_BUFFERS_COUNT 1000

RippleDetector::RippleDetector()
    : GenericProcessor("RippleDetector")
{
	//Initiate variables
    calibrationBuffers = CALIBRATION_BUFFERS_COUNT;
    currentBuffer = 0;
    rmsMean = 0.0;
    rmsStandardDeviation = 0.0;
    threshold = 0.0;
    thresholdSds = 1.0f;
	thresholdTime = 5.0f;
	sampleRate = CoreServices::getGlobalSampleRate();

    setProcessorType(PROCESSOR_TYPE_FILTER);
    createEventChannels();

	//These code lines write the input data in a file.
	//It is useful for debugging purposes...
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
	//Update parameters according to user's interface
	const DataChannel *in = getDataChannel(0);

	outputChannel = pRippleDetectorEditor->_pluginUi._outChannel - 1;
	inputChannel = pRippleDetectorEditor->_pluginUi._inChannel - 1;
	thresholdSds = pRippleDetectorEditor->_pluginUi._thresholdSds;
	thresholdTime = pRippleDetectorEditor->_pluginUi._thresholdTime;
	refractoryTime = pRippleDetectorEditor->_pluginUi._refractoryTime;
	rmsBlockSize = pRippleDetectorEditor->_pluginUi._rmsSamples;
	bufferSize = rInBuffer.getNumSamples();
	realNumberOfSamples = GenericProcessor::getNumSamples(inputChannel);

	//printf("bufferSize %d\n", bufferSize);
	//printf("realNumberOfSamples %d\n", realNumberOfSamples);

	//Sometimes it occurs that the number of input samples is 0
	//In this case, end the process
	if (realNumberOfSamples == 0) return;
	
	if (pRippleDetectorEditor->_pluginUi._calibrate == true)
	{
		printf("Recalibrating...\n");
		pRippleDetectorEditor->_pluginUi._calibrate = false;
		isCalibrating = true;
		currentBuffer = 0;

		//Reset calibration RMS array
		calibrationRms.clear();
	}

	//Check parameter's range and adjust if necessary.
	if (refractoryTime < MINIMUM_REFRACTORY_TIME)
		refractoryTime = MINIMUM_REFRACTORY_TIME;

	if (rmsBlockSize < MINIMUM_RMS_BLOCK_SIZE)
		rmsBlockSize = MINIMUM_RMS_BLOCK_SIZE;

	if (rmsBlockSize > realNumberOfSamples) {
		/*pRippleDetectorEditor->_pluginUi._sliderRmsSamples->setValue(realNumberOfSamples);
		pRippleDetectorEditor->_pluginUi._rmsSamples = realNumberOfSamples;*/
		rmsBlockSize = realNumberOfSamples;
	}

	if (thresholdSds < MINIMUM_THRESHOLD_AMP)
		thresholdSds = MINIMUM_THRESHOLD_AMP;

	if (thresholdTime < MINIMUM_THRESHOLD_TIME)
		thresholdTime = MINIMUM_THRESHOLD_TIME;

	//Define threshold and numRmsSamplesThresholdTime
	threshold = rmsMean + thresholdSds * rmsStandardDeviation;
	numRmsSamplesThresholdTime = ceil((sampleRate*thresholdTime / 1000) / rmsBlockSize);

	//Get raw data
	const float *rSamples = rInBuffer.getReadPointer(inputChannel);

	//Guarantee that the RMS buffer will be clean
	localRms.clear();

	//Write input data into a string, so it becomes easier to analyze.
	//For debugging purposes only...
	//String completeString;
	//for (int i = 0; i < bufferSize; i++)
	//{
	//	completeString += String(rSamples[i]);
	//	completeString += " ";
	//}

	//Calculate RMS for each subblock inside buffer
	for (int rms_index = 0; rms_index < realNumberOfSamples; rms_index += rmsBlockSize)
	{
		//Get rid of extra 0-valued samples in the end of the buffer
		if (rms_index + rmsBlockSize > realNumberOfSamples)
		{
			rmsEndIndex = realNumberOfSamples;
		}
		else {
			rmsEndIndex = rms_index + rmsBlockSize;
		}

		//RMS calculation
		double rms = calculateRms(rSamples, rms_index, rmsEndIndex);

		//Calculate average between RMSs to determine baseline threshold
		if (isCalibrating)
		{
			//Add variables to be used as a calibration basis
			calibrationRms.push_back(rms);
			rmsMean += rms;
		}
		else
		{
			//Set buffer value
			localRms.push_back(rms);
		}
	}

	//Check if plugin needs to stop calibration and calculate its statistics for threshold estimation
	if (isCalibrating)
	{
		//Printf which calibration buffer is being used
		//printf("Ripple Detection - calibration buffer processed: %d out of %d\n", currentBuffer, calibrationBuffers);
		calibrate();
	}
	else
	{
		detectRipples(localRms);
	}

	//Count how many buffers have been processed
	currentBuffer++;
}

//Calculate the RMS of rInBuffer data from position initIndex (included) to endIndexOpen (not included)
double RippleDetector::calculateRms(const float *rInBuffer, int initIndex, int endIndexOpen)
{
	double sum = 0.0;
	for (int cnt = initIndex; cnt < endIndexOpen; cnt++)
	{
		sum += pow(rInBuffer[cnt], 2.0);
		if(writeToFile) file << rInBuffer[cnt] << "\n";
	}

	return sqrt(sum / (endIndexOpen - initIndex));
}

void RippleDetector::calibrate()
{
	if (currentBuffer == calibrationBuffers)
	{
		printf("Calibration finished...\n");

		//Set flag to false to end the calibration period
		isCalibrating = false;

		//Calculate statistics
		rmsMean = rmsMean / (double)calibrationRms.size();

		//Calculate standard deviation
		for (unsigned int rms_sample = 0; rms_sample < calibrationRms.size(); rms_sample++)
		{
			rmsStandardDeviation += pow(calibrationRms[rms_sample] - rmsMean, 2.0);
		}
		rmsStandardDeviation = sqrt(rmsStandardDeviation / ((double)calibrationRms.size() - 1.0));

		threshold = rmsMean + thresholdSds * rmsStandardDeviation;

		//Printf calculated statistics
		printf(
			"RMS Mean: %f\n"
			"RMS Deviation: %f\n"
			"Threshold amplifier %f\n"
			"Calculated RMS Threshold: %f\n",
			rmsMean, rmsStandardDeviation, thresholdSds, threshold);
	}
}

void RippleDetector::detectRipples(std::vector<double> &rInRmsBuffer)
{
	//Iterate over RMS sample blocks inside buffer
    for (unsigned int rms_sample = 0; rms_sample < rInRmsBuffer.size(); rms_sample++)
    {
        double sample = rInRmsBuffer[rms_sample];

		//Detect the first time the sample is above the amplitude threshold
		if (detectionEnabled && sample > threshold)
		{
			detected = true;
			detectionEnabled = false;
			//Start the time threshold blocks counting
			rmsSamplesCount = 0;
		}

		//If RMS drops below threshold, reset the algorithm
		if (detected && sample <= threshold) {
			detected = false;
			detectionEnabled = true;
			continue;
		}

		//printf("detected %d, can_detect %d, refractory %d, sample %f, thresh %f\n", detected, detectionEnabled, onRefractoryTime, sample, threshold);

		//Send TTL to 0 again when refractory time starts
		if (refractoryTimeStartFlag)
		{
			sendTtlEvent(rms_sample, 0);
			refractoryTimeStartFlag = false;
		}

		//If RMS detection flag persists for more than "thresholdTime"
		//milliseconds, send a TTL event and start refractory time
		if (detected && sample > threshold && rmsSamplesCount >= numRmsSamplesThresholdTime)
		{
			//printf("Ripple detected!\n");
			sendTtlEvent(rms_sample, 1);

			//Start refractory time
			onRefractoryTime = true;
			refractoryTimeStartFlag = true;
			refractoryTimeStart = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

			//Reset detection flag and time threshold window
			detected = false;
			rmsSamplesCount = 0;
		}

		//Check refractory time to enable detection again
		timeNow = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());
		if (!refractoryTimeStartFlag && onRefractoryTime && (timeNow.count() - refractoryTimeStart.count() >= refractoryTime))
		{
			onRefractoryTime = false;
			detectionEnabled = true;
			refractoryTimeStart = timeNow;
		}

		//Update RMS samples counting
		rmsSamplesCount++;
    }
}

void RippleDetector::sendTtlEvent(int rmsIndex, int val)
{
	//Send event only when the animal is not moving
	if (!isPluginEnabled)
		return;

	//Timestamp for this sample
	uint64 time_stamp = getTimestamp(inputChannel) + rmsIndex;

	uint8 ttlData;
	uint8 output_event_channel = outputChannel;
	ttlData = val << outputChannel;
	TTLEventPtr ttl = TTLEvent::createTTLEvent(pTtlEventChannel, time_stamp, &ttlData, sizeof(uint8), output_event_channel);
	addEvent(pTtlEventChannel, ttl, rmsIndex);
}

void RippleDetector::handleEvent(const EventChannel *rInEventInfo, const MidiMessage &rInEvent, int samplePosition)
{
}
