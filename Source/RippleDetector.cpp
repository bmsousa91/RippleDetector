// v060422
#include "RippleDetector.h"
#include "RippleDetectorEditor.h"

#define CALIBRATION_DURATION_SECONDS 20

RippleDetectorSettings::RippleDetectorSettings()
{}

TTLEventPtr RippleDetectorSettings::createEvent(int64 outputLine, int64 sample_number, bool state)
{

	return TTLEvent::createTTLEvent(
		eventChannel,
		sample_number,
		outputLine,
		state
	);			

}

RippleDetector::RippleDetector() : GenericProcessor("Ripple Detector")
{

	/* Ripple Detection Settings */
	addSelectedChannelsParameter(
		Parameter::STREAM_SCOPE, 
		"Ripple_Input", 
		"The continuous channel to analyze", 
		1
	);

	addIntParameter(
		Parameter::STREAM_SCOPE,
		"Ripple_Out",
		"The output TTL line",
		1, 		//deafult
		1, 		//min
		16		//max
	);

	addFloatParameter(
		Parameter::STREAM_SCOPE,
		"ripple_std",
		"Number of standard deviations above the average to be the amplitude threshold",
		 5, 	//default 
		 0, 	//min
		 9999,  //max
		 1  	//step
	);

    addFloatParameter(
		Parameter::STREAM_SCOPE,
		"time_thresh",
		"time threshold value",
		10,
		0,
		9999,
		1		
	);

	addFloatParameter(
		Parameter::STREAM_SCOPE, 
		"refr_time", 
		"refractory value", 
		140, 
		0, 
		999999,
		1
	);

    addFloatParameter(
		Parameter::STREAM_SCOPE,
		"rms_samples",
		"rms samples value",
		128,
		1,
		2048,
		1
	);

	/* EMG / ACC Movement Detection Settings */
	addCategoricalParameter(
		Parameter::STREAM_SCOPE,
		"mov_detect",
		"Use movement to supress ripple detection",
		{ "OFF", "ACC", "EMG" },
		0
	);

	addSelectedChannelsParameter(
		Parameter::STREAM_SCOPE,
		"mov_input",
		"The continuous channel to analyze",
		1
	);

	addIntParameter(
		Parameter::STREAM_SCOPE,
		"mov_out",
		"EMG/ACC output TTL channel: raise event when movement is detected and ripple detection is disabled",
		1,
		1,
		16
	);

	addFloatParameter(
		Parameter::STREAM_SCOPE,
		"mov_std", 
		"Number of standard deviations above the average to be the amplitude threshold for the EMG/ACC",
		5, 
		0,
		9999,
		1
	);

    addFloatParameter(
		Parameter::STREAM_SCOPE,
		"min_steady_time",
		"Minimum time steady (in milliseconds). The minimum time below the EMG/ACC threshold to enable detection",
		5000,
		0,
		999999,
		1
	);

	addFloatParameter(
		Parameter::STREAM_SCOPE,
		"min_mov_time",
		"Minimum time with movement (in milliseconds). The minimum time above the EMG/ACC threshold to disable detection",
		10,
		0,
		999999,
		1
	);

	ed = (RippleDetectorEditor*)getEditor();
}

// Update settings
void RippleDetector::updateSettings()
{

	settings.update(getDataStreams());

	for (auto stream : getDataStreams())
	{

		settings[stream->getStreamId()]->rmsEndIdx = 0;
		settings[stream->getStreamId()]->rmsMean = 0;
		settings[stream->getStreamId()]->rmsStdDev = 0;
		settings[stream->getStreamId()]->movRmsMean = 0;
		settings[stream->getStreamId()]->movRmsStdDev = 0;
		settings[stream->getStreamId()]->threshold = 0;
		settings[stream->getStreamId()]->movThreshold = 0;

		settings[stream->getStreamId()]->rippleSds = 0.0;
		settings[stream->getStreamId()]->timeThreshold = 0;
		settings[stream->getStreamId()]->refractoryTime = 0;
		settings[stream->getStreamId()]->rmsSamples = 0;
		settings[stream->getStreamId()]->movSds = 0.0;
		settings[stream->getStreamId()]->minTimeWoMov = 0;
		settings[stream->getStreamId()]->minTimeWMov = 0;

		settings[stream->getStreamId()]->counterAboveThresh = 0;
		settings[stream->getStreamId()]->counterMovUpThresh = 0;
		settings[stream->getStreamId()]->counterMovDownThresh = 0;

		settings[stream->getStreamId()]->pointsProcessed = 0;

		calibrationRmsValues[stream->getStreamId()].clear();
		calibrationMovRmsValues[stream->getStreamId()].clear();

		settings[stream->getStreamId()]->calibrationPoints = stream->getSampleRate() * CALIBRATION_DURATION_SECONDS;

		parameterValueChanged(stream->getParameter("Ripple_Input"));
		parameterValueChanged(stream->getParameter("Ripple_Out"));
		parameterValueChanged(stream->getParameter("ripple_std"));
		parameterValueChanged(stream->getParameter("time_thresh"));
		parameterValueChanged(stream->getParameter("refr_time"));
		parameterValueChanged(stream->getParameter("rms_samples"));
		parameterValueChanged(stream->getParameter("mov_detect"));
		parameterValueChanged(stream->getParameter("mov_input"));
		parameterValueChanged(stream->getParameter("mov_out"));
		parameterValueChanged(stream->getParameter("mov_std"));
		parameterValueChanged(stream->getParameter("min_steady_time"));
		parameterValueChanged(stream->getParameter("min_mov_time"));

		//Add AUX channels to use for accelerometer data
		settings[stream->getStreamId()]->auxChannelIndices.clear();
		for (auto& channel : stream->getContinuousChannels())
		{
			if (channel->getChannelType() == ContinuousChannel::Type::AUX)
				settings[stream->getStreamId()]->auxChannelIndices.push_back(channel->getLocalIndex());
		}

		//Add event channels to use for detection data
		EventChannel::Settings s {
			EventChannel::Type::TTL,
			"Ripple detector output",
			"Triggers when a ripple or movement is detected on the input channel",
			"dataderived.ripple",
			getDataStream(stream->getStreamId())
		};
		eventChannels.add(new EventChannel(s));
		eventChannels.getLast()->addProcessor(processorInfo.get());
		settings[stream->getStreamId()]->eventChannel = eventChannels.getLast();

	}
}

// Create and return editor
AudioProcessorEditor *RippleDetector::createEditor()
{
    editor = std::make_unique<RippleDetectorEditor>(this);

    /*
	RippleDetectorEditor* ed = (RippleDetectorEditor*)getEditor();
	NO_EMG_CHANNEL_ID = ed->_noMovChannelId - 1;	//Get the id for "-" and "ACCEL" options in the EMG/ACCEL combobox
	ACCEL_CHANNEL_ID = ed->_accelChannelId - 1;		//Get the id for "-" and "ACCEL" options in the EMG/ACCEL combobox
    */

    return editor.get();
}

void RippleDetector::parameterValueChanged(Parameter* param)
{

	String paramName = param->getName();
	int streamId = param->getStreamId();

	if (paramName.equalsIgnoreCase("Ripple_Input"))
	{
		int localIndex = (int)param->getValue();
		int globalIndex = getDataStream(param->getStreamId())->getContinuousChannels()[localIndex]->getGlobalIndex();
		settings[streamId]->rippleInputChannel = globalIndex;
	}
	else if (paramName.equalsIgnoreCase("Ripple_Out"))
	{
        settings[streamId]->rippleOutputChannel = (int)param->getValue() - 1;
	}
	else if (paramName.equalsIgnoreCase("ripple_std"))
	{
		settings[streamId]->rippleSds = (float)param->getValue();
	}
	else if (paramName.equalsIgnoreCase("Time_Thresh"))
	{
		settings[streamId]->timeThreshold = (int)param->getValue();
		settings[streamId]->numSamplesTimeThreshold = 
			ceil(getDataStream(streamId)->getSampleRate() * settings[streamId]->timeThreshold / 1000);
	}
	else if (paramName.equalsIgnoreCase("Refr_Time"))
	{
		settings[streamId]->refractoryTime = (int)param->getValue();
	}
	else if (paramName.equalsIgnoreCase("RMS_Samples"))
	{
		settings[streamId]->rmsSamples = (int)param->getValue();
	}
	else if (paramName.equalsIgnoreCase("mov_detect"))
	{
		settings[streamId]->movSwitch = ((CategoricalParameter*)param)->getValueAsString();

		//Check if ACC was chosen and how many AUX channels are available
		if (settings[streamId]->movSwitch.equalsIgnoreCase("ACC"))
		{
			int auxChannelCount = settings[streamId]->auxChannelIndices.size();
			if (!auxChannelCount)
			{
				settings[streamId]->movSwitch = "OFF";
				((CategoricalParameter*)param)->setNextValue("OFF");
				AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                "WARNING", "No AUX channels were detected in this stream to compute acceleration. Switching to OFF.");
			}
			else
			{
				String msg = String(auxChannelCount) + " aux channels were detected in this stream. ";
				msg += "All available channels will be used to compute the acceleration magnitude.";
				AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "INFO", msg);
			}
		}
		settings[streamId]->movSwitchEnabled = !(settings[streamId]->movSwitch).equalsIgnoreCase("OFF");
		settings[streamId]->movChannChanged = true;
	}
	else if (paramName.equalsIgnoreCase("mov_input"))
	{
		int localIndex = (int)param->getValue();
		int globalIndex = getDataStream(param->getStreamId())->getContinuousChannels()[localIndex]->getGlobalIndex();
		settings[streamId]->movementInputChannel = globalIndex;
		settings[streamId]->movChannChanged = true;
	}
	else if (paramName.equalsIgnoreCase("mov_out"))
	{
		settings[streamId]->movementOutputChannel = (int)param->getValue() - 1;
	}
	else if (paramName.equalsIgnoreCase("mov_std"))
	{
		settings[streamId]->movSds = (float)param->getValue();
	}
	else if (paramName.equalsIgnoreCase("Min_Steady_Time"))
	{
		settings[streamId]->minTimeWoMov = (int)param->getValue();
		settings[streamId]->minMovSamplesBelowThresh = 
			ceil(getDataStream(streamId)->getSampleRate() * settings[streamId]->minTimeWoMov / 1000);
	}
	else if (paramName.equalsIgnoreCase("Min_Mov_Time"))
	{
		settings[streamId]->minTimeWMov = (int)param->getValue();
		settings[streamId]->minMovSamplesAboveThresh = 
			ceil(getDataStream(streamId)->getSampleRate() * settings[streamId]->minTimeWMov / 1000);
	}


}

// Data acquisition and manipulation loop
void RippleDetector::process(AudioBuffer<float>& buffer)
{

	for (auto stream : getDataStreams())
	{
		if ((*stream)["enable_stream"])
		{

			const uint16 streamId = stream->getStreamId();
			const int64 firstSampleInBlock = getFirstSampleNumberForBlock(streamId);
			const uint32 numSamplesInBlock = getNumSamplesInBlock(streamId);

			float sampleRate = stream->getSampleRate();

			if (!numSamplesInBlock) return;

			// Enable detection again if the mov. detector channel is "-" or if calibration button was clicked
			if (!settings[streamId]->pluginEnabled && (settings[streamId]->movSwitchEnabled || shouldCalibrate))
			{
				settings[streamId]->pluginEnabled = true;
				TTLEventPtr event = settings[streamId]->createEvent(
					settings[streamId]->movementOutputChannel,firstSampleInBlock,false);
				addEvent(event, 0);
			}

			settings[streamId]->numSamplesTimeThreshold = ceil(sampleRate * settings[streamId]->timeThreshold / 1000);
			settings[streamId]->minMovSamplesBelowThresh = ceil(sampleRate * settings[streamId]->minTimeWoMov / 1000);
			settings[streamId]->minMovSamplesAboveThresh = ceil(sampleRate * settings[streamId]->minTimeWMov / 1000);

			settings[streamId]->threshold = settings[streamId]->rmsMean + settings[streamId]->rippleSds * settings[streamId]->rmsStdDev;
			settings[streamId]->movThreshold = settings[streamId]->movRmsMean + settings[streamId]->movSds * settings[streamId]->movRmsStdDev;

			// Check if the number of samples to calculate the RMS is not 
			// larger than the total number of samples provided in this cycle and adjust if necessary
			if (settings[streamId]->rmsSamples > numSamplesInBlock)
				settings[streamId]->rmsSamples = numSamplesInBlock;

			// Check if need to calibrate
			if (shouldCalibrate || (settings[streamId]->movChannChanged && settings[streamId]->movementInputChannel > 0))
			{
				LOGC("Calibrating...");
				settings[streamId]->isCalibrating = true;
				settings[streamId]->movChannChanged = false;

				settings[streamId]->pointsProcessed = 0;

				calibrationRmsValues[streamId].clear();
				calibrationMovRmsValues[streamId].clear();

				shouldCalibrate = false;
			}

			const float *rippleData = buffer.getReadPointer(settings[streamId]->rippleInputChannel, 0);
			const float *emgData { NULL };
			const float *accelData[3] { NULL, NULL, NULL };
			std::vector<float> accMagnit;
			accMagnit.clear();

			if (settings[streamId]->movSwitchEnabled)
			{
				if (settings[streamId]->movSwitch.equalsIgnoreCase("ACC"))
				{
					for (int i = 0; i < settings[streamId]->auxChannelIndices.size(); i++)
					{
						accelData[i] = buffer.getReadPointer(settings[streamId]->auxChannelIndices[i], 0);
					}
					accMagnit = calculateAccelMod(accelData, numSamplesInBlock);
				}
				else //EMG
				{
					emgData = buffer.getReadPointer(settings[streamId]->movementInputChannel, 0);
				}
			}

			rmsValuesArray[streamId].clear();
			movRmsValuesArray[streamId].clear();
			rmsNumSamplesArray[streamId].clear();
			movRmsNumSamplesArray[streamId].clear();

			for (int rmsStartIdx = 0; rmsStartIdx < numSamplesInBlock; rmsStartIdx += settings[streamId]->rmsSamples)
			{
				if (rmsStartIdx + settings[streamId]->rmsSamples > numSamplesInBlock)
					settings[streamId]->rmsEndIdx = numSamplesInBlock;
				else
					settings[streamId]->rmsEndIdx = rmsStartIdx + settings[streamId]->rmsSamples;

				double rms = calculateRms(rippleData, rmsStartIdx, settings[streamId]->rmsEndIdx);

				double movRms = 0;
				if (settings[streamId]->movSwitchEnabled)
				{
					if (settings[streamId]->movSwitch.equalsIgnoreCase("ACC"))
						movRms = calculateRms(accMagnit, rmsStartIdx, settings[streamId]->rmsEndIdx);
					else //EMG
						movRms = calculateRms(emgData, rmsStartIdx, settings[streamId]->rmsEndIdx);
				}

				if (settings[streamId]->isCalibrating)
				{
					calibrationRmsValues[streamId].push_back(rms);
					settings[streamId]->rmsMean += rms;

					if (settings[streamId]->movSwitchEnabled)
					{
						calibrationMovRmsValues[streamId].push_back(movRms);
						settings[streamId]->movRmsMean += movRms;
					}
				}
				else
				{
					rmsValuesArray[streamId].push_back(rms);
					rmsNumSamplesArray[streamId].push_back(settings[streamId]->rmsEndIdx - rmsStartIdx);
					
					if (settings[streamId]->movSwitchEnabled)
					{
						movRmsValuesArray[streamId].push_back(movRms);
						movRmsNumSamplesArray[streamId].push_back(settings[streamId]->rmsEndIdx - rmsStartIdx);
					}
				}

			}

			if (settings[streamId]->isCalibrating)
			{
				settings[streamId]->pointsProcessed += numSamplesInBlock;
				if (settings[streamId]->pointsProcessed >= settings[streamId]->calibrationPoints)
					finishCalibration(streamId);
			}
			else
			{
				detectRipples(streamId);
				if (settings[streamId]->movSwitchEnabled)
					evalMovement(streamId);
			}

		}
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
void RippleDetector::finishCalibration(uint64 streamId)
{
		LOGD("Calibration finished!");

		// Set flag to false to end the calibration period
		settings[streamId]->isCalibrating = false;

		// Calculate RMS mean and standard deviation and the final amplitude threshold
		int numCalibrationPoints = calibrationRmsValues[streamId].size();
		printf("Got	%d calibration points\n", numCalibrationPoints);
		printf("RMS mean before: %f\n", settings[streamId]->rmsMean); 
		settings[streamId]->rmsMean = settings[streamId]->rmsMean / (double)numCalibrationPoints;
		printf("RMS mean after: %f\n", settings[streamId]->rmsMean); 
		for (unsigned int idx = 0; idx < numCalibrationPoints; idx++)
		{
			settings[streamId]->rmsStdDev += pow(calibrationRmsValues[streamId][idx] - settings[streamId]->rmsMean, 2.0);
		}
		settings[streamId]->rmsStdDev = sqrt(settings[streamId]->rmsStdDev / ((double)numCalibrationPoints - 1.0));
		settings[streamId]->threshold = settings[streamId]->rmsMean + settings[streamId]->rippleSds * settings[streamId]->rmsStdDev;

		// Calculate EMR/ACC RMS mean and standard deviation
		// if the switching mechanism is enabled
		//TODO: Finish this
		if (settings[streamId]->movSwitchEnabled) 
		{
			int numMovCalibrationPoints = calibrationMovRmsValues[streamId].size();
			settings[streamId]->movRmsMean = settings[streamId]->movRmsMean / (double)numMovCalibrationPoints;
			for (unsigned int idx = 0; idx < numMovCalibrationPoints; idx++)
			{
				settings[streamId]->movRmsStdDev += pow(calibrationMovRmsValues[streamId][idx] - settings[streamId]->movRmsMean, 2.0);
			}
			settings[streamId]->movRmsStdDev = sqrt(settings[streamId]->movRmsStdDev / ((double)numMovCalibrationPoints - 1.0));
			settings[streamId]->movThreshold = settings[streamId]->movRmsMean + settings[streamId]->movSds * settings[streamId]->movRmsStdDev;
		}

		// Print calculated statistics
		if (settings[streamId]->movSwitchEnabled) 
		{
			if (settings[streamId]->movSwitch.equalsIgnoreCase("EMG"))
			{
				printf("Ripple channel -> RMS mean: %f\n"
					"Ripple channel -> RMS std: %f\n"
					"Ripple channel -> threshold amplifier: %f\n"
					"Ripple channel -> final RMS threshold: %f\n"
					"EMG RMS mean: %f\n"
					"EMG RMS std: %f\n"
					"EMG threshold amplifier: %f\n"
					"EMG final RMS threshold: %f\n",
					settings[streamId]->rmsMean, 
					settings[streamId]->rmsStdDev,
					settings[streamId]->rippleSds,
					settings[streamId]->threshold,
					settings[streamId]->movRmsMean, 
					settings[streamId]->movRmsStdDev, 
					settings[streamId]->movSds, 
					settings[streamId]->movThreshold);
			}
			else 
			{
				printf("Ripple channel -> RMS mean: %f\n"
					"Ripple channel -> RMS std: %f\n"
					"Ripple channel -> threshold amplifier: %f\n"
					"Ripple channel -> final RMS threshold: %f\n"
					"Accel. magnit. RMS mean: %f\n"
					"Accel. magnit. RMS std: %f\n"
					"Accel. magnit. threshold amplifier: %f\n"
					"Accel. magnit. final RMS threshold: %f\n",
					settings[streamId]->rmsMean, 
					settings[streamId]->rmsStdDev, 
					settings[streamId]->rippleSds,
					settings[streamId]->threshold,
					settings[streamId]->movRmsMean,
					settings[streamId]->movRmsStdDev,
					settings[streamId]->movSds,
					settings[streamId]->movThreshold);
			}
		}
		else 
		{
			printf("Ripple channel -> RMS mean: %f\n"
				   "Ripple channel -> RMS std: %f\n"
				   "Ripple channel -> threshold amplifier: %f\n"
				   "Ripple channel -> final RMS threshold: %f\n",
				settings[streamId]->rmsMean,
				settings[streamId]->rmsStdDev,
				settings[streamId]->rippleSds,
				settings[streamId]->threshold);
		}
    
}

// Evaluate RMS values in the detection algorithm
void RippleDetector::detectRipples(uint64 streamId)
{

	std::vector<double>& rmsValues = rmsValuesArray[streamId];
	std::vector<int>& rmsNumSamples = rmsNumSamplesArray[streamId];

	// Iterate over RMS blocks inside buffer
    for (unsigned int rmsIdx = 0; rmsIdx < rmsValues.size(); rmsIdx++)
    {
        double rms = rmsValues[rmsIdx];
		int samples = rmsNumSamples[rmsIdx];

		// Reset TTL if ripple was detected during the last iteration
		if (settings[streamId]->rippleDetected) {

			if (settings[streamId]->pluginEnabled) { 
				TTLEventPtr event = settings[streamId]->createEvent(
					settings[streamId]->rippleOutputChannel,
					getFirstSampleNumberForBlock(streamId) + rmsIdx,
					0
				);
				addEvent(event, rmsIdx);
			}
			settings[streamId]->rippleDetected = false;

		}

		// Counter: acumulate time above threshold
		if (rms > settings[streamId]->threshold) {
			settings[streamId]->counterAboveThresh += samples;
		}
		else {
			settings[streamId]->counterAboveThresh = 0;
			settings[streamId]->flagTimeThreshold = false;
		}

		// Set flag to indicate that time threshold was achieved
		if (settings[streamId]->counterAboveThresh > settings[streamId]->numSamplesTimeThreshold) { 
			settings[streamId]->flagTimeThreshold = true; 
		}
		
		// Send TTL if ripple is detected and it is not on refractory period
		if (settings[streamId]->flagTimeThreshold && !settings[streamId]->onRefractoryTime)
		{

			if (settings[streamId]->pluginEnabled) {
				TTLEventPtr event = settings[streamId]->createEvent(
					settings[streamId]->rippleOutputChannel,
					getFirstSampleNumberForBlock(streamId) + rmsIdx,
					1
				);
				addEvent(event, rmsIdx);
				LOGC("Ripple detected on stream: ", streamId);
			}
			else 
			{
				LOGC("Ripple detected on stream", streamId, "but TTL event was blocked by movement detection.\n");
			}

			settings[streamId]->rippleDetected = true;

			// Start refractory period
			settings[streamId]->onRefractoryTime = true;
			settings[streamId]->refractoryTimeStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		}

		//printf("en %d, refrac %d, rms %f, thresh %f, counterAboveThresh %d\n", pluginEnabled, onRefractoryTime, rms, threshold, counterAboveThresh);

		// Check and reset refractory time
		if (settings[streamId]->onRefractoryTime)
		{
			settings[streamId]->timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
			if (settings[streamId]->timeNow.count() - settings[streamId]->refractoryTimeStart.count() >= settings[streamId]->refractoryTime) { 
				settings[streamId]->onRefractoryTime = false;
			}
		}
    }

}

// Evaluate EMG/ACC signal to enable or disable ripple detection
void RippleDetector::evalMovement(uint64 streamId)
{

	// Iterate over RMS blocks inside buffer
	for (unsigned int rmsIdx = 0; rmsIdx < movRmsValuesArray[streamId].size(); rmsIdx++)
	{
		double rms = movRmsValuesArray[streamId][rmsIdx];
		int samples = movRmsNumSamplesArray[streamId][rmsIdx];

		// Counter: acumulate time above or below threshold
		if (rms > settings[streamId]->movThreshold) {
			settings[streamId]->counterMovUpThresh += samples;
			settings[streamId]->flagMovMinTimeDown = false;
		}
		else 
		{
			settings[streamId]->counterMovDownThresh += samples;
			settings[streamId]->flagMovMinTimeUp = false;
			settings[streamId]->counterMovUpThresh = 0;
		}

		// Set flags when minimum time above or below threshold is achieved
		if (settings[streamId]->counterMovUpThresh > settings[streamId]->minMovSamplesAboveThresh) {
			settings[streamId]->flagMovMinTimeUp = true;
			settings[streamId]->counterMovDownThresh = 0;	//Reset counterMovDownThresh only when there is movement for enough time
		}
		if (settings[streamId]->counterMovDownThresh > settings[streamId]->minMovSamplesBelowThresh) {
			settings[streamId]->flagMovMinTimeDown = true;
		}

		// Disable plugin...
		if (settings[streamId]->pluginEnabled && settings[streamId]->flagMovMinTimeUp) {
			settings[streamId]->pluginEnabled = false;
			TTLEventPtr event = settings[streamId]->createEvent(
				settings[streamId]->movementOutputChannel,
				getFirstSampleNumberForBlock(streamId) + rmsIdx,
				1
			);
			addEvent(event, rmsIdx);
		}
		// ... or enable plugin
		if (!settings[streamId]->pluginEnabled && settings[streamId]->flagMovMinTimeDown) {
			settings[streamId]->pluginEnabled = true;
			TTLEventPtr event = settings[streamId]->createEvent(
				settings[streamId]->movementOutputChannel,
				getFirstSampleNumberForBlock(streamId) + rmsIdx,
				0
			);
			addEvent(event, rmsIdx);
		}

		//printf("plugin %d, flagUp %d, flagDown %d, Up %d, Down %d, rms %f, thresh %f\n", pluginEnabled, flagMovMinTimeUp, flagMovMinTimeDown, counterMovUpThresh, counterMovDownThresh, rms, movThreshold);
	}

}

/*
// Save last parameters
void RippleDetector::saveCustomParametersToXml(XmlElement* parentElement)
{
	//XmlElement* mainNode = parentElement->createNewChildElement("RippleDetector");
	//RippleDetectorEditor* ed = (RippleDetectorEditor*)getEditor();

	//mainNode->setAttribute("rippleInputCh", ed->_inChannel);
	//mainNode->setAttribute("input2", m_input2);
	//mainNode->setAttribute("input1gate", m_input1gate);
	//mainNode->setAttribute("input2gate", m_input2gate);
	//mainNode->setAttribute("logicOp", m_logicOp);
	//mainNode->setAttribute("outputChan", m_outputChan);
	//mainNode->setAttribute("window", m_window);
	//mainNode->setAttribute("duration", m_pulseDuration);

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
	//			//m_input2 = mainNode->getIntAttribute("input2");
	//			m_input1gate = mainNode->getBoolAttribute("input1gate");
	//			m_input2gate = mainNode->getBoolAttribute("input2gate");
	//			m_logicOp = mainNode->getIntAttribute("logicOp");
	//			m_outputChan = mainNode->getIntAttribute("outputChan");
	//			m_window = mainNode->getIntAttribute("window");
	//			m_pulseDuration = mainNode->getIntAttribute("duration");

	//			//editor->updateSettings();
	//		}
	//	}
	//}
}
*/
