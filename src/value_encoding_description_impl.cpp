
#include "value_encoding_description_impl.h"

using namespace dbcppp;

std::unique_ptr<IValueEncodingDescription> IValueEncodingDescription::Create(int64_t value, std::string&& description)
{
    return std::make_unique<ValueEncodingDescriptionImpl>(value, std::move(description));
}


ValueEncodingDescriptionImpl::ValueEncodingDescriptionImpl(int64_t value, std::string&& description)
    : _value(value)
    , _description(std::move(description))
{}
int64_t ValueEncodingDescriptionImpl::Value() const
{
    return _value;
}
const std::string& ValueEncodingDescriptionImpl::Description() const
{
    return _description;
}
