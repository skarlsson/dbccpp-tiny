#include <algorithm>
#include "signal_group_impl.h"

using namespace dbcppp;

std::unique_ptr<ISignalGroup> ISignalGroup::Create(
      uint64_t message_id
    , std::string&& name
    , uint64_t repetitions
    , std::vector<std::string>&& signal_names)
{
    return std::make_unique<SignalGroupImpl>(
          message_id
        , std::move(name)
        , repetitions
        , std::move(signal_names));
}


SignalGroupImpl::SignalGroupImpl(
      uint64_t message_id
    , std::string&& name
    , uint64_t repetitions
    , std::vector<std::string>&& signal_names)

    : _message_id(message_id)
    , _name(std::move(name))
    , _repetitions(repetitions)
    , _signal_names(std::move(signal_names))
{}
uint64_t SignalGroupImpl::MessageId() const
{
    return _message_id;
}
const std::string& SignalGroupImpl::Name() const
{
    return _name;
}
uint64_t SignalGroupImpl::Repetitions() const
{
    return _repetitions;
}
const std::string& SignalGroupImpl::SignalNames_Get(std::size_t i) const
{
    return _signal_names[i];
}
uint64_t SignalGroupImpl::SignalNames_Size() const
{
    return _signal_names.size();
}
