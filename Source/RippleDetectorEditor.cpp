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

    setBounds(0, 0, labelWidth, 18);
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
    else 
	{
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
	addComboBoxParameterEditor("Mov_Out", 230, 85);

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

}

void RippleDetectorEditor::buttonClicked(Button*)
{
	/* Calibration button was clicked */
	rippleDetector->shouldCalibrate = true;
}

// Called when settings are updated
void RippleDetectorEditor::updateSettings() {}