#pragma once

#include <string>
#include <memory>
#include <cstddef>
#include <variant>

#include "export.h"
#include "attribute_definition.h"

namespace dbcppp
{
    class DBCPPP_API IAttribute
    {
    public:
        using hex_value_t = int64_t;
        using value_t = std::variant<int64_t, double, std::string>;

        static std::unique_ptr<IAttribute> Create(
              std::string&& name
            , IAttributeDefinition::EObjectType object_type
            , value_t value);
            
        virtual ~IAttribute() = default;
        virtual const std::string& Name() const = 0;
        virtual IAttributeDefinition::EObjectType ObjectType() const = 0;
        virtual const value_t& Value() const = 0;

    };
}