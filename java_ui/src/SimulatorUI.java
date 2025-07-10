import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.nio.file.*;
import java.util.List;
import java.util.Comparator;

public class SimulatorUI extends JFrame {
    private JTextField csvField, capitalField, shortMAField, longMAField, flatFeeField, percentFeeField, slippageField, dividendField;
    private JComboBox<String> shortMATypeBox, longMATypeBox;
    private JTextArea resultArea;
    private JButton runButton, csvBrowseButton, sweepButton;
    private File lastDir = new File(".");

    public SimulatorUI() {
        setTitle("Algorithmic Trading Simulator");
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setSize(700, 600);
        setLayout(new BorderLayout());

        JPanel inputPanel = new JPanel(new GridLayout(0, 3, 5, 5));
        csvField = new JTextField("/Users/pranaynookala/SimulatedTrader/cpp_engine/data/SPX.csv");
        csvBrowseButton = new JButton("Browse");
        csvBrowseButton.addActionListener(e -> chooseFile(csvField));
        inputPanel.add(new JLabel("CSV File:"));
        inputPanel.add(csvField);
        inputPanel.add(csvBrowseButton);

        capitalField = new JTextField("10000");
        inputPanel.add(new JLabel("Initial Capital:"));
        inputPanel.add(capitalField);
        inputPanel.add(new JLabel("USD"));

        shortMAField = new JTextField("50");
        inputPanel.add(new JLabel("Short MA Window:"));
        inputPanel.add(shortMAField);
        shortMATypeBox = new JComboBox<>(new String[]{"SMA", "WMA"});
        inputPanel.add(shortMATypeBox);

        longMAField = new JTextField("200");
        inputPanel.add(new JLabel("Long MA Window:"));
        inputPanel.add(longMAField);
        longMATypeBox = new JComboBox<>(new String[]{"SMA", "WMA"});
        inputPanel.add(longMATypeBox);

        flatFeeField = new JTextField("5.0");
        inputPanel.add(new JLabel("Flat Fee per Trade:"));
        inputPanel.add(flatFeeField);
        inputPanel.add(new JLabel("USD"));

        percentFeeField = new JTextField("0.001");
        inputPanel.add(new JLabel("Percent Fee per Trade:"));
        inputPanel.add(percentFeeField);
        inputPanel.add(new JLabel("(e.g., 0.001 = 0.1%)"));

        slippageField = new JTextField("0.001");
        inputPanel.add(new JLabel("Slippage:"));
        inputPanel.add(slippageField);
        inputPanel.add(new JLabel("(e.g., 0.001 = 0.1%)"));

        dividendField = new JTextField("0.02");
        inputPanel.add(new JLabel("Dividend Yield (annual):"));
        inputPanel.add(dividendField);
        inputPanel.add(new JLabel("(e.g., 0.02 = 2%)"));

        runButton = new JButton("Run Simulation");
        runButton.addActionListener(e -> runSimulation());

        sweepButton = new JButton("Parameter Sweep");
        sweepButton.addActionListener(e -> runSweep());

        JPanel topPanel = new JPanel(new BorderLayout());
        JPanel buttonPanel = new JPanel(new FlowLayout());
        buttonPanel.add(runButton);
        buttonPanel.add(sweepButton);
        topPanel.add(inputPanel, BorderLayout.CENTER);
        topPanel.add(buttonPanel, BorderLayout.SOUTH);

        resultArea = new JTextArea();
        resultArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        resultArea.setEditable(false);
        JScrollPane scrollPane = new JScrollPane(resultArea);

        add(topPanel, BorderLayout.NORTH);
        add(scrollPane, BorderLayout.CENTER);
    }

    private void chooseFile(JTextField field) {
        JFileChooser chooser = new JFileChooser(lastDir);
        int ret = chooser.showOpenDialog(this);
        if (ret == JFileChooser.APPROVE_OPTION) {
            File f = chooser.getSelectedFile();
            field.setText(f.getAbsolutePath());
            lastDir = f.getParentFile();
        }
    }

    private void runSimulation() {
        try {
            // Use absolute paths for config and CSV
            String projectRoot = new File("../..").getCanonicalPath();
            String configPath = projectRoot + "/data/config.json";
            Files.createDirectories(Paths.get(projectRoot + "/data"));
            String csvAbsolutePath = new File(csvField.getText()).getAbsolutePath();
            String config = String.format("{\n" +
                "  \"csv_file\": \"%s\",\n" +
                "  \"short_ma_window\": %s,\n" +
                "  \"long_ma_window\": %s,\n" +
                "  \"short_ma_type\": \"%s\",\n" +
                "  \"long_ma_type\": \"%s\",\n" +
                "  \"initial_capital\": %s,\n" +
                "  \"flat_fee\": %s,\n" +
                "  \"percent_fee\": %s,\n" +
                "  \"slippage\": %s,\n" +
                "  \"dividend_yield\": %s,\n" +
                "  \"position_size\": 1.0\n" +
                "}\n",
                csvAbsolutePath,
                shortMAField.getText(),
                longMAField.getText(),
                shortMATypeBox.getSelectedItem(),
                longMATypeBox.getSelectedItem(),
                capitalField.getText(),
                flatFeeField.getText(),
                percentFeeField.getText(),
                slippageField.getText(),
                dividendField.getText()
            );
            Files.write(Paths.get(configPath), config.getBytes());

            // Run C++ simulator with project root as working directory
            String binaryPath = projectRoot + "/cpp_engine/bin/test_csvparser";
            ProcessBuilder pb = new ProcessBuilder(binaryPath);
            pb.directory(new File(projectRoot));
            pb.redirectErrorStream(true);
            Process proc = pb.start();
            BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream()));
            StringBuilder output = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append("\n");
            }
            proc.waitFor();
            resultArea.setText(output.toString());
        } catch (Exception ex) {
            resultArea.setText("Error: " + ex.getMessage());
            ex.printStackTrace();
        }
    }

    private void runSweep() {
        try {
            int[] shortMAs = {10, 20, 50};
            int[] longMAs = {100, 150, 200};
            String[] types = {"SMA", "WMA"};
            java.util.List<ParameterSweep.Result> results = ParameterSweep.sweep(shortMAs, longMAs, types);
            results.sort(Comparator.comparingDouble((ParameterSweep.Result r) -> -r.cagr));
            StringBuilder sb = new StringBuilder();
            sb.append("Top 5 Results (by CAGR):\n");
            for (int i = 0; i < Math.min(5, results.size()); ++i) {
                sb.append(results.get(i).toString()).append("\n");
            }
            resultArea.setText(sb.toString());
        } catch (Exception ex) {
            resultArea.setText("Error: " + ex.getMessage());
            ex.printStackTrace();
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            SimulatorUI ui = new SimulatorUI();
            ui.setVisible(true);
        });
    }
} 