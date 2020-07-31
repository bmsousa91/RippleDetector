#ifndef __RIPPLE_DETECTOR_EDITOR_H
#define __RIPPLE_DETECTOR_EDITOR_H

#include <EditorHeaders.h> 
#include <sstream>

class RippleDetectorUi : public Component, public ButtonListener, public SliderListener, public ComboBoxListener
{
public:
	RippleDetectorUi();
    ~RippleDetectorUi();
    void updateInputChannels(int channelCount);
	void updateOutputChannels(int channelCount);
	Slider* createSlider(String sliderName, std::vector<double> range, double initValue, bool textBoxEditable);
	Label* createLabel(String labelText);
    void updateSettings();
    void resized() override;
    void paint(Graphics &rInGraphics) override;
    void buttonClicked(Button *pInButtonClicked) override;
    void sliderValueChanged(Slider *pInSliderChanged) override;
    void comboBoxChanged(ComboBox *pInComboBoxChanged) override;

	//Interface objects
    LookAndFeel_V2 *_defaultLookAndFeel;
    ComboBox *_comboChannelSelection;
    ComboBox *_comboOutChannelSelection;
    Slider *_sliderThresholdSds;
	Slider *_sliderThresholdTime;
    Slider *_sliderRefractoryTime;
    Slider *_sliderRmsSamples;
    TextButton *_buttonCalibrate;

    //Labels
    Label *_labelInputChannel;
    Label *_labelOutChannel;
    Label *_labelThresholdSds;
	Label *_labelThresholdTime;
    Label *_labelRefractoryTime;
    Label *_labelRmsSamples;

    //Facade
    int _channelCount;
    int _inChannel;
    int _outChannel;
    bool _calibrate;
    double _thresholdSds;
	double _thresholdTime;
    int _refractoryTime;
    int _rmsSamples;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetectorUi);
};

class RippleDetectorEditor : public GenericEditor
{
public:
    RippleDetectorEditor();
    virtual ~RippleDetectorEditor();
    RippleDetectorEditor(GenericProcessor *rInParentNode, bool useDefaultParameterEditors);
    void updateSettings() override;
    void resized() override;
    RippleDetectorUi _pluginUi;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetectorEditor);
};

static int getNextY(Component *rInReference);
static int getNextX(Component *rInReference);

#endif