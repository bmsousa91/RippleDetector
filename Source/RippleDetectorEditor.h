// v060422
#ifndef __RIPPLE_DETECTOR_EDITOR_H
#define __RIPPLE_DETECTOR_EDITOR_H

#include <EditorHeaders.h> 
#include <sstream>
#include <iomanip>

#include "RippleDetector.h"

class RippleDetectorEditor : public GenericEditor,
                        public Button::Listener,
                        public ComboBox::Listener, 
                        public Label::Listener
{
public:
    RippleDetectorEditor(GenericProcessor* parentNode);
    virtual ~RippleDetectorEditor();

    void updateInputChannels(int channelCount);
	void updateOutputChannels(int channelCount);
    void updateMovChannels(int channelCount);
    void updateMovOutputChannels(int channelCount);

	Label* createLabel(String labelText, Justification just);
    Label* createInputBox(String componentName, String tooltipText);
    ScopedPointer<ComboBox> createComboBox(String tooltipText);

    void buttonClicked(Button *pInButtonClicked);
    void comboBoxChanged(ComboBox *pInComboBoxChanged) override;
    void labelTextChanged(Label* pLabel) override;
    void setInputBoxStatus(Label* pInputBox, bool enabled);
    double adjustAndGetValueFromText(const String& text);
    void updateSettings() override;
    void setComponentsPositions();

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
    
    RippleDetector* rippleDetector;

    std::unique_ptr<UtilityButton> calibrateButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetectorEditor);
};

static int getNextY(Component *rInReference);
static int getNextX(Component *rInReference);
static double setDecimalPlaces(double value, int places);

#endif
