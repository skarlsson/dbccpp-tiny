#pragma once

#include <map>
#include <memory>
#include <optional>
#include <functional>

#include "dbcppp-tiny/value_table.h"
#include "signal_type_impl.h"
#include "value_encoding_description_impl.h"

namespace dbcppp
{
    class SignalType;
    class ValueTableImpl final
        : public IValueTable
    {
    public:
        ValueTableImpl(
              std::string&& name
            , std::optional<SignalTypeImpl>&& signal_type
            , std::vector<ValueEncodingDescriptionImpl>&& value_encoding_descriptions);

        virtual const std::string& Name() const override;
        virtual std::optional<std::reference_wrapper<const ISignalType>> SignalType() const override;
        virtual const IValueEncodingDescription& ValueEncodingDescriptions_Get(std::size_t i) const override;
        virtual uint64_t ValueEncodingDescriptions_Size() const override;

    private:
        std::string _name;
        std::optional<SignalTypeImpl> _signal_type;
        std::vector<ValueEncodingDescriptionImpl> _value_encoding_descriptions;
    };
}