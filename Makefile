# 編譯器與參數
CC = g++
CFLAGS = -Wall -std=c++17 -pthread -I./include -I./include/iniReader -I./include/NiDAQmx/include
LDFLAGS = -L./include/NiDAQmx/lib64/gcc -lnidaqmx -lasound

# 檔案設定
SRCS = main.cpp include/NiDAQ.cpp include/CSVWriter.cpp \
       include/iniReader/INIReader.cpp include/iniReader/ini.c \
       include/AudioDAQ.cpp
OBJS = $(SRCS:.cpp=.o) $(SRCS:.c=.o)

# 最終目標執行檔
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
