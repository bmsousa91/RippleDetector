// v060422
#include "RippleDetectorEditor.h"

CustomTextBoxParameterEditor::CustomTextBoxParameterEditor(Parameter* param) : ParameterEditor(param)
{

    jassert(param->getType() == Parameter::FLOAT_PARAM
        || param->getType() == Parameter::INT_PARAM
        || param->getType() == Parameter::STRING_PARAM);

    parameterNameLabel = std::make_unique<Label>("Parameter name", param->getName());
    Font labelFont = Font("Silkscreen", "Regular", 10);
    int labelWidth = 120; //labelFont.getStringWidth(param->getName());
    parameterNameLabel->setFont(labelFont);
    parameterNameLabel->setColour(Label::textColourId, Colours::darkgrey);
    addAndMakeVisible(parameterNameLabel.get());

    if(param->getType() == Parameter::FLOAT_PARAM)
        valueTextBox = std::make_unique<Label>("Parameter value", String(float(param->getValue())));
    else
        valueTextBox = std::make_unique<Label>("Parameter value", param->getValue().toString());

    valueTextBox->setFont(Font("CP Mono", "Plain", 15));
    valueTextBox->setName(param->getProcessor()->getName() + " (" + String(param->getProcessor()->getNodeId()) + ") - " + param->getName());
    valueTextBox->setColour(Label::textColourId, Colours::white);
    valueTextBox->setColour(Label::backgroundColourId, Colours::grey);
    valueTextBox->setEditable(true);
    valueTextBox->addListener(this);
    valueTextBox->setTooltip(param->getDescription());
    addAndMakeVisible(valueTextBox.get());
    
    finalWidth = std::max(labelWidth, 80);

    setBounds(0, 0, 2*finalWidth, 18);
}

void CustomTextBoxParameterEditor::labelTextChanged(Label* label)
{
    if(param->getType() == Parameter::FLOAT_PARAM)
        param->setNextValue(label->getText().getFloatValue());
    else
        param->setNextValue(label->getText());
}

void CustomTextBoxParameterEditor::updateView()
{
    
    if (param != nullptr)
    {
        valueTextBox->setEditable(true);

        if(param->getType() == Parameter::FLOAT_PARAM)
            valueTextBox->setText(String(float(param->getValue())), dontSendNotification);
        else
            valueTextBox->setText(param->getValue().toString(), dontSendNotification);
    }
    else {
        valueTextBox->setEditable(false);
    }

}

void CustomTextBoxParameterEditor::resized()
{
    parameterNameLabel->setBounds(0, 0, int(3 * finalWidth / 5), 18);
    valueTextBox->setBounds(int(3 * finalWidth / 5), 0, int(2 * finalWidth / 5), 18);
}

// Class constructor
RippleDetectorEditor::RippleDetectorEditor(GenericProcessor* parentNode)
	: GenericEditor(parentNode)
{
    
    rippleDetector = (RippleDetector*)parentNode;
    
	desiredWidth = 460; //Plugin's desired width`

	/* Ripple Detection Settings */
	addSelectedChannelsParameterEditor("Ripple_Input", 10, 40);

	addComboBoxParameterEditor("Ripple_Out", 10, 70);

	Parameter* param = getProcessor()->getParameter("ripple_std");
    addCustomParameterEditor(new CustomTextBoxParameterEditor(param), 95, 25);

	param = getProcessor()->getParameter("time_thresh");
    addCustomParameterEditor(new CustomTextBoxParameterEditor(param), 95, 50);

	param = getProcessor()->getParameter("refr_time");
    addCustomParameterEditor(new CustomTextBoxParameterEditor(param), 95, 75);

	param = getProcessor()->getParameter("rms_samples");
    addCustomParameterEditor(new CustomTextBoxParameterEditor(param), 95, 100);

	/* EMG / ACC Movement Detection Settings */
	addComboBoxParameterEditor("mov_detect", 230, 20);
	addSelectedChannelsParameterEditor("Mov_Input", 230, 65);
	addComboBoxParameterEditor("mov_out", 230, 85);

	param = getProcessor()->getParameter("mov_std");
    addCustomParameterEditor(new CustomTextBoxParameterEditor(param), 325, 25);

	param = getProcessor()->getParameter("min_time_st");
    addCustomParameterEditor(new CustomTextBoxParameterEditor(param), 325, 50);

	param = getProcessor()->getParameter("min_time_mov");
    addCustomParameterEditor(new CustomTextBoxParameterEditor(param), 325, 75);

	/* Calibration Button */
	calibrateButton = std::make_unique<UtilityButton>("Calibrate", titleFont);
	calibrateButton->addListener(this);
    calibrateButton->setRadius(3.0f);
    calibrateButton->setBounds(335, 100, 100, 25);
    addAndMakeVisible(calibrateButton.get());

	/*
	// ------------------------
    // Init comboboxes with available channels
	_comboInChannelSelection = createComboBox("Input channel");
	//updateInputChannels(AVAILABLE_INPUT_CHANNELS);

	_comboOutChannelSelection = createComboBox("Output TTL channel: raise event when ripple is detected");
	//updateOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);

	_comboMovChannelSelection = createComboBox("EMG/ACC input channel");
	//updateMovChannels(AVAILABLE_INPUT_CHANNELS);

	_comboMovOutChannelSelection = createComboBox("EMG/ACC output TTL channel: raise event when movement is detected and ripple detection is disabled");
	//updateMovOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);

	// ------------------------
	// Input box: number of standard deviations above the average to be the amplitude threshold
	_inputBoxSds = createInputBox("sd value", "Number of standard deviations above the average to be the amplitude threshold");
	_inputBoxSds->setText("5", sendNotification);

	// Input box: time threshold (milliseconds)
	_inputBoxTimeThreshold = createInputBox("time threshold value", "Time threshold [ms]");
	_inputBoxTimeThreshold->setText("10", sendNotification);

	// Input box: refractory time (milliseconds)
	_inputBoxRefractory = createInputBox("refractory value", "Refractory time [ms]");
	_inputBoxRefractory->setText("140", sendNotification);

	// Input box: number of samples to calculate SWR RMS values
	_inputBoxRmsSamples = createInputBox("rms samples value", "Number of samples to calculate the RMS");
	_inputBoxRmsSamples->setText("128", sendNotification);

	// Input box: EMG/ACC standard deviations above the average to be the threshold
	_inputBoxMovSds = createInputBox("emg/acc sd value", "Number of standard deviations above the average to be the amplitude threshold for the EMG/ACC");
	_inputBoxMovSds->setText("5", sendNotification);

	// Input box: EMG/ACC minimum time without movement to enable detection
	_inputBoxMinTimeWoMov = createInputBox("emg/acc min time wo movement", "Minimum time steady (in milliseconds). The minimum time below the EMG/ACC threshold to enable detection");
	_inputBoxMinTimeWoMov->setText("5000", sendNotification);

	// Input box: EMG/ACC minimum time with movement to disable detection
	_inputBoxMinTimeWMov = createInputBox("emg/acc min time w movement", "Minimum time with movement (in milliseconds). The minimum time above the EMG/ACC threshold to disable detection");
	_inputBoxMinTimeWMov->setText("10", sendNotification);

	// ------------------------
	// Create labels
	_labelInputChannel = createLabel("Input", Justification::bottomLeft);
	_labelOutputChannel = createLabel("Output", Justification::bottomLeft);
	_labelMovChannel = createLabel("EMG/ACC", Justification::bottomLeft);
	_labelMovOutputChannel = createLabel("Mov. Out", Justification::bottomLeft);
	_labelSds = createLabel("SD", Justification::centredRight);
	_labelTimeThreshold = createLabel("Time Thr.", Justification::centredRight);
	_labelRefractoryTime = createLabel("Refr. Time", Justification::centredRight);
	_labelRmsSamples = createLabel("RMS Samp.", Justification::centredRight);
	_labelMovSds = createLabel("EMG/ACC SD", Justification::centredRight);
	_labelMinTimeWoMov = createLabel("Min Time St", Justification::centredRight);
	_labelMinTimeWMov = createLabel("Min Time Mov", Justification::centredRight);

	// ------------------------
	// Create the "Recalibrate" button
	_buttonCalibrate = new UtilityButton("CALIBRATE", Font("Default", 12, Font::plain));
	//_buttonCalibrate->addListener(this);
	_buttonCalibrate->setClickingTogglesState(false);
	_buttonCalibrate->setTooltip("Click this button to recalibrate threshold values for ripple detection and for the movement detection mechanism");
	addAndMakeVisible(_buttonCalibrate);

	// Set the position of all components
	setComponentsPositions();
	*/
}

// Class destructor
RippleDetectorEditor::~RippleDetectorEditor()
{
}


// Set the position of each component (labels, inputs, comboboxes...)
/*
void RippleDetectorEditor::setComponentsPositions() {

	// Set the position, width and heigh of the elements: x, y, width, height
	const int top_margin = 25;
	const int left_margin = 5;
	const int ySpaceInputs = 5;
	const int xSpaceColumns = 5;
	const int ibHeight = 18;
	const int ilHeight = 18;
	const int ilWidth = 80;
	const int eilWidth = 90;

	// Set the limits, size, and location of each component
	// Input
	_labelInputChannel->setBounds(left_margin, top_margin + 2, 60, 18);
	_comboInChannelSelection->setBounds(left_margin, getNextY(_labelInputChannel), 60, 20);
	// Output
	_labelOutputChannel->setBounds(left_margin, getNextY(_comboInChannelSelection), 60, 20);
	_comboOutChannelSelection->setBounds(left_margin, getNextY(_labelOutputChannel), 60, 20);

	// SWR SDs
	_labelSds->setBounds(getNextX(_labelInputChannel) + xSpaceColumns, top_margin + 5, ilWidth, ilHeight);
	_inputBoxSds->setBounds(getNextX(_labelSds), top_margin + 5, 60, ibHeight);
	// SWR time threshold
	_labelTimeThreshold->setBounds(getNextX(_labelInputChannel) + xSpaceColumns, getNextY(_labelSds) + ySpaceInputs, ilWidth, ilHeight);
	_inputBoxTimeThreshold->setBounds(getNextX(_labelSds), getNextY(_inputBoxSds) + ySpaceInputs, 60, ibHeight);
	// SWR refractory time
	_labelRefractoryTime->setBounds(getNextX(_labelInputChannel) + xSpaceColumns, getNextY(_labelTimeThreshold) + ySpaceInputs, ilWidth, ilHeight);
	_inputBoxRefractory->setBounds(getNextX(_labelSds), getNextY(_inputBoxTimeThreshold) + ySpaceInputs, 60, ibHeight);
	// SWR RMS number of samples
	_labelRmsSamples->setBounds(getNextX(_labelInputChannel) + xSpaceColumns, getNextY(_labelRefractoryTime) + ySpaceInputs, ilWidth, ilHeight);
	_inputBoxRmsSamples->setBounds(getNextX(_labelSds), getNextY(_inputBoxRefractory) + ySpaceInputs, 60, ibHeight);

	// EMG/ACC channel combo
	_labelMovChannel->setBounds(getNextX(_inputBoxRmsSamples) + 3 * xSpaceColumns, top_margin + 2, 60, 18);
	_comboMovChannelSelection->setBounds(getNextX(_inputBoxRmsSamples) + 3 * xSpaceColumns, getNextY(_labelMovChannel), 60, 20);

	_labelMovOutputChannel->setBounds(getNextX(_inputBoxRmsSamples) + 3 * xSpaceColumns, getNextY(_comboMovChannelSelection), 60, 20);
	_comboMovOutChannelSelection->setBounds(getNextX(_inputBoxRmsSamples) + 3 * xSpaceColumns, getNextY(_labelMovOutputChannel), 60, 20);

	// EMG/ACC SDs
	_labelMovSds->setBounds(getNextX(_comboMovChannelSelection) + xSpaceColumns, top_margin + 5, eilWidth, ilHeight);
	_inputBoxMovSds->setBounds(getNextX(_labelMovSds), top_margin + 5, 60, ibHeight);
	// EMG/ACC minimum time without movement
	_labelMinTimeWoMov->setBounds(getNextX(_comboMovChannelSelection) + xSpaceColumns, getNextY(_labelMovSds) + ySpaceInputs, eilWidth, ilHeight);
	_inputBoxMinTimeWoMov->setBounds(getNextX(_labelMovSds), getNextY(_inputBoxMovSds) + ySpaceInputs, 60, ibHeight);
	// EMG/ACC minimum time with movement
	_labelMinTimeWMov->setBounds(getNextX(_comboMovChannelSelection) + xSpaceColumns, getNextY(_labelMinTimeWoMov) + ySpaceInputs, eilWidth, ilHeight);
	_inputBoxMinTimeWMov->setBounds(getNextX(_labelMovSds), getNextY(_inputBoxMinTimeWoMov) + ySpaceInputs, 60, ibHeight);

	// Calibrate button
	_buttonCalibrate->setBounds(getNextX(_inputBoxRmsSamples) + 19 * xSpaceColumns, getNextY(_inputBoxMinTimeWMov) + 7, 120, 20);
}
*/

/*
// Update input channel combo and set the first channel in the list
void RippleDetectorEditor::updateInputChannels(int channelCount)
{
    _comboInChannelSelection->clear();
    for (int channel = 1; channel <= channelCount; channel++)
    {
        _comboInChannelSelection->addItem(std::to_string(channel), channel);
    }
    _comboInChannelSelection->setSelectedId(1, dontSendNotification);
}

// Update output channel combo and set the first channel in the list
void RippleDetectorEditor::updateOutputChannels(int channelCount)
{
	_comboOutChannelSelection->clear();
	for (int channel = 1; channel <= channelCount; channel++)
	{
		_comboOutChannelSelection->addItem("TTL" + std::to_string(channel), channel);
	}
	_comboOutChannelSelection->setSelectedId(1, dontSendNotification);
}

// Update EMG/ACC channel combo and set "-"
void RippleDetectorEditor::updateMovChannels(int channelCount)
{
	_comboMovChannelSelection->clear();
	// Add "-"
	_comboMovChannelSelection->addItem("-", _noMovChannelId);
	// Add data channels
	for (int channel = 1; channel <= channelCount; channel++)
	{
		_comboMovChannelSelection->addItem(std::to_string(channel), channel);
	}
	// Add "ACCEL" option if auxiliary inputs were enabled
	if (!_accChannels.empty()) _comboMovChannelSelection->addItem("ACCEL", _accelChannelId);
	
	_comboMovChannelSelection->setSelectedId(_noMovChannelId, dontSendNotification);
}

// Update EMG/ACC output channel combo and set the first channel in the list
void RippleDetectorEditor::updateMovOutputChannels(int channelCount)
{
	_comboMovOutChannelSelection->clear();
	for (int channel = 1; channel <= channelCount; channel++)
	{
		_comboMovOutChannelSelection->addItem("TTL" + std::to_string(channel), channel);
	}
	_comboMovOutChannelSelection->setSelectedId(3, dontSendNotification);
}
*/

// Create a label object and make it visible
Label* RippleDetectorEditor::createLabel(String labelText, Justification just)
{
	Label *label = new Label("label", TRANS(labelText));	//Create the object and set its name
	label->setFont(Font("Small Text", 12, Font::plain));	//Set the font size and style
	label->setJustificationType(just);						//Set the justification of the label
	label->setEditable(false, false, false);					//Set if this label can be editable by the user or not
	label->setColour(Label::textColourId, Colours::darkgrey);	//Set the label colour
	addAndMakeVisible(label);									//Adding the label to the component and turning it visible

	return label;
}

// Create an inputBox object (editable lable) and make it visible
Label* RippleDetectorEditor::createInputBox(String componentName, String tooltipText) {
	Label* inputBox = new Label(componentName, " ");		//Create the object and set its name
	inputBox->addListener(this);										//Add listener
	inputBox->setFont(Font("Default", 15, Font::plain));				//Set the font size and style
	inputBox->setColour(Label::textColourId, Colours::white);			//Set text colour
	inputBox->setColour(Label::backgroundColourId, Colours::grey);		//Set background colour
	inputBox->setEditable(true);										//Set the label editable
	inputBox->setJustificationType(Justification::centred);		//Set the justification
	inputBox->setTooltip(tooltipText);							//Set tooltip text
	addAndMakeVisible(inputBox);								//Adding the inputBox to the component and turning it visible

	return inputBox;
}

// Create a comboBox and make it visible
ScopedPointer<ComboBox> RippleDetectorEditor::createComboBox(String tooltipText) {
	ScopedPointer<ComboBox> combo = new ComboBox();
	combo->addListener(this);
	combo->setTooltip(tooltipText);
	addAndMakeVisible(combo);

	return combo;
}

// Called when any button is clicked
void RippleDetectorEditor::buttonClicked(Button *pInButtonClicked)
{
	/* Calibrate button was clicked */
	rippleDetector->shouldCalibrate = true;
}

// Called when the value of any combo box changes
void RippleDetectorEditor::comboBoxChanged(ComboBox *pInComboBoxChanged)
{
	/*
    if (pInComboBoxChanged == _comboInChannelSelection)
        _inChannel = pInComboBoxChanged->getSelectedId();

    if (pInComboBoxChanged == _comboOutChannelSelection)
        _outChannel = pInComboBoxChanged->getSelectedId();

	if (pInComboBoxChanged == _comboMovChannelSelection) {
		_movChannel = pInComboBoxChanged->getSelectedId();
		_movChannChanged = true;

		// Disabling input boxes if selected "-"
		if (_movChannel == _noMovChannelId) {
			_movSwitchEnabled = false;
			setInputBoxStatus(_inputBoxMovSds, false);
			setInputBoxStatus(_inputBoxMinTimeWoMov, false);
			setInputBoxStatus(_inputBoxMinTimeWMov, false);
			_comboMovOutChannelSelection->setEnabled(false);
		}
		else {
			_movSwitchEnabled = true;
			setInputBoxStatus(_inputBoxMovSds, true);
			setInputBoxStatus(_inputBoxMinTimeWoMov, true);
			setInputBoxStatus(_inputBoxMinTimeWMov, true);
			_comboMovOutChannelSelection->setEnabled(true);
		}

		// Raise the flag if "ACCEL" was selected
		if (_movChannel == _accelChannelId) _accelerometerSelected = true;
		else _accelerometerSelected = false;
	}

	if (pInComboBoxChanged == _comboMovOutChannelSelection)
		_movOutChannel = pInComboBoxChanged->getSelectedId();
	*/
}

// Called when any input box changes
void RippleDetectorEditor::labelTextChanged(Label* pLabel)
{
	/*
	Value val = pLabel->getTextValue();

	// Check and adjust max and min values for the inputs
	if (pLabel == _inputBoxSds) {
		_rippleSds = adjustAndGetValueFromText(val.getValue());
		_rippleSds = setDecimalPlaces(_rippleSds,1);

		pLabel->setText(String(_rippleSds), dontSendNotification);
	}

	if (pLabel == _inputBoxTimeThreshold) {
		_timeThreshold = (int)adjustAndGetValueFromText(val.getValue());

		pLabel->setText(String(_timeThreshold), dontSendNotification);
	}

	if (pLabel == _inputBoxRefractory) {
		_refractoryTime = (int)adjustAndGetValueFromText(val.getValue());

		pLabel->setText(String(_refractoryTime), dontSendNotification);
	}

	if (pLabel == _inputBoxRmsSamples) {
		_rmsSamples = (int)adjustAndGetValueFromText(val.getValue());

		pLabel->setText(String(_rmsSamples), dontSendNotification);
	}
	
	if (pLabel == _inputBoxMovSds) {
		_movSds = (double)adjustAndGetValueFromText(val.getValue());
		_movSds = setDecimalPlaces(_movSds, 1);

		pLabel->setText(String(_movSds), dontSendNotification);
	}

	if (pLabel == _inputBoxMinTimeWoMov) {
		_minTimeWoMov = (int)adjustAndGetValueFromText(val.getValue());

		pLabel->setText(String(_minTimeWoMov), dontSendNotification);
	}

	if (pLabel == _inputBoxMinTimeWMov) {
		_minTimeWMov = (int)adjustAndGetValueFromText(val.getValue());

		pLabel->setText(String(_minTimeWMov), dontSendNotification);
	}
	*/
}

// Enable or disable inputBox
void RippleDetectorEditor::setInputBoxStatus(Label* pInputBox, bool enabled) {
	pInputBox->setEditable(enabled);
	if (enabled) pInputBox->setColour(Label::textColourId, Colours::white);
	else pInputBox->setColour(Label::textColourId, Colours::darkgrey);
}

// Convert text to double
double RippleDetectorEditor::adjustAndGetValueFromText(const String& text) {

	String t(text.trimStart());

	while (t.startsWithChar('+'))
		t = t.substring(1).trimStart();

	return t.initialSectionContainingOnly("0123456789.,-").getDoubleValue();
}

// Round doubles to the desired number of decimal places
double setDecimalPlaces(double value, int places) {
	const double multiplier = std::pow(10.0, places);
	return std::round(value * multiplier) / multiplier;
}

// Get next y coordinates for positioning elements in the plugin interface
static int getNextY(Component *rInReference)
{
	int next_Y = rInReference->getY() + rInReference->getHeight();
	return next_Y;
}

// Get next x coordinates for positioning elements in the plugin interface
static int getNextX(Component *rInReference)
{
	int next_X = rInReference->getX() + rInReference->getWidth();
	return next_X;
}

// Called when settings are updated
void RippleDetectorEditor::updateSettings()
{

	LOGD("RippleDetectorEditor::updateSettings() called");

    //int channelCount = getProcessor()->getNumInputs();

	/*
	// Check if there are AUX channels and store their indices
	const DataChannel* ch;
	_accChannels.clear();
	for (int c = 0; c < channelCount; c++)
	{
		ch = getProcessor()->getDataChannel(c);
		DataChannel::DataChannelTypes tp = ch->getChannelType();
		if (tp == DataChannel::DataChannelTypes::AUX_CHANNEL) _accChannels.push_back(ch->getSourceIndex());
	}
	*/

	/*
    updateInputChannels(channelCount);
	updateOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);
	updateMovChannels(channelCount);
	updateMovOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);
	*/

}
