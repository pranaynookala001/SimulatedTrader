import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ParameterSweep {
    public static class Result {
        public int shortMA, longMA;
        public String shortType, longType;
        public double cagr, sharpe, maxDD, totalReturn;
        public Result(int s, int l, String st, String lt, double c, double sh, double m, double t) {
            shortMA = s; longMA = l; shortType = st; longType = lt;
            cagr = c; sharpe = sh; maxDD = m; totalReturn = t;
        }
        public String toString() {
            return String.format("%3d/%3d %3s/%3s  CAGR: %.2f%%  Sharpe: %.2f  MaxDD: %.2f%%  Return: %.2f%%",
                shortMA, longMA, shortType, longType, cagr*100, sharpe, maxDD*100, totalReturn*100);
        }
    }

    public static List<Result> sweep(int[] shortMAs, int[] longMAs, String[] types) throws Exception {
        List<Result> results = new ArrayList<>();
        for (int s : shortMAs) {
            for (int l : longMAs) {
                if (s >= l) continue;
                for (String st : types) {
                    for (String lt : types) {
                        // Write config
                        String config = String.format("{\n" +
                            "  \"short_ma_window\": %d,\n" +
                            "  \"long_ma_window\": %d,\n" +
                            "  \"short_ma_type\": \"%s\",\n" +
                            "  \"long_ma_type\": \"%s\",\n" +
                            "  \"initial_capital\": 10000.0,\n" +
                            "  \"flat_fee\": 5.0,\n" +
                            "  \"percent_fee\": 0.001,\n" +
                            "  \"slippage\": 0.001,\n" +
                            "  \"dividend_yield\": 0.02,\n" +
                            "  \"position_size\": 1.0\n" +
                            "}\n", s, l, st, lt);
                        Files.write(Paths.get("cpp_engine/data/config.json"), config.getBytes());
                        // Run simulator
                        ProcessBuilder pb = new ProcessBuilder("../cpp_engine/bin/test_csvparser");
                        pb.redirectErrorStream(true);
                        Process proc = pb.start();
                        BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream()));
                        String line;
                        double cagr = 0, sharpe = 0, maxdd = 0, ret = 0;
                        while ((line = reader.readLine()) != null) {
                            if (line.contains("CAGR:")) cagr = parsePct(line);
                            if (line.contains("Sharpe Ratio:")) sharpe = parseNum(line);
                            if (line.contains("Max Drawdown:")) maxdd = parsePct(line);
                            if (line.contains("Total Return:")) ret = parsePct(line);
                        }
                        proc.waitFor();
                        results.add(new Result(s, l, st, lt, cagr/100, sharpe, maxdd/100, ret/100));
                    }
                }
            }
        }
        return results;
    }

    private static double parsePct(String line) {
        int idx = line.indexOf(":");
        if (idx < 0) return 0;
        String pct = line.substring(idx+1).replace("%","").trim();
        try { return Double.parseDouble(pct); } catch (Exception e) { return 0; }
    }
    private static double parseNum(String line) {
        int idx = line.indexOf(":");
        if (idx < 0) return 0;
        String num = line.substring(idx+1).trim();
        try { return Double.parseDouble(num); } catch (Exception e) { return 0; }
    }
} 