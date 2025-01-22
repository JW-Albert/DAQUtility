// CSVWriter.h
#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <cstring>

using namespace std;

class CSVWriter {
public:
    // Constructor: Initialize the CSVWriter with the number of channels and output directory
    CSVWriter(int numChannels, const string& outputDir = "output/");

    // Destructor: Clean up resources and ensure threads are stopped
    ~CSVWriter();

    // Start the thread responsible for writing data to CSV files
    void start();

    // Stop the CSV writing thread and clear resources
    void stop();

    // Add a block of data to the queue for asynchronous writing
    void addDataBlock(vector<double>&& dataBlock);

private:
    int numChannels;                           // Number of channels in the data
    string outputDir;                          // Directory where CSV files are saved
    queue<vector<double>> dataQueue;           // Queue to hold data blocks for writing
    mutex queueMutex;                          // Mutex for synchronizing access to the queue
    condition_variable dataAvailable;          // Condition variable to notify the writer thread
    atomic<bool> stopThreads;                  // Flag to signal the thread to stop
    thread writerThread;                       // Thread responsible for writing data to CSV files

    // Main function for the writer thread to process and save data
    void writeToCSV();

    // Generate a unique filename for the CSV file based on the current timestamp
    string generateFilename();
};

#endif // CSV_WRITER_H