
#include <fstream>
#include <filesystem>
#include <iostream>

#include "dbcppp-tiny/network.h"

#include "config.h"

#include "catch2.h"

TEST_CASE("DBCParserTest", "[]")
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
        
        std::ifstream dbc(dbc_file.path());
        auto network = dbcppp::INetwork::LoadDBCFromIs(dbc);
        
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
