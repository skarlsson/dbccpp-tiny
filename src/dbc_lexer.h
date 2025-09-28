#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <cstdint>
#include <algorithm>
#include <cctype>

namespace dbcppp {

// Token types for DBC parsing
enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    IDENTIFIER,
    
    // Keywords
    VERSION,
    NS_,
    NS_DESC_,
    BS_,
    BU_,
    BO_,
    SG_,
    CM_,
    BA_DEF_,
    BA_DEF_DEF_,
    BA_,
    VAL_,
    VAL_TABLE_,
    SIG_GROUP_,
    SIG_VALTYPE_,
    BO_TX_BU_,
    CAT_DEF_,
    CAT_,
    FILTER,
    EV_DATA_,
    ENVVAR_DATA_,
    SGTYPE_,
    SGTYPE_VAL_,
    BA_DEF_SGTYPE_,
    BA_SGTYPE_,
    SIG_TYPE_REF_,
    SIGTYPE_VALTYPE_,
    BA_DEF_REL_,
    BA_REL_,
    BA_DEF_DEF_REL_,
    BU_SG_REL_,
    BU_EV_REL_,
    BU_BO_REL_,
    SG_MUL_VAL_,
    EV_,
    
    // Special symbols
    COLON,
    SEMICOLON,
    COMMA,
    AT,
    PLUS,
    MINUS,
    PIPE,          // |
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    
    // Multiplexer indicators
    MUX_M,         // M
    MUX_m,         // m followed by number
    
    // End of file
    END_OF_FILE,
    
    // Error
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType t, const std::string& v, size_t l, size_t c)
        : type(t), value(v), line(l), column(c) {}
};

class DBCLexer {
private:
    std::string input_;
    size_t pos_;
    size_t line_;
    size_t column_;
    
    char peek() const {
        if (pos_ >= input_.size()) return '\0';
        return input_[pos_];
    }
    
    char peek(size_t offset) const {
        if (pos_ + offset >= input_.size()) return '\0';
        return input_[pos_ + offset];
    }
    
    char advance() {
        if (pos_ >= input_.size()) return '\0';
        char ch = input_[pos_++];
        if (ch == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        return ch;
    }
    
    void skipWhitespace() {
        while (std::isspace(peek())) {
            advance();
        }
    }
    
    void skipComment() {
        if (peek() == '/' && peek(1) == '/') {
            // Single line comment
            advance(); advance();
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
        } else if (peek() == '/' && peek(1) == '*') {
            // Block comment
            advance(); advance();
            while (peek() != '\0') {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
        }
    }
    
    Token readNumber() {
        size_t start_line = line_;
        size_t start_col = column_;
        std::string value;
        bool is_float = false;
        bool is_hex = false;
        
        // Check for hex
        if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
            value += advance(); // 0
            value += advance(); // x
            is_hex = true;
            while (std::isxdigit(peek())) {
                value += advance();
            }
        } else {
            // Check for negative
            if (peek() == '-') {
                value += advance();
            }
            
            // Read digits
            while (std::isdigit(peek())) {
                value += advance();
            }
            
            // Check for decimal point
            if (peek() == '.' && std::isdigit(peek(1))) {
                is_float = true;
                value += advance(); // .
                while (std::isdigit(peek())) {
                    value += advance();
                }
            }
            
            // Check for scientific notation
            if ((peek() == 'e' || peek() == 'E') && 
                (std::isdigit(peek(1)) || 
                 ((peek(1) == '+' || peek(1) == '-') && std::isdigit(peek(2))))) {
                is_float = true;
                value += advance(); // e/E
                if (peek() == '+' || peek() == '-') {
                    value += advance();
                }
                while (std::isdigit(peek())) {
                    value += advance();
                }
            }
        }
        
        return Token(is_float ? TokenType::FLOAT : TokenType::INTEGER, value, start_line, start_col);
    }
    
    Token readString() {
        size_t start_line = line_;
        size_t start_col = column_;
        std::string value;
        
        advance(); // Skip opening quote
        
        while (peek() != '"' && peek() != '\0') {
            if (peek() == '\\' && peek(1) == '"') {
                advance(); // Skip backslash
                value += advance(); // Add quote
            } else if (peek() == '\\' && peek(1) == '\\') {
                advance(); // Skip backslash
                value += advance(); // Add backslash
            } else {
                value += advance();
            }
        }
        
        if (peek() == '"') {
            advance(); // Skip closing quote
        }
        
        return Token(TokenType::STRING, value, start_line, start_col);
    }
    
    Token readIdentifier() {
        size_t start_line = line_;
        size_t start_col = column_;
        std::string value;
        
        // First character must be letter or underscore
        if (std::isalpha(peek()) || peek() == '_') {
            value += advance();
        }
        
        // Subsequent characters can be letters, digits, or underscore
        while (std::isalnum(peek()) || peek() == '_') {
            value += advance();
        }
        
        // Check for keywords
        TokenType type = TokenType::IDENTIFIER;
        if (value == "VERSION") type = TokenType::VERSION;
        else if (value == "NS_") type = TokenType::NS_;
        else if (value == "NS_DESC_") type = TokenType::NS_DESC_;
        else if (value == "BS_") type = TokenType::BS_;
        else if (value == "BU_") type = TokenType::BU_;
        else if (value == "BO_") type = TokenType::BO_;
        else if (value == "SG_") type = TokenType::SG_;
        else if (value == "CM_") type = TokenType::CM_;
        else if (value == "BA_DEF_") type = TokenType::BA_DEF_;
        else if (value == "BA_DEF_DEF_") type = TokenType::BA_DEF_DEF_;
        else if (value == "BA_") type = TokenType::BA_;
        else if (value == "VAL_") type = TokenType::VAL_;
        else if (value == "VAL_TABLE_") type = TokenType::VAL_TABLE_;
        else if (value == "SIG_GROUP_") type = TokenType::SIG_GROUP_;
        else if (value == "SIG_VALTYPE_") type = TokenType::SIG_VALTYPE_;
        else if (value == "BO_TX_BU_") type = TokenType::BO_TX_BU_;
        else if (value == "CAT_DEF_") type = TokenType::CAT_DEF_;
        else if (value == "CAT_") type = TokenType::CAT_;
        else if (value == "FILTER") type = TokenType::FILTER;
        else if (value == "EV_DATA_") type = TokenType::EV_DATA_;
        else if (value == "ENVVAR_DATA_") type = TokenType::ENVVAR_DATA_;
        else if (value == "SGTYPE_") type = TokenType::SGTYPE_;
        else if (value == "SGTYPE_VAL_") type = TokenType::SGTYPE_VAL_;
        else if (value == "BA_DEF_SGTYPE_") type = TokenType::BA_DEF_SGTYPE_;
        else if (value == "BA_SGTYPE_") type = TokenType::BA_SGTYPE_;
        else if (value == "SIG_TYPE_REF_") type = TokenType::SIG_TYPE_REF_;
        else if (value == "SIGTYPE_VALTYPE_") type = TokenType::SIGTYPE_VALTYPE_;
        else if (value == "BA_DEF_REL_") type = TokenType::BA_DEF_REL_;
        else if (value == "BA_REL_") type = TokenType::BA_REL_;
        else if (value == "BA_DEF_DEF_REL_") type = TokenType::BA_DEF_DEF_REL_;
        else if (value == "BU_SG_REL_") type = TokenType::BU_SG_REL_;
        else if (value == "BU_EV_REL_") type = TokenType::BU_EV_REL_;
        else if (value == "BU_BO_REL_") type = TokenType::BU_BO_REL_;
        else if (value == "SG_MUL_VAL_") type = TokenType::SG_MUL_VAL_;
        else if (value == "EV_") type = TokenType::EV_;
        // Note: "M" is NOT automatically treated as MUX_M here because it could be a regular identifier
        // The parser will handle the context to determine if it's a multiplexer
        else if (value.size() > 1 && value[0] == 'm') {
            // Check for multiplexer patterns: m<num> or m<num>M
            bool is_mux = true;
            size_t digit_end = 1;
            
            // Check digits after 'm'
            while (digit_end < value.size() && std::isdigit(value[digit_end])) {
                digit_end++;
            }
            
            // Must have at least one digit
            if (digit_end > 1) {
                // Either ends with digits only, or ends with 'M'
                if (digit_end == value.size() || 
                    (digit_end == value.size() - 1 && value[digit_end] == 'M')) {
                    type = TokenType::MUX_m;
                } else {
                    is_mux = false;
                }
            } else {
                is_mux = false;
            }
            
            if (!is_mux) {
                // Not a multiplexer pattern, treat as regular identifier
                type = TokenType::IDENTIFIER;
            }
        }
        
        return Token(type, value, start_line, start_col);
    }
    
public:
    DBCLexer(const std::string& input) 
        : input_(input), pos_(0), line_(1), column_(1) {}
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        
        while (pos_ < input_.size()) {
            skipWhitespace();
            skipComment();
            skipWhitespace();
            
            if (pos_ >= input_.size()) break;
            
            size_t start_line = line_;
            size_t start_col = column_;
            char ch = peek();
            
            if (ch == '\0') {
                break;
            } else if (ch == ':') {
                advance();
                tokens.emplace_back(TokenType::COLON, ":", start_line, start_col);
            } else if (ch == ';') {
                advance();
                tokens.emplace_back(TokenType::SEMICOLON, ";", start_line, start_col);
            } else if (ch == ',') {
                advance();
                tokens.emplace_back(TokenType::COMMA, ",", start_line, start_col);
            } else if (ch == '@') {
                advance();
                tokens.emplace_back(TokenType::AT, "@", start_line, start_col);
            } else if (ch == '+') {
                advance();
                tokens.emplace_back(TokenType::PLUS, "+", start_line, start_col);
            } else if (ch == '-' && !std::isdigit(peek(1))) {
                advance();
                tokens.emplace_back(TokenType::MINUS, "-", start_line, start_col);
            } else if (ch == '|') {
                advance();
                tokens.emplace_back(TokenType::PIPE, "|", start_line, start_col);
            } else if (ch == '(') {
                advance();
                tokens.emplace_back(TokenType::LPAREN, "(", start_line, start_col);
            } else if (ch == ')') {
                advance();
                tokens.emplace_back(TokenType::RPAREN, ")", start_line, start_col);
            } else if (ch == '[') {
                advance();
                tokens.emplace_back(TokenType::LBRACKET, "[", start_line, start_col);
            } else if (ch == ']') {
                advance();
                tokens.emplace_back(TokenType::RBRACKET, "]", start_line, start_col);
            } else if (ch == '"') {
                tokens.push_back(readString());
            } else if (std::isdigit(ch) || (ch == '-' && std::isdigit(peek(1)))) {
                tokens.push_back(readNumber());
            } else if (std::isalpha(ch) || ch == '_') {
                tokens.push_back(readIdentifier());
            } else {
                // Unknown character
                std::string unknown(1, advance());
                tokens.emplace_back(TokenType::UNKNOWN, unknown, start_line, start_col);
            }
        }
        
        tokens.emplace_back(TokenType::END_OF_FILE, "", line_, column_);
        return tokens;
    }
};

} // namespace dbcppp