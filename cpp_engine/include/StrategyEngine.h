#pragma once
#include <vector>
#include <limits>
#include <string>
#include "CSVParser.h"

struct Trade {
    std::string date;
    std::string action; // "BUY" or "SELL"
    double price;
    int shares;
    double cash;
    double portfolio_value;
};

class StrategyEngine {
public:
    StrategyEngine(const std::vector<PriceData>& price_data);
    std::vector<double> calculateSMA(size_t window) const;
    std::vector<double> calculateWMA(size_t window) const;
    std::vector<int> generateCrossoverSignals(const std::vector<double>& shortMA, const std::vector<double>& longMA) const;
    std::vector<Trade> simulateTrades(const std::vector<int>& signals, double initial_capital = 10000.0, double flat_fee = 0.0, double percent_fee = 0.0, double slippage = 0.0, double dividend_yield = 0.0) const;
    std::vector<double> getEquityCurve(const std::vector<int>& signals, double initial_capital = 10000.0, double flat_fee = 0.0, double percent_fee = 0.0, double slippage = 0.0, double dividend_yield = 0.0) const;
    std::vector<double> getBenchmarkEquityCurve(double initial_capital = 10000.0, double dividend_yield = 0.0) const;
private:
    std::vector<PriceData> data;
}; 