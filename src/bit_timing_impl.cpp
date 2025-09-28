#include "bit_timing_impl.h"

using namespace dbcppp;

std::unique_ptr<IBitTiming> IBitTiming::Create(uint64_t baudrate, uint64_t BTR1, uint64_t BTR2)
{
    return std::make_unique<BitTimingImpl>(baudrate, BTR1, BTR2);
}
BitTimingImpl::BitTimingImpl()
    : _baudrate(0)
    , _BTR1(0)
    , _BTR2(0)
{}
BitTimingImpl::BitTimingImpl(uint64_t baudrate, uint64_t BTR1, uint64_t BTR2)
    : _baudrate(std::move(baudrate))
    , _BTR1(std::move(BTR1))
    , _BTR2(std::move(BTR2))
{}
uint64_t BitTimingImpl::Baudrate() const
{
    return _baudrate;
}
uint64_t BitTimingImpl::BTR1() const
{
    return _BTR1;
}
uint64_t BitTimingImpl::BTR2() const
{
    return _BTR2;
}
