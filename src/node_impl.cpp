#include <algorithm>
#include "node_impl.h"

using namespace dbcppp;

std::unique_ptr<INode> INode::Create(
    std::string&& name,
    std::vector<std::unique_ptr<IAttribute>>&& attribute_values)
{
    std::vector<AttributeImpl> avs;
    for (auto& av : attribute_values)
    {
        avs.push_back(std::move(static_cast<AttributeImpl&>(*av)));
        av.reset(nullptr);
    }
    return std::make_unique<NodeImpl>(std::move(name), std::move(avs));
}
NodeImpl::NodeImpl(std::string&& name, std::vector<AttributeImpl>&& attribute_values)
    : _name(std::move(name))
    , _attribute_values(std::move(attribute_values))
{}
const std::string& NodeImpl::Name() const
{
    return _name;
}
const IAttribute& NodeImpl::AttributeValues_Get(std::size_t i) const
{
    return _attribute_values[i];
}
uint64_t NodeImpl::AttributeValues_Size() const
{
    return _attribute_values.size();
}
