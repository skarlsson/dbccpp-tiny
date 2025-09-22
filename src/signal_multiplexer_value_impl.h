
#pragma once

#include <dbcppp-tiny/signal_multiplexer_value.h>

namespace dbcppp
{
    class SignalMultiplexerValueImpl
        : public ISignalMultiplexerValue
    {
    public:
        SignalMultiplexerValueImpl(
              std::string&& switch_name
            , std::vector<Range>&& value_ranges);
            
        virtual const std::string& SwitchName() const override;
        virtual const Range& ValueRanges_Get(std::size_t i) const override;
        virtual uint64_t ValueRanges_Size() const override;

    private:
        std::string _switch_name;
        std::vector<Range> _value_ranges;
    };
}
