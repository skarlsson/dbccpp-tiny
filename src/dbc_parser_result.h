#pragma once

#include <string>
#include <variant>
#include <memory>
#include <optional>

namespace dbcppp {

// Error codes for parsing
enum class ParseErrorCode {
    None = 0,
    UnexpectedToken,
    InvalidValueType,
    InvalidInteger,
    MissingMessageId,
    InvalidAttributeValue,
    UnexpectedEndOfFile,
    InvalidSignalFormat,
    InvalidMultiplexer,
    InvalidNodeName,
    InvalidMessageFormat,
    InvalidFloatFormat,
    InvalidStringFormat,
    MemoryAllocationFailed
};

// Parse error with code, message, and location
struct ParseError {
    ParseErrorCode code;
    std::string message;
    size_t line;
    size_t column;
    
    ParseError(ParseErrorCode c, const std::string& msg, size_t l, size_t col)
        : code(c), message(msg), line(l), column(col) {}
    
    std::string toString() const {
        return "Parse error at line " + std::to_string(line) + 
               ", column " + std::to_string(column) + ": " + message;
    }
};

// Result type that can hold either a value or an error
template<typename T>
class Result {
private:
    std::variant<T, ParseError> data_;
    
public:
    // Success constructor
    Result(T&& value) : data_(std::move(value)) {}
    Result(const T& value) : data_(value) {}
    
    // Error constructor
    Result(const ParseError& error) : data_(error) {}
    Result(ParseErrorCode code, const std::string& msg, size_t line, size_t col)
        : data_(ParseError(code, msg, line, col)) {}
    
    // Check if result is successful
    bool isOk() const {
        return std::holds_alternative<T>(data_);
    }
    
    bool isError() const {
        return std::holds_alternative<ParseError>(data_);
    }
    
    // Get value (only valid if isOk())
    T& value() {
        return std::get<T>(data_);
    }
    
    const T& value() const {
        return std::get<T>(data_);
    }
    
    // Get error (only valid if isError())
    const ParseError& error() const {
        return std::get<ParseError>(data_);
    }
    
    // Move value out (only valid if isOk())
    T&& moveValue() {
        return std::get<T>(std::move(data_));
    }
    
    // Monadic operations for chaining
    template<typename F>
    auto andThen(F&& f) -> Result<decltype(f(std::declval<T>()))> {
        if (isOk()) {
            return f(value());
        }
        return error();
    }
    
    // Get value or default
    T valueOr(T&& defaultValue) {
        if (isOk()) {
            return value();
        }
        return std::move(defaultValue);
    }
};

// Helper to create success result
template<typename T>
Result<T> Ok(T&& value) {
    return Result<T>(std::forward<T>(value));
}

// Helper to create error result
template<typename T>
Result<T> Err(ParseErrorCode code, const std::string& msg, size_t line, size_t col) {
    return Result<T>(ParseError(code, msg, line, col));
}

template<typename T>
Result<T> Err(const ParseError& error) {
    return Result<T>(error);
}

// Specialization for void Result
template<>
class Result<void> {
private:
    std::optional<ParseError> error_;
    
public:
    Result() : error_(std::nullopt) {} // Success
    Result(const ParseError& error) : error_(error) {}
    Result(ParseErrorCode code, const std::string& msg, size_t line, size_t col)
        : error_(ParseError(code, msg, line, col)) {}
    
    bool isOk() const { return !error_.has_value(); }
    bool isError() const { return error_.has_value(); }
    
    const ParseError& error() const { return error_.value(); }
};

// Helper for void success
inline Result<void> Ok() {
    return Result<void>();
}

} // namespace dbcppp