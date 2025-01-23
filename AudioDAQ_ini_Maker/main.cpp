// main.cpp
#include "AudioDAQ.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

#define MAX 10 // Maximum number of audio devices

using namespace std;

// Class to handle writing data to an INI file
class INIWriter {
public:
    // Constructor: Initialize the INIWriter with a file path
    explicit INIWriter(const string& filename) : filename(filename) {}

    // Set a value in a specific section with a given key and value
    void setValue(const string& section, const string& key, const string& value) {
        data[section][key] = value;
    }

    // Save the INI data to a file
    void save() {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << filename << " for writing." << endl;
            return;
        }

        // Write each section and its key-value pairs to the file
        for (const auto& section : data) {
            file << "[" << section.first << "]\n";
            for (const auto& keyValue : section.second) {
                file << keyValue.first << "=" << keyValue.second << "\n";
            }
            file << "\n"; // Add a blank line between sections
        }

        file.close();
    }

private:
    string filename; // Path to the INI file
    map<string, map<string, string>> data; // Store sections and key-value pairs
};

int main( void ) {
    AudioDAQ audioDaq;

    // List all available audio devices
    audioDaq.outputDevices();

    // Prompt the user to select an audio device
    int choice[MAX];
    char input;
    int deviceCount = 0;

    cout << endl;
    while (true) {
        cout << "Device " << to_string(deviceCount) << endl;
        cout << "Please select an audio device and enter the number (Enter \"q\" to exit the selection.): ";
        cin >> input;

        if (input == 'q') {
            break;
        }

        choice[deviceCount] = input - '0';
        deviceCount++;
    }

    // Prompt the user to enter the sample rate
    unsigned int sampleRate;
    cout << "Please enter the sample rate (e.g., 44100): ";
    cin >> sampleRate;

    // Save the selected device and sample rate to an INI file
    for (int i = 0 ;i < deviceCount ;i++) {
        INIWriter writer("../API/AudioDAQ_" + to_string(i + 1) + ".ini");
        writer.setValue("AudioDAQ", "device", to_string(choice[i]));
        writer.setValue("AudioDAQ", "sampleRate", to_string(sampleRate));
        writer.save();
    }

    cout << "The ini file has been successfully updated!" << endl;

    return 0;
}
