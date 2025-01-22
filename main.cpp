// main.cpp
#include "NiDAQ.h"
#include "AudioDAQ.h"
#include "CSVWriter.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <atomic>

using namespace std;

int main ( void ) {
    // Initialize NiDAQHandler and AudioDAQ instances for data acquisition
    NiDAQHandler handler;
    AudioDAQ audioDaq;

    // Path to configuration files for hardware initialization
    const char* NiDAQconfigPath = "API/NiDAQ.ini";
    const char* AudioDAQconfigPath = "API/AudioDAQ.ini";
    

    // Initialize DAQ task using configuration file
    TaskInfo info = handler.prepareTask(NiDAQconfigPath);
    audioDaq.initDevices(AudioDAQconfigPath);

    // Validate initialization, ensure sample rate and channel count are valid
    if (info.sampleRate <= 0 || info.numChannels <= 0) {
        cerr << "Preparation failed, unable to initialize hardware." << endl;
        return 1; // Exit on initialization error
    }

    // Initialize CSVWriter for logging data from NiDAQ and AudioDAQ
    CSVWriter NiDAQcsv(info.numChannels, "output/NiDAQ/");
    NiDAQcsv.start();

    CSVWriter AudioDAQcsv(1, "output/AudioDAQ/");
    AudioDAQcsv.start();

    // Start NiDAQ task and validate success
    if (handler.startTask() != 0) {
        cerr << "Failed to start NiDAQ task." << endl;
        return 1; // Exit if task start fails
    }

    // Start audio data capture
    audioDaq.startCapture();

    cout << "Start reading data, press Ctrl+C to terminate the program." << endl;

    // Variables to track loop iterations and data updates
    int NiDAQTimer = 0;
    int NiDAQtmpTimer = 0; // Tracks last data read times for NiDAQ

    int AudioDAQTimer = 0;
    int AudioDAQtmpTimer = 0; // Tracks last data read times for AudioDAQ

    while (true) {
        // Check if new data is available from NiDAQ
        int NiDAQtmpTimes = handler.getReadTimes();
        if (NiDAQtmpTimes > NiDAQtmpTimer) {
            // Retrieve the latest data buffer from NiDAQ
            double* dataBuffer = handler.getDataBuffer();

            NiDAQTimer++;
            cout << "NiDAQ Program Count: " << NiDAQTimer << endl;
            cout << "NiDAQ Package Count: " << NiDAQtmpTimes << endl;

            // Store data in a vector for CSV logging
            vector<double> dataBlock(dataBuffer, dataBuffer + info.sampleRate * info.numChannels);
            NiDAQcsv.addDataBlock(move(dataBlock));

            // Update read times
            NiDAQtmpTimer = NiDAQtmpTimes;
        }

        // Check if new data is available from AudioDAQ
        int AudioDAQtmpTimes = audioDaq.getTimes();
        if (AudioDAQtmpTimes > AudioDAQtmpTimer) {
            AudioDAQTimer++;

            cout << "AudioDAQ Program Count: " << AudioDAQTimer << endl;
            cout << "AudioDAQ Package Count: " << AudioDAQtmpTimes << endl;

            // Retrieve the latest audio buffer and log to CSV
            auto buffer = audioDaq.getBuffer();
            AudioDAQcsv.addDataBlock(move(buffer));

            // Update read times
            AudioDAQtmpTimer = AudioDAQtmpTimes;
        }
    }

    // Stop and clean up all tasks and resources
    handler.stopAndClearTask();
    NiDAQcsv.stop();

    audioDaq.stopCapture();
    AudioDAQcsv.stop();

    return 0;
}
