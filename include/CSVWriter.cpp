// CSVWriter.cpp
#include "CSVWriter.h"
#include <cstring>
#include <chrono>
#include <ctime>

// Constructor: Initialize the CSVWriter with the specified number of channels and output directory
CSVWriter::CSVWriter(int numChannels, const string& outputDir)
    : numChannels(numChannels), outputDir(outputDir), stopThreads(false) {
}

// Destructor: Ensure all resources and threads are properly cleaned up
CSVWriter::~CSVWriter() {
    stop();
}

// Start the CSV writing thread
void CSVWriter::start() {
    writerThread = thread(&CSVWriter::writeToCSV, this);
}

// Stop the CSV writing thread and ensure proper cleanup
void CSVWriter::stop() {
    stopThreads.store(true); // Signal the thread to stop
    dataAvailable.notify_all(); // Wake up the thread if waiting
    if (writerThread.joinable()) {
        writerThread.join(); // Wait for the thread to finish
    }
}

// Add a block of data to the queue for writing to CSV
void CSVWriter::addDataBlock(vector<double>&& dataBlock) {
    {
        lock_guard<mutex> lock(queueMutex); // Lock the queue while modifying
        dataQueue.push(move(dataBlock));   // Move the data into the queue
    }
    dataAvailable.notify_one(); // Notify the writer thread that data is available
}

// Thread function for writing data blocks to CSV files
void CSVWriter::writeToCSV() {
    while (!stopThreads.load() || !dataQueue.empty()) {
        unique_lock<mutex> lock(queueMutex);
        // Wait until data is available or stop is signaled
        dataAvailable.wait(lock, [this] { return !dataQueue.empty() || stopThreads.load(); });

        if (!dataQueue.empty()) {
            // Retrieve the next data block from the queue
            vector<double> dataBlock = move(dataQueue.front());
            dataQueue.pop();
            lock.unlock(); // Unlock the queue to allow other operations

            // Generate a filename for the new CSV file
            string filename = generateFilename();
            ofstream file(filename);
            if (!file.is_open()) {
                cerr << "Failed to create file: " << filename << endl;
                continue;
            }

            // Write data to the CSV file
            for (size_t i = 0; i < dataBlock.size(); i += numChannels) {
                for (int j = 0; j < numChannels; ++j) {
                    file << dataBlock[i + j];
                    if (j < numChannels - 1) {
                        file << ","; // Separate columns with commas
                    }
                }
                file << "\n"; // Newline after each row
            }

            cout << "File written: " << filename << endl;
        }
    }
}

// Generate a unique filename based on the current timestamp
string CSVWriter::generateFilename() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    auto duration = now.time_since_epoch();
    auto millis = chrono::duration_cast<chrono::milliseconds>(duration).count() % 1000;
    tm local_time;

#ifdef _WIN32
    localtime_s(&local_time, &now_time);
#else
    localtime_r(&now_time, &local_time);
#endif

    char buffer[64];
    // Format the timestamp as "YYYY_MM_DD_HH_MM_SS"
    strftime(buffer, sizeof(buffer), "%Y_%m_%d_%H_%M_%S", &local_time);
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "_%03lld.csv", millis);

    return outputDir + buffer; // Return the full path for the CSV file
}