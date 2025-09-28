// Test for DAG-based filtered DBC parsing
// This demonstrates loading only the CAN signals needed by a DAG definition

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <string>
#include <regex>
#include <chrono>
#include "dbcppp-tiny/network.h"
#include "config.h"

// Simple YAML parser to extract DBC signal names from DAG
std::set<std::string> extractSignalsFromDAG(const std::string& yaml_file) {
    std::set<std::string> signals;
    std::ifstream file(yaml_file);
    if (!file.is_open()) {
        std::cerr << "Could not open DAG file: " << yaml_file << std::endl;
        return signals;
    }

    std::string line;
    bool in_source_section = false;

    // Simple regex to find DBC signal names
    std::regex name_pattern(R"(\s*name:\s*(\w+))");
    std::regex type_pattern(R"(\s*type:\s*dbc)");

    std::string current_type;

    while (std::getline(file, line)) {
        // Check for type: dbc
        std::smatch type_match;
        if (std::regex_match(line, type_match, type_pattern)) {
            current_type = "dbc";
            in_source_section = true;
            continue;
        }

        // If we found a DBC source, look for the name
        if (in_source_section && current_type == "dbc") {
            std::smatch name_match;
            if (std::regex_match(line, name_match, name_pattern)) {
                std::string signal_name = name_match[1];
                signals.insert(signal_name);
                std::cout << "DAG requires signal: " << signal_name << std::endl;
                in_source_section = false;
                current_type.clear();
            }
        }

        // Reset on new mapping entry
        if (line.find("- signal:") != std::string::npos) {
            in_source_section = false;
            current_type.clear();
        }
    }

    file.close();
    return signals;
}

// Helper to get file size
long getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return -1;
    long size = file.tellg();
    file.close();
    return size;
}

// Helper to measure memory (platform-specific, simplified)
size_t getCurrentMemoryUsage() {
    // On Linux, read from /proc/self/status
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) return 0;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("VmRSS:") == 0) {
            std::istringstream iss(line.substr(6));
            size_t mem_kb;
            iss >> mem_kb;
            return mem_kb * 1024; // Convert to bytes
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    const std::string test_dir = std::string(TEST_FILES_PATH) + "/dbc/";
    const std::string dag_file = test_dir + "model3_mappings_dag.yaml";
    const std::string dbc_file = test_dir + "Model3CAN.dbc";

    std::cout << "========================================" << std::endl;
    std::cout << "DAG-Filtered DBC Parsing Test" << std::endl;
    std::cout << "========================================" << std::endl;

    // Get file sizes
    long dag_size = getFileSize(dag_file);
    long dbc_size = getFileSize(dbc_file);

    std::cout << "DAG file size: " << dag_size << " bytes" << std::endl;
    std::cout << "DBC file size: " << dbc_size << " bytes" << std::endl;

    // Step 1: Parse DAG to get required signals
    std::cout << "\nStep 1: Parsing DAG for signal dependencies..." << std::endl;
    auto dag_signals = extractSignalsFromDAG(dag_file);
    std::cout << "Found " << dag_signals.size() << " signals in DAG" << std::endl;

    // Step 2: Load full DBC without filtering (baseline)
    std::cout << "\n========================================" << std::endl;
    std::cout << "Baseline: Loading FULL DBC (no filter)" << std::endl;
    std::cout << "========================================" << std::endl;

    auto mem_before_full = getCurrentMemoryUsage();
    auto start_full = std::chrono::high_resolution_clock::now();

    auto network_full = dbcppp::INetwork::LoadDBCFromFile(dbc_file.c_str());

    auto end_full = std::chrono::high_resolution_clock::now();
    auto mem_after_full = getCurrentMemoryUsage();

    if (network_full) {
        // Count signals
        uint64_t total_signals = 0;
        uint64_t total_messages = network_full->Messages_Size();

        for (uint64_t i = 0; i < total_messages; i++) {
            const auto& msg = network_full->Messages_Get(i);
            total_signals += msg.Signals_Size();
        }

        auto parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_full - start_full);

        std::cout << "Parse time: " << parse_time.count() << " ms" << std::endl;
        std::cout << "Memory used: " << (mem_after_full - mem_before_full) / 1024 << " KB" << std::endl;
        std::cout << "Messages loaded: " << total_messages << std::endl;
        std::cout << "Signals loaded: " << total_signals << std::endl;
    }

    // Step 3: Load DBC with DAG filtering
    std::cout << "\n========================================" << std::endl;
    std::cout << "Optimized: Loading DBC with DAG filter" << std::endl;
    std::cout << "========================================" << std::endl;

    // Create filters based on DAG requirements
    std::set<uint32_t> kept_messages;  // Will be populated as we find signals

    auto message_filter = [&kept_messages](uint32_t msg_id, const std::string& msg_name) {
        // Initially accept all messages to scan their signals
        // We'll refine this in a second pass if needed
        return true;
    };

    auto signal_filter = [&dag_signals, &kept_messages](const std::string& sig_name, uint32_t msg_id) {
        bool keep = dag_signals.count(sig_name) > 0;
        if (keep) {
            kept_messages.insert(msg_id);
        }
        return keep;
    };

    auto mem_before_filtered = getCurrentMemoryUsage();
    auto start_filtered = std::chrono::high_resolution_clock::now();

    auto network_filtered = dbcppp::INetwork::LoadDBCFromFile(dbc_file.c_str(),
                                                               message_filter,
                                                               signal_filter);

    auto end_filtered = std::chrono::high_resolution_clock::now();
    auto mem_after_filtered = getCurrentMemoryUsage();

    if (network_filtered) {
        // Count what we kept
        uint64_t filtered_signals = 0;
        uint64_t filtered_messages = 0;

        for (uint64_t i = 0; i < network_filtered->Messages_Size(); i++) {
            const auto& msg = network_filtered->Messages_Get(i);
            uint64_t msg_signals = msg.Signals_Size();
            if (msg_signals > 0) {
                filtered_messages++;
                filtered_signals += msg_signals;

                // Show some examples
                if (filtered_messages <= 3) {
                    std::cout << "  Message: " << msg.Name() << " (0x" << std::hex << msg.Id() << std::dec << ")" << std::endl;
                    for (uint64_t j = 0; j < msg_signals && j < 5; j++) {
                        std::cout << "    Signal: " << msg.Signals_Get(j).Name() << std::endl;
                    }
                }
            }
        }

        auto parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_filtered - start_filtered);

        std::cout << "\nParse time: " << parse_time.count() << " ms" << std::endl;
        std::cout << "Memory used: " << (mem_after_filtered - mem_before_filtered) / 1024 << " KB" << std::endl;
        std::cout << "Messages kept: " << filtered_messages << " (with signals)" << std::endl;
        std::cout << "Signals kept: " << filtered_signals << std::endl;

        // Verify all DAG signals were found
        std::cout << "\nVerifying DAG signals..." << std::endl;
        std::set<std::string> found_signals;
        for (uint64_t i = 0; i < network_filtered->Messages_Size(); i++) {
            const auto& msg = network_filtered->Messages_Get(i);
            for (uint64_t j = 0; j < msg.Signals_Size(); j++) {
                found_signals.insert(msg.Signals_Get(j).Name());
            }
        }

        int missing = 0;
        for (const auto& required : dag_signals) {
            if (found_signals.count(required) == 0) {
                std::cout << "  WARNING: Signal '" << required << "' not found in DBC" << std::endl;
                missing++;
            }
        }

        if (missing == 0) {
            std::cout << "  ✓ All " << dag_signals.size() << " DAG signals found!" << std::endl;
        } else {
            std::cout << "  ✗ Missing " << missing << " signals" << std::endl;
        }

        // Calculate savings
        if (network_full) {
            uint64_t full_signals = 0;
            for (uint64_t i = 0; i < network_full->Messages_Size(); i++) {
                full_signals += network_full->Messages_Get(i).Signals_Size();
            }

            std::cout << "\n========================================" << std::endl;
            std::cout << "Savings Summary" << std::endl;
            std::cout << "========================================" << std::endl;

            float signal_reduction = 100.0f * (1.0f - (float)filtered_signals / full_signals);
            float message_reduction = 100.0f * (1.0f - (float)filtered_messages / network_full->Messages_Size());
            float memory_reduction = 100.0f * (1.0f - (float)(mem_after_filtered - mem_before_filtered) /
                                               (mem_after_full - mem_before_full));

            std::cout << "Signal reduction: " << signal_reduction << "%" << std::endl;
            std::cout << "Message reduction: " << message_reduction << "%" << std::endl;
            std::cout << "Memory reduction: " << memory_reduction << "%" << std::endl;
        }
    }

    return 0;
}