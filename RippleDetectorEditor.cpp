#include "RippleDetectorEditor.h"

#define AVAILABLE_INPUT_CHANNELS 32
#define AVAILABLE_TTL_OUTPUT_CHANNELS 8

#define MIN_AMP_THRESHOLD 0
#define MAX_AMP_THRESHOLD 9999

#define MIN_TIME_THRESHOLD 0
#define MAX_TIME_THRESHOLD 9999

#define MIN_REFRACTORY_TIME 0
#define MAX_REFRACTORY_TIME 999999

#define MIN_RMS_BLOCK_SIZE 1
#define MAX_RMS_BLOCK_SIZE 2048

#define MIN_MOV_AMP_THRESHOLD 0
#define MAX_MOV_AMP_THRESHOLD 9999

#define MIN_TIME_MOV_BELOW_THRESHOLD 0
#define MAX_TIME_MOV_BELOW_THRESHOLD 999999

#define MIN_TIME_MOV_ABOVE_THRESHOLD 0
#define MAX_TIME_MOV_ABOVE_THRESHOLD 999999

// Class constructor
RippleDetectorEditor::RippleDetectorEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors = true)
	: GenericEditor(parentNode, useDefaultParameterEditors)
{
	desiredWidth = 460; //Plugin's desired width

	// ------------------------
    // Init comboboxes with available channels
	_comboInChannelSelection = createComboBox("Input channel");
	updateInputChannels(AVAILABLE_INPUT_CHANNELS);

	_comboOutChannelSelection = createComboBox("Output TTL channel: raise event when ripple is detected");
	updateOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);

	_comboMovChannelSelection = createComboBox("EMG/ACC input channel");
	updateMovChannels(AVAILABLE_INPUT_CHANNELS);

	_comboMovOutChannelSelection = createComboBox("EMG/ACC output TTL channel: raise event when movement is detected and ripple detection is disabled");
	updateMovOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);

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
	_inputBoxMinTimeWoMov->setText("3000", sendNotification);

	// Input box: EMG/ACC minimum time with movement to disable detection
	_inputBoxMinTimeWMov = createInputBox("emg/acc min time w movement", "Minimum time with movement (in milliseconds). The minimum time above the EMG/ACC threshold to disable detection");
	_inputBoxMinTimeWMov->setText("5", sendNotification);

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
	_buttonCalibrate->addListener(this);
	_buttonCalibrate->setClickingTogglesState(false);
	_buttonCalibrate->setTooltip("Click this button to recalibrate threshold values for ripple detection and for the movement detection mechanism");
	addAndMakeVisible(_buttonCalibrate);

	// Set the position of all components
	setComponentsPositions();
}

// Class destructor
RippleDetectorEditor::~RippleDetectorEditor()
{
}

// Set the position of each component (labels, inputs, comboboxes...)
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
    if(pInButtonClicked == _buttonCalibrate){
        _calibrate = true;
    }
}

// Called when the value of any combo box changes
void RippleDetectorEditor::comboBoxChanged(ComboBox *pInComboBoxChanged)
{
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
}

// Called when any input box changes
void RippleDetectorEditor::labelTextChanged(Label* pLabel)
{
	Value val = pLabel->getTextValue();

	// Check and adjust max and min values for the inputs
	if (pLabel == _inputBoxSds) {
		_rippleSds = adjustAndGetValueFromText(val.getValue());
		_rippleSds = setDecimalPlaces(_rippleSds,1);

		if (_rippleSds < MIN_AMP_THRESHOLD) {
			_rippleSds = MIN_AMP_THRESHOLD;
		}
		else if(_rippleSds > MAX_AMP_THRESHOLD) {
			_rippleSds = MAX_AMP_THRESHOLD;
		}
		pLabel->setText(String(_rippleSds), dontSendNotification);
	}

	if (pLabel == _inputBoxTimeThreshold) {
		_timeThreshold = (int)adjustAndGetValueFromText(val.getValue());

		if (_timeThreshold < MIN_TIME_THRESHOLD) {
			_timeThreshold = MIN_TIME_THRESHOLD;
		}
		else if (_timeThreshold > MAX_TIME_THRESHOLD) {
			_timeThreshold = MAX_TIME_THRESHOLD;
		}
		pLabel->setText(String(_timeThreshold), dontSendNotification);
	}

	if (pLabel == _inputBoxRefractory) {
		_refractoryTime = (int)adjustAndGetValueFromText(val.getValue());

		if (_refractoryTime < MIN_REFRACTORY_TIME) {
			_refractoryTime = MIN_REFRACTORY_TIME;
		}
		else if (_refractoryTime > MAX_REFRACTORY_TIME) {
			_refractoryTime = MAX_REFRACTORY_TIME;
		}
		pLabel->setText(String(_refractoryTime), dontSendNotification);
	}

	if (pLabel == _inputBoxRmsSamples) {
		_rmsSamples = (int)adjustAndGetValueFromText(val.getValue());

		if (_rmsSamples < MIN_RMS_BLOCK_SIZE) {
			_rmsSamples = MIN_RMS_BLOCK_SIZE;
		}
		else if (_rmsSamples > MAX_RMS_BLOCK_SIZE) {
			_rmsSamples = MAX_RMS_BLOCK_SIZE;
		}
		pLabel->setText(String(_rmsSamples), dontSendNotification);
	}
	
	if (pLabel == _inputBoxMovSds) {
		_movSds = (double)adjustAndGetValueFromText(val.getValue());
		_movSds = setDecimalPlaces(_movSds, 1);

		if (_movSds < MIN_MOV_AMP_THRESHOLD) {
			_movSds = MIN_MOV_AMP_THRESHOLD;
		}
		else if (_movSds > MAX_MOV_AMP_THRESHOLD) {
			_movSds = MAX_MOV_AMP_THRESHOLD;
		}
		pLabel->setText(String(_movSds), dontSendNotification);
	}

	if (pLabel == _inputBoxMinTimeWoMov) {
		_minTimeWoMov = (int)adjustAndGetValueFromText(val.getValue());

		if (_minTimeWoMov < MIN_TIME_MOV_BELOW_THRESHOLD) {
			_minTimeWoMov = MIN_TIME_MOV_BELOW_THRESHOLD;
		}
		else if (_minTimeWoMov > MAX_TIME_MOV_BELOW_THRESHOLD) {
			_minTimeWoMov = MAX_TIME_MOV_BELOW_THRESHOLD;
		}
		pLabel->setText(String(_minTimeWoMov), dontSendNotification);
	}

	if (pLabel == _inputBoxMinTimeWMov) {
		_minTimeWMov = (int)adjustAndGetValueFromText(val.getValue());

		if (_minTimeWMov < MIN_TIME_MOV_ABOVE_THRESHOLD) {
			_minTimeWMov = MIN_TIME_MOV_ABOVE_THRESHOLD;
		}
		else if (_minTimeWMov > MAX_TIME_MOV_ABOVE_THRESHOLD) {
			_minTimeWMov = MAX_TIME_MOV_ABOVE_THRESHOLD;
		}
		pLabel->setText(String(_minTimeWMov), dontSendNotification);
	}
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
    int channelCount = getProcessor()->getNumInputs();

	// Check if there are AUX channels and store their indices
	const DataChannel* ch;
	_accChannels.clear();
	for (int c = 0; c < channelCount; c++)
	{
		ch = getProcessor()->getDataChannel(c);
		DataChannel::DataChannelTypes tp = ch->getChannelType();
		if (tp == DataChannel::DataChannelTypes::AUX_CHANNEL) _accChannels.push_back(ch->getSourceIndex());
	}

    updateInputChannels(channelCount);
	updateOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);
	updateMovChannels(channelCount);
	updateMovOutputChannels(AVAILABLE_TTL_OUTPUT_CHANNELS);
}