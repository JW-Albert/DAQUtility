// main.cpp
#include "AudioDAQ.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

// Class to handle writing data to an INI file
class INIWriter {
public:
    // Constructor: Initialize the INIWriter with a file path
    explicit INIWriter(const std::string& filename) : filename(filename) {}

    // Set a value in a specific section with a given key and value
    void setValue(const std::string& section, const std::string& key, const std::string& value) {
        data[section][key] = value;
    }

    // Save the INI data to a file
    void save() {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << " for writing." << std::endl;
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
    std::string filename; // Path to the INI file
    std::map<std::string, std::map<std::string, std::string>> data; // Store sections and key-value pairs
};

int main( void ) {
    AudioDAQ audioDaq;

    // List all available audio devices
    audioDaq.outputDevices();

    // Prompt the user to select an audio device
    int choice;
    std::cout << "\nPlease select an audio device (enter the number and press Enter): ";
    std::cin >> choice;

    // Prompt the user to enter the sample rate
    unsigned int sampleRate;
    std::cout << "Please enter the sample rate (e.g., 44100): ";
    std::cin >> sampleRate;

    // Save the selected device and sample rate to an INI file
    INIWriter writer("../API/AudioDAQ.ini");
    writer.setValue("AudioDAQ", "device", std::to_string(choice));
    writer.setValue("AudioDAQ", "sampleRate", std::to_string(sampleRate));

    writer.save();
    std::cout << "The ini file has been successfully updated!" << std::endl;

    return 0;
}
