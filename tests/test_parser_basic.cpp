// Basic parser tests
#include "../src/DBCParser.h"
#include <iostream>
#include <cassert>

using namespace dbcppp;

void test_version() {
    std::cout << "Testing VERSION parsing...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(VERSION "1.0.0")");
    assert(result.isOk());
    auto& network = result.value();
    assert(network->version.version == "1.0.0");
    
    result = parser.parse(R"(VERSION "")");
    assert(result.isOk());
    assert(result.value()->version.version == "");
    
    std::cout << "VERSION test passed!\n";
}

void test_nodes() {
    std::cout << "Testing BU_ (nodes) parsing...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2 Gateway TestNode
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->nodes.size() == 4);
    assert(network->nodes[0].name == "ECU1");
    assert(network->nodes[1].name == "ECU2");
    assert(network->nodes[2].name == "Gateway");
    assert(network->nodes[3].name == "TestNode");
    
    std::cout << "Nodes test passed!\n";
}

void test_simple_message() {
    std::cout << "Testing simple message parsing...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2

BO_ 123 TestMessage: 8 ECU1
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->messages.size() == 1);
    auto& msg = network->messages[0];
    assert(msg.id == 123);
    assert(msg.name == "TestMessage");
    assert(msg.size == 8);
    assert(msg.transmitter == "ECU1");
    assert(msg.signals.empty());
    
    std::cout << "Simple message test passed!\n";
}

void test_simple_signal() {
    std::cout << "Testing simple signal parsing...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2

BO_ 100 TestMsg: 8 ECU1
 SG_ TestSignal : 0|16@1+ (1,0) [0|65535] "units" ECU2
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->messages.size() == 1);
    auto& msg = network->messages[0];
    assert(msg.signals.size() == 1);
    
    auto& sig = msg.signals[0];
    assert(sig.name == "TestSignal");
    assert(sig.start_bit == 0);
    assert(sig.length == 16);
    assert(sig.byte_order == '1');  // Intel
    assert(sig.value_type == '+');   // Unsigned
    assert(sig.factor == 1.0);
    assert(sig.offset == 0.0);
    assert(sig.minimum == 0.0);
    assert(sig.maximum == 65535.0);
    assert(sig.unit == "units");
    assert(sig.receivers.size() == 1);
    assert(sig.receivers[0] == "ECU2");
    assert(sig.mux_type == AST::MultiplexerType::None);
    
    std::cout << "Simple signal test passed!\n";
}

void test_signal_formats() {
    std::cout << "Testing various signal formats...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 100 TestMsg: 8 ECU1
 SG_ Intel_Unsigned : 0|16@1+ (1,0) [0|65535] ""
 SG_ Intel_Signed : 16|16@1- (1,0) [-32768|32767] ""
 SG_ Motorola_Unsigned : 32|16@0+ (1,0) [0|65535] ""
 SG_ Motorola_Signed : 48|16@0- (1,0) [-32768|32767] ""
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    auto& sigs = network->messages[0].signals;
    assert(sigs.size() == 4);
    
    assert(sigs[0].byte_order == '1' && sigs[0].value_type == '+');
    assert(sigs[1].byte_order == '1' && sigs[1].value_type == '-');
    assert(sigs[2].byte_order == '0' && sigs[2].value_type == '+');
    assert(sigs[3].byte_order == '0' && sigs[3].value_type == '-');
    
    std::cout << "Signal formats test passed!\n";
}

void test_signal_scaling() {
    std::cout << "Testing signal scaling factors...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 100 TestMsg: 8 ECU1
 SG_ Scaled1 : 0|8@1+ (0.5,10) [10|137.5] "units"
 SG_ Scaled2 : 8|8@1+ (0.1,-20) [-20|5.5] "degC"
 SG_ Scaled3 : 16|16@1- (0.25,-8192) [-16384|8191.75] "rpm"
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    auto& sigs = network->messages[0].signals;
    assert(sigs.size() == 3);
    
    assert(sigs[0].factor == 0.5 && sigs[0].offset == 10);
    assert(sigs[0].minimum == 10 && sigs[0].maximum == 137.5);
    
    assert(sigs[1].factor == 0.1 && sigs[1].offset == -20);
    assert(sigs[1].minimum == -20 && sigs[1].maximum == 5.5);
    
    assert(sigs[2].factor == 0.25 && sigs[2].offset == -8192);
    assert(sigs[2].minimum == -16384 && sigs[2].maximum == 8191.75);
    
    std::cout << "Signal scaling test passed!\n";
}

void test_value_tables() {
    std::cout << "Testing value tables...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

VAL_TABLE_ Gear 0 "Park" 1 "Reverse" 2 "Neutral" 3 "Drive" ;
VAL_TABLE_ OnOff 0 "Off" 1 "On" ;
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->value_tables.size() == 2);
    
    auto& vt1 = network->value_tables[0];
    assert(vt1.name == "Gear");
    assert(vt1.descriptions.size() == 4);
    assert(vt1.descriptions[0].value == 0 && vt1.descriptions[0].description == "Park");
    assert(vt1.descriptions[3].value == 3 && vt1.descriptions[3].description == "Drive");
    
    auto& vt2 = network->value_tables[1];
    assert(vt2.name == "OnOff");
    assert(vt2.descriptions.size() == 2);
    
    std::cout << "Value tables test passed!\n";
}

void test_comments() {
    std::cout << "Testing comments...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 100 TestMsg: 8 ECU1
 SG_ TestSignal : 0|16@1+ (1,0) [0|65535] "" 

CM_ "Network level comment";
CM_ BU_ ECU1 "Node comment";
CM_ BO_ 100 "Message comment";
CM_ SG_ 100 TestSignal "Signal comment";
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->comments.size() == 4);
    
    assert(network->comments[0].type == AST::Comment::Type::Network);
    assert(network->comments[0].text == "Network level comment");
    
    assert(network->comments[1].type == AST::Comment::Type::Node);
    assert(network->comments[1].node_name == "ECU1");
    
    assert(network->comments[2].type == AST::Comment::Type::Message);
    assert(network->comments[2].message_id == 100);
    
    assert(network->comments[3].type == AST::Comment::Type::Signal);
    assert(network->comments[3].signal_name == "TestSignal");
    
    std::cout << "Comments test passed!\n";
}

int main() {
    try {
        test_version();
        test_nodes();
        test_simple_message();
        test_simple_signal();
        test_signal_formats();
        test_signal_scaling();
        test_value_tables();
        test_comments();
        
        std::cout << "\nAll basic parser tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}