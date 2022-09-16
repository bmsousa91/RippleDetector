// v060422
#ifndef __RIPPLE_DETECTOR_EDITOR_H
#define __RIPPLE_DETECTOR_EDITOR_H

#include <EditorHeaders.h> 
#include <sstream>
#include <iomanip>

#include "RippleDetector.h"


class CustomTextBoxParameterEditor : public ParameterEditor,
    public Label::Listener
{
public:

    /** Constructor */
    CustomTextBoxParameterEditor(Parameter* param);

    /** Destructor */
    virtual ~CustomTextBoxParameterEditor() { }

    /** Called when the text box contents are changed*/
    void labelTextChanged(Label* label) override;

    /** Must ensure that editor state matches underlying parameter */
    virtual void updateView() override;

    /** Sets sub-component locations */
    virtual void resized() override;

private:
    std::unique_ptr<Label> parameterNameLabel;
    std::unique_ptr<Label> valueTextBox;

    int finalWidth;
};

class RippleDetectorEditor : public GenericEditor,
                        public Button::Listener
{
public:
    RippleDetectorEditor(GenericProcessor* parentNode);
    virtual ~RippleDetectorEditor() {}

    void buttonClicked(Button*);
    void updateSettings() override;

private:
    
    RippleDetector* rippleDetector;

    std::unique_ptr<UtilityButton> calibrateButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetectorEditor);
};

#endif
