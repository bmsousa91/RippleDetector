// v060422
#include "RippleDetector.h"
#include "RippleDetectorEditor.h"

#define CALIBRATION_DURATION_SECONDS 20

RippleDetectorSettings::RippleDetectorSettings()
{}

TTLEventPtr RippleDetectorSettings::createRippleEvent(int64 sample_number, bool state)
{

	return TTLEvent::createTTLEvent(
		rippleEventChannel,
		sample_number,
		rippleOutputChannel,
		state
	);				

}

TTLEventPtr RippleDetectorSettings::createMovementEvent(int64 sample_number, bool state)
{

	return TTLEvent::createTTLEvent(
		movementEventChannel,
		sample_number,
		movementOutputChannel,
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
	addSelectedChannelsParameter(
		Parameter::STREAM_SCOPE,
		"emg_acc_in",
		"The continuous channel to analyze",
		1
	);

	addIntParameter(
		Parameter::STREAM_SCOPE,
		"emg_acc_out",
		"EMG/ACC output TTL channel: raise event when movement is detected and ripple detection is disabled",
		1,
		1,
		16
	);

	addFloatParameter(
		Parameter::STREAM_SCOPE,
		"emg_acc_std", 
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

// Create event channels
void RippleDetector::createEventChannels()
{
    /*
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
     */
}

// Update settings
void RippleDetector::updateSettings()
{

	settings.update(getDataStreams());

	for (auto stream : getDataStreams())
	{

		//TODO: Move these into map in RippleDetector class
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

		// Reset calibration arrays
		calibrationRmsValues[stream->getStreamId()].clear();
		calibrationMovRmsValues[stream->getStreamId()].clear();

		settings[stream->getStreamId()]->calibrationPoints = stream->getSampleRate() * CALIBRATION_DURATION_SECONDS;

		parameterValueChanged(stream->getParameter("Ripple_Input"));
		parameterValueChanged(stream->getParameter("Ripple_Out"));
		parameterValueChanged(stream->getParameter("ripple_std"));
		parameterValueChanged(stream->getParameter("time_thresh"));
		parameterValueChanged(stream->getParameter("refr_time"));
		parameterValueChanged(stream->getParameter("rms_samples"));
		parameterValueChanged(stream->getParameter("emg_acc_in"));
		parameterValueChanged(stream->getParameter("emg_acc_out"));
		parameterValueChanged(stream->getParameter("emg_acc_std"));
		parameterValueChanged(stream->getParameter("min_steady_time"));
		parameterValueChanged(stream->getParameter("min_mov_time"));

		EventChannel::Settings rippleSettings {
			EventChannel::Type::TTL,
			"Ripple detector output",
			"Triggers when a ripple is detected on the input channel",
			"dataderived.ripple",
			getDataStream(stream->getStreamId())
		};

		eventChannels.add(new EventChannel(rippleSettings));
		eventChannels.getLast()->addProcessor(processorInfo.get());
		settings[stream->getStreamId()]->rippleEventChannel = eventChannels.getLast();

		
		EventChannel::Settings movementSettings {
			EventChannel::Type::TTL,
			"Movement detector output",
			"Triggers when a movement is detected on the input channel",
			"dataderived.movement",
			getDataStream(stream->getStreamId())
		};

		eventChannels.add(new EventChannel(movementSettings));
		eventChannels.getLast()->addProcessor(processorInfo.get());
		settings[stream->getStreamId()]->movementEventChannel = eventChannels.getLast();
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
	else if (paramName.equalsIgnoreCase("EMG_ACC_In"))
	{
		int localIndex = (int)param->getValue();
		int globalIndex = getDataStream(param->getStreamId())->getContinuousChannels()[localIndex]->getGlobalIndex();
		settings[streamId]->movementInputChannel = globalIndex;
	}
	else if (paramName.equalsIgnoreCase("EMG_ACC_Out"))
	{
		settings[streamId]->movementOutputChannel = (int)param->getValue() - 1;
	}
	else if (paramName.equalsIgnoreCase("EMG_ACC_STD"))
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

			// Enable detection again if the mov. detector channel is "-" or if calibration button was clicked
			if (!pluginEnabled && (settings[streamId]->movSwitchEnabled || shouldCalibrate))
			{
				pluginEnabled = true;
				settings[streamId]->createMovementEvent(firstSampleInBlock,false);
			}

			// Compute thresholds
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

				settings[streamId]->pointsProcessed = 0;

				calibrationRmsValues[streamId].clear();
				calibrationMovRmsValues[streamId].clear();

				shouldCalibrate = false;
			}

			const float *rippleData = buffer.getReadPointer(settings[streamId]->rippleInputChannel, 0);

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

				//TODO: Calculate movRms

				if (settings[streamId]->isCalibrating)
				{
					calibrationRmsValues[streamId].push_back(rms);
					settings[streamId]->rmsMean += rms;

					//TODO: Calculate movRms
				}
				else
				{
					rmsValuesArray[streamId].push_back(rms);
					rmsNumSamplesArray[streamId].push_back(settings[streamId]->rmsEndIdx - rmsStartIdx);
					
					//TODO: Calculate movRms
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
				detectRipples(rmsValuesArray[streamId], rmsNumSamplesArray[streamId]);
				//TODO: evalMovement
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
		/*
		if (movSwitchEnabled) {
			int numMovCalibrationPoints = calibrationMovRmsValues[streamId].size();
			movRmsMean = movRmsMean / (double)numMovCalibrationPoints;
			for (unsigned int idx = 0; idx < numMovCalibrationPoints; idx++)
			{
				movRmsStdDev += pow(calibrationMovRmsValues[idx] - movRmsMean, 2.0);
			}
			movRmsStdDev = sqrt(movRmsStdDev / ((double)numMovCalibrationPoints - 1.0));
			movThreshold = movRmsMean + uiMovSds * movRmsStdDev;
		}
		*/

		// Print calculated statistics
		bool movSwitchEnabled = false;
		if (movSwitchEnabled) {
			/* TODO: Finish this
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
			*/
		}
		else {
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
void RippleDetector::detectRipples(std::vector<double>& rmsValuesArr, std::vector<int>& rmsNumSamplesArray)
{
    /*
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
    */
}

// Evaluate EMG/ACC signal to enable or disable ripple detection
void RippleDetector::evalMovement(std::vector<double>& movRmsValuesArr, std::vector<int>& movRmsNumSamplesArray)
{
    
    /*
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

		//printf("plugin %d, flagUp %d, flagDown %d, Up %d, Down %d, rms %f, thresh %f\n", pluginEnabled, flagMovMinTimeUp, flagMovMinTimeDown, counterMovUpThresh, counterMovDownThresh, rms, movThreshold);
	}
    */
}

// Send TTL output signal
void RippleDetector::sendTtlEvent(int rmsIndex, int val, int outputChannel)
{
    /*
	// Timestamp for this sample
	uint64 time_stamp = getTimestamp(uiInputChannel) + rmsIndex;

	uint8 ttlData;
	uint8 output_event_channel = outputChannel;
	ttlData = val << outputChannel;
	TTLEventPtr ttl = TTLEvent::createTTLEvent(pTtlEventChannel, time_stamp, &ttlData, sizeof(uint8), output_event_channel);
	addEvent(pTtlEventChannel, ttl, rmsIndex);
     */
}

// Handle events

void RippleDetector::handleTTLEvent (TTLEventPtr event)
{
    /* PK: Copied from PhaseDetector, implement for RippleDetector
    const uint16 eventStream = event->getStreamId();
    
    if (settings[eventStream]->gateLine > -1)
    {
     
        if (settings[eventStream]->gateLine == event->getLine())
            settings[eventStream]->isActive = event->getState();
        
    }
    */

}

/*
void RippleDetector::handleEvent(const EventChannel *rInEventInfo, const MidiMessage &rInEvent, int samplePosition)
{
}
 */

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
