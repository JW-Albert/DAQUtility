#ifndef AUDIO_DAQ_H
#define AUDIO_DAQ_H

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <cstring>
#include <alsa/asoundlib.h>

// Include INIReader for configuration parsing
#include "./iniReader/INIReader.h"
extern "C" {
#include "./iniReader/ini.h"
}

using namespace std;

class AudioDAQ {
public:
    // Constructor: Initialize the AudioDAQ object
    AudioDAQ();

    // Destructor: Clean up resources used by AudioDAQ
    ~AudioDAQ();

    // Initialize audio devices using settings from an INI file
    void initDevices(const char* filename);

    // List available audio devices
    void listDevices();

    // Output the list of available audio devices to the console
    void outputDevices();

    // Select a specific audio device by index
    void selectDevice(int choice);

    // Configure the selected audio device with a specific sample rate
    void configureDevice(unsigned int sampleRate);

    // Start capturing audio data
    void startCapture();

    // Stop capturing audio data
    void stopCapture();

    // Retrieve the buffer containing the captured audio data
    vector<double> getBuffer() const;

    // Get the total number of data blocks captured
    int getTimes() const;

    // Get the current sample rate of the audio device
    unsigned int getSampleRate() const;

private:
    // Structure representing an audio device
    struct AudioDevice {
        int card;                // Card index of the audio device
        std::string name;        // Human-readable name of the device
    };

    snd_pcm_t* pcmHandle;                      // PCM handle for ALSA audio device
    snd_pcm_hw_params_t* hwParams;             // Hardware parameters for ALSA
    int choice;                                // Index of the selected device
    vector<AudioDevice> devices;          // List of detected audio devices
    string selectedDevice;                // Identifier for the selected device
    vector<double> buffer;                // Buffer for storing captured audio data
    unsigned int sampleRate;                   // Sampling rate in Hz
    int times;                                 // Number of captured data blocks
    atomic<bool> capturing;               // Flag indicating if capturing is active
    thread captureThread;                 // Thread for capturing audio data

    // Internal method for the capture loop
    void captureLoop();
};

#endif // AUDIO_DAQ_H
