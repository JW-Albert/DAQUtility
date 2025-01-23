# 定義變數
CXX = g++
CXXFLAGS = -I../include -I../include/iniReader -std=c++17 -Wall
LDFLAGS = -lasound
TARGET = main
SRCS = main.cpp ../include/AudioDAQ.cpp ../include/iniReader/INIReader.cpp ../include/iniReader/ini.c
OBJS = $(SRCS:.cpp=.o)

# 預設目標
all: $(TARGET)

# 編譯可執行檔
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 編譯物件檔
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET)

# 測試用：列印變數
print-%:
	@echo $* = $($*)
