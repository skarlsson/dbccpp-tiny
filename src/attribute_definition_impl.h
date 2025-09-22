#pragma once

#include "dbcppp-tiny/attribute_definition.h"

namespace dbcppp
{
    class AttributeDefinitionImpl final
        : public IAttributeDefinition
    {
    public:
        AttributeDefinitionImpl(std::string&& name, EObjectType object_type, value_type_t value_type);
        
        virtual EObjectType ObjectType() const override;
        virtual const std::string& Name() const override;
        virtual const value_type_t& ValueType() const override;

    private:
        std::string _name;
        EObjectType _object_type;
        value_type_t _value_type;
    };
}
