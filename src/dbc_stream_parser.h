#pragma once

#include "file_reader.h"
#include "dbcast.h"
#include "dbc_parser_result.h"
#include "dbc_lexer.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <cctype>

namespace dbcppp {

// Streaming DBC parser that processes files line-by-line
// This avoids loading the entire file into memory
class DBCStreamParser {
public:
    DBCStreamParser() : current_message_(nullptr) {}

    // Parse DBC from a line reader (file or string)
    Result<std::unique_ptr<AST::Network>> parse(ILineReader& reader) {
        auto network = std::make_unique<AST::Network>();
        std::string line;
        std::string accumulated;
        bool in_multiline = false;
        bool in_ns_section = false;  // Special handling for NS_ section

        while (reader.readLine(line)) {
            // Skip empty lines and pure comments
            std::string trimmed = trim(line);
            if (trimmed.empty() || (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '/')) {
                // Empty line ends NS_ section
                if (in_ns_section && trimmed.empty()) {
                    in_ns_section = false;
                }
                continue;
            }

            // Special handling for NS_ section
            if (trimmed == "NS_ :" || trimmed == "NS_:") {
                // Parse NS_ header
                auto result = parseStatement(trimmed, network.get());
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(ParseErrorCode::UnexpectedToken,
                               result.error().toString(), reader.getLineNumber(), 0);
                }
                in_ns_section = true;
                continue;
            }

            // If we're in NS_ section and line starts with tab/spaces, it's a symbol
            if (in_ns_section && (line[0] == '\t' || line[0] == ' ')) {
                // Add symbol to network
                network->new_symbols.push_back(trimmed);
                continue;
            } else if (in_ns_section) {
                // Non-indented line ends NS_ section
                in_ns_section = false;
                // Fall through to process this line normally
            }

            // Check if this is a continuation of a multi-line statement
            if (in_multiline || shouldAccumulate(trimmed)) {
                accumulated += " " + trimmed;

                // Check if statement is complete
                if (isStatementComplete(accumulated)) {
                    auto result = parseStatement(accumulated, network.get());
                    if (result.isError()) {
                        return Err<std::unique_ptr<AST::Network>>(ParseErrorCode::UnexpectedToken,
                                   result.error().toString(), reader.getLineNumber(), 0);
                    }
                    accumulated.clear();
                    in_multiline = false;
                } else {
                    in_multiline = true;
                }
            } else {
                // Single-line statement
                auto result = parseStatement(trimmed, network.get());
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(ParseErrorCode::UnexpectedToken,
                               result.error().toString(), reader.getLineNumber(), 0);
                }
            }
        }

        // Check for incomplete statement
        if (!accumulated.empty()) {
            return Err<std::unique_ptr<AST::Network>>(ParseErrorCode::UnexpectedEndOfFile,
                       "Incomplete statement at end of file", reader.getLineNumber(), 0);
        }

        return Ok(std::move(network));
    }

    // Parse from file
    Result<std::unique_ptr<AST::Network>> parseFile(const char* filename) {
        FileLineReaderAdapter reader(filename);
        if (!reader.isOpen()) {
            return Err<std::unique_ptr<AST::Network>>(ParseErrorCode::UnexpectedToken,
                       std::string("Cannot open file: ") + filename, 0, 0);
        }
        return parse(reader);
    }

    // Parse from string (for compatibility)
    Result<std::unique_ptr<AST::Network>> parseString(const std::string& input) {
        StringLineReaderAdapter reader(input);
        return parse(reader);
    }

private:
    AST::Message* current_message_;  // Track current message for signals

    // Remove quotes from string
    std::string removeQuotes(const std::string& str) {
        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
            return str.substr(1, str.size() - 2);
        }
        return str;
    }

    // Check if string is numeric
    bool isNumeric(const std::string& str) {
        if (str.empty()) return false;
        size_t start = 0;
        if (str[0] == '-' || str[0] == '+') start = 1;
        if (start >= str.length()) return false;
        for (size_t i = start; i < str.length(); i++) {
            if (!std::isdigit(str[i])) return false;
        }
        return true;
    }

    // Trim whitespace from string
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    // Check if we should start accumulating lines
    bool shouldAccumulate(const std::string& line) {
        // Signal definitions might span multiple lines
        return line.find("SG_") == 0 && line.find(";") == std::string::npos;
    }

    // Check if a potentially multi-line statement is complete
    bool isStatementComplete(const std::string& statement) {
        // For signals, check for complete receiver list
        if (statement.find("SG_") == 0) {
            // Signal is complete when we have the receiver list
            // Format: SG_ name : bit_info (scale,offset) [min|max] "unit" receivers
            // Receivers can be: Receiver1,Receiver2 or Vector__XXX

            // Simple check: count quotes and ensure we have even number (unit string)
            int quote_count = 0;
            for (char c : statement) {
                if (c == '"') quote_count++;
            }
            if (quote_count < 2 || quote_count % 2 != 0) return false;

            // After the unit string, we should have receivers
            size_t last_quote = statement.rfind('"');
            if (last_quote != std::string::npos && last_quote < statement.length() - 1) {
                std::string after_quote = trim(statement.substr(last_quote + 1));
                return !after_quote.empty();
            }
        }
        return true;
    }

    // Parse a complete statement
    Result<void> parseStatement(const std::string& statement, AST::Network* network) {
        // Skip lines that are just whitespace or tabs (indented lines in NS_ section)
        std::string trimmed = trim(statement);
        if (trimmed.empty()) {
            return Result<void>();
        }

        // Tokenize the statement
        DBCLexer lexer(statement);
        auto tokens = lexer.tokenize();

        if (tokens.empty()) return Ok();

        // Route to appropriate parser based on first token
        const std::string& keyword = tokens[0].value;

        if (keyword == "VERSION") {
            return parseVersion(tokens, network);
        } else if (keyword == "NS_") {
            return parseNewSymbols(tokens, network);
        } else if (keyword == "BS_") {
            return parseBitTiming(tokens, network);
        } else if (keyword == "BU_") {
            return parseNodes(tokens, network);
        } else if (keyword == "BO_") {
            return parseMessage(tokens, network);
        } else if (keyword == "SG_") {
            return parseSignal(tokens, network);
        } else if (keyword == "CM_") {
            return parseComment(tokens, network);
        } else if (keyword == "VAL_") {
            return parseValueTable(tokens, network);
        } else if (keyword == "BA_DEF_") {
            return parseAttributeDefinition(tokens, network);
        } else if (keyword == "BA_") {
            return parseAttribute(tokens, network);
        } else if (keyword == "VAL_TABLE_") {
            return parseValueTableDef(tokens, network);
        } else if (keyword == "BO_TX_BU_") {
            return parseMessageTransmitters(tokens, network);
        } else if (keyword == "SG_MUL_VAL_") {
            return parseSignalMultiplexer(tokens, network);
        }

        // Unknown statement - ignore for compatibility
        return Result<void>();
    }

    // Parse VERSION statement
    Result<void> parseVersion(const std::vector<Token>& tokens, AST::Network* network) {
        if (tokens.size() < 2) {
            return Err<void>(ParseErrorCode::UnexpectedToken, "Invalid VERSION statement", 0, 0);
        }
        network->version.version = tokens[1].value;
        // Remove quotes if present
        if (network->version.version.size() >= 2 &&
            network->version.version.front() == '"' &&
            network->version.version.back() == '"') {
            network->version.version = network->version.version.substr(1, network->version.version.size() - 2);
        }
        return Result<void>();
    }

    // Parse NS_ (new symbols) statement
    Result<void> parseNewSymbols(const std::vector<Token>& tokens, AST::Network* network) {
        // NS_ : symbol1 symbol2 ...
        // Note: symbols can be on the same line or following lines
        size_t i = 1;
        if (i < tokens.size() && tokens[i].type == TokenType::COLON) {
            i++;
        }
        // Add all remaining tokens as symbols (filtering out END_OF_FILE)
        while (i < tokens.size() && tokens[i].type != TokenType::END_OF_FILE) {
            // Only add identifiers and known keywords as symbols
            if (tokens[i].type == TokenType::IDENTIFIER ||
                tokens[i].type == TokenType::NS_DESC_ ||
                tokens[i].type == TokenType::CM_ ||
                tokens[i].type == TokenType::BA_DEF_ ||
                tokens[i].type == TokenType::BA_ ||
                tokens[i].type == TokenType::VAL_ ||
                tokens[i].type == TokenType::CAT_DEF_ ||
                tokens[i].type == TokenType::CAT_ ||
                tokens[i].type == TokenType::FILTER ||
                tokens[i].type == TokenType::BO_TX_BU_ ||
                tokens[i].type == TokenType::SIG_GROUP_) {
                network->new_symbols.push_back(tokens[i].value);
            }
            i++;
        }
        return Result<void>();
    }

    // Parse BS_ (bit timing) statement
    Result<void> parseBitTiming(const std::vector<Token>& tokens, AST::Network* network) {
        // BS_: baudrate : BTR1, BTR2
        // BS_ can be empty (just "BS_:")
        if (tokens.size() < 2) return Result<void>();  // Empty BS_

        size_t i = 1;
        if (i < tokens.size() && tokens[i].value == ":") {
            i++;
            // BS_: with no values - this is valid
            if (i >= tokens.size()) {
                return Result<void>();
            }
        }

        AST::BitTiming bt;
        bt.baudrate = 0;
        bt.btr1 = 0;
        bt.btr2 = 0;

        // Try to parse baudrate if present and numeric
        if (i < tokens.size() && isNumeric(tokens[i].value)) {
            bt.baudrate = std::stoull(tokens[i].value);
            i++;
        }

        if (i < tokens.size() && tokens[i].value == ":") i++;

        // Try to parse BTR1 if present and numeric
        if (i < tokens.size() && isNumeric(tokens[i].value)) {
            bt.btr1 = std::stoull(tokens[i].value);
            i++;
        }

        if (i < tokens.size() && tokens[i].value == ",") i++;

        // Try to parse BTR2 if present and numeric
        if (i < tokens.size() && isNumeric(tokens[i].value)) {
            bt.btr2 = std::stoull(tokens[i].value);
        }

        network->bit_timing = bt;
        return Result<void>();
    }

    // Parse BU_ (nodes) statement
    Result<void> parseNodes(const std::vector<Token>& tokens, AST::Network* network) {
        // BU_ node1 node2 ...
        for (size_t i = 1; i < tokens.size(); i++) {
            AST::NodeDef node;
            node.name = tokens[i].value;
            network->nodes.push_back(std::move(node));
        }
        return Result<void>();
    }

    // Parse BO_ (message) statement
    Result<void> parseMessage(const std::vector<Token>& tokens, AST::Network* network) {
        // BO_ message_id message_name : message_size transmitter
        if (tokens.size() < 5) {
            return Err<void>(ParseErrorCode::InvalidMessageFormat, "Invalid message definition", 0, 0);
        }

        AST::Message message;
        // Parse message ID (can be extended format like 0x80000001)
        try {
            if (tokens[1].value.find("0x") == 0 || tokens[1].value.find("0X") == 0) {
                message.id = std::stoull(tokens[1].value, nullptr, 16);
            } else {
                message.id = std::stoull(tokens[1].value);
            }
        } catch (...) {
            return Err<void>(ParseErrorCode::InvalidMessageFormat, "Invalid message ID", 0, 0);
        }
        message.name = tokens[2].value;

        size_t i = 3;
        if (tokens[i].value == ":") i++;

        if (i >= tokens.size()) {
            return Err<void>(ParseErrorCode::InvalidMessageFormat, "Missing message size", 0, 0);
        }
        try {
            message.size = std::stoull(tokens[i].value);
        } catch (...) {
            return Err<void>(ParseErrorCode::InvalidMessageFormat, "Invalid message size", 0, 0);
        }
        i++;

        if (i < tokens.size()) {
            message.transmitter = tokens[i].value;
        }

        network->messages.push_back(std::move(message));
        current_message_ = &network->messages.back();

        return Result<void>();
    }

    // Parse SG_ (signal) statement
    Result<void> parseSignal(const std::vector<Token>& tokens, AST::Network* network) {
        if (!current_message_) {
            return Err<void>(ParseErrorCode::UnexpectedToken, "Signal without message", 0, 0);
        }

        AST::Signal signal;
        // Initialize defaults
        signal.factor = 1.0;
        signal.offset = 0.0;
        signal.minimum = 0.0;
        signal.maximum = 0.0;
        signal.mux_type = AST::MultiplexerType::None;
        signal.mux_value = 0;

        // SG_ signal_name multiplexer : start|size@endianness+/- (factor,offset) [min|max] "unit" receivers
        size_t i = 1;
        if (i >= tokens.size()) return Err<void>(ParseErrorCode::InvalidSignalFormat, "Invalid signal", 0, 0);

        signal.name = tokens[i].value;
        i++;

        // Check for multiplexer indicator (M, m0, m1, etc.)
        if (i < tokens.size() && tokens[i].value != ":") {
            if (tokens[i].value == "M") {
                signal.mux_type = AST::MultiplexerType::MuxSwitch;
                i++;
            } else if (tokens[i].value[0] == 'm' && tokens[i].value.length() > 1) {
                signal.mux_type = AST::MultiplexerType::MuxValue;
                try {
                    signal.mux_value = std::stoull(tokens[i].value.substr(1));
                } catch (...) {}
                i++;
            }
        }

        // Skip to after colon
        while (i < tokens.size() && tokens[i].value != ":") i++;
        if (i < tokens.size() && tokens[i].value == ":") i++;

        // Parse bit position and size
        if (i < tokens.size()) {
            std::string bit_info = tokens[i].value;
            size_t pipe_pos = bit_info.find('|');
            if (pipe_pos != std::string::npos) {
                try {
                    signal.start_bit = std::stoull(bit_info.substr(0, pipe_pos));
                } catch (...) {}

                size_t at_pos = bit_info.find('@', pipe_pos);
                if (at_pos != std::string::npos) {
                    try {
                        signal.length = std::stoull(bit_info.substr(pipe_pos + 1, at_pos - pipe_pos - 1));
                    } catch (...) {}
                    signal.byte_order = bit_info[at_pos + 1];  // '0' = Motorola, '1' = Intel

                    // Find sign indicator
                    size_t sign_pos = at_pos + 2;
                    if (sign_pos < bit_info.length()) {
                        signal.value_type = bit_info[sign_pos];  // '+' = unsigned, '-' = signed
                    }
                }
            }
            i++;
        }

        // Parse (factor,offset)
        if (i < tokens.size() && tokens[i].value == "(") {
            i++;
            if (i < tokens.size()) {
                try {
                    signal.factor = std::stod(tokens[i].value);
                } catch (...) {}
                i++;
            }
            if (i < tokens.size() && tokens[i].value == ",") {
                i++;
                if (i < tokens.size()) {
                    try {
                        signal.offset = std::stod(tokens[i].value);
                    } catch (...) {}
                    i++;
                }
            }
            if (i < tokens.size() && tokens[i].value == ")") i++;
        }

        // Parse [min|max]
        if (i < tokens.size() && tokens[i].value == "[") {
            i++;
            if (i < tokens.size()) {
                try {
                    signal.minimum = std::stod(tokens[i].value);
                } catch (...) {}
                i++;
            }
            if (i < tokens.size() && tokens[i].value == "|") {
                i++;
                if (i < tokens.size()) {
                    try {
                        signal.maximum = std::stod(tokens[i].value);
                    } catch (...) {}
                    i++;
                }
            }
            if (i < tokens.size() && tokens[i].value == "]") i++;
        }

        // Parse "unit"
        if (i < tokens.size()) {
            signal.unit = removeQuotes(tokens[i].value);
            i++;
        }

        // Parse receivers (comma-separated list or Vector__XXX)
        while (i < tokens.size()) {
            if (tokens[i].value != ",") {
                signal.receivers.push_back(tokens[i].value);
            }
            i++;
        }

        current_message_->signals.push_back(std::move(signal));
        return Result<void>();
    }

    // Stub implementations for other statement types
    Result<void> parseComment(const std::vector<Token>& tokens, AST::Network* network) {
        // TODO: Implement comment parsing
        return Result<void>();
    }

    Result<void> parseValueTable(const std::vector<Token>& tokens, AST::Network* network) {
        // TODO: Implement value table parsing
        return Result<void>();
    }

    Result<void> parseAttributeDefinition(const std::vector<Token>& tokens, AST::Network* network) {
        // BA_DEF_ [object_type] "attribute_name" value_type [min] [max];
        // object_type: BU_, BO_, SG_, EV_
        if (tokens.size() < 4) {
            return Err<void>(ParseErrorCode::UnexpectedToken, "Invalid attribute definition", 0, 0);
        }

        AST::AttributeDefinition attr_def;
        size_t i = 1;

        // Parse object type (optional, defaults to Network)
        if (tokens[i].value == "BU_") {
            attr_def.object_type = AST::AttributeDefinition::ObjectType::Node;
            i++;
        } else if (tokens[i].value == "BO_") {
            attr_def.object_type = AST::AttributeDefinition::ObjectType::Message;
            i++;
        } else if (tokens[i].value == "SG_") {
            attr_def.object_type = AST::AttributeDefinition::ObjectType::Signal;
            i++;
        } else {
            attr_def.object_type = AST::AttributeDefinition::ObjectType::Network;
        }

        // Parse attribute name (should be quoted)
        if (i >= tokens.size()) {
            return Err<void>(ParseErrorCode::UnexpectedToken, "Missing attribute name", 0, 0);
        }
        attr_def.name = removeQuotes(tokens[i].value);
        i++;

        // Parse value type
        if (i >= tokens.size()) {
            return Err<void>(ParseErrorCode::UnexpectedToken, "Missing value type", 0, 0);
        }

        std::string value_type = tokens[i].value;
        i++;

        attr_def.value_type = value_type;

        if (value_type == "INT" || value_type == "HEX" || value_type == "FLOAT") {
            // Parse min value
            if (i < tokens.size() && tokens[i].value != ";") {
                try {
                    attr_def.min_value = std::stod(tokens[i].value);
                    i++;
                } catch (...) {
                    // Not a number, skip
                }
            }

            // Parse max value
            if (i < tokens.size() && tokens[i].value != ";") {
                try {
                    attr_def.max_value = std::stod(tokens[i].value);
                    i++;
                } catch (...) {
                    // Not a number, skip
                }
            }
        } else if (value_type == "STRING") {
            // String type has no additional parameters
        } else if (value_type == "ENUM") {
            // Parse enum values (comma-separated, quoted)
            while (i < tokens.size() && tokens[i].value != ";") {
                if (tokens[i].value != ",") {
                    attr_def.enum_values.push_back(removeQuotes(tokens[i].value));
                }
                i++;
            }
        }

        network->attribute_definitions.push_back(std::move(attr_def));
        return Result<void>();
    }

    Result<void> parseAttribute(const std::vector<Token>& tokens, AST::Network* network) {
        // TODO: Implement attribute parsing
        return Result<void>();
    }

    Result<void> parseValueTableDef(const std::vector<Token>& tokens, AST::Network* network) {
        // TODO: Implement value table definition parsing
        return Result<void>();
    }

    Result<void> parseMessageTransmitters(const std::vector<Token>& tokens, AST::Network* network) {
        // TODO: Implement message transmitters parsing
        return Result<void>();
    }

    Result<void> parseSignalMultiplexer(const std::vector<Token>& tokens, AST::Network* network) {
        // TODO: Implement signal multiplexer parsing
        return Result<void>();
    }
};

} // namespace dbcppp