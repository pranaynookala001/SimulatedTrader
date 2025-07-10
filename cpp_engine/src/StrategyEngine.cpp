#include "../include/StrategyEngine.h"
#include <limits>
#include <cmath>

StrategyEngine::StrategyEngine(const std::vector<PriceData>& price_data)
    : data(price_data) {}

std::vector<double> StrategyEngine::calculateSMA(size_t window) const {
    std::vector<double> sma;
    if (window == 0 || data.size() < window) return sma;
    double sum = 0.0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += data[i].close;
        if (i >= window) {
            sum -= data[i - window].close;
        }
        if (i >= window - 1) {
            sma.push_back(sum / window);
        } else {
            sma.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    return sma;
}

std::vector<double> StrategyEngine::calculateWMA(size_t window) const {
    std::vector<double> wma;
    if (window == 0 || data.size() < window) return wma;
    double weight_sum = window * (window + 1) / 2.0;
    for (size_t i = 0; i < data.size(); ++i) {
        if (i + 1 < window) {
            wma.push_back(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        double weighted_sum = 0.0;
        for (size_t j = 0; j < window; ++j) {
            weighted_sum += data[i - j].close * (window - j);
        }
        wma.push_back(weighted_sum / weight_sum);
    }
    return wma;
}

std::vector<int> StrategyEngine::generateCrossoverSignals(const std::vector<double>& shortMA, const std::vector<double>& longMA) const {
    std::vector<int> signals(data.size(), 0);
    for (size_t i = 1; i < data.size(); ++i) {
        if (std::isnan(shortMA[i - 1]) || std::isnan(longMA[i - 1]) || std::isnan(shortMA[i]) || std::isnan(longMA[i])) {
            continue;
        }
        // Golden cross: short MA crosses above long MA
        if (shortMA[i - 1] <= longMA[i - 1] && shortMA[i] > longMA[i]) {
            signals[i] = 1; // Buy
        }
        // Death cross: short MA crosses below long MA
        else if (shortMA[i - 1] >= longMA[i - 1] && shortMA[i] < longMA[i]) {
            signals[i] = -1; // Sell
        }
    }
    return signals;
}

std::vector<Trade> StrategyEngine::simulateTrades(const std::vector<int>& signals, double initial_capital, double flat_fee, double percent_fee, double slippage, double dividend_yield) const {
    std::vector<Trade> trades;
    double cash = initial_capital;
    int shares = 0;
    double portfolio_value = initial_capital;
    for (size_t i = 0; i < data.size(); ++i) {
        // Daily dividend
        if (shares > 0 && dividend_yield > 0.0) {
            cash += shares * data[i].close * (dividend_yield / 252.0);
        }
        double exec_price = data[i].close;
        if (signals[i] == 1 && shares == 0) { // Buy
            exec_price *= (1.0 + slippage);
            double total_fee = flat_fee + percent_fee * (exec_price);
            int buy_shares = static_cast<int>((cash - total_fee) / exec_price);
            if (buy_shares > 0) {
                double cost = buy_shares * exec_price + total_fee;
                cash -= cost;
                shares += buy_shares;
                portfolio_value = cash + shares * exec_price;
                trades.push_back({data[i].date, "BUY", exec_price, buy_shares, cash, portfolio_value});
            }
        } else if (signals[i] == -1 && shares > 0) { // Sell
            exec_price *= (1.0 - slippage);
            double total_fee = flat_fee + percent_fee * (exec_price);
            cash += shares * exec_price - total_fee;
            trades.push_back({data[i].date, "SELL", exec_price, shares, cash, cash});
            shares = 0;
            portfolio_value = cash;
        } else {
            portfolio_value = cash + shares * exec_price;
        }
    }
    // Final state: if holding shares, log final value
    if (shares > 0) {
        double exec_price = data.back().close * (1.0 - slippage);
        double total_fee = flat_fee + percent_fee * (exec_price);
        cash += shares * exec_price - total_fee;
        trades.push_back({data.back().date, "SELL", exec_price, shares, cash, cash});
    }
    return trades;
}

std::vector<double> StrategyEngine::getEquityCurve(const std::vector<int>& signals, double initial_capital, double flat_fee, double percent_fee, double slippage, double dividend_yield) const {
    std::vector<double> equity_curve;
    double cash = initial_capital;
    int shares = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        if (shares > 0 && dividend_yield > 0.0) {
            cash += shares * data[i].close * (dividend_yield / 252.0);
        }
        double exec_price = data[i].close;
        if (signals[i] == 1 && shares == 0) {
            exec_price *= (1.0 + slippage);
            double total_fee = flat_fee + percent_fee * (exec_price);
            int buy_shares = static_cast<int>((cash - total_fee) / exec_price);
            if (buy_shares > 0) {
                cash -= buy_shares * exec_price + total_fee;
                shares += buy_shares;
            }
        } else if (signals[i] == -1 && shares > 0) {
            exec_price *= (1.0 - slippage);
            double total_fee = flat_fee + percent_fee * (exec_price);
            cash += shares * exec_price - total_fee;
            shares = 0;
        }
        equity_curve.push_back(cash + shares * exec_price);
    }
    return equity_curve;
}

std::vector<double> StrategyEngine::getBenchmarkEquityCurve(double initial_capital, double dividend_yield) const {
    std::vector<double> equity_curve;
    if (data.empty()) return equity_curve;
    int shares = static_cast<int>(initial_capital / data[0].close);
    double cash = initial_capital - shares * data[0].close;
    for (size_t i = 0; i < data.size(); ++i) {
        if (dividend_yield > 0.0) {
            cash += shares * data[i].close * (dividend_yield / 252.0);
        }
        equity_curve.push_back(cash + shares * data[i].close);
    }
    return equity_curve;
} 