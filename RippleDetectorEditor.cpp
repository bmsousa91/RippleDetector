#include "RippleDetectorEditor.h"

#define SPACING 10
#define TOP_MARGIN 30
#define AVAILABLE_CHANNELS 8

//Class constructor
RippleDetectorUi::RippleDetectorUi()
{
    //Set the style of the GUI
    _defaultLookAndFeel = new LookAndFeel_V2();
    setLookAndFeel(_defaultLookAndFeel);

    //Init available channels
    _comboChannelSelection = new ComboBox();
    updateInputChannels(AVAILABLE_CHANNELS);
    _comboChannelSelection->addListener(this);
    addAndMakeVisible(_comboChannelSelection);

    _comboOutChannelSelection = new ComboBox();
	updateOutputChannels(AVAILABLE_CHANNELS);
    _comboOutChannelSelection->addListener(this);
    addAndMakeVisible(_comboOutChannelSelection);

	//Slider parameters
	std::vector<double> range(3);
	double initValue;
	bool textBoxEditable = false;
	
	//Slider creation: number of standard deviations above the average to be the amplitude threshold
	range = { 0.0, 20.0, 0.5 }; //{min, max, increment}
	initValue = 3.0;
	_sliderThresholdSds = createSlider("Threshold SDs", range, initValue, textBoxEditable);

	//Slider creation: time threshold (milliseconds)
	range = { 0, 5000, 1 }; //{min, max, increment}
	initValue = 5;
	_sliderThresholdTime = createSlider("Time Threshold", range, initValue, textBoxEditable);

	//Slider creation: refractory time (milliseconds)
	range = { 0, 10000, 100 }; //{min, max, increment}
	initValue = 1000;
	_sliderRefractoryTime = createSlider("Refractory Time", range, initValue, textBoxEditable);

	//Slider creation: number of samples on each RMS calculation block
	range = { 1, 2048, 1 }; //{min, max, increment}
	initValue = 128;
	_sliderRmsSamples = createSlider("RMS Block Size", range, initValue, textBoxEditable);
    
	//Create labels
	_labelInputChannel = createLabel("Input Channel");
	_labelOutChannel = createLabel("Output Channel");
	_labelThresholdSds = createLabel("SDs");
	_labelThresholdTime = createLabel("Time Thr. (ms)");
	_labelRefractoryTime = createLabel("Refr. Time (ms)");
	_labelRmsSamples = createLabel("RMS Samples");

	//Create "Recalibrate" button
	_buttonCalibrate = new TextButton("Recalibrate");			// Create the object and set its name
	_buttonCalibrate->setConnectedEdges(Button::ConnectedOnLeft	// Set the style of the corners of this button
		| Button::ConnectedOnRight 
		| Button::ConnectedOnTop 
		| Button::ConnectedOnBottom); 
    _buttonCalibrate->setColour(TextButton::buttonNormal,		// Set colour according to its position inside the button, the position is represented by the ID
		Colour::fromRGBA(61, 78, 69, 255));
	_buttonCalibrate->addListener(this);						// Adding a listener for this button, in this case it is this class
	addAndMakeVisible(_buttonCalibrate);						// Adding the button into this component and turning it visible

    setSize(100, 100);
}

//Class destructor
RippleDetectorUi::~RippleDetectorUi()
{
}

void RippleDetectorUi::paint(Graphics &rInGraphics)
{
}

//Update input channel combo and set the first channel in the list
void RippleDetectorUi::updateInputChannels(int channelCount)
{
    _comboChannelSelection->clear();
    for (int channel = 1; channel <= channelCount; channel++)
    {
        _comboChannelSelection->addItem(std::to_string(channel), channel);
    }
    _comboChannelSelection->setSelectedId(1);
}

//Update output channel combo and set the first channel in the list
void RippleDetectorUi::updateOutputChannels(int channelCount)
{
	_comboOutChannelSelection->clear();
	for (int channel = 1; channel <= channelCount; channel++)
	{
		_comboOutChannelSelection->addItem(std::to_string(channel), channel);
	}
	_comboOutChannelSelection->setSelectedId(1);
}

//Create a slider object and make it visible
Slider* RippleDetectorUi::createSlider(String sliderName, std::vector<double> range, double initValue, bool textBoxEditable)
{
	Slider *slider = new Slider(sliderName);						//Create the object and set its name
	slider->setRange(range[0], range[1], range[2]);					//Set the minium value, the max value and the increment interval
	slider->setValue(initValue);									//Set the initial value
	slider->setSliderStyle(Slider::Rotary);							//Set the style of the styler, horizontal or vertical
	slider->setTextBoxIsEditable(textBoxEditable);					//Set the visual style of the slider
	slider->setTextBoxStyle(Slider::TextBoxRight, false, 80, 20);	//Set the visual style of the slider
	slider->setTextValueSuffix("");									//Set the sufix of the value shown at the left of the slider
	slider->addListener(this);										//Adding a listener, the class that will handle this slider
	addAndMakeVisible(slider);										//Make the slider visible

	return slider;
}

//Create a label object and make it visible
Label* RippleDetectorUi::createLabel(String labelText)
{
	Label *label = new Label("label", TRANS(labelText));		// Create the object and set its name
	label->setFont(Font(15.10f, Font::plain));					// Set the font size and style
	label->setJustificationType(Justification::centredLeft);	// Set the justification of the label
	label->setEditable(false, false, false);					// Set if this label can be editable by the user or not
	label->setColour(TextEditor::textColourId, Colours::black);	// Set the label colour
	addAndMakeVisible(label);									// Adding the label to the component and turning it visible

	return label;
}

void RippleDetectorUi::resized()
{
    //Set the limits of the UI_canvas
    setBounds(0, 0, getWidth(), getHeight());

    //Set the position, width and heigh of the elements - x, y, width, height
    int top_margin = 25;
    int left_margin = 5;

    //Set the limits, size, and location of each component for them to be visible
    _labelInputChannel->setBounds(left_margin, top_margin, 100, 15);
    _comboChannelSelection->setBounds(left_margin, getNextY(_labelInputChannel), 100, 20);

    _labelOutChannel->setBounds(getNextX(_comboChannelSelection), top_margin, 100, 15);
    _comboOutChannelSelection->setBounds(getNextX(_comboChannelSelection), getNextY(_labelInputChannel), 100, 20);

    _buttonCalibrate->setBounds(getNextX(_comboOutChannelSelection), getNextY(_labelInputChannel), 100, 20);

    _labelThresholdSds->setBounds(left_margin, getNextY(_comboChannelSelection), 150, 15);
    _sliderThresholdSds->setBounds(left_margin, getNextY(_labelThresholdSds), 100, 20);

	_labelThresholdTime->setBounds(getNextX(_sliderThresholdSds), getNextY(_comboChannelSelection), 150, 15);
	_sliderThresholdTime->setBounds(getNextX(_sliderThresholdSds), getNextY(_labelThresholdTime), 100, 20);

    _labelRefractoryTime->setBounds(getNextX(_sliderThresholdTime), getNextY(_comboChannelSelection), 150, 15);
    _sliderRefractoryTime->setBounds(getNextX(_sliderThresholdTime), getNextY(_labelRefractoryTime), 100, 20);

    _labelRmsSamples->setBounds(getNextX(_sliderRefractoryTime), getNextY(_comboChannelSelection), 150, 15);
    _sliderRmsSamples->setBounds(getNextX(_sliderRefractoryTime), getNextY(_labelRmsSamples), 100, 20);
}

//Called when any button is clicked
void RippleDetectorUi::buttonClicked(Button *pInButtonClicked)
{
    if(pInButtonClicked == _buttonCalibrate){
        _calibrate = true;
    }
}

//Called when the value of any input box changes
void RippleDetectorUi::sliderValueChanged(Slider *pInSliderChanged)
{
    if (pInSliderChanged == _sliderThresholdSds)
        _thresholdSds = pInSliderChanged->getValue();

	if (pInSliderChanged == _sliderThresholdTime)
		_thresholdTime = pInSliderChanged->getValue();

	if (pInSliderChanged == _sliderRefractoryTime)
		_refractoryTime = (int)pInSliderChanged->getValue();

    if (pInSliderChanged == _sliderRmsSamples)
        _rmsSamples = (int)pInSliderChanged->getValue();
}

//Called when the value of any combo box changes
void RippleDetectorUi::comboBoxChanged(ComboBox *pInComboBoxChanged)
{
    if (pInComboBoxChanged == _comboChannelSelection)
        _inChannel = pInComboBoxChanged->getSelectedId();

    if (pInComboBoxChanged == _comboOutChannelSelection)
        _outChannel = pInComboBoxChanged->getSelectedId();
}

//Get next y coordinates for positioning elements in the plugin interface
static int getNextY(Component *rInReference)
{
	int next_Y = rInReference->getY() + rInReference->getHeight() + SPACING;
	return next_Y;
}

//Get next x coordinates for positioning elements in the plugin interface
static int getNextX(Component *rInReference)
{
	int next_X = rInReference->getX() + rInReference->getWidth() + SPACING;
	return next_X;
}

//----------------------------------------------------------------------

//Editor constructor
RippleDetectorEditor::RippleDetectorEditor(GenericProcessor *pInParentNode, bool useDefaultParameterEditors)
    : GenericEditor(pInParentNode, useDefaultParameterEditors)
{
    addAndMakeVisible(&_pluginUi);
    const int plugin_witdh = 480;
    setDesiredWidth(plugin_witdh);
}

//Editor destructor
RippleDetectorEditor::~RippleDetectorEditor()
{
}

//Called when settings are update
void RippleDetectorEditor::updateSettings()
{
    int channelCount = getProcessor()->getNumInputs();
    _pluginUi.updateInputChannels(channelCount);
}

//Resize editor
void RippleDetectorEditor::resized()
{
    GenericEditor::resized();

    const int x_pos_initial = 0;
    const int y_pos_inttial = 0;
    const int width = 850;
    const int height = 200;

    //Set the position and bounds of this group of elements inside the plugin UI
    _pluginUi.setBounds(x_pos_initial, y_pos_inttial, width, height);
}
