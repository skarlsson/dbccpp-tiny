#pragma once

#include <vector>
#include <memory>

#include "dbcppp-tiny/node.h"
#include "attribute_impl.h"

namespace dbcppp
{
    class NodeImpl final
        : public INode
    {
    public:
        NodeImpl(
              std::string&& name
            , std::vector<AttributeImpl>&& attribute_values);
            
        virtual const std::string& Name() const override;
        virtual const IAttribute& AttributeValues_Get(std::size_t i) const override;
        virtual uint64_t AttributeValues_Size() const override;

    private:
        std::string _name;
        std::vector<AttributeImpl> _attribute_values;
    };
}
