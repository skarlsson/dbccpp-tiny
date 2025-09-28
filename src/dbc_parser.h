#pragma once

#include "dbc_lexer.h"
#include "dbcast.h"
#include "dbc_parser_result.h"
#include "log.h"
#include <memory>
#include <algorithm>

namespace dbcppp {

class DBCParser {
private:
    std::vector<Token> tokens_;
    size_t pos_;
    
    const Token& current() const {
        if (pos_ >= tokens_.size()) {
            static Token eof(TokenType::END_OF_FILE, "", 0, 0);
            return eof;
        }
        return tokens_[pos_];
    }
    
    const Token& peek(size_t offset = 1) const {
        if (pos_ + offset >= tokens_.size()) {
            static Token eof(TokenType::END_OF_FILE, "", 0, 0);
            return eof;
        }
        return tokens_[pos_ + offset];
    }
    
    void advance() {
        if (pos_ < tokens_.size()) {
            pos_++;
        }
    }
    
    bool match(TokenType type) {
        if (current().type == type) {
            advance();
            return true;
        }
        return false;
    }
    
    Result<void> expect(TokenType type, const std::string& message = "") {
        if (current().type != type) {
            std::string msg = message.empty() ? 
                "Expected " + tokenTypeToString(type) + " but got " + tokenTypeToString(current().type) :
                message;
            return Err<void>(ParseErrorCode::UnexpectedToken, msg, current().line, current().column);
        }
        advance();
        return Ok();
    }
    
    template<typename T>
    Result<T> expectAndReturn(TokenType type, const std::string& message, T&& value) {
        if (auto res = expect(type, message); res.isError()) {
            return Err<T>(res.error());
        }
        return Ok(std::forward<T>(value));
    }
    
    std::string tokenTypeToString(TokenType type) {
        switch (type) {
            case TokenType::INTEGER: return "INTEGER";
            case TokenType::FLOAT: return "FLOAT";
            case TokenType::STRING: return "STRING";
            case TokenType::IDENTIFIER: return "IDENTIFIER";
            case TokenType::COLON: return "COLON";
            case TokenType::SEMICOLON: return "SEMICOLON";
            case TokenType::COMMA: return "COMMA";
            case TokenType::VERSION: return "VERSION";
            case TokenType::NS_: return "NS_";
            case TokenType::BS_: return "BS_";
            case TokenType::BU_: return "BU_";
            case TokenType::BO_: return "BO_";
            case TokenType::SG_: return "SG_";
            case TokenType::VAL_TABLE_: return "VAL_TABLE_";
            case TokenType::END_OF_FILE: return "END_OF_FILE";
            // ... add other cases as needed
            default: return "UNKNOWN";
        }
    }
    
    // Parse functions for each DBC element
    Result<AST::Version> parseVersion() {
        AST::Version version;
        version.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::VERSION); res.isError()) {
            return Err<AST::Version>(res.error());
        }
        
        if (current().type != TokenType::STRING) {
            return Err<AST::Version>(ParseErrorCode::UnexpectedToken,
                "Expected string for version", current().line, current().column);
        }
        
        version.version = current().value;
        advance();
        
        return Ok(std::move(version));
    }
    
    Result<std::vector<std::string>> parseNewSymbols() {
        std::vector<std::string> symbols;
        
        if (auto res = expect(TokenType::NS_); res.isError()) {
            return Err<std::vector<std::string>>(res.error());
        }
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<std::vector<std::string>>(res.error());
        }
        
        // NS_ section lists symbol names that may include keywords
        while (current().type != TokenType::BS_ && 
               current().type != TokenType::BU_ && 
               current().type != TokenType::END_OF_FILE) {
            // Skip empty lines and collect identifiers
            if (current().type == TokenType::IDENTIFIER ||
                current().type == TokenType::NS_DESC_ ||
                current().type == TokenType::CM_ ||
                current().type == TokenType::BA_DEF_ ||
                current().type == TokenType::BA_ ||
                current().type == TokenType::VAL_ ||
                current().type == TokenType::BA_DEF_DEF_) {
                symbols.push_back(current().value);
            }
            advance();
        }
        
        return Ok(std::move(symbols));
    }
    
    Result<std::optional<AST::BitTiming>> parseBitTiming() {
        if (!match(TokenType::BS_)) {
            return Ok(std::optional<AST::BitTiming>{});
        }
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<std::optional<AST::BitTiming>>(res.error());
        }
        
        // Bit timing is optional
        if (current().type != TokenType::INTEGER) {
            return Ok(std::optional<AST::BitTiming>{});
        }
        
        AST::BitTiming bt;
        bt.baudrate = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<std::optional<AST::BitTiming>>(res.error());
        }
        
        if (current().type != TokenType::INTEGER) {
            return Err<std::optional<AST::BitTiming>>(ParseErrorCode::UnexpectedToken,
                "Expected integer for BTR1", current().line, current().column);
        }
        bt.btr1 = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::COMMA); res.isError()) {
            return Err<std::optional<AST::BitTiming>>(res.error());
        }
        
        if (current().type != TokenType::INTEGER) {
            return Err<std::optional<AST::BitTiming>>(ParseErrorCode::UnexpectedToken,
                "Expected integer for BTR2", current().line, current().column);
        }
        bt.btr2 = std::stoull(current().value);
        advance();
        
        return Ok(std::optional<AST::BitTiming>(std::move(bt)));
    }
    
    Result<std::vector<AST::NodeDef>> parseNodes() {
        std::vector<AST::NodeDef> nodes;
        
        if (auto res = expect(TokenType::BU_); res.isError()) {
            return Err<std::vector<AST::NodeDef>>(res.error());
        }
        
        while (current().type == TokenType::IDENTIFIER) {
            AST::NodeDef node;
            node.pos = {current().line, current().column};
            node.name = current().value;
            nodes.push_back(node);
            advance();
        }
        
        return Ok(std::move(nodes));
    }
    
    Result<AST::Signal> parseSignal() {
        AST::Signal signal;
        signal.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::SG_); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Signal name
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected signal name", current().line, current().column);
        }
        signal.name = current().value;
        advance();
        
        // Optional multiplexer indicator
        if (current().type == TokenType::MUX_M || 
            (current().type == TokenType::IDENTIFIER && current().value == "M")) {
            signal.mux_type = AST::MultiplexerType::MuxSwitch;
            advance();
        } else if (current().type == TokenType::MUX_m) {
            // Handle extended multiplexer syntax like m0M, m1, etc.
            std::string mux_str = current().value;
            if (mux_str.back() == 'M') {
                // Extended multiplexer (e.g., m0M)
                signal.mux_type = AST::MultiplexerType::MuxValue;
                signal.mux_value = std::stoull(mux_str.substr(1, mux_str.length() - 2));
                // Note: This signal is also a multiplexer switch, but our AST doesn't support this yet
                // TODO: Add extended multiplexer support to AST
            } else {
                signal.mux_type = AST::MultiplexerType::MuxValue;
                // Extract the number from m<num>
                signal.mux_value = std::stoull(mux_str.substr(1));
            }
            advance();
        }
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Start bit
        if (current().type != TokenType::INTEGER) {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected integer for start bit", current().line, current().column);
        }
        signal.start_bit = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::PIPE); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Signal size
        if (current().type != TokenType::INTEGER) {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected integer for signal size", current().line, current().column);
        }
        signal.length = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::AT); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Byte order
        if (current().type != TokenType::INTEGER || current().value.empty()) {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected byte order (0 or 1)", current().line, current().column);
        }
        signal.byte_order = current().value[0];
        advance();
        
        // Value type (signed/unsigned)
        if (current().type == TokenType::PLUS) {
            signal.value_type = '+';
            advance();
        } else if (current().type == TokenType::MINUS) {
            signal.value_type = '-';
            advance();
        } else {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected + or - for signal value type", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::LPAREN); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Factor
        if (current().type == TokenType::FLOAT || current().type == TokenType::INTEGER) {
            signal.factor = std::stod(current().value);
            advance();
        } else {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected factor value", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::COMMA); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Offset
        if (current().type == TokenType::FLOAT || current().type == TokenType::INTEGER) {
            signal.offset = std::stod(current().value);
            advance();
        } else if (current().type == TokenType::MINUS) {
            advance();
            if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
                return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                    "Expected number after minus sign", current().line, current().column);
            }
            signal.offset = -std::stod(current().value);
            advance();
        } else {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected offset value", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::RPAREN); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        if (auto res = expect(TokenType::LBRACKET); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Minimum
        if (current().type == TokenType::FLOAT || current().type == TokenType::INTEGER) {
            signal.minimum = std::stod(current().value);
            advance();
        } else if (current().type == TokenType::MINUS) {
            advance();
            if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
                return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                    "Expected number after minus sign", current().line, current().column);
            }
            signal.minimum = -std::stod(current().value);
            advance();
        } else {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected minimum value", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::PIPE); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Maximum
        if (current().type == TokenType::FLOAT || current().type == TokenType::INTEGER) {
            signal.maximum = std::stod(current().value);
            advance();
        } else if (current().type == TokenType::PLUS) {
            advance();
            if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
                return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                    "Expected number after plus sign", current().line, current().column);
            }
            signal.maximum = std::stod(current().value);
            advance();
        } else if (current().type == TokenType::MINUS) {
            advance();
            if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
                return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                    "Expected number after minus sign", current().line, current().column);
            }
            signal.maximum = -std::stod(current().value);
            advance();
        } else {
            return Err<AST::Signal>(ParseErrorCode::UnexpectedToken,
                "Expected maximum value", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::RBRACKET); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        
        // Unit
        if (auto res = expect(TokenType::STRING); res.isError()) {
            return Err<AST::Signal>(res.error());
        }
        signal.unit = tokens_[pos_ - 1].value;
        
        // Receivers
        while (current().type == TokenType::IDENTIFIER && current().value != "SG_") {
            signal.receivers.push_back(current().value);
            advance();
            
            // Skip optional comma
            if (current().type == TokenType::COMMA) {
                advance();
            }
        }
        
        return Ok(std::move(signal));
    }
    
    Result<AST::Message> parseMessage() {
        AST::Message message;
        message.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::BO_); res.isError()) {
            return Err<AST::Message>(res.error());
        }
        
        // Message ID
        if (current().type != TokenType::INTEGER) {
            return Err<AST::Message>(ParseErrorCode::UnexpectedToken,
                "Expected message ID", current().line, current().column);
        }
        message.id = std::stoull(current().value);
        advance();
        
        // Message name
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::Message>(ParseErrorCode::UnexpectedToken,
                "Expected message name", current().line, current().column);
        }
        message.name = current().value;
        advance();
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<AST::Message>(res.error());
        }
        
        // DLC
        if (current().type != TokenType::INTEGER) {
            return Err<AST::Message>(ParseErrorCode::UnexpectedToken,
                "Expected message size (DLC)", current().line, current().column);
        }
        message.size = std::stoull(current().value);
        advance();
        
        // Transmitter
        if (current().type == TokenType::IDENTIFIER) {
            message.transmitter = current().value;
            advance();
        }
        
        // Parse signals
        while (current().type == TokenType::SG_) {
            auto signalResult = parseSignal();
            if (signalResult.isError()) {
                return Err<AST::Message>(signalResult.error());
            }
            message.signals.push_back(signalResult.value());
        }
        
        return Ok(std::move(message));
    }
    
    Result<AST::ValueTable> parseValueTable() {
        AST::ValueTable vt;
        vt.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::VAL_TABLE_); res.isError()) {
            return Err<AST::ValueTable>(res.error());
        }
        
        // Value table name
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::ValueTable>(ParseErrorCode::UnexpectedToken,
                "Expected value table name", current().line, current().column);
        }
        vt.name = current().value;
        advance();
        
        // Parse value descriptions
        while (current().type == TokenType::INTEGER) {
            AST::ValueEncodingDescription desc;
            desc.value = std::stoll(current().value);
            advance();
            
            if (auto res = expect(TokenType::STRING); res.isError()) {
                return Err<AST::ValueTable>(res.error());
            }
            desc.description = tokens_[pos_ - 1].value;
            
            vt.descriptions.push_back(desc);
        }
        
        // Semicolon is optional in some DBC files
        if (current().type == TokenType::SEMICOLON) {
            advance();
        }
        
        return Ok(std::move(vt));
    }
    
    Result<AST::Comment> parseComment() {
        AST::Comment comment;
        comment.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::CM_); res.isError()) {
            return Err<AST::Comment>(res.error());
        }
        
        // Determine comment type
        if (current().type == TokenType::BO_) {
            advance();
            comment.type = AST::Comment::Type::Message;
            if (current().type != TokenType::INTEGER) {
                return Err<AST::Comment>(ParseErrorCode::UnexpectedToken,
                    "Expected message ID", current().line, current().column);
            }
            comment.message_id = std::stoull(current().value);
            advance();
        } else if (current().type == TokenType::SG_) {
            advance();
            comment.type = AST::Comment::Type::Signal;
            if (current().type != TokenType::INTEGER) {
                return Err<AST::Comment>(ParseErrorCode::UnexpectedToken,
                    "Expected message ID", current().line, current().column);
            }
            comment.message_id = std::stoull(current().value);
            advance();
            if (current().type != TokenType::IDENTIFIER) {
                return Err<AST::Comment>(ParseErrorCode::UnexpectedToken,
                    "Expected signal name", current().line, current().column);
            }
            comment.signal_name = current().value;
            advance();
        } else if (current().type == TokenType::BU_) {
            advance();
            comment.type = AST::Comment::Type::Node;
            if (current().type != TokenType::IDENTIFIER) {
                return Err<AST::Comment>(ParseErrorCode::UnexpectedToken,
                    "Expected node name", current().line, current().column);
            }
            comment.node_name = current().value;
            advance();
        } else {
            comment.type = AST::Comment::Type::Network;
        }
        
        if (auto res = expect(TokenType::STRING); res.isError()) {
            return Err<AST::Comment>(res.error());
        }
        comment.text = tokens_[pos_ - 1].value;
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::Comment>(res.error());
        }
        
        return Ok(std::move(comment));
    }
    
    Result<AST::SignalMultiplexerValue> parseSignalMultiplexerValue() {
        AST::SignalMultiplexerValue smv;
        smv.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::SG_MUL_VAL_); res.isError()) {
            return Err<AST::SignalMultiplexerValue>(res.error());
        }
        
        if (current().type != TokenType::INTEGER) {
            return Err<AST::SignalMultiplexerValue>(ParseErrorCode::UnexpectedToken,
                "Expected message ID", current().line, current().column);
        }
        smv.message_id = std::stoull(current().value);
        advance();
        
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::SignalMultiplexerValue>(ParseErrorCode::UnexpectedToken,
                "Expected signal name", current().line, current().column);
        }
        smv.signal_name = current().value;
        advance();
        
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::SignalMultiplexerValue>(ParseErrorCode::UnexpectedToken,
                "Expected switch name", current().line, current().column);
        }
        smv.switch_name = current().value;
        advance();
        
        // Parse value ranges
        while (current().type != TokenType::SEMICOLON) {
            AST::SignalMultiplexerValue::Range range;
            
            // Handle single value or range
            if (current().type == TokenType::INTEGER) {
                range.from = std::stoull(current().value);
                advance();
                
                if (current().type == TokenType::MINUS) {
                    advance();
                    if (current().type != TokenType::INTEGER) {
                        return Err<AST::SignalMultiplexerValue>(ParseErrorCode::UnexpectedToken,
                            "Expected integer after minus in range", current().line, current().column);
                    }
                    range.to = std::stoull(current().value);
                    advance();
                } else {
                    // Single value, not a range
                    range.to = range.from;
                }
            } else {
                return Err<AST::SignalMultiplexerValue>(ParseErrorCode::UnexpectedToken,
                    "Expected integer value in SG_MUL_VAL_", current().line, current().column);
            }
            
            smv.value_ranges.push_back(range);
            
            // Optional comma
            if (current().type == TokenType::COMMA) {
                advance();
            }
        }
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::SignalMultiplexerValue>(res.error());
        }
        
        return Ok(std::move(smv));
    }
    
    Result<AST::AttributeDefinition> parseAttributeDefinition() {
        AST::AttributeDefinition def;
        def.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::BA_DEF_); res.isError()) {
            return Err<AST::AttributeDefinition>(res.error());
        }
        
        // Object type (optional)
        if (current().type == TokenType::BU_) {
            def.object_type = AST::AttributeDefinition::ObjectType::Node;
            advance();
        } else if (current().type == TokenType::BO_) {
            def.object_type = AST::AttributeDefinition::ObjectType::Message;
            advance();
        } else if (current().type == TokenType::SG_) {
            def.object_type = AST::AttributeDefinition::ObjectType::Signal;
            advance();
        } else if (current().type == TokenType::EV_) {
            def.object_type = AST::AttributeDefinition::ObjectType::EnvironmentVariable;
            advance();
        } else {
            def.object_type = AST::AttributeDefinition::ObjectType::Network;
        }
        
        // Attribute name
        if (auto res = expect(TokenType::STRING); res.isError()) {
            return Err<AST::AttributeDefinition>(res.error());
        }
        def.name = tokens_[pos_ - 1].value;
        
        // Value type
        if (current().type == TokenType::IDENTIFIER || current().type == TokenType::STRING) {
            def.value_type = current().value;
            advance();
            
            // Handle numeric ranges
            if (def.value_type == "INT" || def.value_type == "HEX" || def.value_type == "FLOAT") {
                if (current().type == TokenType::INTEGER || current().type == TokenType::FLOAT) {
                    def.min_value = std::stod(current().value);
                    advance();
                    if (current().type == TokenType::INTEGER || current().type == TokenType::FLOAT) {
                        def.max_value = std::stod(current().value);
                        advance();
                    } else {
                        return Err<AST::AttributeDefinition>(ParseErrorCode::UnexpectedToken,
                            "Expected max value for numeric range", current().line, current().column);
                    }
                }
            }
            // Handle enum values
            else if (def.value_type == "ENUM") {
                while (current().type == TokenType::STRING) {
                    def.enum_values.push_back(current().value);
                    advance();
                    if (current().type == TokenType::COMMA) {
                        advance();
                    }
                }
            }
            // Handle string default
            else if (def.value_type == "STRING") {
                if (current().type == TokenType::STRING) {
                    def.default_value = current().value;
                    advance();
                }
            }
        } else {
            return Err<AST::AttributeDefinition>(ParseErrorCode::UnexpectedToken,
                "Expected attribute value type", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::AttributeDefinition>(res.error());
        }
        
        return Ok(std::move(def));
    }
    
    Result<AST::AttributeValue_t> parseAttributeValue() {
        AST::AttributeValue_t attr;
        attr.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::BA_); res.isError()) {
            return Err<AST::AttributeValue_t>(res.error());
        }
        
        // Attribute name
        if (auto res = expect(TokenType::STRING); res.isError()) {
            return Err<AST::AttributeValue_t>(res.error());
        }
        attr.attribute_name = tokens_[pos_ - 1].value;
        
        // Determine attribute type
        if (current().type == TokenType::BU_) {
            advance();
            attr.type = AST::AttributeValue_t::Type::Node;
            if (current().type != TokenType::IDENTIFIER) {
                return Err<AST::AttributeValue_t>(ParseErrorCode::UnexpectedToken,
                    "Expected node name", current().line, current().column);
            }
            attr.node_name = current().value;
            advance();
        } else if (current().type == TokenType::BO_) {
            advance();
            attr.type = AST::AttributeValue_t::Type::Message;
            if (current().type != TokenType::INTEGER) {
                return Err<AST::AttributeValue_t>(ParseErrorCode::UnexpectedToken,
                    "Expected message ID", current().line, current().column);
            }
            attr.message_id = std::stoull(current().value);
            advance();
        } else if (current().type == TokenType::SG_) {
            advance();
            attr.type = AST::AttributeValue_t::Type::Signal;
            if (current().type != TokenType::INTEGER) {
                return Err<AST::AttributeValue_t>(ParseErrorCode::UnexpectedToken,
                    "Expected message ID", current().line, current().column);
            }
            attr.message_id = std::stoull(current().value);
            advance();
            if (current().type != TokenType::IDENTIFIER) {
                return Err<AST::AttributeValue_t>(ParseErrorCode::UnexpectedToken,
                    "Expected signal name", current().line, current().column);
            }
            attr.signal_name = current().value;
            advance();
        } else {
            attr.type = AST::AttributeValue_t::Type::Network;
        }
        
        // Attribute value
        if (current().type == TokenType::INTEGER) {
            attr.value = std::stoll(current().value);
            advance();
        } else if (current().type == TokenType::FLOAT) {
            attr.value = std::stod(current().value);
            advance();
        } else if (current().type == TokenType::STRING) {
            attr.value = current().value;
            advance();
        } else {
            return Err<AST::AttributeValue_t>(ParseErrorCode::UnexpectedToken,
                "Expected attribute value", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::AttributeValue_t>(res.error());
        }
        
        return Ok(std::move(attr));
    }
    
    Result<AST::MessageTransmitter> parseMessageTransmitter() {
        AST::MessageTransmitter mt;
        mt.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::BO_TX_BU_); res.isError()) {
            return Err<AST::MessageTransmitter>(res.error());
        }
        
        // Message ID
        if (current().type != TokenType::INTEGER) {
            return Err<AST::MessageTransmitter>(ParseErrorCode::UnexpectedToken,
                "Expected message ID", current().line, current().column);
        }
        mt.message_id = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<AST::MessageTransmitter>(res.error());
        }
        
        // Transmitters
        while (current().type == TokenType::IDENTIFIER) {
            mt.transmitters.push_back(current().value);
            advance();
            if (current().type == TokenType::COMMA) {
                advance();
            }
        }
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::MessageTransmitter>(res.error());
        }
        
        return Ok(std::move(mt));
    }
    
    Result<AST::ValueDescription> parseValueDescription() {
        AST::ValueDescription vd;
        vd.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::VAL_); res.isError()) {
            return Err<AST::ValueDescription>(res.error());
        }
        
        // Signal value description only
        if (current().type == TokenType::INTEGER) {
            vd.type = AST::ValueDescription::Type::Signal;
            vd.message_id = std::stoull(current().value);
            advance();
            
            if (current().type != TokenType::IDENTIFIER) {
                return Err<AST::ValueDescription>(ParseErrorCode::UnexpectedToken,
                    "Expected signal name", current().line, current().column);
            }
            vd.object_name = current().value;
            advance();
        } else {
            return Err<AST::ValueDescription>(ParseErrorCode::UnexpectedToken,
                "Expected message ID for value description", current().line, current().column);
        }
        
        // Parse value descriptions
        while (current().type == TokenType::INTEGER) {
            AST::ValueEncodingDescription ved;
            ved.value = std::stoll(current().value);
            advance();
            
            if (auto res = expect(TokenType::STRING); res.isError()) {
                return Err<AST::ValueDescription>(res.error());
            }
            ved.description = tokens_[pos_ - 1].value;
            
            vd.descriptions.push_back(ved);
        }
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::ValueDescription>(res.error());
        }
        
        return Ok(std::move(vd));
    }
    
    Result<AST::AttributeDefault> parseAttributeDefault() {
        AST::AttributeDefault def;
        def.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::BA_DEF_DEF_); res.isError()) {
            return Err<AST::AttributeDefault>(res.error());
        }
        
        if (auto res = expect(TokenType::STRING); res.isError()) {
            return Err<AST::AttributeDefault>(res.error());
        }
        def.name = tokens_[pos_ - 1].value;
        
        // Parse attribute value
        if (current().type == TokenType::INTEGER) {
            def.value = std::stoll(current().value);
            advance();
        } else if (current().type == TokenType::FLOAT) {
            def.value = std::stod(current().value);
            advance();
        } else if (current().type == TokenType::STRING) {
            def.value = current().value;
            advance();
        } else {
            return Err<AST::AttributeDefault>(ParseErrorCode::UnexpectedToken,
                "Expected attribute value", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::AttributeDefault>(res.error());
        }
        
        return Ok(std::move(def));
    }
    
    Result<AST::SignalGroup> parseSignalGroup() {
        AST::SignalGroup sg;
        sg.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::SIG_GROUP_); res.isError()) {
            return Err<AST::SignalGroup>(res.error());
        }
        
        if (current().type != TokenType::INTEGER) {
            return Err<AST::SignalGroup>(ParseErrorCode::UnexpectedToken,
                "Expected message ID", current().line, current().column);
        }
        sg.message_id = std::stoull(current().value);
        advance();
        
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::SignalGroup>(ParseErrorCode::UnexpectedToken,
                "Expected group name", current().line, current().column);
        }
        sg.group_name = current().value;
        advance();
        
        if (current().type != TokenType::INTEGER) {
            return Err<AST::SignalGroup>(ParseErrorCode::UnexpectedToken,
                "Expected repetitions count", current().line, current().column);
        }
        sg.repetitions = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<AST::SignalGroup>(res.error());
        }
        
        // Parse signal names
        while (current().type == TokenType::IDENTIFIER) {
            sg.signal_names.push_back(current().value);
            advance();
        }
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::SignalGroup>(res.error());
        }
        
        return Ok(std::move(sg));
    }
    
    Result<AST::SignalExtendedValueType> parseSignalExtendedValueType() {
        AST::SignalExtendedValueType sevt;
        sevt.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::SIG_VALTYPE_); res.isError()) {
            return Err<AST::SignalExtendedValueType>(res.error());
        }
        
        if (current().type != TokenType::INTEGER) {
            return Err<AST::SignalExtendedValueType>(ParseErrorCode::UnexpectedToken,
                "Expected message ID", current().line, current().column);
        }
        sevt.message_id = std::stoull(current().value);
        advance();
        
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::SignalExtendedValueType>(ParseErrorCode::UnexpectedToken,
                "Expected signal name", current().line, current().column);
        }
        sevt.signal_name = current().value;
        advance();
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<AST::SignalExtendedValueType>(res.error());
        }
        
        if (current().type != TokenType::INTEGER) {
            return Err<AST::SignalExtendedValueType>(ParseErrorCode::UnexpectedToken,
                "Expected value type", current().line, current().column);
        }
        sevt.value_type = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::SignalExtendedValueType>(res.error());
        }
        
        return Ok(std::move(sevt));
    }
    
    Result<AST::SignalType> parseSignalType() {
        AST::SignalType st;
        st.pos = {current().line, current().column};
        
        if (auto res = expect(TokenType::SGTYPE_); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected signal type name", current().line, current().column);
        }
        st.name = current().value;
        advance();
        
        if (auto res = expect(TokenType::COLON); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::INTEGER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected signal size", current().line, current().column);
        }
        st.size = std::stoull(current().value);
        advance();
        
        if (auto res = expect(TokenType::AT); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::INTEGER || current().value.empty()) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected byte order", current().line, current().column);
        }
        st.byte_order = current().value[0];
        advance();
        
        // Value type
        if (current().type == TokenType::PLUS) {
            st.value_type = '+';
            advance();
        } else if (current().type == TokenType::MINUS) {
            st.value_type = '-';
            advance();
        } else {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected + or - for value type", current().line, current().column);
        }
        
        if (auto res = expect(TokenType::LPAREN); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected factor value", current().line, current().column);
        }
        st.factor = std::stod(current().value);
        advance();
        
        if (auto res = expect(TokenType::COMMA); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected offset value", current().line, current().column);
        }
        st.offset = std::stod(current().value);
        advance();
        
        if (auto res = expect(TokenType::RPAREN); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (auto res = expect(TokenType::LBRACKET); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected minimum value", current().line, current().column);
        }
        st.minimum = std::stod(current().value);
        advance();
        
        if (auto res = expect(TokenType::PIPE); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected maximum value", current().line, current().column);
        }
        st.maximum = std::stod(current().value);
        advance();
        
        if (auto res = expect(TokenType::RBRACKET); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (auto res = expect(TokenType::STRING); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        st.unit = tokens_[pos_ - 1].value;
        
        if (current().type != TokenType::FLOAT && current().type != TokenType::INTEGER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected default value", current().line, current().column);
        }
        st.default_value = std::stod(current().value);
        advance();
        
        if (auto res = expect(TokenType::COMMA); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        if (current().type != TokenType::IDENTIFIER) {
            return Err<AST::SignalType>(ParseErrorCode::UnexpectedToken,
                "Expected value table name", current().line, current().column);
        }
        st.value_table = current().value;
        advance();
        
        if (auto res = expect(TokenType::SEMICOLON); res.isError()) {
            return Err<AST::SignalType>(res.error());
        }
        
        return Ok(std::move(st));
    }
    
public:
    Result<std::unique_ptr<AST::Network>> parse(const std::string& input) {
        // Tokenize
        DBCLexer lexer(input);
        tokens_ = lexer.tokenize();
        pos_ = 0;
        
        auto network = std::make_unique<AST::Network>();
        
        // Parse VERSION
        auto versionResult = parseVersion();
        if (versionResult.isError()) {
            return Err<std::unique_ptr<AST::Network>>(versionResult.error());
        }
        network->version = versionResult.value();
        
        // Parse optional sections in order
        if (current().type == TokenType::NS_) {
            auto symbolsResult = parseNewSymbols();
            if (symbolsResult.isError()) {
                return Err<std::unique_ptr<AST::Network>>(symbolsResult.error());
            }
            network->new_symbols = symbolsResult.value();
        }
        
        // Parse BS_ (bit timing)
        auto bitTimingResult = parseBitTiming();
        if (bitTimingResult.isError()) {
            return Err<std::unique_ptr<AST::Network>>(bitTimingResult.error());
        }
        network->bit_timing = bitTimingResult.value();
        
        // Parse BU_ (nodes)
        auto nodesResult = parseNodes();
        if (nodesResult.isError()) {
            return Err<std::unique_ptr<AST::Network>>(nodesResult.error());
        }
        network->nodes = nodesResult.value();
        
        // Parse remaining elements
        while (current().type != TokenType::END_OF_FILE) {
            if (current().type == TokenType::VAL_TABLE_) {
                auto result = parseValueTable();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->value_tables.push_back(result.value());
            } else if (current().type == TokenType::BO_) {
                auto result = parseMessage();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->messages.push_back(result.value());
            } else if (current().type == TokenType::CM_) {
                auto result = parseComment();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->comments.push_back(result.value());
            } else if (current().type == TokenType::BA_DEF_) {
                auto result = parseAttributeDefinition();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                // Check if this is an environment variable attribute and discard it
                if (result.value().object_type == AST::AttributeDefinition::ObjectType::EnvironmentVariable) {
                    LOG_INFO("Discarding EV_ attribute definition: '%s' (environment variables not supported on embedded)",
                             result.value().name.c_str());
                } else {
                    network->attribute_definitions.push_back(result.value());
                }
            } else if (current().type == TokenType::BA_) {
                auto result = parseAttributeValue();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->attribute_values.push_back(result.value());
            } else if (current().type == TokenType::BO_TX_BU_) {
                auto result = parseMessageTransmitter();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->message_transmitters.push_back(result.value());
            } else if (current().type == TokenType::SG_MUL_VAL_) {
                auto result = parseSignalMultiplexerValue();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->signal_multiplexer_values.push_back(result.value());
            } else if (current().type == TokenType::VAL_) {
                auto result = parseValueDescription();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->value_descriptions.push_back(result.value());
            } else if (current().type == TokenType::BA_DEF_DEF_) {
                auto result = parseAttributeDefault();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->attribute_defaults.push_back(result.value());
            } else if (current().type == TokenType::SIG_GROUP_) {
                auto result = parseSignalGroup();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->signal_groups.push_back(result.value());
            } else if (current().type == TokenType::SIG_VALTYPE_) {
                auto result = parseSignalExtendedValueType();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->signal_extended_value_types.push_back(result.value());
            } else if (current().type == TokenType::SGTYPE_) {
                auto result = parseSignalType();
                if (result.isError()) {
                    return Err<std::unique_ptr<AST::Network>>(result.error());
                }
                network->signal_types.push_back(result.value());
            } else {
                // Skip unhandled tokens for now
                advance();
            }
        }
        
        return Ok(std::move(network));
    }
};

} // namespace dbcppp