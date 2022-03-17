#include "RippleDetector.h"
#include "RippleDetectorEditor.h"

#define CALIBRATION_DURATION_SECONDS 20

RippleDetector::RippleDetector()
    : GenericProcessor("Ripple Detector")
{
	// Init variables
    rmsMean = 0.0;
	rmsStd = 0.0;
	movRmsMean = 0.0;
	movRmsStd = 0.0;
    threshold = 0.0;
	movThreshold = 0.0;

    uiRippleSds = 0.0;
	uiTimeThreshold = 0;
	uiRefractoryTime = 0;
	uiRmsSamples = 0;
	uiMovSds = 0.0;
	uiMinTimeWoMov = 0;
	uiMinTimeWMov = 0;

	counterAboveThresh = 0;
	counterMovUpThresh = 0;
	counterMovDownThresh = 0;

	pointsProcessed = 0;

	// Reset calibration arrays
	calibrationRmsValues.clear();
	calibrationMovRmsValues.clear();

    setProcessorType(PROCESSOR_TYPE_FILTER);
    createEventChannels();
}

RippleDetector::~RippleDetector()
{
}

// Create event channels
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

// Update settings
void RippleDetector::updateSettings()
{
    const DataChannel *in = getDataChannel(0);
	sampleRate = (in) ? in->getSampleRate() : CoreServices::getGlobalSampleRate();
	calibrationPoints = sampleRate * CALIBRATION_DURATION_SECONDS;
}

// Create and return editor
AudioProcessorEditor *RippleDetector::createEditor()
{
    editor = new RippleDetectorEditor(this, true);

	RippleDetectorEditor* ed = (RippleDetectorEditor*)getEditor();
	NO_EMG_CHANNEL_ID = ed->_noMovChannelId - 1;	//Get the id for "-" and "ACCEL" options in the EMG/ACCEL combobox
	ACCEL_CHANNEL_ID = ed->_accelChannelId - 1;		//Get the id for "-" and "ACCEL" options in the EMG/ACCEL combobox

    return editor;
}

// Data acquisition and manipulation loop
void RippleDetector::process(AudioSampleBuffer &inputData)
{
	//Update parameters according to UI
	const DataChannel *in = getDataChannel(0);
	RippleDetectorEditor* ed = (RippleDetectorEditor*)getEditor();

	uiInputChannel = ed->_inChannel - 1;
	uiOutputChannel = ed->_outChannel - 1;
	uiMovChannel = ed->_movChannel - 1;
	uiMovOutChannel = ed->_movOutChannel - 1;

	auxChannelIndices = ed->_accChannels;

	uiRippleSds = ed->_rippleSds;
	uiTimeThreshold = ed->_timeThreshold;
	uiRefractoryTime = ed->_refractoryTime;
	uiRmsSamples = ed->_rmsSamples;
	uiMovSds = ed->_movSds;
	uiMinTimeWoMov = ed->_minTimeWoMov;
	uiMinTimeWMov = ed->_minTimeWMov;

	movChannChanged = ed->_movChannChanged;
	calibrate = ed->_calibrate;
	movSwitchEnabled = ed->_movSwitchEnabled;
	accelerometerSelected = ed->_accelerometerSelected;

	bufferSize = inputData.getNumSamples();
	realNumberOfSamples = GenericProcessor::getNumSamples(uiInputChannel);

	// Sometimes it occurs that the number of samples is 0
	// In this case, finishes the process
	if (realNumberOfSamples == 0) return;

	// Enable detection again if the mov. detector channel is "-" or if calibration button was clicked
	if ((!movSwitchEnabled && !pluginEnabled) || (!pluginEnabled && calibrate))
	{
		//printf("Detection enabled.\n");
		pluginEnabled = true;
		sendTtlEvent(0, 0, uiMovOutChannel);
	}
	
	// Calculate the number of samples equivalent to the time threshold
	// and update the detection thresholds (in case the user changes the SDs value)
	numSamplesTimeThreshold = ceil(sampleRate * uiTimeThreshold / 1000);
	minMovSamplesBelowThresh = ceil(sampleRate * uiMinTimeWoMov / 1000);
	minMovSamplesAboveThresh = ceil(sampleRate * uiMinTimeWMov / 1000);
	threshold = rmsMean + uiRippleSds * rmsStd;
	movThreshold = movRmsMean + uiMovSds * movRmsStd;

	// Check if the number of samples to calculate the RMS is not 
	// larger than the total number of samples provided in this cycle and adjust if necessary
	if (uiRmsSamples > realNumberOfSamples)
		uiRmsSamples = realNumberOfSamples;

	// Check if recalibration button was clicked or EMG channel changed to a valid id
	if (calibrate == true || (movChannChanged && uiMovChannel != NO_EMG_CHANNEL_ID))
	{
		printf("Calibrating...\n");
		ed->_calibrate = false;			//Reset the state of this variable in its origin
		ed->_movChannChanged = false;	//Reset the state of this variable in its origin
		isCalibrating = true;
		pointsProcessed = 0;

		// Reset calibration RMS array
		calibrationRmsValues.clear();
		calibrationMovRmsValues.clear();
	}

	// Get pointers to ripple data and to movement detector data (EMG or accelerometer)
	const float *pSwrData = inputData.getReadPointer(uiInputChannel);
	const float* pEmgData{NULL};
	const float* pAccelData[3]{NULL, NULL, NULL};
	std::vector<float> accMagnit; accMagnit.clear();

	if (movSwitchEnabled) {
		if (!accelerometerSelected) {	//EMG selected
			pEmgData = inputData.getReadPointer(uiMovChannel);
		}
		else {	//Accelerometer selected
			for (int idx = 0; idx < auxChannelIndices.size(); idx++) {
				pAccelData[idx] = inputData.getReadPointer(auxChannelIndices[idx]);
			}
			// Convert the 3 axes data to the magnitude of the vector
			accMagnit = calculateAccelMod(pAccelData, realNumberOfSamples);
		}
	}

	// Guarante that the arrays with RMS values will be clean
	rmsValuesArray.clear();
	movRmsValuesArray.clear();
	rmsNumSamplesArray.clear(); 
	movRmsNumSamplesArray.clear();

	// Calculate RMS for each subblock of "uiRmsSamples" samples inside buffer
	for (int rmsStartIdx = 0; rmsStartIdx < realNumberOfSamples; rmsStartIdx += uiRmsSamples)
	{
		// Get rid of analysing 0-valued samples in the end of the buffer
		if (rmsStartIdx + uiRmsSamples > realNumberOfSamples)
			rmsEndIdx = realNumberOfSamples;
		else
			rmsEndIdx = rmsStartIdx + uiRmsSamples;
		// RMS calculation (includes rmsStartIdx but excludes rmsEndIdx)
		double rms = calculateRms(pSwrData, rmsStartIdx, rmsEndIdx);

		double movRms = 0; 
		if (movSwitchEnabled) {
			if(!accelerometerSelected) 
				movRms = calculateRms(pEmgData, rmsStartIdx, rmsEndIdx);
			else {
				movRms = calculateRms(accMagnit, rmsStartIdx, rmsEndIdx);
			}
		}

		// Calculate the RMS average to further determine a baseline for the threshold
		if (isCalibrating)
		{
			// Variables to be used as a calibration basis
			calibrationRmsValues.push_back(rms);
			rmsMean += rms;

			if (movSwitchEnabled) {
				calibrationMovRmsValues.push_back(movRms);
				movRmsMean += movRms;
			}
		}
		else
		{
			// Array of RMS values
			rmsValuesArray.push_back(rms);
			rmsNumSamplesArray.push_back(rmsEndIdx - rmsStartIdx);

			if (movSwitchEnabled) {
				movRmsValuesArray.push_back(movRms);
				movRmsNumSamplesArray.push_back(rmsEndIdx - rmsStartIdx);
			}
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
		detectRipples(rmsValuesArray, rmsNumSamplesArray);
		if (movSwitchEnabled) evalMovement(movRmsValuesArray, movRmsNumSamplesArray);
	}
}

// Calculate the RMS of inputData data from position initIndex (included) to endIndex (not included)
double RippleDetector::calculateRms(const float* data, int initIndex, int endIndex)
{
	double sum = 0.0;
	for (int idx = initIndex; idx < endIndex; idx++)
	{
		sum += pow(data[idx], 2.0);
	}

	return sqrt(sum / (endIndex - initIndex));
}
double RippleDetector::calculateRms(std::vector<float> data, int initIndex, int endIndex)
{
	double sum = 0.0;
	for (int idx = initIndex; idx < endIndex; idx++)
	{
		sum += pow(data[idx], 2.0);
	}

	return sqrt(sum / (endIndex - initIndex));
}

// Calculate the modulus of the accelerometer vector
std::vector<float> RippleDetector::calculateAccelMod(const float* axis[3], int numberOfSamples)
{
	std::vector<float> modArr;
	for (int p = 0; p < numberOfSamples; p++)
	{
		modArr.push_back(sqrt(pow(axis[0][p], 2.0) + pow(axis[1][p], 2.0) + pow(axis[2][p], 2.0)));
	}
	return modArr;
}

// Called when calibration step is over
void RippleDetector::finishCalibration()
{
		printf("Calibration finished!\n");

		// Set flag to false to end the calibration period
		isCalibrating = false;

		// Calculate RMS mean and standard deviation and the final amplitude threshold
		rmsMean = rmsMean / (double)calibrationRmsValues.size();
		for (unsigned int idx = 0; idx < calibrationRmsValues.size(); idx++)
		{
			rmsStd += pow(calibrationRmsValues[idx] - rmsMean, 2.0);
		}
		rmsStd = sqrt(rmsStd / ((double)calibrationRmsValues.size() - 1.0));
		threshold = rmsMean + uiRippleSds * rmsStd;

		// Calculate EMR/ACC RMS mean and standard deviation
		// if the switching mechanism is enabled
		if (movSwitchEnabled) {
			movRmsMean = movRmsMean / (double)calibrationMovRmsValues.size();
			for (unsigned int idx = 0; idx < calibrationMovRmsValues.size(); idx++)
			{
				movRmsStd += pow(calibrationMovRmsValues[idx] - movRmsMean, 2.0);
			}
			movRmsStd = sqrt(movRmsStd / ((double)calibrationMovRmsValues.size() - 1.0));
			movThreshold = movRmsMean + uiMovSds * movRmsStd;
		}

		// Print calculated statistics
		if (movSwitchEnabled) {
			if (!accelerometerSelected)
			{
				printf("Ripple channel -> RMS mean: %f\n"
					"Ripple channel -> RMS std: %f\n"
					"Ripple channel -> threshold amplifier: %f\n"
					"Ripple channel -> final RMS threshold: %f\n"
					"EMG RMS mean: %f\n"
					"EMG RMS std: %f\n"
					"EMG threshold amplifier: %f\n"
					"EMG final RMS threshold: %f\n",
					rmsMean, rmsStd, uiRippleSds, threshold,
					movRmsMean, movRmsStd, uiMovSds, movThreshold);
			}
			else {
				printf("Ripple channel -> RMS mean: %f\n"
					"Ripple channel -> RMS std: %f\n"
					"Ripple channel -> threshold amplifier: %f\n"
					"Ripple channel -> final RMS threshold: %f\n"
					"Accel. magnit. RMS mean: %f\n"
					"Accel. magnit. RMS std: %f\n"
					"Accel. magnit. threshold amplifier: %f\n"
					"Accel. magnit. final RMS threshold: %f\n",
					rmsMean, rmsStd, uiRippleSds, threshold,
					movRmsMean, movRmsStd, uiMovSds, movThreshold);
			}
		}
		else {
			printf("Ripple channel -> RMS mean: %f\n"
				   "Ripple channel -> RMS std: %f\n"
				   "Ripple channel -> threshold amplifier: %f\n"
				   "Ripple channel -> final RMS threshold: %f\n",
				rmsMean, rmsStd, uiRippleSds, threshold);
		}
}

// Evaluate RMS values in the detection algorithm
void RippleDetector::detectRipples(std::vector<double>& rmsValuesArr, std::vector<int>& rmsNumSamplesArray)
{
	// Iterate over RMS blocks inside buffer
    for (unsigned int rmsIdx = 0; rmsIdx < rmsValuesArr.size(); rmsIdx++)
    {
        double rms = rmsValuesArr[rmsIdx];
		int samples = rmsNumSamplesArray[rmsIdx];

		// Reset TTL if ripple was detected during the last iteration
		if (rippleDetected) {
			if (pluginEnabled) { sendTtlEvent(rmsIdx, 0, uiOutputChannel); }
			rippleDetected = false;
		}

		// Counter: acumulate time above threshold
		if (rms > threshold) {
			counterAboveThresh += samples;
		}
		else {
			counterAboveThresh = 0;
			flagTimeThreshold = false;
		}

		// Set flag to indicate that time threshold was achieved
		if (counterAboveThresh > numSamplesTimeThreshold) { flagTimeThreshold = true; }
		
		// Send TTL if ripple is detected and it is not on refractory period
		if (flagTimeThreshold && !onRefractoryTime)
		{
			if (pluginEnabled) {
				sendTtlEvent(rmsIdx, 1, uiOutputChannel);
				printf("Ripple detected!\n");
			}
			else printf("Ripple detected, but TTL event was blocked by movement detection.\n");

			rippleDetected = true;

			// Start refractory period
			onRefractoryTime = true;
			refractoryTimeStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		}

		//printf("en %d, refrac %d, rms %f, thresh %f, counterAboveThresh %d\n", pluginEnabled, onRefractoryTime, rms, threshold, counterAboveThresh);

		// Check and reset refractory time
		if (onRefractoryTime)
		{
			timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
			if (timeNow.count() - refractoryTimeStart.count() >= uiRefractoryTime) { onRefractoryTime = false; }
		}
    }
}

// Evaluate EMG/ACC signal to enable or disable ripple detection
void RippleDetector::evalMovement(std::vector<double>& movRmsValuesArr, std::vector<int>& movRmsNumSamplesArray)
{
	// Iterate over RMS blocks inside buffer
	for (unsigned int rmsIdx = 0; rmsIdx < movRmsValuesArr.size(); rmsIdx++)
	{
		double rms = movRmsValuesArr[rmsIdx];
		int samples = movRmsNumSamplesArray[rmsIdx];

		// Counter: acumulate time above or below threshold
		if (rms > movThreshold) {
			counterMovUpThresh += samples;
			flagMovMinTimeDown = false;
		}
		else {
			counterMovDownThresh += samples;
			flagMovMinTimeUp = false;
			counterMovUpThresh = 0;
		}

		// Set flags when minimum time above or below threshold is achieved
		if (counterMovUpThresh > minMovSamplesAboveThresh) {
			flagMovMinTimeUp = true;
			counterMovDownThresh = 0;	//Reset counterMovDownThresh only when there is movement for enough time
		}
		if (counterMovDownThresh > minMovSamplesBelowThresh) {
			flagMovMinTimeDown = true;
		}

		// Disable plugin...
		if (pluginEnabled && flagMovMinTimeUp) {
			pluginEnabled = false;
			sendTtlEvent(rmsIdx, 1, uiMovOutChannel);
		}
		// ... or enable plugin
		if (!pluginEnabled && flagMovMinTimeDown) {
			pluginEnabled = true;
			sendTtlEvent(rmsIdx, 0, uiMovOutChannel);
		}

		/*printf("plugin %d, flagUp %d, flagDown %d, Up %d, Down %d, rms %f, thresh %f\n", 
			pluginEnabled, flagMovMinTimeUp, flagMovMinTimeDown, counterMovUpThresh, counterMovDownThresh, rms, movThreshold);*/
	}
}

// Send TTL output signal
void RippleDetector::sendTtlEvent(int rmsIndex, int val, int outputChannel)
{
	// Timestamp for this sample
	uint64 time_stamp = getTimestamp(uiInputChannel) + rmsIndex;

	uint8 ttlData;
	uint8 output_event_channel = outputChannel;
	ttlData = val << outputChannel;
	TTLEventPtr ttl = TTLEvent::createTTLEvent(pTtlEventChannel, time_stamp, &ttlData, sizeof(uint8), output_event_channel);
	addEvent(pTtlEventChannel, ttl, rmsIndex);
}

// Handle events
void RippleDetector::handleEvent(const EventChannel *rInEventInfo, const MidiMessage &rInEvent, int samplePosition)
{
}

// Save last parameters
void RippleDetector::saveCustomParametersToXml(XmlElement* parentElement)
{
	//XmlElement* mainNode = parentElement->createNewChildElement("RippleDetector");
	//RippleDetectorEditor* ed = (RippleDetectorEditor*)getEditor();

	//mainNode->setAttribute("rippleInputCh", ed->_inChannel);
	///*mainNode->setAttribute("input2", m_input2);
	//mainNode->setAttribute("input1gate", m_input1gate);
	//mainNode->setAttribute("input2gate", m_input2gate);
	//mainNode->setAttribute("logicOp", m_logicOp);
	//mainNode->setAttribute("outputChan", m_outputChan);
	//mainNode->setAttribute("window", m_window);
	//mainNode->setAttribute("duration", m_pulseDuration);*/

}

// Load last parameters
void RippleDetector::loadCustomParametersFromXml()
{
	//if (parametersAsXml != nullptr)
	//{
	//	forEachXmlChildElement(*parametersAsXml, mainNode)
	//	{
	//		if (mainNode->hasTagName("RippleDetector"))
	//		{
	//			RippleDetectorEditor* ed = (RippleDetectorEditor*)getEditor();

	//			ed->_inChannel = mainNode->getIntAttribute("rippleInputCh");
	//			/*m_input2 = mainNode->getIntAttribute("input2");
	//			m_input1gate = mainNode->getBoolAttribute("input1gate");
	//			m_input2gate = mainNode->getBoolAttribute("input2gate");
	//			m_logicOp = mainNode->getIntAttribute("logicOp");
	//			m_outputChan = mainNode->getIntAttribute("outputChan");
	//			m_window = mainNode->getIntAttribute("window");
	//			m_pulseDuration = mainNode->getIntAttribute("duration");*/

	//			//editor->updateSettings();
	//		}
	//	}
	//}
}