// CSVWriter.cpp
#include "CSVWriter.h"

// Constructor: Initialize the CSVWriter with the specified number of channels and output directory
CSVWriter::CSVWriter(int numChannels, const string& outputDir, const string& label)
    : numChannels(numChannels), outputDir(outputDir), label(label) {
}


// Destructor: Ensure all resources and threads are properly cleaned up
CSVWriter::~CSVWriter() {
    stop();
}

// Add a block of data to the queue for writing to CSV
void CSVWriter::addDataBlock(vector<double>&& dataBlock) {
    lock_guard<mutex> lock(queueMutex);
    dataQueue.push(move(dataBlock));
}

// Write all queued data blocks to a single CSV file
void CSVWriter::saveToCSV() {
    dataAvailable.notify_one();
}

// �I���g�J������禡
void CSVWriter::writeToCSV() {
    while (!stopThreads || !dataQueue.empty()) {
        unique_lock<mutex> lock(queueMutex);
        dataAvailable.wait(lock, [this] { return !dataQueue.empty() || stopThreads; });

        while (!dataQueue.empty()) {
            vector<double> dataBlock = move(dataQueue.front());
            dataQueue.pop();
            lock.unlock();

            string filename = generateFilename();
            ofstream file(filename, ios::app); // �ϥ� append �Ҧ�
            if (!file.is_open()) {
                cerr << "Failed to create file: " << filename << endl;
                continue;
            }

            for (size_t i = 0; i < dataBlock.size(); i += numChannels) {
                for (int j = 0; j < numChannels; ++j) {
                    file << dataBlock[i + j];
                    if (j < numChannels - 1) {
                        file << ",";
                    }
                }
                file << "\n";
            }

            cout << "File written: " << filename << endl;
        }
    }
}

// �Ұʼg�J�����
void CSVWriter::start() {
    stopThreads = false;
    writerThread = thread(&CSVWriter::writeToCSV, this);
}

void CSVWriter::stop() {
    lock_guard<mutex> lock(queueMutex);
    while (!dataQueue.empty()) {
        dataQueue.pop(); // Clear the data queue.
    }
    cout << "CSVWriter resources have been cleaned up." << endl;
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
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &local_time);
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "_%03lld", millis);

    return outputDir + "/" + buffer + "_" + label + ".csv"; // Return the full path for the CSV file
}
