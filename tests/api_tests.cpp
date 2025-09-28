
#include <catch2/catch_test_macros.hpp>
#include <dbcppp-tiny/network.h>

using namespace dbcppp;

TEST_CASE("API Test: AttributeDefinition", "[unit]")
{
    constexpr const char* test_dbc =
        "VERSION \"\"\n"
        "NS_ :\n"
        "BS_:\n"
        "BU_:\n"
        "BA_DEF_ BO_  \"AD_Name\" INT 1 3000;";
    
    SECTION("CPP API")
    {
        std::istringstream iss(test_dbc);
        auto net = INetwork::LoadDBCFromIs(iss);
        REQUIRE(net);

        REQUIRE(net->AttributeDefinitions_Size() == 1);
        REQUIRE(net->AttributeDefinitions_Get(0).ObjectType() == IAttributeDefinition::EObjectType::Message);
        REQUIRE(net->AttributeDefinitions_Get(0).Name() == "AD_Name");
        auto value_type = std::get_if<IAttributeDefinition::ValueTypeInt>(&net->AttributeDefinitions_Get(0).ValueType());
        REQUIRE(value_type);
        REQUIRE(value_type->minimum == 1);
        REQUIRE(value_type->maximum == 3000);
    }
}
TEST_CASE("API Test: BitTiming", "[unit]")
{
    constexpr const char* test_dbc =
        "VERSION \"\"\n"
        "NS_ :\n"
        "BS_: 1 : 2, 3\n"
        "BU_:\n";
    
    SECTION("CPP API")
    {
        std::istringstream iss(test_dbc);
        auto net = INetwork::LoadDBCFromIs(iss);
        REQUIRE(net);

        REQUIRE(net->BitTiming().Baudrate() == 1);
        REQUIRE(net->BitTiming().BTR1() == 2);
        REQUIRE(net->BitTiming().BTR2() == 3);
    }
}
TEST_CASE("API Test: Signal", "[unit]")
{
    constexpr const char* test_dbc =
        "VERSION \"\"\n"
        "NS_ :\n"
        "BS_: 1 : 2, 3\n"
        "BU_:\n"
        "BO_ 1 Msg0: 8 Sender0\n"
        "  SG_ Sig0: 0|1@1+ (1,0) [1|12] \"Unit0\" Vector__XXX\n"
        "  SG_ Sig1 m0 : 1|1@0- (1,0) [1|12] \"Unit1\" Recv0, Recv1\n"
        "  SG_ Sig2 M : 2|1@0- (1,0) [1|12] \"Unit2\" Recv0, Recv1\n";
    
    SECTION("CPP API")
    {
        std::istringstream iss(test_dbc);
        auto net = INetwork::LoadDBCFromIs(iss);
        REQUIRE(net);

        REQUIRE(net->Messages_Size() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Size() == 3);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Name() == "Sig0");
        REQUIRE(net->Messages_Get(0).Signals_Get(0).MultiplexerIndicator() == ISignal::EMultiplexer::NoMux);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).StartBit() == 0);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).BitSize() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).ByteOrder() == ISignal::EByteOrder::LittleEndian);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).ValueType() == ISignal::EValueType::Unsigned);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Factor() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Offset() == 0);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Minimum() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Maximum() == 12);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Unit() == "Unit0");
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Receivers_Size() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(0).Receivers_Get(0) == "Vector__XXX");

        REQUIRE(net->Messages_Get(0).Signals_Get(1).Name() == "Sig1");
        REQUIRE(net->Messages_Get(0).Signals_Get(1).MultiplexerIndicator() == ISignal::EMultiplexer::MuxValue);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).StartBit() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).BitSize() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).ByteOrder() == ISignal::EByteOrder::BigEndian);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).ValueType() == ISignal::EValueType::Signed);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Factor() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Offset() == 0);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Minimum() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Maximum() == 12);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Unit() == "Unit1");
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Receivers_Size() == 2);
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Receivers_Get(0) == "Recv0");
        REQUIRE(net->Messages_Get(0).Signals_Get(1).Receivers_Get(1) == "Recv1");
        
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Name() == "Sig2");
        REQUIRE(net->Messages_Get(0).Signals_Get(2).MultiplexerIndicator() == ISignal::EMultiplexer::MuxSwitch);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).StartBit() == 2);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).BitSize() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).ByteOrder() == ISignal::EByteOrder::BigEndian);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).ValueType() == ISignal::EValueType::Signed);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Factor() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Offset() == 0);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Minimum() == 1);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Maximum() == 12);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Unit() == "Unit2");
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Receivers_Size() == 2);
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Receivers_Get(0) == "Recv0");
        REQUIRE(net->Messages_Get(0).Signals_Get(2).Receivers_Get(1) == "Recv1");
    }
}
TEST_CASE("API Test: Message", "[unit]")
{
    constexpr const char* test_dbc =
        "VERSION \"\"\n"
        "NS_ :\n"
        "BS_: 1 : 2, 3\n"
        "BU_:\n"
        "BO_ 1 Msg0: 8 Sender0\n"
        "  SG_ Sig0: 0|1@1+ (1,0) [1|12] \"Unit0\" Vector__XXX\n"
        "  SG_ Sig1 m0 : 1|1@0- (1,0) [1|12] \"Unit1\" Recv0, Recv1\n"
        "  SG_ Sig2 M : 2|1@0- (1,0) [1|12] \"Unit2\" Recv0, Recv1\n";
    
    SECTION("CPP API")
    {
        std::istringstream iss(test_dbc);
        auto net = INetwork::LoadDBCFromIs(iss);
        REQUIRE(net);

        REQUIRE(net->Messages_Size() == 1);
        REQUIRE(net->Messages_Get(0).Id() == 1);
        REQUIRE(net->Messages_Get(0).MessageSize() == 8);
        REQUIRE(net->Messages_Get(0).Transmitter() == "Sender0");
        REQUIRE(net->Messages_Get(0).Signals_Size() == 3);
    }
}
