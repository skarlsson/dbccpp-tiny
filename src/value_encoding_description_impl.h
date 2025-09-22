
#pragma once

#include <string>
#include <dbcppp-tiny/value_encoding_description.h>

namespace dbcppp
{
    class ValueEncodingDescriptionImpl
        : public IValueEncodingDescription
    {
    public:
        ValueEncodingDescriptionImpl(int64_t value, std::string&& description);
        virtual int64_t Value() const override;
        virtual const std::string& Description() const override;

    private:
        int64_t _value;
        std::string _description;
    };
}
