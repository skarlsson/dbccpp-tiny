// Attribute parser tests
#include "../src/DBCParser.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace dbcppp;

void test_attribute_definitions() {
    std::cout << "Testing attribute definitions...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BA_DEF_ "BusType" STRING ;
BA_DEF_ "DataRate" INT 125000 1000000;
BA_DEF_ "FloatAttr" FLOAT -100.5 100.5;
BA_DEF_ "NetVersion" HEX 0 255;
BA_DEF_ BO_ "GenMsgCycleTime" INT 0 3600000;
BA_DEF_ SG_ "GenSigStartValue" FLOAT -100000 100000;
BA_DEF_ BU_ "NodeAddress" INT 0 63;
BA_DEF_ "BusMode" ENUM "CAN", "CAN-FD", "LIN";
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->attribute_definitions.size() == 8);
    
    // Check string attribute
    auto& attr0 = network->attribute_definitions[0];
    assert(attr0.name == "BusType");
    assert(attr0.value_type == "STRING");
    assert(attr0.object_type == AST::AttributeDefinition::ObjectType::Network);
    
    // Check int with range
    auto& attr1 = network->attribute_definitions[1];
    assert(attr1.name == "DataRate");
    assert(attr1.value_type == "INT");
    assert(attr1.min_value.has_value() && *attr1.min_value == 125000);
    assert(attr1.max_value.has_value() && *attr1.max_value == 1000000);
    
    // Check float with range
    auto& attr2 = network->attribute_definitions[2];
    assert(attr2.value_type == "FLOAT");
    assert(attr2.min_value.has_value() && std::abs(*attr2.min_value - (-100.5)) < 0.001);
    
    // Check object-specific attributes
    assert(network->attribute_definitions[4].object_type == AST::AttributeDefinition::ObjectType::Message);
    assert(network->attribute_definitions[5].object_type == AST::AttributeDefinition::ObjectType::Signal);
    assert(network->attribute_definitions[6].object_type == AST::AttributeDefinition::ObjectType::Node);
    
    // Check enum
    auto& attr7 = network->attribute_definitions[7];
    assert(attr7.name == "BusMode");
    assert(attr7.value_type == "ENUM");
    assert(attr7.enum_values.size() == 3);
    assert(attr7.enum_values[0] == "CAN");
    assert(attr7.enum_values[2] == "LIN");
    
    std::cout << "Attribute definitions test passed!\n";
}

void test_attribute_values() {
    std::cout << "Testing attribute values...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 Gateway

BO_ 100 TestMsg: 8 ECU1
 SG_ TestSignal : 0|8@1+ (1,0) [0|255] ""

BA_DEF_ "BusType" STRING ;
BA_DEF_ "DataRate" INT 0 1000000;
BA_DEF_ BO_ "GenMsgCycleTime" INT 0 3600000;
BA_DEF_ SG_ "GenSigStartValue" FLOAT -100000 100000;
BA_DEF_ BU_ "NodeType" STRING ;

BA_ "BusType" "CAN-FD";
BA_ "DataRate" 500000;
BA_ "GenMsgCycleTime" BO_ 100 20;
BA_ "GenSigStartValue" SG_ 100 TestSignal 127.5;
BA_ "NodeType" BU_ ECU1 "PowerTrain";
BA_ "NodeType" BU_ Gateway "Communication";
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->attribute_values.size() == 6);
    
    // Check network attributes
    auto& attr0 = network->attribute_values[0];
    assert(attr0.type == AST::AttributeValue_t::Type::Network);
    assert(attr0.attribute_name == "BusType");
    assert(std::get<std::string>(attr0.value) == "CAN-FD");
    
    auto& attr1 = network->attribute_values[1];
    assert(attr1.type == AST::AttributeValue_t::Type::Network);
    assert(std::get<int64_t>(attr1.value) == 500000);
    
    // Check message attribute
    auto& attr2 = network->attribute_values[2];
    assert(attr2.type == AST::AttributeValue_t::Type::Message);
    assert(attr2.message_id == 100);
    assert(std::get<int64_t>(attr2.value) == 20);
    
    // Check signal attribute
    auto& attr3 = network->attribute_values[3];
    assert(attr3.type == AST::AttributeValue_t::Type::Signal);
    assert(attr3.message_id == 100);
    assert(attr3.signal_name == "TestSignal");
    assert(std::abs(std::get<double>(attr3.value) - 127.5) < 0.001);
    
    // Check node attributes
    auto& attr4 = network->attribute_values[4];
    assert(attr4.type == AST::AttributeValue_t::Type::Node);
    assert(attr4.node_name == "ECU1");
    assert(std::get<std::string>(attr4.value) == "PowerTrain");
    
    std::cout << "Attribute values test passed!\n";
}

// Environment variables test removed - feature removed from library
/*
void test_environment_variables() {
    // Test removed - environment variables feature was removed for embedded version
}
*/

void test_message_transmitters() {
    std::cout << "Testing message transmitters...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2 Gateway

BO_ 100 Msg1: 8 ECU1
BO_ 200 Msg2: 8 ECU2
BO_ 300 Msg3: 8 Gateway

BO_TX_BU_ 100 : ECU2, Gateway;
BO_TX_BU_ 200 : ECU1;
BO_TX_BU_ 300 : ECU1, ECU2, Gateway;
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->message_transmitters.size() == 3);
    
    auto& mt0 = network->message_transmitters[0];
    assert(mt0.message_id == 100);
    assert(mt0.transmitters.size() == 2);
    assert(mt0.transmitters[0] == "ECU2");
    assert(mt0.transmitters[1] == "Gateway");
    
    auto& mt2 = network->message_transmitters[2];
    assert(mt2.message_id == 300);
    assert(mt2.transmitters.size() == 3);
    
    std::cout << "Message transmitters test passed!\n";
}

void test_complex_attributes() {
    std::cout << "Testing complex attribute scenario...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ Master Slave1 Slave2

BO_ 0x100 StatusMsg: 4 Master
 SG_ Status : 0|8@1+ (1,0) [0|255] "" Slave1 Slave2
 SG_ Counter : 8|8@1+ (1,0) [0|255] "" Slave1
 SG_ Checksum : 16|16@1+ (1,0) [0|65535] "" Slave1 Slave2

BA_DEF_ "DatabaseVersion" STRING ;
BA_DEF_ "BusSpeed" INT 125000 1000000;
BA_DEF_ BO_ "GenMsgCycleTime" INT 10 10000;
BA_DEF_ BO_ "GenMsgSendType" ENUM "cyclic", "spontaneous", "cyclicAndSpontaneous";
BA_DEF_ SG_ "GenSigInactiveValue" INT 0 255;
BA_DEF_ BU_ "NodeLayerModules" STRING ;

BA_ "DatabaseVersion" "v2.1.0";
BA_ "BusSpeed" 500000;

BA_ "GenMsgCycleTime" BO_ 0x100 100;
BA_ "GenMsgSendType" BO_ 0x100 "cyclic";

BA_ "GenSigInactiveValue" SG_ 0x100 Status 0xFF;
BA_ "GenSigInactiveValue" SG_ 0x100 Counter 0;

BA_ "NodeLayerModules" BU_ Master "NM,TP,DIAG";
BA_ "NodeLayerModules" BU_ Slave1 "NM";
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    // Verify all elements parsed
    assert(network->messages.size() == 1);
    assert(network->attribute_definitions.size() == 6);
    assert(network->attribute_values.size() == 8);
    
    // Check hex message ID handling
    assert(network->messages[0].id == 0x100);
    
    // Verify enum attribute
    auto enum_attr = std::find_if(network->attribute_definitions.begin(), 
                                  network->attribute_definitions.end(),
                                  [](const auto& def) { return def.name == "GenMsgSendType"; });
    assert(enum_attr != network->attribute_definitions.end());
    assert(enum_attr->value_type == "ENUM");
    assert(enum_attr->enum_values.size() == 3);
    
    std::cout << "Complex attributes test passed!\n";
}

int main() {
    try {
        test_attribute_definitions();
        test_attribute_values();
        // test_environment_variables(); // Removed - environment variables feature removed
        test_message_transmitters();
        test_complex_attributes();
        
        std::cout << "\nAll attribute parser tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}