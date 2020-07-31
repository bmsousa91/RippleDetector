# Ripple Detector - Open Ephys Plugin
Open Ephys plugin to detect hippocampal ripple events.

## Installation
Create a new plugin for Open Ephys using the RippleDetector codes. In short words, you will first have to download the Open Ephys GUI source code and compile it (https://open-ephys.atlassian.net/wiki/spaces/OEW/pages/491544/Installation). Then create the build files for the plugin and compile it. You will find detailed instruction in the Open Ephys wiki website (https://open-ephys.atlassian.net/wiki/spaces/OEW/pages/950297/Tutorial+Add+a+custom+processor).

## Ripple detection algorithm
The ripple detection algorithm works in two steps:
- Calibration: this is the initial stage when threshold for ripple detection is calculated. First, the RMS value is calculated for 1000 consecutive buffer blocks, each block containing a number of "RMS Samples" samples. The final mean and standard deviation (standardDeviation) are obtained and we have the final detection threshold (detectionThreshold) as:

      detectionThreshold = mean + "SDs" * standardDeviation


- Detection: this is when ripples are being identified online. The RMS value of each buffer subblock is calculated and tested against the amplitude threshold. If this value is kept above the amplitude threshold for the time window defined in the parameter "Time Threshold", ripple events are generated. After the event generation, the refractory time starts and new ripple events are detected by the plugin only after this time window ends. Each ripple event is marked on the LFP Viewer as a vertical line.

## Usage
- Use this plugin after a Bandpass Filter block filtered in the ripple band;
- Use this plugin before a sink block (LFP Viewer or Pulse Pal blocks, for instance).

## Plugin interface and parameters set by the user
- Input channel: the number of the input channel from which data will be processed;
- Output channel: the number of the output channel where detection events are marked;
- SDs: number of standard deviations above the average to be the amplitude threshold;
- Time Threshold (ms): the RMS value must be above the amplitude threshold for a minimum of X milliseconds to detect a ripple. X is the value this parameter represents; 
- Refractory Time (ms): represents a time window just after ripple detection in which new ripples are unable to be detected;
- RMS Samples: number of samples inside each buffer subblock to calculate the RMS value.

- Button "Recalibrate": starts the calibration process again, calculating a new amplitude threshold;



