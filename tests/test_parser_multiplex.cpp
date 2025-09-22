// Multiplexed signal parser tests
#include "../src/DBCParser.h"
#include <iostream>
#include <cassert>

using namespace dbcppp;

void test_simple_multiplex() {
    std::cout << "Testing simple multiplexed signals...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2

BO_ 200 MuxMsg: 8 ECU1
 SG_ MuxSwitch M : 0|8@1+ (1,0) [0|3] "" ECU2  
 SG_ Signal_A m0 : 8|16@1+ (0.1,0) [0|100] "%" ECU2
 SG_ Signal_B m1 : 8|16@1+ (1,0) [0|65535] "" ECU2
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->messages.size() == 1);
    auto& msg = network->messages[0];
    assert(msg.signals.size() == 3);
    
    // Check mux switch
    assert(msg.signals[0].name == "MuxSwitch");
    assert(msg.signals[0].mux_type == AST::MultiplexerType::MuxSwitch);
    
    // Check muxed signals
    assert(msg.signals[1].name == "Signal_A");
    assert(msg.signals[1].mux_type == AST::MultiplexerType::MuxValue);
    assert(msg.signals[1].mux_value == 0);
    
    assert(msg.signals[2].name == "Signal_B");
    assert(msg.signals[2].mux_type == AST::MultiplexerType::MuxValue);
    assert(msg.signals[2].mux_value == 1);
    
    std::cout << "Simple multiplex test passed!\n";
}

void test_complex_multiplex() {
    std::cout << "Testing complex multiplexed message...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ Gateway ECU1 ECU2

BO_ 300 ComplexMux: 8 Gateway
 SG_ Mode M : 0|3@1+ (1,0) [0|7] "" ECU1 ECU2
 SG_ Common_Signal : 3|5@1+ (1,0) [0|31] "" ECU1
 SG_ Mode0_Speed m0 : 8|16@1+ (0.01,0) [0|655.35] "km/h" ECU1
 SG_ Mode0_Accel m0 : 24|8@1- (0.1,-12.8) [-12.8|12.7] "m/s^2" ECU1
 SG_ Mode1_Temp m1 : 8|8@1+ (1,-40) [-40|215] "degC" ECU2
 SG_ Mode1_Press m1 : 16|16@1+ (0.1,0) [0|6553.5] "kPa" ECU2
 SG_ Mode2_Status m2 : 8|8@1+ (1,0) [0|255] "" ECU1
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    auto& msg = network->messages[0];
    assert(msg.signals.size() == 7);
    
    // Verify mux switch
    assert(msg.signals[0].mux_type == AST::MultiplexerType::MuxSwitch);
    assert(msg.signals[0].start_bit == 0);
    assert(msg.signals[0].length == 3);
    
    // Verify common signal (not muxed)
    assert(msg.signals[1].name == "Common_Signal");
    assert(msg.signals[1].mux_type == AST::MultiplexerType::None);
    
    // Verify mode 0 signals
    assert(msg.signals[2].mux_type == AST::MultiplexerType::MuxValue);
    assert(msg.signals[2].mux_value == 0);
    assert(msg.signals[3].mux_value == 0);
    
    // Verify mode 1 signals
    assert(msg.signals[4].mux_value == 1);
    assert(msg.signals[5].mux_value == 1);
    
    // Verify mode 2 signal
    assert(msg.signals[6].mux_value == 2);
    
    std::cout << "Complex multiplex test passed!\n";
}

void test_multiplex_large_values() {
    std::cout << "Testing multiplexed signals with large mux values...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 400 LargeMux: 8 ECU1
 SG_ MuxId M : 0|16@1+ (1,0) [0|65535] ""
 SG_ Data_0 m0 : 16|8@1+ (1,0) [0|255] ""
 SG_ Data_99 m99 : 16|8@1+ (1,0) [0|255] ""
 SG_ Data_255 m255 : 16|8@1+ (1,0) [0|255] ""
 SG_ Data_1000 m1000 : 16|8@1+ (1,0) [0|255] ""
 SG_ Data_65535 m65535 : 16|8@1+ (1,0) [0|255] ""
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    auto& msg = network->messages[0];
    assert(msg.signals[0].mux_type == AST::MultiplexerType::MuxSwitch);
    assert(msg.signals[0].length == 16); // Can hold values up to 65535
    
    assert(msg.signals[1].mux_value == 0);
    assert(msg.signals[2].mux_value == 99);
    assert(msg.signals[3].mux_value == 255);
    assert(msg.signals[4].mux_value == 1000);
    assert(msg.signals[5].mux_value == 65535);
    
    std::cout << "Large mux values test passed!\n";
}

void test_mixed_signals() {
    std::cout << "Testing mixed muxed and non-muxed signals...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 500 MixedMsg: 8 ECU1
 SG_ Always_Present1 : 0|8@1+ (1,0) [0|255] ""
 SG_ MuxSwitch M : 8|4@1+ (1,0) [0|15] ""
 SG_ Always_Present2 : 12|4@1+ (1,0) [0|15] ""
 SG_ Muxed_A m0 : 16|16@1+ (1,0) [0|65535] ""
 SG_ Muxed_B m1 : 16|8@1+ (1,0) [0|255] ""
 SG_ Muxed_C m1 : 24|8@1+ (1,0) [0|255] ""
 SG_ Always_Present3 : 32|32@1+ (1,0) [0|4294967295] ""
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    auto& msg = network->messages[0];
    
    // Count signal types
    int non_mux = 0, mux_switch = 0, mux_value = 0;
    for (const auto& sig : msg.signals) {
        if (sig.mux_type == AST::MultiplexerType::None) non_mux++;
        else if (sig.mux_type == AST::MultiplexerType::MuxSwitch) mux_switch++;
        else if (sig.mux_type == AST::MultiplexerType::MuxValue) mux_value++;
    }
    
    assert(non_mux == 3);      // Always_Present1/2/3
    assert(mux_switch == 1);   // MuxSwitch
    assert(mux_value == 3);    // Muxed_A/B/C
    
    // Verify multiple signals can share same mux value
    assert(msg.signals[4].mux_value == 1);
    assert(msg.signals[5].mux_value == 1);
    
    std::cout << "Mixed signals test passed!\n";
}

void test_multiplex_bit_positions() {
    std::cout << "Testing multiplexed signal bit positions...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 600 BitTest: 8 ECU1
 SG_ Mux M : 7|1@1+ (1,0) [0|1] ""
 SG_ Mode0_Bits m0 : 0|7@1+ (1,0) [0|127] ""
 SG_ Mode0_Word m0 : 8|16@1+ (1,0) [0|65535] ""
 SG_ Mode1_Byte1 m1 : 0|8@1+ (1,0) [0|255] ""
 SG_ Mode1_Byte2 m1 : 8|8@1+ (1,0) [0|255] ""
 SG_ Mode1_Byte3 m1 : 16|8@1+ (1,0) [0|255] ""
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    auto& msg = network->messages[0];
    
    // Mux at bit 7 (last bit of first byte)
    assert(msg.signals[0].start_bit == 7);
    assert(msg.signals[0].length == 1);
    
    // Mode 0 signals can overlap with mux bit since different modes
    assert(msg.signals[1].start_bit == 0);
    assert(msg.signals[1].length == 7);
    
    std::cout << "Bit positions test passed!\n";
}

void test_extended_multiplexing() {
    std::cout << "Testing extended multiplexing (SG_MUL_VAL_)...\n";
    DBCParser parser;
    
    auto result = parser.parse(R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 700 ExtMux: 8 ECU1
 SG_ Level1_Mux M : 0|8@1+ (1,0) [0|255] ""
 SG_ Level2_Mux m0 M : 8|8@1+ (1,0) [0|255] ""
 SG_ Data m0 : 16|16@1+ (1,0) [0|65535] ""

SG_MUL_VAL_ 700 Data Level2_Mux 0-10, 20-30, 40-40 ;
)");
    
    assert(result.isOk());
    auto& network = result.value();
    
    assert(network->signal_multiplexer_values.size() == 1);
    auto& smv = network->signal_multiplexer_values[0];
    
    assert(smv.message_id == 700);
    assert(smv.signal_name == "Data");
    assert(smv.switch_name == "Level2_Mux");
    assert(smv.value_ranges.size() == 3);
    
    assert(smv.value_ranges[0].from == 0 && smv.value_ranges[0].to == 10);
    assert(smv.value_ranges[1].from == 20 && smv.value_ranges[1].to == 30);
    assert(smv.value_ranges[2].from == 40 && smv.value_ranges[2].to == 40);
    
    std::cout << "Extended multiplexing test passed!\n";
}

int main() {
    try {
        test_simple_multiplex();
        test_complex_multiplex();
        test_multiplex_large_values();
        test_mixed_signals();
        test_multiplex_bit_positions();
        test_extended_multiplexing();
        
        std::cout << "\nAll multiplex parser tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}