// main.cpp
#include "./include/NiDAQ.h"         // Include the header file for NiDAQ handler
#include "./include/AudioDAQ.h"      // Include the header file for AudioDAQ handler
#include "./include/CSVWriter.h"     // Include the header file for CSVWriter utility
#include <iostream>
#include <chrono>                    // Include chrono library for timestamp generation
#include <vector>
#include <string>
#include <atomic>
#include <termios.h>                 // Include for terminal input settings
#include <unistd.h>                  // Include for POSIX API (UNIX system calls)
#include <fcntl.h>                   // Include for file control options (e.g., non-blocking mode)
#include <filesystem>                // Include for directory operations
#include "./include/iniReader/INIReader.h" // Include for reading INI configuration files

using namespace std;
namespace fs = filesystem;            // Alias for filesystem namespace

// Global variable to store the original terminal settings
struct termios original_tty; 

/**
 * @brief Set terminal mode to non-blocking and disable echo.
 * 
 * This function modifies the terminal settings to allow real-time input detection
 * without requiring the user to press "Enter". The input is set to non-blocking mode,
 * meaning `read()` will return immediately instead of waiting for input.
 */
void setNonBlockingMode() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);  // Get the current terminal attributes
    original_tty = tty;             // Store the original terminal attributes for restoration later

    tty.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode (line buffering) and echoing of input
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); // Set non-blocking mode
}

/**
 * @brief Restore terminal attributes to their original state.
 * 
 * This function ensures that when the program terminates, the terminal settings 
 * return to normal to avoid unexpected behavior in the shell.
 */
void resetTerminalMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tty); // Restore the original terminal attributes
}

/**
 * @brief Get the current timestamp in "YYYYMMDDHHMMSS" format.
 * 
 * This function generates a timestamp based on the system clock, which is used for 
 * labeling files and directories to ensure unique names.
 * 
 * @return A string representing the current timestamp.
 */
string getCurrentTime() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm localTime;
    localtime_r(&now_time, &localTime); // Convert to local time

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &localTime);
    return string(buffer);
}

int main( void ) {
    atexit(resetTerminalMode); // Ensure terminal mode is restored when the program exits
    
    while (true) {
        system("clear"); // Clear terminal screen for better readability

        // Load configuration file for setting parameters
        const string iniFilePath = "API/Master.ini";
        INIReader reader(iniFilePath);

        if (reader.ParseError() < 0) {
            cerr << "Cannot load INI file: " << iniFilePath << endl;
            return 1;
        }

        // Read the "SaveUnit" setting (time interval in seconds)
        const string targetSection = "SaveUnit";
        const string targetKey = "second";
        int SaveUnit = reader.GetInteger(targetSection, targetKey, 60);
        cout << "[" << targetSection << "] " << targetKey << " = " << SaveUnit << endl;

        // Initialize DAQ devices
        NiDAQHandler niDaq;
        AudioDAQ audioDaq_1;
        AudioDAQ audioDaq_2;

        // Configuration file paths for each device
        const char* NiDAQconfigPath = "API/NiDAQ.ini";
        const char* audioDaq_1configPath = "API/AudioDAQ_1.ini";
        const char* audioDaq_2configPath = "API/AudioDAQ_2.ini";

        // Hardware initialization
        TaskInfo info = niDaq.prepareTask(NiDAQconfigPath);
        audioDaq_1.initDevices(audioDaq_1configPath);
        audioDaq_2.initDevices(audioDaq_2configPath);

        // Check if DAQ initialization was successful
        if (info.sampleRate <= 0 || info.numChannels <= 0) {
            cerr << "NiDAQ Initialization failed." << endl;
            return 1;
        }
        if (audioDaq_1.getSampleRate() <= 0 || audioDaq_2.getSampleRate() <= 0) {
            cerr << "AudioDAQ Initialization failed." << endl;
            return 1;
        }
        cout << "Initialization completed." << endl;

        // Prompt user for label input
        string label;
        cout << "Please enter the label of the data: ";
        cin >> label;
        string folder = getCurrentTime() + "_" + label;

        // Create output directories for data storage
        fs::create_directory("output/NiDAQ/" + folder);
        fs::create_directory("output/AudioDAQ_1/" + folder);
        fs::create_directory("output/AudioDAQ_2/" + folder);

        // Initialize CSVWriter objects for saving data
        CSVWriter NiDAQcsv(info.numChannels, "output/NiDAQ/" + folder, label);
        CSVWriter audioDaq_1csv(1, "output/AudioDAQ_1/" + folder, label);
        CSVWriter audioDaq_2csv(1, "output/AudioDAQ_2/" + folder, label);

        // Start DAQ tasks
        if (niDaq.startTask() != 0) {
            cerr << "Failed to start NiDAQ task." << endl;
            return 1;
        }
        audioDaq_1.startCapture();
        audioDaq_2.startCapture();

        int NiDAQTimer = 0, NiDAQtmpTimer = 0;
        int audioDaq_1Timer = 0, audioDaq_1tmpTimer = 0;
        int audioDaq_2Timer = 0, audioDaq_2tmpTimer = 0;

        bool isRunning = true;
        char ch;
        cout << "Start reading data, press 'Q' or 'q' to terminate the program." << endl;

        setNonBlockingMode(); // Enable non-blocking input mode

        while (isRunning) {
            // Check for user input (non-blocking)
            if (read(STDIN_FILENO, &ch, 1) > 0) {
                if (ch == 'Q' || ch == 'q') {
                    isRunning = false;
                    cout << "Saving final data before exit..." << endl;
                    resetTerminalMode(); // Restore terminal settings before exiting
                    break;
                }
                cout << "You pressed: " << ch << endl;
            }

            // Process NiDAQ data
            int NiDAQtmpTimes = niDaq.getReadTimes();
            if (NiDAQtmpTimes > NiDAQtmpTimer) {
                double* dataBuffer = niDaq.getDataBuffer();
                vector<double> dataBlock(dataBuffer, dataBuffer + info.sampleRate * info.numChannels);
                NiDAQcsv.addDataBlock(move(dataBlock));
                NiDAQtmpTimer = NiDAQtmpTimes;

                NiDAQTimer++;
                cout << "NiDAQ Program Timer: " << NiDAQTimer << endl;
                cout << "NiDAQ Package Timer: " << NiDAQtmpTimer << endl;
                
                if (NiDAQTimer == SaveUnit) {
                    NiDAQcsv.updateFilename();
                    NiDAQTimer = 0;
                    cout << "NiDAQ CSV Saved" << endl;
                }
            }

            // Process AudioDAQ_1 data
            int audioDaq_1tmpTimes = audioDaq_1.getTimes();
            if (audioDaq_1tmpTimes > audioDaq_1tmpTimer) {
                auto buffer = audioDaq_1.getBuffer();
                audioDaq_1csv.addDataBlock(move(buffer));
                audioDaq_1tmpTimer = audioDaq_1tmpTimes;

                audioDaq_1Timer++;
                cout << "Audio 1 Program Timer: " << audioDaq_1Timer << endl;
                cout << "Audio 1 Package Timer: " << audioDaq_1tmpTimer << endl;

                if (audioDaq_1Timer == SaveUnit) {
                    audioDaq_1csv.updateFilename();
                    audioDaq_1Timer = 0;
                    cout << "AudioDAQ_1 CSV Saved" << endl;
                }
            }

            // Process AudioDAQ_2 data
            int audioDaq_2tmpTimes = audioDaq_2.getTimes();
            if (audioDaq_2tmpTimes > audioDaq_2tmpTimer) {
                auto buffer = audioDaq_2.getBuffer();
                audioDaq_2csv.addDataBlock(move(buffer));
                audioDaq_2tmpTimer = audioDaq_2tmpTimes;

                audioDaq_2Timer++;
                cout << "Audio 2 Program Timer: " << audioDaq_2Timer << endl;
                cout << "Audio 2 Package Timer: " << audioDaq_2tmpTimer << endl;

                if (audioDaq_2Timer == SaveUnit) {
                    audioDaq_2csv.updateFilename();
                    audioDaq_2Timer = 0;
                    cout << "AudioDAQ_1 CSV Saved" << endl;
                }
            }
        }

        resetTerminalMode(); // Restore terminal settings

        cout << "Stopping DAQ and saving remaining data..." << endl;
        niDaq.stopAndClearTask();
        audioDaq_1.stopCapture();
        audioDaq_2.stopCapture();
    }

    return 0;
}
