#include "catch2.h"
#include "../src/dbc_parser.h"
#include <sstream>
#include <fstream>
#include "config.h"

using namespace dbcppp;

TEST_CASE("DBCLexer Basic Token Recognition", "[lexer]") {
    SECTION("Keywords") {
        DBCLexer lexer("VERSION NS_ BS_ BU_ BO_ SG_ CM_ BA_DEF_ BA_ VAL_ VAL_TABLE_");
        auto tokens = lexer.tokenize();
        
        REQUIRE(tokens[0].type == TokenType::VERSION);
        REQUIRE(tokens[1].type == TokenType::NS_);
        REQUIRE(tokens[2].type == TokenType::BS_);
        REQUIRE(tokens[3].type == TokenType::BU_);
        REQUIRE(tokens[4].type == TokenType::BO_);
        REQUIRE(tokens[5].type == TokenType::SG_);
        REQUIRE(tokens[6].type == TokenType::CM_);
        REQUIRE(tokens[7].type == TokenType::BA_DEF_);
        REQUIRE(tokens[8].type == TokenType::BA_);
        REQUIRE(tokens[9].type == TokenType::VAL_);
        REQUIRE(tokens[10].type == TokenType::VAL_TABLE_);
    }
    
    SECTION("Numbers") {
        DBCLexer lexer("123 -456 12.34 -56.78 1.23e4 -5.67e-8 0xFF 0x123ABC");
        auto tokens = lexer.tokenize();
        
        REQUIRE(tokens[0].type == TokenType::INTEGER);
        REQUIRE(tokens[0].value == "123");
        
        REQUIRE(tokens[1].type == TokenType::INTEGER);
        REQUIRE(tokens[1].value == "-456");
        
        REQUIRE(tokens[2].type == TokenType::FLOAT);
        REQUIRE(tokens[2].value == "12.34");
        
        REQUIRE(tokens[3].type == TokenType::FLOAT);
        REQUIRE(tokens[3].value == "-56.78");
        
        REQUIRE(tokens[4].type == TokenType::FLOAT);
        REQUIRE(tokens[4].value == "1.23e4");
        
        REQUIRE(tokens[5].type == TokenType::FLOAT);
        REQUIRE(tokens[5].value == "-5.67e-8");
        
        REQUIRE(tokens[6].type == TokenType::INTEGER);
        REQUIRE(tokens[6].value == "0xFF");
        
        REQUIRE(tokens[7].type == TokenType::INTEGER);
        REQUIRE(tokens[7].value == "0x123ABC");
    }
    
    SECTION("Strings") {
        DBCLexer lexer(R"("hello" "world with spaces" "escaped \" quote" "empty:")");
        auto tokens = lexer.tokenize();
        
        REQUIRE(tokens[0].type == TokenType::STRING);
        REQUIRE(tokens[0].value == "hello");
        
        REQUIRE(tokens[1].type == TokenType::STRING);
        REQUIRE(tokens[1].value == "world with spaces");
        
        REQUIRE(tokens[2].type == TokenType::STRING);
        REQUIRE(tokens[2].value == "escaped \" quote");
        
        REQUIRE(tokens[3].type == TokenType::STRING);
        REQUIRE(tokens[3].value == "empty:");
    }
    
    SECTION("Identifiers and Multiplexer Indicators") {
        DBCLexer lexer("ECU1 Signal_Name M m0 m123 m999");
        auto tokens = lexer.tokenize();
        
        REQUIRE(tokens[0].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[0].value == "ECU1");
        
        REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[1].value == "Signal_Name");
        
        // Note: "M" is now treated as IDENTIFIER, not MUX_M
        // The parser handles context to determine if it's a multiplexer
        REQUIRE(tokens[2].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[2].value == "M");
        
        REQUIRE(tokens[3].type == TokenType::MUX_m);
        REQUIRE(tokens[3].value == "m0");
        
        REQUIRE(tokens[4].type == TokenType::MUX_m);
        REQUIRE(tokens[4].value == "m123");
        
        REQUIRE(tokens[5].type == TokenType::MUX_m);
        REQUIRE(tokens[5].value == "m999");
    }
    
    SECTION("Special Characters") {
        DBCLexer lexer(": ; , @ + - | ( ) [ ]");
        auto tokens = lexer.tokenize();
        
        REQUIRE(tokens[0].type == TokenType::COLON);
        REQUIRE(tokens[1].type == TokenType::SEMICOLON);
        REQUIRE(tokens[2].type == TokenType::COMMA);
        REQUIRE(tokens[3].type == TokenType::AT);
        REQUIRE(tokens[4].type == TokenType::PLUS);
        REQUIRE(tokens[5].type == TokenType::MINUS);
        REQUIRE(tokens[6].type == TokenType::PIPE);
        REQUIRE(tokens[7].type == TokenType::LPAREN);
        REQUIRE(tokens[8].type == TokenType::RPAREN);
        REQUIRE(tokens[9].type == TokenType::LBRACKET);
        REQUIRE(tokens[10].type == TokenType::RBRACKET);
    }
    
    SECTION("Comments") {
        DBCLexer lexer("VERSION // Single line comment\n\"1.0\" /* Block \n comment */ BU_");
        auto tokens = lexer.tokenize();
        
        REQUIRE(tokens[0].type == TokenType::VERSION);
        REQUIRE(tokens[1].type == TokenType::STRING);
        REQUIRE(tokens[1].value == "1.0");
        REQUIRE(tokens[2].type == TokenType::BU_);
    }
}

TEST_CASE("DBCParser Basic Elements", "[parser]") {
    SECTION("Version") {
        std::string dbc = R"(
VERSION "1.0.0"
NS_ :
BS_:
BU_
)";
        DBCParser parser;
        auto result = parser.parse(dbc);
        REQUIRE(result.isOk());
        auto& network = result.value();
        REQUIRE(network->version.version == "1.0.0");
    }
    
    SECTION("Nodes") {
        std::string dbc = R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2 Gateway
)";
        DBCParser parser;
        auto result = parser.parse(dbc);
        REQUIRE(result.isOk());
        auto& network = result.value();
        REQUIRE(network->nodes.size() == 3);
        REQUIRE(network->nodes[0].name == "ECU1");
        REQUIRE(network->nodes[1].name == "ECU2");
        REQUIRE(network->nodes[2].name == "Gateway");
    }
    
    SECTION("Simple Message and Signal") {
        std::string dbc = R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2

BO_ 100 TestMsg: 8 ECU1
 SG_ TestSignal : 0|16@1+ (1,0) [0|65535] "units" ECU2
)";
        DBCParser parser;
        auto result = parser.parse(dbc);
        REQUIRE(result.isOk());
        auto& network = result.value();
        REQUIRE(network->messages.size() == 1);
        auto& msg = network->messages[0];
        REQUIRE(msg.id == 100);
        REQUIRE(msg.name == "TestMsg");
        REQUIRE(msg.size == 8);
        REQUIRE(msg.transmitter == "ECU1");
        
        REQUIRE(msg.signals.size() == 1);
        auto& sig = msg.signals[0];
        REQUIRE(sig.name == "TestSignal");
        REQUIRE(sig.start_bit == 0);
        REQUIRE(sig.length == 16);
        REQUIRE(sig.byte_order == '1');
        REQUIRE(sig.value_type == '+');
        REQUIRE(sig.factor == 1.0);
        REQUIRE(sig.offset == 0.0);
        REQUIRE(sig.minimum == 0.0);
        REQUIRE(sig.maximum == 65535.0);
        REQUIRE(sig.unit == "units");
        REQUIRE(sig.receivers.size() == 1);
        REQUIRE(sig.receivers[0] == "ECU2");
    }
}

TEST_CASE("DBCParser Multiplexed Signals", "[parser][multiplex]") {
    std::string dbc = R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2

BO_ 200 MuxMsg: 8 ECU1
 SG_ MuxSwitch M : 0|8@1+ (1,0) [0|3] "" ECU2  
 SG_ Signal_A m0 : 8|16@1+ (0.1,0) [0|100] "%" ECU2
 SG_ Signal_B m1 : 8|16@1+ (1,-10) [0|65535] "" ECU2
 SG_ Signal_C m2 : 8|8@1- (1,0) [-128|127] "" ECU2
)";
    DBCParser parser;
    auto result = parser.parse(dbc);
    REQUIRE(result.isOk());
    auto& network = result.value();
    
    REQUIRE(network->messages.size() == 1);
    auto& msg = network->messages[0];
    REQUIRE(msg.signals.size() == 4);
    
    // Check multiplexer switch
    auto& mux_switch = msg.signals[0];
    REQUIRE(mux_switch.name == "MuxSwitch");
    REQUIRE(mux_switch.mux_type == AST::MultiplexerType::MuxSwitch);
    
    // Check multiplexed signals
    auto& sig_a = msg.signals[1];
    REQUIRE(sig_a.name == "Signal_A");
    REQUIRE(sig_a.mux_type == AST::MultiplexerType::MuxValue);
    REQUIRE(sig_a.mux_value == 0);
    
    auto& sig_b = msg.signals[2];
    REQUIRE(sig_b.name == "Signal_B");
    REQUIRE(sig_b.mux_type == AST::MultiplexerType::MuxValue);
    REQUIRE(sig_b.mux_value == 1);
    REQUIRE(sig_b.offset == -10.0);
    
    auto& sig_c = msg.signals[3];
    REQUIRE(sig_c.name == "Signal_C");
    REQUIRE(sig_c.mux_type == AST::MultiplexerType::MuxValue);
    REQUIRE(sig_c.mux_value == 2);
    REQUIRE(sig_c.value_type == '-'); // Signed
}

TEST_CASE("DBCParser Value Tables", "[parser]") {
    std::string dbc = R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

VAL_TABLE_ GearPosition 0 "Park" 1 "Reverse" 2 "Neutral" 3 "Drive" ;
VAL_TABLE_ DoorStatus 0 "Closed" 1 "Open" ;
)";
    DBCParser parser;
    auto result = parser.parse(dbc);
    REQUIRE(result.isOk());
    auto& network = result.value();
    
    REQUIRE(network->value_tables.size() == 2);
    
    auto& vt1 = network->value_tables[0];
    REQUIRE(vt1.name == "GearPosition");
    REQUIRE(vt1.descriptions.size() == 4);
    REQUIRE(vt1.descriptions[0].value == 0);
    REQUIRE(vt1.descriptions[0].description == "Park");
    REQUIRE(vt1.descriptions[3].value == 3);
    REQUIRE(vt1.descriptions[3].description == "Drive");
    
    auto& vt2 = network->value_tables[1];
    REQUIRE(vt2.name == "DoorStatus");
    REQUIRE(vt2.descriptions.size() == 2);
}

TEST_CASE("DBCParser Comments", "[parser]") {
    std::string dbc = R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1 ECU2

BO_ 100 TestMsg: 8 ECU1
 SG_ TestSignal : 0|16@1+ (1,0) [0|65535] "" ECU2

CM_ "This is a network comment";
CM_ BU_ ECU1 "First ECU";
CM_ BO_ 100 "Test message";
CM_ SG_ 100 TestSignal "Test signal comment";
)";
    DBCParser parser;
    auto result = parser.parse(dbc);
    REQUIRE(result.isOk());
    auto& network = result.value();
    
    REQUIRE(network->comments.size() == 4);
    
    auto& c1 = network->comments[0];
    REQUIRE(c1.type == AST::Comment::Type::Network);
    REQUIRE(c1.text == "This is a network comment");
    
    auto& c2 = network->comments[1];
    REQUIRE(c2.type == AST::Comment::Type::Node);
    REQUIRE(c2.node_name == "ECU1");
    REQUIRE(c2.text == "First ECU");
    
    auto& c3 = network->comments[2];
    REQUIRE(c3.type == AST::Comment::Type::Message);
    REQUIRE(c3.message_id == 100);
    REQUIRE(c3.text == "Test message");
    
    auto& c4 = network->comments[3];
    REQUIRE(c4.type == AST::Comment::Type::Signal);
    REQUIRE(c4.message_id == 100);
    REQUIRE(c4.signal_name == "TestSignal");
    REQUIRE(c4.text == "Test signal comment");
}

TEST_CASE("DBCParser Signal Byte Order and Sign", "[parser]") {
    std::string dbc = R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 100 TestMsg: 8 ECU1
 SG_ Intel_Unsigned : 0|16@1+ (1,0) [0|65535] ""
 SG_ Intel_Signed : 16|16@1- (1,0) [-32768|32767] ""
 SG_ Motorola_Unsigned : 32|16@0+ (1,0) [0|65535] ""
 SG_ Motorola_Signed : 48|16@0- (1,0) [-32768|32767] ""
)";
    DBCParser parser;
    auto result = parser.parse(dbc);
    REQUIRE(result.isOk());
    auto& network = result.value();
    
    REQUIRE(network->messages[0].signals.size() == 4);
    auto& sigs = network->messages[0].signals;
    
    REQUIRE(sigs[0].byte_order == '1'); // Intel
    REQUIRE(sigs[0].value_type == '+'); // Unsigned
    
    REQUIRE(sigs[1].byte_order == '1'); // Intel
    REQUIRE(sigs[1].value_type == '-'); // Signed
    
    REQUIRE(sigs[2].byte_order == '0'); // Motorola
    REQUIRE(sigs[2].value_type == '+'); // Unsigned
    
    REQUIRE(sigs[3].byte_order == '0'); // Motorola
    REQUIRE(sigs[3].value_type == '-'); // Signed
}

TEST_CASE("DBCParser Error Handling", "[parser][error]") {
    SECTION("Missing version") {
        std::string dbc = "BU_ ECU1";
        DBCParser parser;
        auto result = parser.parse(dbc);
        REQUIRE(result.isError());
    }
    
    SECTION("Invalid signal format") {
        std::string dbc = R"(
VERSION ""
NS_ :
BS_:
BU_ ECU1

BO_ 100 Msg: 8 ECU1
 SG_ BadSignal : 0|16@1 (1,0) [0|100] ""
)";
        DBCParser parser;
        auto result = parser.parse(dbc);
        REQUIRE(result.isError());
    }
    
    SECTION("Parse error contains line and column") {
        std::string dbc = "VERSION \"1.0\"\nINVALID_TOKEN"; // Invalid token after version
        DBCParser parser;
        auto result = parser.parse(dbc);
        REQUIRE(result.isError());
        REQUIRE(result.error().line > 0);
        REQUIRE(result.error().column > 0);
    }
}

TEST_CASE("DBCParser Complex DBC", "[parser][integration]") {
    std::string dbc = R"(
VERSION "1.0"


NS_ : 
	NS_DESC_
	CM_
	BA_DEF_
	BA_
	VAL_
	BA_DEF_DEF_

BS_: 500000 : 0,0

BU_ ECU1 ECU2 Gateway

VAL_TABLE_ Gear 0 "P" 1 "R" 2 "N" 3 "D" 4 "S" ;

BO_ 100 EngineData: 8 ECU1
 SG_ EngineSpeed : 0|16@1+ (0.25,0) [0|16383.75] "rpm" ECU2 Gateway
 SG_ EngineTemp : 16|8@1+ (1,-40) [-40|215] "degC" ECU2
 SG_ ThrottlePos : 24|8@1+ (0.4,0) [0|102] "%" Gateway

BO_ 200 TransmissionData: 8 Gateway
 SG_ GearSelector M : 0|3@1+ (1,0) [0|5] "" ECU1 ECU2
 SG_ VehicleSpeed m0 : 8|16@1+ (0.01,0) [0|655.35] "km/h" ECU1
 SG_ ClutchPedal m1 : 8|8@1+ (0.4,0) [0|102] "%" ECU1
 SG_ GearEngaged : 32|8@1+ (1,0) [0|6] "" ECU1

CM_ "Example DBC with various features";
CM_ BU_ ECU1 "Engine Control Unit";
CM_ BU_ Gateway "Central Gateway";
CM_ BO_ 100 "Engine operating parameters";
CM_ SG_ 100 EngineSpeed "Current engine RPM";

VAL_ 200 GearSelector 0 "P" 1 "R" 2 "N" 3 "D" 4 "S" ;
VAL_ 200 GearEngaged 0 "None" 1 "1st" 2 "2nd" 3 "3rd" 4 "4th" 5 "5th" 6 "6th" ;

BA_DEF_ "BusType" STRING ;
BA_DEF_ BO_ "GenMsgCycleTime" INT 0 3600000;
BA_DEF_ SG_ "GenSigStartValue" FLOAT -100000000000 100000000000;

BA_ "BusType" "CAN";
BA_ "GenMsgCycleTime" BO_ 100 100;
BA_ "GenMsgCycleTime" BO_ 200 50;
BA_ "GenSigStartValue" SG_ 100 EngineSpeed 0;
)";
    
    DBCParser parser;
    auto result = parser.parse(dbc);
    REQUIRE(result.isOk());
    auto& network = result.value();
    
    // Verify basic structure
    REQUIRE(network->version.version == "1.0");
    REQUIRE(network->nodes.size() == 3);
    REQUIRE(network->messages.size() == 2);
    REQUIRE(network->value_tables.size() == 1);
    REQUIRE(network->comments.size() == 5);
    
    // Verify attributes
    REQUIRE(network->attribute_definitions.size() == 3);
    REQUIRE(network->attribute_values.size() == 4);
    
    // Verify multiplexed message
    auto& trans_msg = network->messages[1];
    REQUIRE(trans_msg.signals[0].mux_type == AST::MultiplexerType::MuxSwitch);
    REQUIRE(trans_msg.signals[1].mux_type == AST::MultiplexerType::MuxValue);
    REQUIRE(trans_msg.signals[2].mux_type == AST::MultiplexerType::MuxValue);
    REQUIRE(trans_msg.signals[3].mux_type == AST::MultiplexerType::None);
}

TEST_CASE("DBCParser Real DBC Files", "[parser][files]") {
    auto testFile = [](const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            INFO("Test file not found: " + filename);
            REQUIRE(false);
            return;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        DBCParser parser;
        auto result = parser.parse(buffer.str());
        if (!result.isOk()) {
            INFO("Parse error in " << filename << " at line " << result.error().line << ", column " << result.error().column);
            FAIL("Failed to parse DBC file");
        }
        auto& network = result.value();
        // Basic validation
        REQUIRE(network != nullptr);
        REQUIRE(network->messages.size() > 0);
    };
    
    SECTION("Test.dbc") {
        testFile(std::string(TEST_FILES_PATH) + "/dbc/Test.dbc");
    }
    
    SECTION("multiplex.dbc") {
        testFile(std::string(TEST_FILES_PATH) + "/dbc/multiplex.dbc");
    }
    
    SECTION("attributes.dbc") {
        testFile(std::string(TEST_FILES_PATH) + "/dbc/attributes.dbc");
    }
}