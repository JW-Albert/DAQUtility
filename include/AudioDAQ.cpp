// AudioDAQ.cpp
#include "AudioDAQ.h"

// Callback function for handling parsed INI file data
static int handler(void* user, const char* section, const char* name, const char* value) {
    auto* ini_data = reinterpret_cast<std::map<std::string, std::map<std::string, std::string>>*>(user);
    (*ini_data)[section][name] = value;
    return 1;
}

// Filter sections of the INI data containing a specific keyword
std::vector<std::string> AudioDAQfilterSections(
    const std::map<std::string, std::map<std::string, std::string>>& ini_data,
    const std::string& section_keyword) {
    std::vector<std::string> result;
    for (const auto& section : ini_data) {
        if (section.first.find(section_keyword) != std::string::npos) {
            result.push_back(section.first);
        }
    }
    return result;
}

// Constructor: Initialize member variables and allocate ALSA hardware parameters
AudioDAQ::AudioDAQ()
    : pcmHandle(nullptr),                     // PCM handle for ALSA device
      hwParams(nullptr),                      // Hardware parameters for ALSA
      choice(0),                              // Default device choice
      devices(),                              // List of audio devices
      selectedDevice(""),                     // Selected device name
      buffer(),                               // Buffer to store audio data
      sampleRate(0),                          // Sampling rate in Hz
      times(0),                               // Number of captured data chunks
      capturing(false),                       // Capture state flag
      captureThread() {                       // Thread for capturing audio data
    snd_pcm_hw_params_alloca(&hwParams);
}

// Destructor: Stop capture and clean up ALSA resources
AudioDAQ::~AudioDAQ() {
    stopCapture();
    if (pcmHandle) {
        snd_pcm_close(pcmHandle);
    }
}

// List available ALSA audio devices
void AudioDAQ::listDevices() {
    snd_ctl_t* handle;
    snd_ctl_card_info_t* info;
    snd_ctl_card_info_alloca(&info);

    for (int card = 0;; card++) {
        char cardName[32];
        snprintf(cardName, sizeof(cardName), "hw:%d", card);

        if (snd_ctl_open(&handle, cardName, 0) < 0) {
            break;
        }

        if (snd_ctl_card_info(handle, info) >= 0) {
            devices.push_back({card, snd_ctl_card_info_get_name(info)});
        }

        snd_ctl_close(handle);
    }
}

// Output the list of available audio devices
void AudioDAQ::outputDevices() {
    listDevices();
    if (devices.empty()) {
        throw std::runtime_error("No audio devices detected.");
    }

    std::cout << "Available audio devices:" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << i << ": " << devices[i].name << std::endl;
    }
}

// Select an audio device by index
void AudioDAQ::selectDevice(int choice) {
    listDevices();
    if (choice < 0 || choice >= static_cast<int>(devices.size())) {
        throw std::invalid_argument("Invalid device selection.");
    }

    selectedDevice = "hw:" + std::to_string(devices[choice].card);
    std::cout << "Selected device: " << devices[choice].name << std::endl;
}

// Initialize devices with settings from an INI configuration file
void AudioDAQ::initDevices(const char* filename) {
    std::map<std::string, std::map<std::string, std::string>> ini_data;

    if (ini_parse(filename, handler, &ini_data) < 0) {
        std::cerr << "Error: Unable to load the specified INI file: " << filename << std::endl;
        return;
    }

    auto filtered_sections = AudioDAQfilterSections(ini_data, "AudioDAQ");

    try {
        for (const auto& section : filtered_sections) {
            choice = std::stoi(ini_data[section]["device"]);
            sampleRate = std::stoi(ini_data[section]["sampleRate"]);
            std::cout << "Loaded config from section: " << section << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing INI file: " << e.what() << std::endl;
    }

    selectDevice(choice);
    configureDevice(sampleRate);
}

// Configure the audio device with a specified sample rate
void AudioDAQ::configureDevice(unsigned int sampleRate) {
    this->sampleRate = sampleRate;

    if (snd_pcm_open(&pcmHandle, selectedDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0) < 0) {
        throw std::runtime_error("Unable to open audio device: " + selectedDevice);
    }

    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_hw_params_any(pcmHandle, hwParams);
    snd_pcm_hw_params_set_access(pcmHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcmHandle, hwParams, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcmHandle, hwParams, 1);
    snd_pcm_hw_params_set_rate_near(pcmHandle, hwParams, &this->sampleRate, nullptr);
    snd_pcm_hw_params(pcmHandle, hwParams);

    std::cout << "Device configured with sample rate: " << this->sampleRate << std::endl;
}

// Start capturing audio data
void AudioDAQ::startCapture() {
    if (capturing) {
        throw std::runtime_error("Capture is already running.");
    }

    capturing = true;
    captureThread = std::thread(&AudioDAQ::captureLoop, this);
    std::cout << "Capture started." << std::endl;
}

// Stop capturing audio data
void AudioDAQ::stopCapture() {
    if (capturing) {
        capturing = false;
        if (captureThread.joinable()) {
            captureThread.join();
        }
        std::cout << "Capture stopped." << std::endl;
    }
}

// Main loop for capturing audio data
void AudioDAQ::captureLoop() {
    const int bufferSize = sampleRate;
    short tempBuffer[bufferSize];

    while (capturing) {
        int err = snd_pcm_readi(pcmHandle, tempBuffer, bufferSize);
        if (err == -EPIPE) {
            std::cerr << "Capture overrun! Audio data lost." << std::endl;
            snd_pcm_prepare(pcmHandle);
        } else if (err < 0) {
            std::cerr << "Read error: " << snd_strerror(err) << std::endl;
        } else {
            buffer.clear();
            for (int i = 0; i < err; ++i) {
                buffer.push_back(static_cast<double>(tempBuffer[i]));
            }
            times++;
        }
    }
}

// Retrieve the buffer containing the captured audio data
std::vector<double> AudioDAQ::getBuffer() const {
    return buffer;
}

// Get the total number of data blocks captured
int AudioDAQ::getTimes() const {
    return times;
}

// Get the current sample rate of the device
unsigned int AudioDAQ::getSampleRate() const {
    return sampleRate;
}
