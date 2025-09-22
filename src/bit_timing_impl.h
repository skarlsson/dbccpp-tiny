#pragma once

#include "dbcppp-tiny/bit_timing.h"

namespace dbcppp
{
    class BitTimingImpl final
        : public IBitTiming
    {
    public:
        BitTimingImpl();
        BitTimingImpl(uint64_t baudrate, uint64_t BTR1, uint64_t BTR2);
        
        virtual uint64_t Baudrate() const override;
        virtual uint64_t BTR1() const override;
        virtual uint64_t BTR2() const override;

    private:
        uint64_t _baudrate;
        uint64_t _BTR1;
        uint64_t _BTR2;
    };
}