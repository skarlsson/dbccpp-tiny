#include "attribute_impl.h"
#include "dbcppp-tiny/network.h"

using namespace dbcppp;

std::unique_ptr<IAttribute> IAttribute::Create(
      std::string&& name
    , IAttributeDefinition::EObjectType object_type
    , value_t value)
{
    return std::make_unique<AttributeImpl>(
          std::move(name)
        , object_type
        , std::move(value));
}

AttributeImpl::AttributeImpl(std::string&& name, IAttributeDefinition::EObjectType object_type, IAttribute::value_t value)
    : _name(std::move(name))
    , _object_type(std::move(object_type))
    , _value(std::move(value))
{}
const std::string& AttributeImpl::Name() const
{
    return _name;
}
IAttributeDefinition::EObjectType AttributeImpl::ObjectType() const
{
    return _object_type;
};
const IAttribute::value_t& AttributeImpl::Value() const
{
    return _value;
}
