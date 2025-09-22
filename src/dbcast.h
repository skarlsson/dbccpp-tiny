#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <cstdint>
#include <memory>

namespace dbcppp {
namespace AST {

// Position information for error reporting
struct Position {
    size_t line;
    size_t column;
    
    Position() : line(0), column(0) {}
    Position(size_t l, size_t c) : line(l), column(c) {}
};

// Base class for AST nodes with position tracking
struct Node {
    Position pos;
    virtual ~Node() = default;
};

// Type for attribute values (can be int, double, or string)
using AttributeValue = std::variant<int64_t, double, std::string>;

struct Version : Node {
    std::string version;
};

struct NewSymbols : Node {
    std::vector<std::string> symbols;
};

struct BitTiming : Node {
    uint64_t baudrate;
    uint64_t btr1;
    uint64_t btr2;
};

struct NodeDef : Node {
    std::string name;
};

struct ValueEncodingDescription : Node {
    int64_t value;
    std::string description;
};

struct ValueTable : Node {
    std::string name;
    std::vector<ValueEncodingDescription> descriptions;
};

// Multiplexer types
enum class MultiplexerType {
    None,           // Not a multiplexer signal
    MuxSwitch,      // M - multiplexer switch
    MuxValue        // m<num> - multiplexed signal
};

struct Signal : Node {
    std::string name;
    MultiplexerType mux_type = MultiplexerType::None;
    uint64_t mux_value = 0;  // Only used when mux_type is MuxValue
    uint64_t start_bit;
    uint64_t length;
    char byte_order;      // '0' = Motorola (big endian), '1' = Intel (little endian)
    char value_type;      // '+' = unsigned, '-' = signed
    double factor;
    double offset;
    double minimum;
    double maximum;
    std::string unit;
    std::vector<std::string> receivers;
};

struct Message : Node {
    uint64_t id;
    std::string name;
    uint64_t size;
    std::string transmitter;
    std::vector<Signal> signals;
};

struct MessageTransmitter : Node {
    uint64_t message_id;
    std::vector<std::string> transmitters;
};


struct Comment : Node {
    enum class Type {
        Network,
        Node,
        Message,
        Signal
    };
    
    Type type;
    std::string text;
    
    // Used depending on type
    std::string node_name;      // For Node comments
    uint64_t message_id;        // For Message/Signal comments
    std::string signal_name;    // For Signal comments
};

struct AttributeDefinition : Node {
    enum class ObjectType {
        Network,
        Node,
        Message,
        Signal,
        RelNode,
        RelMessage,
        RelSignal
    };
    
    ObjectType object_type;
    std::string name;
    std::string value_type;
    
    // For numeric types
    std::optional<double> min_value;
    std::optional<double> max_value;
    
    // For enum types
    std::vector<std::string> enum_values;
    
    // For string types
    std::optional<std::string> default_value;
};

struct AttributeDefault : Node {
    std::string name;
    AttributeValue value;
};

struct AttributeValue_t : Node {
    enum class Type {
        Network,
        Node,
        Message, 
        Signal
    };
    
    Type type;
    std::string attribute_name;
    AttributeValue value;
    
    // Used depending on type
    std::string node_name;      // For Node
    uint64_t message_id;        // For Message/Signal
    std::string signal_name;    // For Signal
};

struct ValueDescription : Node {
    enum class Type {
        Signal
    };
    
    Type type;
    uint64_t message_id;     // For Signal only
    std::string object_name; // Signal name
    std::vector<ValueEncodingDescription> descriptions;
};

struct SignalExtendedValueType : Node {
    uint64_t message_id;
    std::string signal_name;
    uint64_t value_type;
};

struct SignalMultiplexerValue : Node {
    uint64_t message_id;
    std::string signal_name;
    std::string switch_name;
    
    struct Range {
        uint64_t from;
        uint64_t to;
    };
    std::vector<Range> value_ranges;
};

struct SignalGroup : Node {
    uint64_t message_id;
    std::string group_name;
    uint64_t repetitions;
    std::vector<std::string> signal_names;
};

struct SignalType : Node {
    std::string name;
    uint64_t size;
    char byte_order;
    char value_type;
    double factor;
    double offset;
    double minimum;
    double maximum;
    std::string unit;
    double default_value;
    std::string value_table;
};

// Main network structure
struct Network : Node {
    Version version;
    std::vector<std::string> new_symbols;
    std::optional<BitTiming> bit_timing;
    std::vector<NodeDef> nodes;
    std::vector<ValueTable> value_tables;
    std::vector<Message> messages;
    std::vector<MessageTransmitter> message_transmitters;
    std::vector<SignalType> signal_types;
    std::vector<Comment> comments;
    std::vector<AttributeDefinition> attribute_definitions;
    std::vector<AttributeDefault> attribute_defaults;
    std::vector<AttributeValue_t> attribute_values;
    std::vector<ValueDescription> value_descriptions;
    std::vector<SignalGroup> signal_groups;
    std::vector<SignalExtendedValueType> signal_extended_value_types;
    std::vector<SignalMultiplexerValue> signal_multiplexer_values;
};

} // namespace AST
} // namespace dbcppp