#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct PriceData {
    std::string date;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    long volume = 0;
};

class CSVParser {
public:
    static std::vector<PriceData> parseCSV(const std::string& filename);
private:
    static void warnMalformedLine(size_t line_num, const std::string& line);
}; 