// main.cpp
#include "./include/NiDAQ.h"
#include "./include/AudioDAQ.h"
#include "./include/CSVWriter.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include "./include/iniReader/INIReader.h"
extern "C" {
#include "./include/iniReader/ini.h"
}

using namespace std;
namespace fs = filesystem;

// 設置非阻塞模式和禁用回顯
void setNonBlockingMode() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~(ICANON | ECHO); // 禁用行緩衝 & 禁用回顯
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); // 設置非阻塞模式
}

// 還原終端屬性
void resetTerminalMode() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= (ICANON | ECHO); // 啟用行緩衝 & 啟用回顯
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

string getCurrentTime() {
    auto now = chrono::system_clock::now();   // 取得現在時間
    time_t now_time = chrono::system_clock::to_time_t(now); // 轉為 time_t

    struct tm localTime;
    localtime_r(&now_time, &localTime);  // 取得當地時間（Linux）

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &localTime);

    return string(buffer);  // 回傳格式化後的時間字串
}

int main ( void ) {
    const string iniFilePath = "API/Master.ini";
    INIReader reader(iniFilePath);

    if (reader.ParseError() < 0) {
        cerr << "Cannot load INI file: " << iniFilePath << endl;
        return 1;
    }

    // A map used to store INI data.
    map<string, map<string, string>> ini_data;

    vector<string> sections = {"SaveUnit"};
    for (const string& section : sections) {
        if (reader.HasSection(section)) {
            vector<string> keys = {"second", "first"};
            for (const string& key : keys) {
                if (reader.HasValue(section, key)) {
                    ini_data[section][key] = reader.Get(section, key, "");
                }
            }
        }
    }

    // Target section and key.
    const string targetSection = "SaveUnit";
    const string targetKey = "second";
    int SaveUnit = 0;

     if (ini_data.find(targetSection) != ini_data.end() &&
        ini_data[targetSection].find(targetKey) != ini_data[targetSection].end()) {
        string value = ini_data[targetSection][targetKey];
        SaveUnit = stoi(value);
        cout << "[" << targetSection << "] " << targetKey << " = " << value << endl;
    } else {
        cerr << "Cannot find section: " << targetSection << " or key: " << targetKey << endl;
        return 1;
    }

    // Initialize NiDAQHandler and AudioDAQ instances for data acquisition
    NiDAQHandler niDaq;
    AudioDAQ audioDaq_1;
    AudioDAQ audioDaq_2;

    // Path to configuration files for hardware initialization
    const char* NiDAQconfigPath = "API/NiDAQ.ini";
    const char* audioDaq_1configPath = "API/AudioDAQ_1.ini";
    const char* audioDaq_2configPath = "API/AudioDAQ_2.ini";

    // Initialize DAQ task using configuration file
    TaskInfo info = niDaq.prepareTask(NiDAQconfigPath);
    audioDaq_1.initDevices(audioDaq_1configPath);
    audioDaq_2.initDevices(audioDaq_2configPath);

    // Validate initialization, ensure sample rate and channel count are valid
    if (info.sampleRate <= 0 || info.numChannels <= 0) {
        cerr << "NiDAQ Preparation failed, unable to initialize hardware." << endl;
        return 1; // Exit on initialization error
    }

    if (audioDaq_1.getSampleRate() <= 0) {
        cerr << "AudioDAQ_1 Initialization failed, unable to initialize hardware." << endl;
        return 1; // Exit on initialization error
    }

    if (audioDaq_2.getSampleRate() <= 0) {
        cerr << "AudioDAQ_2 Initialization failed, unable to initialize hardware." << endl;
        return 1; // Exit on initialization error
    }

readDataLoop:

    string lable;
    cout << "Please enter the label of the data: ";
    cin >> lable;
    string folder = getCurrentTime() + "_" + lable;

    // Initialize CSVWriter for logging data from NiDAQ and AudioDAQ
    fs::create_directory("output/NiDAQ/" + folder);
    CSVWriter NiDAQcsv(info.numChannels, "output/NiDAQ/" + folder , lable);
    fs::create_directory("output/AudioDAQ_1/" + folder);
    CSVWriter audioDaq_1csv(1, "output/AudioDAQ_1/" + folder , lable);
    fs::create_directory("output/AudioDAQ_2/" + folder);
    CSVWriter audioDaq_2csv(1, "output/AudioDAQ_2/" + folder , lable);

    // Start NiDAQ task and validate success
    if (niDaq.startTask() != 0) {
        cerr << "Failed to start NiDAQ task." << endl;
        return 1; // Exit if task start fails
    }

    // Start audio data capture
    audioDaq_1.startCapture();
    audioDaq_2.startCapture();

    // Variables to track loop iterations and data updates
    int NiDAQTimer = 0;
    int NiDAQtmpTimer = 0; // Tracks last data read times for NiDAQ
    int NiDAQsaveCounter = 0; // Counter for saving data to CSV

    int audioDaq_1Timer = 0;
    int audioDaq_1tmpTimer = 0; // Tracks last data read times for AudioDAQ
    int audioDaq_1saveCounter = 0; // Counter for saving data to CSV

    int audioDaq_2Timer = 0;
    int audioDaq_2tmpTimer = 0; // Tracks last data read times for AudioDAQ
    int audioDaq_2saveCounter = 0; // Counter for saving data to CSV

    // 啟動 CSVWriter 的背景執行緒
    NiDAQcsv.start();
    audioDaq_1csv.start();
    audioDaq_2csv.start();

    bool isRunning = true;
    char ch;
    cout << "Start reading data, press 'Q' or 'q' to terminate the program." << endl;

    while (isRunning) {
        setNonBlockingMode(); // 設置非阻塞模式
        if (read(STDIN_FILENO, &ch, 1) > 0) { // 讀取一個字元
            if (ch == 'Q' || ch == 'q') {
                isRunning = false;
                break; // 按下 'Q' 或 'q' 時退出迴圈
            }
            cout << "You pressed: " << ch << endl;
        }

        // Check if new data is available from NiDAQ
        int NiDAQtmpTimes = niDaq.getReadTimes();
        if (NiDAQtmpTimes > NiDAQtmpTimer) {
            // Retrieve the latest data buffer from NiDAQ
            double* dataBuffer = niDaq.getDataBuffer();

            NiDAQTimer++;
            cout << "NiDAQ Program Count: " << NiDAQTimer << endl;
            cout << "NiDAQ Package Count: " << NiDAQtmpTimes << endl;

            // Store data in a vector for CSV logging
            vector<double> dataBlock(dataBuffer, dataBuffer + info.sampleRate * info.numChannels);
            NiDAQcsv.addDataBlock(move(dataBlock));

            // Update read times
            NiDAQtmpTimer = NiDAQtmpTimes;
            NiDAQsaveCounter++;

            if(NiDAQsaveCounter == SaveUnit) {
                // Save data to CSV file
                NiDAQcsv.saveToCSV();
                NiDAQsaveCounter = 0;
            }
        }

        // Check if new data is available from AudioDAQ_1
        int audioDaq_1tmpTimes = audioDaq_1.getTimes();
        if (audioDaq_1tmpTimes > audioDaq_1tmpTimer) {
            audioDaq_1Timer++;

            cout << "audioDaq_1 Program Count: " << audioDaq_1Timer << endl;
            cout << "audioDaq_1 Package Count: " << audioDaq_1tmpTimes << endl;

            // Retrieve the latest audio buffer and log to CSV
            auto buffer = audioDaq_1.getBuffer();
            audioDaq_1csv.addDataBlock(move(buffer));

            // Update read times
            audioDaq_1tmpTimer = audioDaq_1tmpTimes;
            audioDaq_1saveCounter++;

            if (audioDaq_1saveCounter == SaveUnit) {
                // Save data to CSV file
                audioDaq_1csv.saveToCSV();
                audioDaq_1saveCounter = 0;
            }
        }

        // Check if new data is available from AudioDAQ_2
        int audioDaq_2tmpTimes = audioDaq_2.getTimes();
        if (audioDaq_2tmpTimes > audioDaq_2tmpTimer) {
            audioDaq_2Timer++;

            cout << "audioDaq_2 Program Count: " << audioDaq_2Timer << endl;
            cout << "audioDaq_2 Package Count: " << audioDaq_2tmpTimes << endl;

            // Retrieve the latest audio buffer and log to CSV
            auto buffer = audioDaq_2.getBuffer();
            audioDaq_2csv.addDataBlock(move(buffer));

            // Update read times
            audioDaq_2tmpTimer = audioDaq_2tmpTimes;
            audioDaq_2saveCounter++;

            if (audioDaq_2saveCounter == SaveUnit) {
                // Save data to CSV file
                audioDaq_2csv.saveToCSV();
                audioDaq_2saveCounter = 0;
            }
        }
    }
    resetTerminalMode(); // 還原終端屬性

    // Stop and clean up all tasks and resources
    niDaq.stopAndClearTask();
    audioDaq_1.stopCapture();
    audioDaq_2.stopCapture();

    // Clean up CSVWriter resources
    NiDAQcsv.stop();
    audioDaq_1csv.stop();
    audioDaq_2csv.stop();

    goto readDataLoop;

    return 0;
}
