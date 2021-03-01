#include "op_kernel_selector.h"
#include <map>
#include <memory>
#include "common/log_inner.h"
#include "op_kernel_registry.h"

namespace acl {
bool OpKernelSelector::Register(const std::string &opType, aclopCompileFunc func)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto iter = selectors_.emplace(opType, func);
    return iter.second;
}

void OpKernelSelector::Unregister(const std::string &opType)
{
    std::lock_guard<std::mutex> lock(mu_);
    selectors_.erase(opType);
}

aclopCompileFunc OpKernelSelector::GetSelectFunc(const std::string &opType)
{
    std::lock_guard<std::mutex> lock(mu_);
    aclopCompileFunc func = nullptr;
    auto iter = selectors_.find(opType);
    if (iter != selectors_.end()) {
        func = iter->second;
    }

    return func;
}

aclError OpKernelSelector::InsertAclop2KernelDesc(const AclOp &aclOp, std::shared_ptr<OpKernelDesc> &desc) {
    ACL_LOG_DEBUG("start InsertAclop2KernelDesc");
    ACL_REQUIRES_NOT_NULL(desc);
    desc->opType = aclOp.opType;

    for (int i = 0; i < aclOp.numInputs; ++i) {
        ACL_REQUIRES_NOT_NULL(aclOp.inputDesc[i]);
        desc->inputDescArr.emplace_back(*(aclOp.inputDesc[i]));
    }
    ACL_LOG_DEBUG("Insert inputDescArr success!");
    
    for (int i = 0; i < aclOp.numOutputs; ++i) {
        ACL_REQUIRES_NOT_NULL(aclOp.outputDesc[i]);
        desc->outputDescArr.emplace_back(*(aclOp.outputDesc[i]));
    }
    ACL_LOG_DEBUG("Insert outputDescArr success!");

    // if aclOp.opAttr is nullptr, desc->opAttr is a empty attr object
    if (aclOp.opAttr != nullptr) {
        for (auto attrVal : aclOp.opAttr->Attrs()) {
            desc->opAttr.EmplaceAttr(attrVal.first, attrVal.second);
        }
    }
    ACL_LOG_DEBUG("Insert attr success!");
}

aclError OpKernelSelector::SelectOpKernel(const AclOp &aclOp)
{
    auto func = GetSelectFunc(aclOp.opType);
    if (func == nullptr) {
        ACL_LOG_WARN("Op not found, opType = %s", aclOp.opType.c_str());
        return ACL_ERROR_BIN_SELECTOR_NOT_REGISTERED;
    }

    auto desc = std::shared_ptr<OpKernelDesc>(new (std::nothrow)OpKernelDesc);
    ACL_CHECK_MALLOC_RESULT(desc);
    ACL_REQUIRES_OK(InsertAclop2KernelDesc(aclOp, desc));
    ACL_LOG_DEBUG("To invoke select func, opType = %s", aclOp.opType.c_str());
    auto ret = func(aclOp.numInputs, aclOp.inputDesc, aclOp.numOutputs, aclOp.outputDesc, aclOp.opAttr, desc.get());
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Call][Compile]Failed to call op compile, result = %d", ret);
        return ret;
    }

    ACL_LOG_DEBUG("selecting kernel succeeded. kernelId = %s", desc->kernelId.c_str());
    desc->stubFunc = OpKernelRegistry::GetInstance().GetStubFunc(aclOp.opType, desc->kernelId);
    if (desc->stubFunc == nullptr) {
        ACL_LOG_INNER_ERROR("Stub function not registered. kernelId = %s", desc->kernelId.c_str());
        return ACL_ERROR_KERNEL_NOT_FOUND;
    }
    std::shared_ptr<OpKernelDesc> agingDesc = nullptr;
    kernelDescMap_.Insert(aclOp, desc, agingDesc);
    return ACL_SUCCESS;
}

aclError OpKernelSelector::GetOpKernelDesc(const AclOp &aclOp, std::shared_ptr<OpKernelDesc> &desc)
{
    return kernelDescMap_.Get(aclOp, desc);
}

bool OpKernelSelector::HasSelectFunc(const std::string &opType)
{
    return selectors_.count(opType) != 0;
}
} // namespace acl