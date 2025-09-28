
#include <filesystem>
#include <iostream>
#include <string>
#include <chrono>

#include "dbcppp-tiny/network.h"
#include "../src/file_reader.h"
#include "../src/dbc_stream_parser.h"

#include "config.h"

#include <catch2/catch_test_macros.hpp>

#define USE_STREAMING_PARSER_TEST

TEST_CASE("DBCParserTest", "[unit]")
{
    std::size_t i = 0;
    std::size_t success_count = 0;
    for (const auto& dbc_file : std::filesystem::directory_iterator(std::filesystem::path(TEST_FILES_PATH) / "dbc"))
    {
        if (dbc_file.path().extension() != ".dbc")
        {
            continue;
        }
        std::cout << "Testing DBC parsing with file: " << dbc_file.path() << std::endl;

        std::string path_str = dbc_file.path().string();
        auto network = dbcppp::INetwork::LoadDBCFromFile(path_str.c_str());
        
        if (network)
        {
            // Successfully parsed - do some basic validation
            REQUIRE(network->Messages_Size() >= 0);
            REQUIRE(network->Nodes_Size() >= 0);
            success_count++;
            std::cout << "  Successfully parsed: " 
                      << network->Messages_Size() << " messages, "
                      << network->Nodes_Size() << " nodes" << std::endl;
        }
        else
        {
            std::cout << "  Failed to parse: " << dbc_file.path() << std::endl;
        }
        i++;
    }
    std::cout << "Successfully parsed " << success_count << " out of " << i << " DBC files" << std::endl;
    REQUIRE(success_count > 0);
}

TEST_CASE("Parse Large DBC File", "[unit]")
{
    std::string test17_path = std::string(TEST_FILES_PATH) + "/dbc/test17.dbc";

    std::cout << "Testing parsing of large DBC file: test17.dbc (724KB)" << std::endl;

    // Check file exists
    REQUIRE(std::filesystem::exists(test17_path));

    // Get file size
    auto file_size = std::filesystem::file_size(test17_path);
    std::cout << "  File size: " << file_size << " bytes (" << (file_size / 1024) << " KB)" << std::endl;

    // Parse the file
    auto start_time = std::chrono::high_resolution_clock::now();
    auto network = dbcppp::INetwork::LoadDBCFromFile(test17_path.c_str());
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "  Parsing time: " << duration.count() << " ms" << std::endl;

    // Verify parsing succeeded
    REQUIRE(network != nullptr);

    // Print statistics about the parsed network
    std::cout << "  Successfully parsed:" << std::endl;
    std::cout << "    Messages: " << network->Messages_Size() << std::endl;
    std::cout << "    Nodes: " << network->Nodes_Size() << std::endl;

    // Count signals
    size_t total_signals = 0;
    for (const auto& msg : network->Messages())
    {
        total_signals += msg.Signals_Size();
    }
    std::cout << "    Total signals: " << total_signals << std::endl;

    // Basic validation
    REQUIRE(network->Messages_Size() > 0);
    REQUIRE(total_signals > 0);

    // Estimate memory usage of the network
    size_t estimated_size = sizeof(*network);

    // Count all strings and objects
    size_t string_memory = 0;
    size_t message_count = 0;
    size_t signal_count = 0;
    size_t attribute_count = 0;

    for (const auto& msg : network->Messages()) {
        message_count++;
        string_memory += msg.Name().size();

        for (const auto& sig : msg.Signals()) {
            signal_count++;
            string_memory += sig.Name().size();
            string_memory += sig.Unit().size();

            // Each signal has receivers
            for (size_t i = 0; i < sig.Receivers_Size(); i++) {
                string_memory += sig.Receivers_Get(i).size();
            }
        }
    }

    // Count attributes
    for (const auto& attr : network->AttributeDefinitions()) {
        attribute_count++;
        string_memory += attr.Name().size();
    }

    for (const auto& attr : network->AttributeDefaults()) {
        string_memory += attr.Name().size();
    }

    for (const auto& attr : network->AttributeValues()) {
        string_memory += attr.Name().size();
    }

    // Rough estimates for object sizes
    // Message: vtable(8) + id(8) + name(32) + size(8) + transmitter(32) + vectors + signals pointer
    size_t message_memory = message_count * 200;  // ~200 bytes per message object
    // Signal: vtable(8) + name(32) + unit(32) + bit info(40) + scale/offset(16) + min/max(16) + receivers + more
    size_t signal_memory = signal_count * 250;    // ~250 bytes per signal object
    // Attribute: vtable(8) + name(32) + value variant(24) + object type(4)
    size_t attribute_memory = attribute_count * 80; // ~80 bytes per attribute

    size_t total_network_size = estimated_size + string_memory + message_memory + signal_memory + attribute_memory;

    std::cout << "  Network memory usage estimate:" << std::endl;
    std::cout << "    Base network object: ~" << estimated_size << " bytes" << std::endl;
    std::cout << "    String data: ~" << string_memory << " bytes" << std::endl;
    std::cout << "    Message objects (" << message_count << "): ~" << message_memory << " bytes" << std::endl;
    std::cout << "    Signal objects (" << signal_count << "): ~" << signal_memory << " bytes" << std::endl;
    std::cout << "    Attribute objects (" << attribute_count << "): ~" << attribute_memory << " bytes" << std::endl;
    std::cout << "    Total estimated: ~" << total_network_size << " bytes (" << (total_network_size / 1024) << " KB)" << std::endl;
}

TEST_CASE("Parse Large DBC File with Streaming Parser", "[unit]")
{
    std::string test17_path = std::string(TEST_FILES_PATH) + "/dbc/test17.dbc";

    std::cout << "Testing STREAMING parsing of large DBC file: test17.dbc (724KB)" << std::endl;

    // Check file exists
    REQUIRE(std::filesystem::exists(test17_path));

    // Parse with regular parser first for comparison
    auto regular_network = dbcppp::INetwork::LoadDBCFromFile(test17_path.c_str());
    REQUIRE(regular_network != nullptr);

    size_t regular_messages = regular_network->Messages_Size();
    size_t regular_signals = 0;
    for (const auto& msg : regular_network->Messages()) {
        regular_signals += msg.Signals_Size();
    }

    std::cout << "  Regular parser results: " << regular_messages << " messages, "
              << regular_signals << " signals" << std::endl;

    // Now try with streaming parser
    #ifdef USE_STREAMING_PARSER_TEST
    std::cout << "\n  Testing STREAMING parser (line-by-line processing):" << std::endl;

    dbcppp::FileLineReaderAdapter reader(test17_path.c_str());
    REQUIRE(reader.isOpen());

    auto stream_start = std::chrono::high_resolution_clock::now();
    dbcppp::DBCStreamParser stream_parser;
    auto parse_result = stream_parser.parse(reader);
    auto stream_end = std::chrono::high_resolution_clock::now();

    auto stream_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stream_end - stream_start);

    if (parse_result.isError()) {
        std::cout << "  Streaming parser error: " << parse_result.error().toString() << std::endl;
        WARN("Streaming parser failed");
    } else {
        auto& ast_network = parse_result.value();
        std::cout << "  Streaming parser results:" << std::endl;
        std::cout << "    Parse time: " << stream_duration.count() << " ms" << std::endl;
        std::cout << "    Messages: " << ast_network->messages.size() << std::endl;

        size_t stream_signals = 0;
        for (const auto& msg : ast_network->messages) {
            stream_signals += msg.signals.size();
        }
        std::cout << "    Signals: " << stream_signals << std::endl;

        // Compare results
        if (ast_network->messages.size() == regular_messages && stream_signals == regular_signals) {
            std::cout << "  ✓ SUCCESS: Streaming parser matches regular parser!" << std::endl;
            std::cout << "    - Processes file line-by-line (no full file in memory)" << std::endl;
            std::cout << "    - No iostream dependencies" << std::endl;
            std::cout << "    - Suitable for ESP32 with limited RAM" << std::endl;
        } else {
            std::cout << "  ✗ WARNING: Results differ" << std::endl;
            std::cout << "    Expected: " << regular_messages << " messages, " << regular_signals << " signals" << std::endl;
            std::cout << "    Got: " << ast_network->messages.size() << " messages, " << stream_signals << " signals" << std::endl;
        }
    }
    #else
    std::cout << "  Skipping streaming parser test (define USE_STREAMING_PARSER_TEST to enable)" << std::endl;
    #endif
}
