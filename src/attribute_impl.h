
#pragma once

#include "dbcppp-tiny/attribute.h"

namespace dbcppp
{
    class AttributeImpl final
        : public IAttribute
    {
    public:
        AttributeImpl(std::string&& name, IAttributeDefinition::EObjectType object_type, IAttribute::value_t value);

        virtual const std::string& Name() const override;
        virtual IAttributeDefinition::EObjectType ObjectType() const override;
        virtual const value_t& Value() const override;

    private:
        std::string _name;
        IAttributeDefinition::EObjectType _object_type;
        IAttribute::value_t _value;
    };
}