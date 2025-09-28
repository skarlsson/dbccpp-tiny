#include <algorithm>
#include "attribute_definition_impl.h"

using namespace dbcppp;

std::unique_ptr<IAttributeDefinition> IAttributeDefinition::Create(
      std::string&& name
    , EObjectType object_type
    , value_type_t&& value_type)
{
    return std::make_unique<AttributeDefinitionImpl>(
          std::move(name)
        , object_type
        , std::move(value_type));
}

AttributeDefinitionImpl::AttributeDefinitionImpl(std::string&& name, EObjectType object_type, value_type_t value_type)
    : _name(std::move(name))
    , _object_type(std::move(object_type))
    , _value_type(std::move(value_type))
{}
IAttributeDefinition::EObjectType AttributeDefinitionImpl::ObjectType() const
{
    return _object_type;
}
const std::string& AttributeDefinitionImpl::Name() const
{
    return _name;
}
const IAttributeDefinition::value_type_t& AttributeDefinitionImpl::ValueType() const
{
    return _value_type;
}
