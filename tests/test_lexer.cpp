// Focused lexer tests
#include "../src/DBCLexer.h"
#include <iostream>
#include <cassert>

using namespace dbcppp;

void test_keywords() {
    std::cout << "Testing keywords...\n";
    DBCLexer lexer("VERSION NS_ BS_ BU_ BO_ SG_ CM_ BA_DEF_ BA_ VAL_ VAL_TABLE_ SG_MUL_VAL_");
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].type == TokenType::VERSION);
    assert(tokens[1].type == TokenType::NS_);
    assert(tokens[2].type == TokenType::BS_);
    assert(tokens[3].type == TokenType::BU_);
    assert(tokens[4].type == TokenType::BO_);
    assert(tokens[5].type == TokenType::SG_);
    assert(tokens[6].type == TokenType::CM_);
    assert(tokens[7].type == TokenType::BA_DEF_);
    assert(tokens[8].type == TokenType::BA_);
    assert(tokens[9].type == TokenType::VAL_);
    assert(tokens[10].type == TokenType::VAL_TABLE_);
    assert(tokens[11].type == TokenType::SG_MUL_VAL_);
    
    std::cout << "Keywords test passed!\n";
}

void test_numbers() {
    std::cout << "Testing numbers...\n";
    DBCLexer lexer("123 -456 12.34 -56.78 1.23e4 -5.67e-8 0xFF 0x123ABC");
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].type == TokenType::INTEGER && tokens[0].value == "123");
    assert(tokens[1].type == TokenType::INTEGER && tokens[1].value == "-456");
    assert(tokens[2].type == TokenType::FLOAT && tokens[2].value == "12.34");
    assert(tokens[3].type == TokenType::FLOAT && tokens[3].value == "-56.78");
    assert(tokens[4].type == TokenType::FLOAT && tokens[4].value == "1.23e4");
    assert(tokens[5].type == TokenType::FLOAT && tokens[5].value == "-5.67e-8");
    assert(tokens[6].type == TokenType::INTEGER && tokens[6].value == "0xFF");
    assert(tokens[7].type == TokenType::INTEGER && tokens[7].value == "0x123ABC");
    
    std::cout << "Numbers test passed!\n";
}

void test_strings() {
    std::cout << "Testing strings...\n";
    DBCLexer lexer(R"("hello" "world with spaces" "escaped \" quote" "")");
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].type == TokenType::STRING && tokens[0].value == "hello");
    assert(tokens[1].type == TokenType::STRING && tokens[1].value == "world with spaces");
    assert(tokens[2].type == TokenType::STRING && tokens[2].value == "escaped \" quote");
    assert(tokens[3].type == TokenType::STRING && tokens[3].value == "");
    
    std::cout << "Strings test passed!\n";
}

void test_multiplexer_indicators() {
    std::cout << "Testing multiplexer indicators...\n";
    DBCLexer lexer("M m0 m1 m99 m123 mNotMux mixed123");
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].type == TokenType::MUX_M && tokens[0].value == "M");
    assert(tokens[1].type == TokenType::MUX_m && tokens[1].value == "m0");
    assert(tokens[2].type == TokenType::MUX_m && tokens[2].value == "m1");
    assert(tokens[3].type == TokenType::MUX_m && tokens[3].value == "m99");
    assert(tokens[4].type == TokenType::MUX_m && tokens[4].value == "m123");
    assert(tokens[5].type == TokenType::IDENTIFIER && tokens[5].value == "mNotMux");
    assert(tokens[6].type == TokenType::IDENTIFIER && tokens[6].value == "mixed123");
    
    std::cout << "Multiplexer indicators test passed!\n";
}

void test_special_chars() {
    std::cout << "Testing special characters...\n";
    DBCLexer lexer(": ; , @ + - | ( ) [ ]");
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].type == TokenType::COLON);
    assert(tokens[1].type == TokenType::SEMICOLON);
    assert(tokens[2].type == TokenType::COMMA);
    assert(tokens[3].type == TokenType::AT);
    assert(tokens[4].type == TokenType::PLUS);
    assert(tokens[5].type == TokenType::MINUS);
    assert(tokens[6].type == TokenType::PIPE);
    assert(tokens[7].type == TokenType::LPAREN);
    assert(tokens[8].type == TokenType::RPAREN);
    assert(tokens[9].type == TokenType::LBRACKET);
    assert(tokens[10].type == TokenType::RBRACKET);
    
    std::cout << "Special characters test passed!\n";
}

void test_comments() {
    std::cout << "Testing comments...\n";
    DBCLexer lexer("VERSION // Single line comment\n\"1.0\" /* Block \n comment */ BU_");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 4); // VERSION, "1.0", BU_, EOF
    assert(tokens[0].type == TokenType::VERSION);
    assert(tokens[1].type == TokenType::STRING && tokens[1].value == "1.0");
    assert(tokens[2].type == TokenType::BU_);
    assert(tokens[3].type == TokenType::END_OF_FILE);
    
    std::cout << "Comments test passed!\n";
}

void test_line_tracking() {
    std::cout << "Testing line and column tracking...\n";
    DBCLexer lexer("VERSION\n  \"1.0\"\nBU_ ECU1");
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].line == 1 && tokens[0].column == 1); // VERSION
    assert(tokens[1].line == 2 && tokens[1].column == 3); // "1.0"
    assert(tokens[2].line == 3 && tokens[2].column == 1); // BU_
    assert(tokens[3].line == 3 && tokens[3].column == 5); // ECU1
    
    std::cout << "Line tracking test passed!\n";
}

int main() {
    try {
        test_keywords();
        test_numbers();
        test_strings();
        test_multiplexer_indicators();
        test_special_chars();
        test_comments();
        test_line_tracking();
        
        std::cout << "\nAll lexer tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}