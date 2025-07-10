CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -I../include
SRC = src/main.cpp src/CSVParser.cpp src/StrategyEngine.cpp
OUT = bin/test_csvparser

all: $(OUT)

$(OUT): $(SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC)

clean:
	rm -rf bin 