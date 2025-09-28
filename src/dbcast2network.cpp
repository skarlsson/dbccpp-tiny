#include <iterator>
#include <regex>
#include <variant>
#include <unordered_map>
#include <cassert>
#include <algorithm>

#include "dbcppp-tiny/network.h"
#include "dbcast.h"
#include "dbc_parser.h"
#include "file_reader.h"
#include "log.h"

using namespace dbcppp;

namespace
{
using AttributeList = std::vector<const AST::AttributeValue_t*>;

struct SignalCache
{
    AttributeList Attributes;
    const AST::ValueDescription* ValueDescriptions = nullptr;
};

struct MessageCache
{
    std::unordered_map<std::string, SignalCache> Signals;
    AttributeList Attributes;
};

struct NodeCache
{
    AttributeList Attributes;
};

struct Cache
{

    AttributeList NetworkAttributes;
    std::unordered_map<std::string, NodeCache> Nodes;
    std::unordered_map<uint64_t, MessageCache> Messages;
};

} // anon

static auto getVersion(const AST::Network& net)
{
    return net.version.version;
}

static auto getNewSymbols(const AST::Network& net)
{
    return net.new_symbols;
}

static auto getSignalType(const AST::Network& net, const AST::ValueTable& vt)
{
    std::optional<std::unique_ptr<ISignalType>> signal_type;
    auto iter = std::find_if(net.signal_types.begin(), net.signal_types.end(),
        [&](const auto& st)
        {
            return st.value_table == vt.name;
        });
    if (iter != net.signal_types.end())
    {
        auto& st = *iter;
        signal_type = ISignalType::Create(
              std::string(st.name)
            , st.size
            , st.byte_order == '0' ? ISignal::EByteOrder::BigEndian : ISignal::EByteOrder::LittleEndian
            , st.value_type == '+' ? ISignal::EValueType::Unsigned : ISignal::EValueType::Signed
            , st.factor
            , st.offset
            , st.minimum
            , st.maximum
            , std::string(st.unit)
            , st.default_value
            , std::string(st.value_table));
    }
    return signal_type;
}

static auto getValueTables(const AST::Network& net)
{
    std::vector<std::unique_ptr<IValueTable>> value_tables;
    for (const auto& vt : net.value_tables)
    {
        auto sig_type = getSignalType(net, vt);
        std::vector<std::unique_ptr<IValueEncodingDescription>> copy_ved;
        for (const auto& ved : vt.descriptions)
        {
            auto pved = IValueEncodingDescription::Create(ved.value, std::string(ved.description));
            copy_ved.push_back(std::move(pved));
        }
        auto nvt = IValueTable::Create(std::string(vt.name), std::move(sig_type), std::move(copy_ved));
        value_tables.push_back(std::move(nvt));
    }
    return value_tables;
}

static auto getBitTiming(const AST::Network& net)
{
    std::unique_ptr<IBitTiming> bit_timing;
    if (net.bit_timing)
    {
        bit_timing = IBitTiming::Create(net.bit_timing->baudrate, net.bit_timing->btr1, net.bit_timing->btr2);
    }
    else
    {
        bit_timing = IBitTiming::Create(0, 0, 0);
    }
    return bit_timing;
}

static auto convertAttributeValue(const AST::AttributeValue& value)
{
    IAttribute::value_t result;
    std::visit([&](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int64_t>)
            result = IAttribute::value_t(std::in_place_type<int64_t>, v);
        else if constexpr (std::is_same_v<T, double>)
            result = IAttribute::value_t(std::in_place_type<double>, v);
        else if constexpr (std::is_same_v<T, std::string>)
            result = IAttribute::value_t(std::in_place_type<std::string>, v);
    }, value);
    return result;
}

static auto getAttributeValues(const AST::Network& net, const AST::NodeDef& n, Cache const& cache)
{
    std::vector<std::unique_ptr<IAttribute>> attribute_values;

    auto node_it = cache.Nodes.find(n.name);

    if (node_it != cache.Nodes.end()) {
        attribute_values.reserve(node_it->second.Attributes.size());

        for (auto av : node_it->second.Attributes) {
            if (av->type == AST::AttributeValue_t::Type::Node && av->node_name == n.name) {
                auto name = av->attribute_name;
                auto value = convertAttributeValue(av->value);
                auto attribute = IAttribute::Create(std::string(av->attribute_name), IAttributeDefinition::EObjectType::Node, std::move(value));
                attribute_values.emplace_back(std::move(attribute));
            }
        }
    }

    return attribute_values;
}


static auto getNodes(const AST::Network& net, Cache const& cache)
{
    std::vector<std::unique_ptr<INode>> nodes;
    for (const auto& n : net.nodes)
    {
        auto attribute_values = getAttributeValues(net, n, cache);
        auto nn = INode::Create(std::string(n.name), std::move(attribute_values));
        nodes.push_back(std::move(nn));
    }
    return nodes;
}

static auto getAttributeValues(const AST::Network& net, const AST::Message& m, const AST::Signal& s, Cache const& cache)
{
    std::vector<std::unique_ptr<IAttribute>> attribute_values;
    auto const message_it = cache.Messages.find(m.id);

    if (message_it != cache.Messages.end()) {
        auto const signal_it = message_it->second.Signals.find(s.name);

        if (signal_it != message_it->second.Signals.end()) {
            attribute_values.reserve(signal_it->second.Attributes.size());

            for (auto av : signal_it->second.Attributes)
            {
                if (av->type == AST::AttributeValue_t::Type::Signal) {
                    auto value = convertAttributeValue(av->value);
                    auto attribute = IAttribute::Create(std::string(av->attribute_name), IAttributeDefinition::EObjectType::Signal, std::move(value));
                    attribute_values.emplace_back(std::move(attribute));
                }
            }
        }
    }

    return attribute_values;
}

static auto getValueDescriptions(const AST::Network& net, const AST::Message& m, const AST::Signal& s, Cache const& cache)
{
    std::vector<std::unique_ptr<IValueEncodingDescription>> value_descriptions;
    auto const message_it = cache.Messages.find(m.id);

    if (message_it != cache.Messages.end()) {
        auto const signal_it = message_it->second.Signals.find(s.name);

        if (signal_it != message_it->second.Signals.end()) {
            if (signal_it->second.ValueDescriptions) {
                value_descriptions.reserve(signal_it->second.ValueDescriptions->descriptions.size());

                for (const auto& vd : signal_it->second.ValueDescriptions->descriptions)
                {
                    auto pvd = IValueEncodingDescription::Create(vd.value, std::string(vd.description));
                    value_descriptions.push_back(std::move(pvd));
                }
            }
        }
    }
    return value_descriptions;
}


static auto getSignalExtendedValueType(const AST::Network& net, const AST::Message& m, const AST::Signal& s)
{
    ISignal::EExtendedValueType extended_value_type = ISignal::EExtendedValueType::Integer;
    auto iter = std::find_if(net.signal_extended_value_types.begin(), net.signal_extended_value_types.end(),
        [&](const AST::SignalExtendedValueType& sev)
        {
            return sev.message_id == m.id && sev.signal_name == s.name;
        });
    if (iter != net.signal_extended_value_types.end())
    {
        switch (iter->value_type)
        {
        case 1: extended_value_type = ISignal::EExtendedValueType::Float; break;
        case 2: extended_value_type = ISignal::EExtendedValueType::Double; break;
        }
    }
    return extended_value_type;
}

static auto getSignalMultiplexerValues(const AST::Network& net, const std::string& s, const uint64_t m)
{
    std::vector<std::unique_ptr<ISignalMultiplexerValue>> signal_multiplexer_values;
    for (const auto& gsmv : net.signal_multiplexer_values)
    {
        if (gsmv.signal_name == s && gsmv.message_id == m)
        {
            auto switch_name = gsmv.switch_name;
            std::vector<ISignalMultiplexerValue::Range> value_ranges;
            for (const auto& r : gsmv.value_ranges)
            {
                value_ranges.push_back({static_cast<std::size_t>(r.from), static_cast<std::size_t>(r.to)});
            }
            auto signal_multiplexer_value = ISignalMultiplexerValue::Create(
                  std::move(switch_name)
                , std::move(value_ranges));
            signal_multiplexer_values.push_back(std::move(signal_multiplexer_value));
        }
    }
    return signal_multiplexer_values;
}

static auto getSignals(const AST::Network& net, const AST::Message& m, Cache const& cache)
{
    std::vector<std::unique_ptr<ISignal>> signals;

    signals.reserve(m.signals.size());

    for (const AST::Signal& s : m.signals)
    {
        std::vector<std::string> receivers = s.receivers;
        auto attribute_values = getAttributeValues(net, m, s, cache);
        auto value_descriptions = getValueDescriptions(net, m, s, cache);
        auto extended_value_type = getSignalExtendedValueType(net, m, s);
        auto multiplexer_indicator = ISignal::EMultiplexer::NoMux;
        auto signal_multiplexer_values = getSignalMultiplexerValues(net, s.name, m.id);
        uint64_t multiplexer_switch_value = 0;
        
        switch (s.mux_type)
        {
        case AST::MultiplexerType::None:
            multiplexer_indicator = ISignal::EMultiplexer::NoMux;
            break;
        case AST::MultiplexerType::MuxSwitch:
            multiplexer_indicator = ISignal::EMultiplexer::MuxSwitch;
            break;
        case AST::MultiplexerType::MuxValue:
            multiplexer_indicator = ISignal::EMultiplexer::MuxValue;
            multiplexer_switch_value = s.mux_value;
            break;
        }

        auto ns = ISignal::Create(
              m.size
            , std::string(s.name)
            , multiplexer_indicator
            , multiplexer_switch_value
            , s.start_bit
            , s.length
            , s.byte_order == '0' ? ISignal::EByteOrder::BigEndian : ISignal::EByteOrder::LittleEndian
            , s.value_type == '+' ? ISignal::EValueType::Unsigned : ISignal::EValueType::Signed
            , s.factor
            , s.offset
            , s.minimum
            , s.maximum
            , std::string(s.unit)
            , std::move(receivers)
            , std::move(attribute_values)
            , std::move(value_descriptions)
            , extended_value_type
            , std::move(signal_multiplexer_values));
            
        if (ns->Error(ISignal::EErrorCode::SignalExceedsMessageSize))
        {
            LOG_WARNING("Signal '%s::%s' start_bit + bit_size exceeds the byte size of the message! "
                       "Ignoring this error will lead to garbage data when using the decode function of this signal.",
                       m.name.c_str(), s.name.c_str());
        }
        if (ns->Error(ISignal::EErrorCode::WrongBitSizeForExtendedDataType))
        {
            LOG_WARNING("Signal '%s::%s' bit_size does not fit the bit size of the specified ExtendedValueType.",
                       m.name.c_str(), s.name.c_str());
        }
        if (ns->Error(ISignal::EErrorCode::MaschinesFloatEncodingNotSupported))
        {
            LOG_WARNING("Signal '%s::%s' uses type float but the system this program is running on "
                       "does not use IEEE 754 encoding for floats.",
                       m.name.c_str(), s.name.c_str());
        }
        if (ns->Error(ISignal::EErrorCode::MaschinesDoubleEncodingNotSupported))
        {
            LOG_WARNING("Signal '%s::%s' uses type double but the system this program is running on "
                       "does not use IEEE 754 encoding for doubles.",
                       m.name.c_str(), s.name.c_str());
        }
        signals.emplace_back(std::move(ns));
    }
    return signals;
}

static auto getMessageTransmitters(const AST::Network& net, const AST::Message& m)
{
    std::vector<std::string> message_transmitters;
    auto iter_mt = std::find_if(net.message_transmitters.begin(), net.message_transmitters.end(),
        [&](const AST::MessageTransmitter& mt)
        {
            return mt.message_id == m.id;
        });
    if (iter_mt != net.message_transmitters.end())
    {
        message_transmitters = iter_mt->transmitters;
    }
    return message_transmitters;
}

static auto getAttributeValues(const AST::Network& net, const AST::Message& m, Cache const& cache)
{
    std::vector<std::unique_ptr<IAttribute>> attribute_values;

    auto message_it = cache.Messages.find(m.id);

    if (message_it != cache.Messages.end()) {
        attribute_values.reserve(message_it->second.Attributes.size());

        for (auto av: message_it->second.Attributes) {
            if (av->type == AST::AttributeValue_t::Type::Message) {
                auto value = convertAttributeValue(av->value);
                auto attribute = IAttribute::Create(std::string(av->attribute_name), IAttributeDefinition::EObjectType::Message, std::move(value));
                attribute_values.emplace_back(std::move(attribute));
            }
        }
    }
    return attribute_values;
}


static auto getSignalGroups(const AST::Network& net, const AST::Message& m)
{
    std::vector<std::unique_ptr<ISignalGroup>> signal_groups;
    for (const auto& sg : net.signal_groups)
    {
        if (sg.message_id == m.id)
        {
            auto signal_group = ISignalGroup::Create(
                  sg.message_id
                , std::string(sg.group_name)
                , sg.repetitions
                , std::vector<std::string>(sg.signal_names));
            signal_groups.push_back(std::move(signal_group));
        }
    }
    return signal_groups;
}

static auto getMessages(const AST::Network& net, Cache const& cache)
{
    std::vector<std::unique_ptr<IMessage>> messages;

    messages.reserve(net.messages.size());

    for (const auto& m : net.messages)
    {
        auto message_transmitters = getMessageTransmitters(net, m);
        auto signals = getSignals(net, m, cache);
        auto attribute_values = getAttributeValues(net, m, cache);
        auto signal_groups = getSignalGroups(net, m);
        auto msg = IMessage::Create(
              m.id
            , std::string(m.name)
            , m.size
            , std::string(m.transmitter)
            , std::move(message_transmitters)
            , std::move(signals)
            , std::move(attribute_values)
            , std::move(signal_groups));
        if (msg->Error() == IMessage::EErrorCode::MuxValeWithoutMuxSignal)
        {
            LOG_WARNING("Message '%s' has mux value but no mux signal!", msg->Name().c_str());
        }
        messages.emplace_back(std::move(msg));
    }
    return messages;
}

static auto getMessagesFiltered(const AST::Network& net, Cache const& cache,
    INetwork::MessageFilter message_filter,
    INetwork::SignalFilter signal_filter)
{
    std::vector<std::unique_ptr<IMessage>> messages;
    uint32_t discarded_messages = 0;
    uint32_t discarded_signals = 0;

    for (const auto& m : net.messages)
    {
        // Check if we should keep this message
        if (!message_filter(m.id, std::string(m.name))) {
            discarded_messages++;
            continue;
        }

        auto message_transmitters = getMessageTransmitters(net, m);

        // Filter signals based on signal_filter
        auto all_signals = getSignals(net, m, cache);
        std::vector<std::unique_ptr<ISignal>> filtered_signals;
        for (auto& sig : all_signals) {
            if (signal_filter(sig->Name(), m.id)) {
                filtered_signals.push_back(std::move(sig));
            } else {
                discarded_signals++;
            }
        }

        auto attribute_values = getAttributeValues(net, m, cache);
        auto signal_groups = getSignalGroups(net, m);

        auto msg = IMessage::Create(
              m.id
            , std::string(m.name)
            , m.size
            , std::string(m.transmitter)
            , std::move(message_transmitters)
            , std::move(filtered_signals)
            , std::move(attribute_values)
            , std::move(signal_groups));

        if (msg->Error() == IMessage::EErrorCode::MuxValeWithoutMuxSignal)
        {
            LOG_WARNING("Message '%s' has mux value but no mux signal!", msg->Name().c_str());
        }
        messages.emplace_back(std::move(msg));
    }

    if (discarded_messages > 0 || discarded_signals > 0) {
        LOG_INFO("Filter discarded %u messages and %u signals", discarded_messages, discarded_signals);
    }

    return messages;
}

static auto getAttributeDefinitions(const AST::Network& net)
{
    std::vector<std::unique_ptr<IAttributeDefinition>> attribute_definitions;
    
    for (const auto& ad : net.attribute_definitions)
    {
        IAttributeDefinition::EObjectType object_type;
        
        switch (ad.object_type)
        {
        case AST::AttributeDefinition::ObjectType::Network:
            object_type = IAttributeDefinition::EObjectType::Network;
            break;
        case AST::AttributeDefinition::ObjectType::Node:
            object_type = IAttributeDefinition::EObjectType::Node;
            break;
        case AST::AttributeDefinition::ObjectType::Message:
            object_type = IAttributeDefinition::EObjectType::Message;
            break;
        case AST::AttributeDefinition::ObjectType::Signal:
            object_type = IAttributeDefinition::EObjectType::Signal;
            break;
        case AST::AttributeDefinition::ObjectType::RelNode:
            object_type = IAttributeDefinition::EObjectType::Node;  // Map to Node for now
            break;
        case AST::AttributeDefinition::ObjectType::RelMessage:
            object_type = IAttributeDefinition::EObjectType::Message;  // Map to Message for now
            break;
        case AST::AttributeDefinition::ObjectType::RelSignal:
            object_type = IAttributeDefinition::EObjectType::Signal;  // Map to Signal for now
            break;
        default:
            LOG_ERROR("Unknown attribute definition object type: %d", static_cast<int>(ad.object_type));
            continue;  // Skip this attribute definition
        }

        IAttributeDefinition::value_type_t value_type;
        
        // Determine the attribute value type based on the value_type string
        if (ad.value_type == "INT" || ad.value_type == "HEX") {
            if (ad.value_type == "INT") {
                IAttributeDefinition::ValueTypeInt type;
                type.minimum = ad.min_value.value_or(0);
                type.maximum = ad.max_value.value_or(0);
                value_type = type;
            } else {
                IAttributeDefinition::ValueTypeHex type;
                type.minimum = ad.min_value.value_or(0);
                type.maximum = ad.max_value.value_or(0);
                value_type = type;
            }
        }
        else if (ad.value_type == "FLOAT") {
            IAttributeDefinition::ValueTypeFloat type;
            type.minimum = ad.min_value.value_or(0);
            type.maximum = ad.max_value.value_or(0);
            value_type = type;
        }
        else if (ad.value_type == "STRING") {
            value_type = IAttributeDefinition::ValueTypeString();
        }
        else if (ad.value_type == "ENUM") {
            IAttributeDefinition::ValueTypeEnum type;
            type.values = ad.enum_values;
            value_type = type;
        }
        
        auto nad = IAttributeDefinition::Create(std::string(ad.name), object_type, std::move(value_type));
        attribute_definitions.push_back(std::move(nad));
    }
    return attribute_definitions;
}

static auto getAttributeDefaults(const AST::Network& net)
{
    std::vector<std::unique_ptr<IAttribute>> attribute_defaults;
    for (auto& ad : net.attribute_defaults)
    {
        auto value = convertAttributeValue(ad.value);
        auto nad = IAttribute::Create(std::string(ad.name), IAttributeDefinition::EObjectType::Network, value);
        attribute_defaults.push_back(std::move(nad));
    }
    return attribute_defaults;
}

static auto getAttributeValues(const AST::Network& net, Cache const& cache)
{
    std::vector<std::unique_ptr<IAttribute>> attribute_values;

    attribute_values.reserve(cache.NetworkAttributes.size());

    for (auto av : cache.NetworkAttributes)
    {
        if (av->type == AST::AttributeValue_t::Type::Network) {
            auto value = convertAttributeValue(av->value);
            auto attribute = IAttribute::Create(
                std::string(av->attribute_name)
                , IAttributeDefinition::EObjectType::Network
                , std::move(value));
            attribute_values.emplace_back(std::move(attribute));
        }
    }
    return attribute_values;
}


std::unique_ptr<INetwork> DBCAST2Network(const AST::Network& net)
{
    Cache cache;

    // Build cache for attributes
    for (const auto& av : net.attribute_values)
    {
        switch (av.type) {
        case AST::AttributeValue_t::Type::Network:
            cache.NetworkAttributes.emplace_back(&av);
            break;
        case AST::AttributeValue_t::Type::Node:
            cache.Nodes[av.node_name].Attributes.emplace_back(&av);
            break;
        case AST::AttributeValue_t::Type::Message:
            cache.Messages[av.message_id].Attributes.emplace_back(&av);
            break;
        case AST::AttributeValue_t::Type::Signal:
            cache.Messages[av.message_id].Signals[av.signal_name].Attributes.emplace_back(&av);
            break;
        }
    }

    // Build cache for value descriptions
    for (const auto& vd : net.value_descriptions)
    {
        if (vd.type == AST::ValueDescription::Type::Signal) {
            // This is a signal value description
            cache.Messages[vd.message_id].Signals[vd.object_name].ValueDescriptions = &vd;
        }
    }


    return INetwork::Create(
          getVersion(net)
        , getNewSymbols(net)
        , getBitTiming(net)
        , getNodes(net, cache)
        , getValueTables(net)
        , getMessages(net, cache)
        , getAttributeDefinitions(net)
        , getAttributeDefaults(net)
        , getAttributeValues(net, cache)
);
}

// Filtered version of DBCAST2Network
std::unique_ptr<INetwork> DBCAST2NetworkFiltered(const AST::Network& net,
    INetwork::MessageFilter message_filter,
    INetwork::SignalFilter signal_filter)
{
    Cache cache;

    // Build cache for attributes - only for messages/signals we'll keep
    for (const auto& av : net.attribute_values)
    {
        switch (av.type) {
        case AST::AttributeValue_t::Type::Network:
            cache.NetworkAttributes.emplace_back(&av);
            break;
        case AST::AttributeValue_t::Type::Node:
            cache.Nodes[av.node_name].Attributes.emplace_back(&av);
            break;
        case AST::AttributeValue_t::Type::Message:
            // Only cache if message will be kept
            for (const auto& m : net.messages) {
                if (m.id == av.message_id && message_filter(m.id, std::string(m.name))) {
                    cache.Messages[av.message_id].Attributes.emplace_back(&av);
                    break;
                }
            }
            break;
        case AST::AttributeValue_t::Type::Signal:
            // Only cache if message will be kept
            for (const auto& m : net.messages) {
                if (m.id == av.message_id && message_filter(m.id, std::string(m.name))) {
                    cache.Messages[av.message_id].Signals[av.signal_name].Attributes.emplace_back(&av);
                    break;
                }
            }
            break;
        }
    }

    // Build cache for value descriptions - only for signals we'll keep
    for (const auto& vd : net.value_descriptions)
    {
        if (vd.type == AST::ValueDescription::Type::Signal) {
            // Check if message will be kept
            for (const auto& m : net.messages) {
                if (m.id == vd.message_id && message_filter(m.id, std::string(m.name))) {
                    cache.Messages[vd.message_id].Signals[vd.object_name].ValueDescriptions = &vd;
                    break;
                }
            }
        }
    }

    return INetwork::Create(
          getVersion(net)
        , getNewSymbols(net)
        , getBitTiming(net)
        , getNodes(net, cache)
        , getValueTables(net)
        , getMessagesFiltered(net, cache, message_filter, signal_filter)
        , getAttributeDefinitions(net)
        , getAttributeDefaults(net)
        , getAttributeValues(net, cache)
    );
}

// Implementation without iostream
std::unique_ptr<INetwork> INetwork::LoadDBCFromFile(const char* filename,
    MessageFilter message_filter,
    SignalFilter signal_filter)
{
    // Use FileLineReader to read file without iostream
    FileLineReader reader;
    if (!reader.open(filename)) {
        LOG_ERROR("Cannot open file: %s", filename);
        return nullptr;
    }

    // Read entire file into string
    std::string content;
    std::string line;
    while (reader.readLine(line)) {
        content += line + "\n";
    }
    reader.close();

    // Use existing full parser with filters
    return LoadDBCFromString(content, message_filter, signal_filter);
}

std::unique_ptr<INetwork> INetwork::LoadDBCFromString(const std::string& content,
    MessageFilter message_filter,
    SignalFilter signal_filter)
{
    // Use the existing full DBC parser that has complete implementation
    DBCParser parser;
    auto parseResult = parser.parse(content);

    if (parseResult.isOk()) {
        return DBCAST2NetworkFiltered(*parseResult.value(), message_filter, signal_filter);
    } else {
        LOG_ERROR("Parse error: %s", parseResult.error().toString().c_str());
        return nullptr;
    }
}