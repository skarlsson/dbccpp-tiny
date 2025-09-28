# dbcppp-tiny - Embedded CAN Database Parser

A lightweight C++ library for parsing DBC (CAN Database) files, optimized for embedded systems and high-performance decoding.

# Credits
heavily inspired by dbcppp https://github.com/xR3b0rn/dbcppp

## Features

- **Zero Dependencies**: No external libraries required (removed Boost, libxml2)
- **Exception-Free**: Uses `Result<T>` pattern for error handling
- **Decode-Only**: Optimized for receiving CAN messages (encoding removed)
- **Fast Signal Decoding**: Template-based signal extraction
- **Multiplexed Signal Support**: Full support for multiplexed messages
- **Minimal Memory Footprint**: ~650KB release binary with -O2

## Building

```bash
mkdir build
cd build
cmake ..
make
```

### Build Options

- `CMAKE_BUILD_TYPE=Release` - Optimized build
- `BUILD_TESTS=ON/OFF` - Build test suite (default: ON)
- `BUILD_EXAMPLES=ON/OFF` - Build examples (default: OFF)

## Usage

```cpp
#include "dbcppp-tiny/network.h"
#include <fstream>

// Load DBC file
std::ifstream dbc("your_can_database.dbc");
auto net = dbcppp-tiny::INetwork::LoadDBCFromIs(dbc);

if (!net) {
    // Handle parse error
    return -1;
}

// Decode CAN frame
uint8_t can_data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
uint32_t can_id = 0x123;

// Find message by ID
for (const auto& msg : net->Messages()) {
    if (msg.Id() == can_id) {
        // Decode all signals
        for (const auto& sig : msg.Signals()) {
            // Handle multiplexed signals
            const auto* mux_sig = msg.MuxSignal();
            if (sig.MultiplexerIndicator() != dbcppp-tiny::ISignal::EMultiplexer::MuxValue ||
                (mux_sig && mux_sig->Decode(can_data) == sig.MultiplexerSwitchValue())) {
                
                // Extract and convert signal
                uint64_t raw = sig.Decode(can_data);
                double physical = sig.RawToPhys(raw);
                
                std::cout << sig.Name() << " = " << physical << " " << sig.Unit() << "\n";
            }
        }
        break;
    }
}
```

## API Overview

### Network
- `LoadDBCFromIs(std::istream&)` - Parse DBC from stream
- `Messages()` - Get all CAN messages
- `Nodes()` - Get all network nodes

### Message
- `Id()` - Get CAN ID
- `Name()` - Get message name
- `Signals()` - Get all signals
- `MuxSignal()` - Get multiplexer signal (if any)
- `Size()` - Get message size in bytes

### Signal
- `Decode(const void* data)` - Extract raw value from CAN data
- `RawToPhys(uint64_t raw)` - Convert raw to physical value
- `Name()` - Get signal name
- `Unit()` - Get physical unit
- `MultiplexerIndicator()` - Get multiplex type
- `MultiplexerSwitchValue()` - Get multiplex value

## Error Handling

The library uses a `Result<T>` pattern for error-free operation:

```cpp
auto result = dbcppp-tiny::INetwork::LoadDBCFromIs(stream);
if (!result) {
    // Parse failed
    return -1;
}
auto network = std::move(result.value());
```

## Performance

- **Library Size**: ~650KB (release, -O2)
- **No Dynamic Allocations**: During signal decoding
- **Template-Based Decoding**: Compile-time optimized signal extraction
- **Zero-Copy Design**: Direct decoding from CAN frame buffer

## Supported DBC Features

- Messages and signals (standard and extended CAN)
- Signed/unsigned integers and floats
- Big-endian (Motorola) and little-endian (Intel) byte order
- Multiplexed signals
- Signal scaling (factor/offset)
- Signal units
- Value tables
- Attributes
- Network nodes

## License

MIT License
