Spectral3: 3-Band Spectral EQ with Real-Time Spectrogram
This repository contains the JUCE-based source code for a VST3 plugin called NewProject. The plugin provides a 3-band parametric EQ and a real-time FFT-based spectrogram. You can tweak each band’s frequency, gain, and Q factor, and see your audio’s frequency content on the fly.

How to Use the Compiled VST3
Download the NewProject.vst3 Folder

After building (or from the release artifacts), you’ll have a folder named NewProject.vst3.

This folder is the VST3 “bundle” and must remain intact.

Copy NewProject.vst3 to Your System’s VST3 Folder

On Windows, the standard location is:

makefile
Copy
Edit
C:\Program Files\Common Files\VST3\
On macOS, place it in:

css
Copy
Edit
/Library/Audio/Plug-Ins/VST3/
(or in ~/Library/Audio/Plug-Ins/VST3/ for a user-only install)

Launch Your DAW and Scan for New Plugins

In FL Studio, open Plugin Manager, click “Find plugins,” then check for NewProject (or “visuals” if you changed the name).

In other DAWs (Ableton, Reaper, Cubase, etc.), do a plugin rescan if it doesn’t appear automatically.

Load and Play

Insert NewProject on a mixer channel.

Adjust the frequency, gain, and Q of each band. Watch the spectrogram visualize your audio in real time!

Source File Overview
The core JUCE plugin logic is split into four files:

PluginProcessor.h / PluginProcessor.cpp

Defines the main AudioProcessor class (SpectralEQAudioProcessor).

Manages DSP (the 3 parametric EQ bands) and the real-time FFT for the spectrogram.

Implements createPluginFilter() so JUCE knows how to instantiate this plugin.

PluginEditor.h / PluginEditor.cpp

Defines the AudioProcessorEditor class (SpectralEQAudioProcessorEditor).

Creates the GUI: 3 sets of frequency/gain/Q sliders and a path that visualizes the FFT spectrogram.

Implements layout, rendering, and the 30FPS timer to repaint the spectrogram.

JUCE / Build Infrastructure
The project was created with JUCE 8.x.

The Builds folder contains exporter projects (e.g., VisualStudio2022) that produce the .vst3 file.

Want to Build from Source?
Clone this Repo

bash
Copy
Edit
git clone https://github.com/yourusername/NewProject.git
Open in Projucer (or CMake) and enable VST3.

Build using Visual Studio, Xcode, or your tool of choice.

The resulting .vst3 will appear under Builds/VisualStudio2022/x64/Release/VST3 (or similar).

Enjoy using this project!
Feel free to fork, modify the DSP, or enhance the UI. Contributions or bug reports are welcome. Have fun shaping your sound with the 3-band parametric EQ and seeing real-time frequency data in the spectrogram!
