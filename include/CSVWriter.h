#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

using namespace std;

// CSVWriter class handles writing data to CSV files in a thread-safe manner.
class CSVWriter {
public:
    // Constructor: Initializes CSVWriter with number of channels, output directory, and label.
    CSVWriter(int numChannels, const string& outputDir, const string& label);
    
    // Writes incoming data immediately to the current CSV file.
    void addDataBlock(vector<double>&& dataBlock);
    
    // Updates the filename when `SaveUnit` is reached.
    void updateFilename();

private:
    int numChannels;         // Number of channels in the data
    string outputDir;        // Directory where CSV files will be stored
    string label;            // Label to include in the filename
    string currentFilename;  // Current CSV filename
    mutex fileMutex;         // Mutex for thread safety

    // Generates a new filename based on the current timestamp.
    string generateFilename();
};

#endif // CSV_WRITER_H