# Ripple Detector plugin for the Open Ephys GUI
Open Ephys GUI plugin for ripple detection. It contains an embedded mechanism based on EMG or accelerometer data that blocks ripple events when movement is detected. **This code is compatible with the version v0.5.5 of the Open Ephys GUI. For newer versions of the GUI, you can download the Ripple Detector directly via the Plugin Installer utility.**

## Installation
First, you will have to download the Open Ephys (OE) GUI source code and compile it (https://open-ephys.github.io/gui-docs/Developer-Guide/Compiling-the-GUI.html). The second step is to create the build files for the plugin and compile it. Again, the official OE website provides all the necessary instructions (https://open-ephys.github.io/gui-docs/Developer-Guide/Compiling-plugins.html).

## Usage
- Use this plugin after a Bandpass Filter module, filtering the signal in the ripple frequency band;
- Use this plugin before a sink module (LFP Viewer, Recording Node, or Pulse Pal, for instance).
- Note: the parameters for the best ripple detection performance may vary according to the recording setup, noise level, electrode type, implant quality, and even between animals. We recommend that you perform a first recording session to find the optimal parameters using the File Reader source node with the recorded data.

Below is a screenshot of the module being used with real data (hc-18 dataset available at CRCNS.org – Drieu, Todorova and Zugaro, 2018a; Drieu, Todorova and Zugaro, 2018b). We also created synthetic ripple data to perform preliminary tests of the plugin (you will find these data in the folder "Simulation Data").

![Simulated ripple detection example](Figures/rippleDetectorExample.png)

## Ripple Detector GUI

![Image of RippleDetector](Figures/rippleDetector.png)

- Input: input channel from which ripple data will be processed;
- Output: output TTL channel where detection events are marked;
- SD: number of RMS standard deviations above the mean to calculate the amplitude threshold;
- Time Thr. (ms): time threshold - the minimum period during which the RMS values must be above the amplitude threshold for ripples to be detected; 
- Refr. Time (ms): refractory time - period after each detection event in which new ripples are not detected;
- RMS Samp.: RMS samples - number of samples to calculate the RMS value;

- EMG/ACC: electromyogram or accelerometer data input channel for movement monitoring. If "-" is selected, the mechanism of event blockage based on movement detection is disabled and ripples are not silenced. When the auxiliary channels are enabled (via the Rhythm FPGA module), the "Accel." option appears in the list of available inputs as a virtual channel. If selected, the RMS values are calculated over the magnitude of the acceleration vector. Any other option selected is treated as it was an EMG channel (see the section "Ripple detection algorithm");
- Mov. Out: movement output - output TTL channel that indicates the period when ripple detection is silenced by movement (0: events not blocked, 1: events blocked);
- EMG/ACC SD: number of RMS standard deviations above the mean to calculate the amplitude threshold for movement detection based on EMG or accelerometer;
- Min Time St (ms): minimum time steady - minimum period of immobility (RMS below the amplitude threshold) required to enable ripple detection again after movement is detected;
- Min Time Mov (ms): minimum time of movement - the minimum period during which the RMS values of EMG/accelerometer must be above the corresponding amplitude threshold for movement detection (and ripple silencing);

- Button "Calibrate": recalculates the calibration parameters (RMS mean and standard deviation for both the ripple and movement data);

## Ripple detection algorithm
The ripple detection algorithm works in two steps:
- Calibration: this is the initial stage when the threshold for ripple detection is calculated. The calibration period corresponds to the first 20 seconds of recording. During this period, the incoming filtered data in the ripple frequency band is divided into blocks of N samples, where N is the "RMS Samp." value set in the plugin's GUI. The RMS value is calculated for each block, so we have a total of (20 * sampleRate / N) RMS points. The RMS mean (RMS_mean) and standard deviation (RMS_standardDeviation) for the whole calibration period are calculated and we have the final amplitude threshold (amplitudeThreshold) specified in terms of standard deviations above the mean:

      amplitudeThreshold = RMS_mean + "SD" * RMS_standardDeviation

   where "SD" is the number of standard deviations above the mean. This parameter can be set in the plugin's GUI.

- Detection: this is when ripples are identified online. The RMS value of each block is calculated and tested against the amplitude threshold. If this value is kept above the amplitude threshold for the minimum time period defined by the parameter "Time Thr.", ripple events are generated. After an event is raised, the detection of new ripple events is disabled for "Refr. Time" milliseconds. This parameter can be also adjusted in the plugin's GUI. If the user selects one option in the "EMG/ACC" comboBox, ripple events are not raised when movement is detected.

Summary of the algorithm:

![Ripple detection algorithm](Figures/rippleDetectionAlgorithm.png)

## Citation and more info

If you want to cite the ripple detector or know more about it, please refer to the paper below:

https://iopscience.iop.org/article/10.1088/1741-2552/ac857b

## References

Drieu, C., Todorova, R., & Zugaro, M. (2018a). Nested sequences of hippocampal assemblies during behavior support subsequent sleep replay. Science (New York, N.Y.), 362(6415), 675–679. https://doi.org/10.1126/science.aat2952

Drieu, C., Todorova, R., & Zugaro, M. (2018b). Bilateral recordings from dorsal hippocampal area CA1 from rats transported on a model train and sleeping. CRCNS.org. http://dx.doi.org/10.6080/K0Z899MM.

