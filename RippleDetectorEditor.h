#ifndef __RIPPLE_DETECTOR_EDITOR_H
#define __RIPPLE_DETECTOR_EDITOR_H

#include <EditorHeaders.h> 
#include <sstream>
#include <iomanip>

class RippleDetectorEditor : public GenericEditor, 
                         public Button::Listener, 
                         public ComboBox::Listener, 
                         public Label::Listener
{
public:
    RippleDetectorEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors);
    virtual ~RippleDetectorEditor();

    void updateInputChannels(int channelCount);
	void updateOutputChannels(int channelCount);
    void updateMovChannels(int channelCount);
    void updateMovOutputChannels(int channelCount);

	Label* createLabel(String labelText, Justification just);
    Label* createInputBox(String componentName, String tooltipText);
    ScopedPointer<ComboBox> createComboBox(String tooltipText);

    void buttonClicked(Button *pInButtonClicked) override;
    void comboBoxChanged(ComboBox *pInComboBoxChanged) override;
    void labelTextChanged(Label* pLabel) override;
    void setInputBoxStatus(Label* pInputBox, bool enabled);
    double adjustAndGetValueFromText(const String& text);
    void updateSettings() override;
    void setComponentsPositions();

    int _noMovChannelId = 999;
    int _accelChannelId = 1111;

	// Interface objects
    ScopedPointer<ComboBox> _comboInChannelSelection;
    ScopedPointer<ComboBox> _comboOutChannelSelection;
    ScopedPointer<ComboBox> _comboMovChannelSelection;
    ScopedPointer<ComboBox> _comboMovOutChannelSelection;
    Label* _inputBoxSds;
    Label* _inputBoxTimeThreshold;
    Label* _inputBoxRefractory;
    Label* _inputBoxRmsSamples;
    Label* _inputBoxMovSds;
    Label* _inputBoxMinTimeWoMov;
    Label* _inputBoxMinTimeWMov;
    UtilityButton* _buttonCalibrate;

    // Labels
    Label *_labelInputChannel;
    Label *_labelOutputChannel;
    Label* _labelMovChannel;
    Label* _labelMovOutputChannel;
    Label * _labelSds;
	Label *_labelTimeThreshold;
    Label *_labelRefractoryTime;
    Label *_labelRmsSamples;
    Label* _labelMovSds;
    Label* _labelMinTimeWoMov;
    Label* _labelMinTimeWMov;

    // Facade
    int _inChannel;
    int _outChannel;
    int _movChannel;
    int _movOutChannel;
    std::vector<int> _accChannels;

    double _rippleSds;
	int _timeThreshold;
    int _refractoryTime;
    int _rmsSamples;
    double _movSds;
    int _minTimeWoMov;
    int _minTimeWMov;

    bool _calibrate;
    bool _movChannChanged{ false };
    bool _movSwitchEnabled{ false };
    bool _accelerometerSelected{ false };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetectorEditor);
};

static int getNextY(Component *rInReference);
static int getNextX(Component *rInReference);
static double setDecimalPlaces(double value, int places);

#endif