#include "CSVWriter.h"

// Constructor: Initializes the CSVWriter and generates the first CSV filename.
CSVWriter::CSVWriter(int numChannels, const string& outputDir, const string& label)
    : numChannels(numChannels), outputDir(outputDir), label(label) {
    currentFilename = generateFilename(); // Generate initial filename
}

// Writes incoming data to the file immediately.
void CSVWriter::addDataBlock(vector<double>&& dataBlock) {
    lock_guard<mutex> lock(fileMutex); // Ensure thread safety
    ofstream file(currentFilename, ios::app); // Open file in append mode
    if (!file.is_open()) {
        cerr << "Failed to open file: " << currentFilename << endl;
        return;
    }

    // Write data to CSV file in rows, with values separated by commas.
    for (size_t i = 0; i < dataBlock.size(); i += numChannels) {
        for (int j = 0; j < numChannels; ++j) {
            file << dataBlock[i + j];
            if (j < numChannels - 1) {
                file << ",";
            }
        }
        file << "\n";
    }
    file.close();
}

// Updates the filename when a `SaveUnit` is reached.
void CSVWriter::updateFilename() {
    lock_guard<mutex> lock(fileMutex); // Ensure thread safety
    currentFilename = generateFilename();
}

// Generates a new CSV filename based on the current timestamp.
string CSVWriter::generateFilename() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    tm local_time;

#ifdef _WIN32
    localtime_s(&local_time, &now_time); // Windows-specific function
#else
    localtime_r(&now_time, &local_time); // POSIX function
#endif

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &local_time); // Format timestamp

    return outputDir + "/" + buffer + "_" + label + ".csv"; // Construct filename
}
