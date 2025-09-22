#pragma once

#include "dbcppp-tiny/network.h"
#include "bit_timing_impl.h"
#include "value_table_impl.h"
#include "message_impl.h"
#include "signal_type_impl.h"
#include "attribute_definition_impl.h"
#include "attribute_impl.h"

namespace dbcppp
{
    class NetworkImpl final
        : public INetwork
    {
    public:
        NetworkImpl(
              std::string&& version
            , std::vector<std::string>&& new_symbols
            , BitTimingImpl&& bit_timing
            , std::vector<NodeImpl>&& nodes
            , std::vector<ValueTableImpl>&& value_tables
            , std::vector<MessageImpl>&& messages
            , std::vector<AttributeDefinitionImpl>&& attribute_definitions
            , std::vector<AttributeImpl>&& attribute_defaults
            , std::vector<AttributeImpl>&& attribute_values);
            
        
        virtual const std::string& Version() const override;
        virtual const std::string& NewSymbols_Get(std::size_t i) const override;
        virtual uint64_t NewSymbols_Size() const override;
        virtual const IBitTiming& BitTiming() const override;
        virtual const INode& Nodes_Get(std::size_t i) const override;
        virtual uint64_t Nodes_Size() const override;
        virtual const IValueTable& ValueTables_Get(std::size_t i) const override;
        virtual uint64_t ValueTables_Size() const override;
        virtual const IMessage& Messages_Get(std::size_t i) const override;
        virtual uint64_t Messages_Size() const override;
        virtual const IAttributeDefinition& AttributeDefinitions_Get(std::size_t i) const override;
        virtual uint64_t AttributeDefinitions_Size() const override;
        virtual const IAttribute& AttributeDefaults_Get(std::size_t i) const override;
        virtual uint64_t AttributeDefaults_Size() const override;
        virtual const IAttribute& AttributeValues_Get(std::size_t i) const override;
        virtual uint64_t AttributeValues_Size() const override;
        
        virtual const IMessage* ParentMessage(const ISignal* sig) const override;
        

        std::string& version();
        std::vector<std::string>& newSymbols();
        BitTimingImpl& bitTiming();
        std::vector<NodeImpl>& nodes();
        std::vector<ValueTableImpl>& valueTables();
        std::vector<MessageImpl>& messages();
        std::vector<AttributeDefinitionImpl>& attributeDefinitions();
        std::vector<AttributeImpl>& attributeDefaults();
        std::vector<AttributeImpl>& attributeValues();

    private:
        std::string _version;
        std::vector<std::string> _new_symbols;
        BitTimingImpl _bit_timing;
        std::vector<NodeImpl> _nodes;
        std::vector<ValueTableImpl> _value_tables;
        std::vector<MessageImpl> _messages;
        std::vector<AttributeDefinitionImpl> _attribute_definitions;
        std::vector<AttributeImpl> _attribute_defaults;
        std::vector<AttributeImpl> _attribute_values;
    };
}