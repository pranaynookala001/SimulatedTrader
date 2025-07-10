#include "../include/CSVParser.h"
#include "../include/StrategyEngine.h"
#include "../include/json.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <limits>
#include <string>
#include <cmath>
#include <vector>

using json = nlohmann::json;

struct Config {
    size_t short_ma_window = 50;
    size_t long_ma_window = 200;
    std::string short_ma_type = "SMA";
    std::string long_ma_type = "SMA";
    double initial_capital = 10000.0;
    double transaction_fee = 1.0;
    double flat_fee = 5.0;
    double percent_fee = 0.001;
    double slippage = 0.001;
    double dividend_yield = 0.02;
    double position_size = 1.0;
};

Config load_config(const std::string& path) {
    Config cfg;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Could not open config file: " << path << ". Using defaults." << std::endl;
        return cfg;
    }
    json j;
    f >> j;
    if (j.contains("short_ma_window")) cfg.short_ma_window = j["short_ma_window"];
    if (j.contains("long_ma_window")) cfg.long_ma_window = j["long_ma_window"];
    if (j.contains("short_ma_type")) cfg.short_ma_type = j["short_ma_type"];
    if (j.contains("long_ma_type")) cfg.long_ma_type = j["long_ma_type"];
    if (j.contains("initial_capital")) cfg.initial_capital = j["initial_capital"];
    if (j.contains("transaction_fee")) cfg.transaction_fee = j["transaction_fee"];
    if (j.contains("flat_fee")) cfg.flat_fee = j["flat_fee"];
    if (j.contains("percent_fee")) cfg.percent_fee = j["percent_fee"];
    if (j.contains("slippage")) cfg.slippage = j["slippage"];
    if (j.contains("dividend_yield")) cfg.dividend_yield = j["dividend_yield"];
    if (j.contains("position_size")) cfg.position_size = j["position_size"];
    return cfg;
}

double calc_total_return(double initial, double final) {
    return (final - initial) / initial;
}

double calc_cagr(double initial, double final, double years) {
    return std::pow(final / initial, 1.0 / years) - 1.0;
}

double calc_max_drawdown(const std::vector<double>& equity_curve) {
    double max_dd = 0.0, peak = equity_curve[0];
    for (double v : equity_curve) {
        if (v > peak) peak = v;
        double dd = (peak - v) / peak;
        if (dd > max_dd) max_dd = dd;
    }
    return max_dd;
}

double calc_sharpe(const std::vector<double>& equity_curve) {
    if (equity_curve.size() < 2) return 0.0;
    std::vector<double> rets;
    for (size_t i = 1; i < equity_curve.size(); ++i) {
        rets.push_back((equity_curve[i] - equity_curve[i-1]) / equity_curve[i-1]);
    }
    double mean = 0.0, stddev = 0.0;
    for (double r : rets) mean += r;
    mean /= rets.size();
    for (double r : rets) stddev += (r - mean) * (r - mean);
    stddev = std::sqrt(stddev / (rets.size() - 1));
    return (stddev > 0) ? mean / stddev * std::sqrt(252) : 0.0; // annualized
}

int main() {
    std::string config_path = "data/config.json";
    std::string filename;
    // Read config and get csv_file
    std::ifstream f(config_path);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open config file: " << config_path << std::endl;
        return 1;
    }
    nlohmann::json j;
    f >> j;
    if (j.contains("csv_file")) {
        filename = j["csv_file"];
    } else {
        std::cerr << "Error: config.json missing 'csv_file' field." << std::endl;
        return 1;
    }
    auto data = CSVParser::parseCSV(filename);
    if (data.empty()) {
        std::cerr << "Error: Could not open or parse CSV file: " << filename << std::endl;
        return 1;
    }
    std::cout << "Parsed " << data.size() << " rows." << std::endl;

    Config cfg = load_config(config_path);
    StrategyEngine engine(data);
    // Select MA types
    std::vector<double> short_ma, long_ma;
    if (cfg.short_ma_type == "WMA")
        short_ma = engine.calculateWMA(cfg.short_ma_window);
    else
        short_ma = engine.calculateSMA(cfg.short_ma_window);
    if (cfg.long_ma_type == "WMA")
        long_ma = engine.calculateWMA(cfg.long_ma_window);
    else
        long_ma = engine.calculateSMA(cfg.long_ma_window);

    auto signals = engine.generateCrossoverSignals(short_ma, long_ma);

    // Next-day execution: shift signals forward by 1 day
    std::vector<int> shifted_signals(signals.size(), 0);
    for (size_t i = 0; i + 1 < signals.size(); ++i) {
        shifted_signals[i + 1] = signals[i];
    }
    auto trades = engine.simulateTrades(
        shifted_signals,
        cfg.initial_capital,
        cfg.flat_fee,
        cfg.percent_fee,
        cfg.slippage,
        cfg.dividend_yield
    );
    auto equity_curve = engine.getEquityCurve(
        shifted_signals,
        cfg.initial_capital,
        cfg.flat_fee,
        cfg.percent_fee,
        cfg.slippage,
        cfg.dividend_yield
    );
    auto benchmark_curve = engine.getBenchmarkEquityCurve(cfg.initial_capital, cfg.dividend_yield);

    std::cout << "\nFirst 10 trades (" << cfg.short_ma_type << "-" << cfg.short_ma_window << "/" << cfg.long_ma_type << "-" << cfg.long_ma_window << " crossover, next-day execution):\n";
    std::cout << "Date       Action  Price    Shares   Cash      PortfolioValue" << std::endl;
    for (size_t i = 0; i < trades.size() && i < 10; ++i) {
        const auto& t = trades[i];
        std::cout << t.date << "  " << t.action << "  " << std::fixed << std::setprecision(2) << t.price << "  " << t.shares << "  " << t.cash << "  " << t.portfolio_value << std::endl;
    }
    if (!trades.empty()) {
        const auto& last = trades.back();
        std::cout << "\nFinal Portfolio Value: $" << std::fixed << std::setprecision(2) << last.portfolio_value << std::endl;
    }

    // Performance metrics
    double initial = cfg.initial_capital;
    double final = equity_curve.back();
    double years = (data.size() / 252.0); // 252 trading days/year
    double total_return = calc_total_return(initial, final);
    double cagr = calc_cagr(initial, final, years);
    double max_dd = calc_max_drawdown(equity_curve);
    double sharpe = calc_sharpe(equity_curve);
    int num_trades = 0;
    for (const auto& t : trades) if (t.action == "BUY") ++num_trades;

    double bench_final = benchmark_curve.back();
    double bench_total_return = calc_total_return(initial, bench_final);
    double bench_cagr = calc_cagr(initial, bench_final, years);
    double bench_max_dd = calc_max_drawdown(benchmark_curve);
    double bench_sharpe = calc_sharpe(benchmark_curve);

    std::cout << "\nPerformance Metrics (Strategy):" << std::endl;
    std::cout << "Total Return: " << std::fixed << std::setprecision(2) << (total_return * 100) << "%" << std::endl;
    std::cout << "CAGR: " << std::fixed << std::setprecision(2) << (cagr * 100) << "%" << std::endl;
    std::cout << "Max Drawdown: " << std::fixed << std::setprecision(2) << (max_dd * 100) << "%" << std::endl;
    std::cout << "Sharpe Ratio: " << std::fixed << std::setprecision(2) << sharpe << std::endl;
    std::cout << "Number of Trades: " << num_trades << std::endl;

    std::cout << "\nPerformance Metrics (Buy & Hold Benchmark):" << std::endl;
    std::cout << "Total Return: " << std::fixed << std::setprecision(2) << (bench_total_return * 100) << "%" << std::endl;
    std::cout << "CAGR: " << std::fixed << std::setprecision(2) << (bench_cagr * 100) << "%" << std::endl;
    std::cout << "Max Drawdown: " << std::fixed << std::setprecision(2) << (bench_max_dd * 100) << "%" << std::endl;
    std::cout << "Sharpe Ratio: " << std::fixed << std::setprecision(2) << bench_sharpe << std::endl;

    return 0;
} 