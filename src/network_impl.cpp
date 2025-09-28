#include <algorithm>
#include <cstring>
#include "dbcppp-tiny/network.h"
#include "network_impl.h"
#include "log.h"

using namespace dbcppp;

std::unique_ptr<INetwork> INetwork::Create(
      std::string&& version
    , std::vector<std::string>&& new_symbols
    , std::unique_ptr<IBitTiming>&& bit_timing
    , std::vector<std::unique_ptr<INode>>&& nodes
    , std::vector<std::unique_ptr<IValueTable>>&& value_tables
    , std::vector<std::unique_ptr<IMessage>>&& messages
    , std::vector<std::unique_ptr<IAttributeDefinition>>&& attribute_definitions
    , std::vector<std::unique_ptr<IAttribute>>&& attribute_defaults
    , std::vector<std::unique_ptr<IAttribute>>&& attribute_values)
{
    BitTimingImpl bt = std::move(static_cast<BitTimingImpl&>(*bit_timing));
    bit_timing.reset(nullptr);
    std::vector<NodeImpl> ns;
    std::vector<ValueTableImpl> vts;
    std::vector<MessageImpl> ms;
    std::vector<AttributeDefinitionImpl> ads;
    std::vector<AttributeImpl> avds;
    std::vector<AttributeImpl> avs;
    for (auto& n : nodes)
    {
        ns.push_back(std::move(static_cast<NodeImpl&>(*n)));
        n.reset(nullptr);
    }
    for (auto& vt : value_tables)
    {
        vts.push_back(std::move(static_cast<ValueTableImpl&>(*vt)));
        vt.reset(nullptr);
    }
    for (auto& m : messages)
    {
        ms.push_back(std::move(static_cast<MessageImpl&>(*m)));
        m.reset(nullptr);
    }
    for (auto& ad : attribute_definitions)
    {
        ads.push_back(std::move(static_cast<AttributeDefinitionImpl&>(*ad)));
        ad.reset(nullptr);
    }
    for (auto& ad : attribute_defaults)
    {
        avds.push_back(std::move(static_cast<AttributeImpl&>(*ad)));
        ad.reset(nullptr);
    }
    for (auto& av : attribute_values)
    {
        avs.push_back(std::move(static_cast<AttributeImpl&>(*av)));
        av.reset(nullptr);
    }
    return std::make_unique<NetworkImpl>(
          std::move(version)
        , std::move(new_symbols)
        , std::move(bt)
        , std::move(ns)
        , std::move(vts)
        , std::move(ms)
        , std::move(ads)
        , std::move(avds)
        , std::move(avs));
}

NetworkImpl::NetworkImpl(
      std::string&& version
    , std::vector<std::string>&& new_symbols
    , BitTimingImpl&& bit_timing
    , std::vector<NodeImpl>&& nodes
    , std::vector<ValueTableImpl>&& value_tables
    , std::vector<MessageImpl>&& messages
    , std::vector<AttributeDefinitionImpl>&& attribute_definitions
    , std::vector<AttributeImpl>&& attribute_defaults
    , std::vector<AttributeImpl>&& attribute_values)

    : _version(std::move(version))
    , _new_symbols(std::move(new_symbols))
    , _bit_timing(std::move(bit_timing))
    , _nodes(std::move(nodes))
    , _value_tables(std::move(value_tables))
    , _messages(std::move(messages))
    , _attribute_definitions(std::move(attribute_definitions))
    , _attribute_defaults(std::move(attribute_defaults))
    , _attribute_values(std::move(attribute_values))
{}
const std::string& NetworkImpl::Version() const
{
    return _version;
}
const std::string& NetworkImpl::NewSymbols_Get(std::size_t i) const
{
    return _new_symbols[i];
}
uint64_t NetworkImpl::NewSymbols_Size() const
{
    return _new_symbols.size();
}
const IBitTiming& NetworkImpl::BitTiming() const
{
    return _bit_timing;
}
const INode& NetworkImpl::Nodes_Get(std::size_t i) const
{
    return _nodes[i];
}
uint64_t NetworkImpl::Nodes_Size() const
{
    return _nodes.size();
}
const IValueTable& NetworkImpl::ValueTables_Get(std::size_t i) const
{
    return _value_tables[i];
}
uint64_t NetworkImpl::ValueTables_Size() const
{
    return _value_tables.size();
}
const IMessage& NetworkImpl::Messages_Get(std::size_t i) const
{
    return _messages[i];
}
uint64_t NetworkImpl::Messages_Size() const
{
    return _messages.size();
}
const IAttributeDefinition& NetworkImpl::AttributeDefinitions_Get(std::size_t i) const
{
    return _attribute_definitions[i];
}
uint64_t NetworkImpl::AttributeDefinitions_Size() const
{
    return _attribute_definitions.size();
}
const IAttribute& NetworkImpl::AttributeDefaults_Get(std::size_t i) const
{
    return _attribute_defaults[i];
}
uint64_t NetworkImpl::AttributeDefaults_Size() const
{
    return _attribute_defaults.size();
}
const IAttribute& NetworkImpl::AttributeValues_Get(std::size_t i) const
{
    return _attribute_values[i];
}
uint64_t NetworkImpl::AttributeValues_Size() const
{
    return _attribute_values.size();
}
const IMessage* NetworkImpl::ParentMessage(const ISignal* sig) const
{
    const IMessage* parent = nullptr;
    for (const auto& msg : _messages)
    {
        auto iter = std::find_if(msg.signals().begin(), msg.signals().end(),
            [&](const SignalImpl& other) { return &other == sig; });
        if (iter != msg.signals().end())
        {
            parent = &msg;
            break;
        }
    }
    return parent;
}
std::string& NetworkImpl::version()
{
    return _version;
}
std::vector<std::string>& NetworkImpl::newSymbols()
{
    return _new_symbols;
}
BitTimingImpl& NetworkImpl::bitTiming()
{
    return _bit_timing;
}
std::vector<NodeImpl>& NetworkImpl::nodes()
{
    return _nodes;
}
std::vector<ValueTableImpl>& NetworkImpl::valueTables()
{
    return _value_tables;
}
std::vector<MessageImpl>& NetworkImpl::messages()
{
    return _messages;
}
std::vector<AttributeDefinitionImpl>& NetworkImpl::attributeDefinitions()
{
    return _attribute_definitions;
}
std::vector<AttributeImpl>& NetworkImpl::attributeDefaults()
{
    return _attribute_defaults;
}
std::vector<AttributeImpl>& NetworkImpl::attributeValues()
{
    return _attribute_values;
}

std::map<std::string, std::unique_ptr<INetwork>> INetwork::LoadNetworkFromFile(const std::string& filename)
{
    auto result = std::map<std::string, std::unique_ptr<INetwork>>();

    // Check file extension
    size_t dot_pos = filename.rfind('.');
    if (dot_pos != std::string::npos) {
        std::string ext = filename.substr(dot_pos);
        if (ext == ".dbc") {
            auto net = LoadDBCFromFile(filename.c_str());
            if (net) {
                result.insert(std::make_pair("", std::move(net)));
            }
        }
    }

    return result;
}
