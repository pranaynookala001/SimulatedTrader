#include "../include/CSVParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

void CSVParser::warnMalformedLine(size_t line_num, const std::string& line) {
    std::cerr << "Warning: Malformed line " << line_num << ": " << line << std::endl;
}

std::vector<PriceData> CSVParser::parseCSV(const std::string& filename) {
    std::vector<PriceData> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return data;
    }
    std::string line;
    std::unordered_map<std::string, size_t> col_idx;
    size_t line_num = 0;
    // Parse header
    if (std::getline(file, line)) {
        ++line_num;
        std::stringstream ss(line);
        std::string col;
        size_t idx = 0;
        while (std::getline(ss, col, ',')) {
            col_idx[col] = idx++;
        }
    } else {
        return data;
    }
    // Parse data rows
    while (std::getline(file, line)) {
        ++line_num;
        std::stringstream ss(line);
        std::vector<std::string> fields;
        std::string item;
        while (std::getline(ss, item, ',')) {
            fields.push_back(item);
        }
        try {
            PriceData row;
            row.date = (col_idx.count("Date") && col_idx["Date"] < fields.size()) ? fields[col_idx["Date"]] : "";
            row.open = (col_idx.count("Open") && col_idx["Open"] < fields.size() && !fields[col_idx["Open"]].empty()) ? std::stod(fields[col_idx["Open"]]) : 0.0;
            row.high = (col_idx.count("High") && col_idx["High"] < fields.size() && !fields[col_idx["High"]].empty()) ? std::stod(fields[col_idx["High"]]) : 0.0;
            row.low = (col_idx.count("Low") && col_idx["Low"] < fields.size() && !fields[col_idx["Low"]].empty()) ? std::stod(fields[col_idx["Low"]]) : 0.0;
            row.close = (col_idx.count("Close") && col_idx["Close"] < fields.size() && !fields[col_idx["Close"]].empty()) ? std::stod(fields[col_idx["Close"]]) : 0.0;
            row.volume = (col_idx.count("Volume") && col_idx["Volume"] < fields.size() && !fields[col_idx["Volume"]].empty()) ? std::stol(fields[col_idx["Volume"]]) : 0;
            if (row.date.empty()) throw std::runtime_error("Missing date");
            data.push_back(row);
        } catch (...) {
            warnMalformedLine(line_num, line);
            continue;
        }
    }
    file.close();
    return data;
} 