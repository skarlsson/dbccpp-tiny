#include <algorithm>
#include "signal_multiplexer_value_impl.h"

using namespace dbcppp;

std::unique_ptr<ISignalMultiplexerValue> ISignalMultiplexerValue::Create(
      std::string&& switch_name
    , std::vector<Range>&& value_ranges)
{
    return std::make_unique<SignalMultiplexerValueImpl>(
          std::move(switch_name)
        , std::move(value_ranges));
}

SignalMultiplexerValueImpl::SignalMultiplexerValueImpl(
      std::string&& switch_name
    , std::vector<Range>&& value_ranges)

    : _switch_name(std::move(switch_name))
    , _value_ranges(std::move(value_ranges))
{}
            

const std::string& SignalMultiplexerValueImpl::SwitchName() const
{
    return _switch_name;
}
const ISignalMultiplexerValue::Range& SignalMultiplexerValueImpl::ValueRanges_Get(std::size_t i) const
{
    return _value_ranges[i];
}
uint64_t SignalMultiplexerValueImpl::ValueRanges_Size() const
{
    return _value_ranges.size();
}
