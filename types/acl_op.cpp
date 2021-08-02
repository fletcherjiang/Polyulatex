/**
* @file acl_op.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl_op.h"
#include <sstream>

namespace acl {
AclOp::AclOp(const AclOp& aclOp)
{
    Init(aclOp);
}

AclOp &AclOp::operator=(const AclOp &aclOp)
{
    Init(aclOp);
    return *this;
}

AclOp::~AclOp()
{
    if (this->isCopyConstructor) {
        if (this->inputDesc != nullptr) {
            for (int i = 0; i < this->numInputs; ++i) {
                if (this->inputDesc[i] != nullptr) {
                    delete this->inputDesc[i];
                }
            }
            free(const_cast<aclTensorDesc **>(this->inputDesc));
            this->inputDesc = nullptr;
        }
        if (this->outputDesc != nullptr) {
            for (int i = 0; i < this->numOutputs; ++i) {
                if (this->outputDesc[i] != nullptr) {
                    delete this->outputDesc[i];
                }
            }
            free(const_cast<aclTensorDesc **>(this->outputDesc));
            this->outputDesc = nullptr;
        }

        ACL_DELETE_AND_SET_NULL(this->opAttr);
    }
}

std::string AclOp::DebugString() const
{
    std::stringstream ss;
    ss << "OpType: " << opType << ", ";
    for (int32_t i = 0; i < numInputs; ++i) {
        const aclTensorDesc *desc = inputDesc[i];
        ss << "InputDesc[" << i << "]: ";
        ss << desc->DebugString() << " ";
    }

    for (int32_t i = 0; i < numOutputs; ++i) {
        const aclTensorDesc *desc = outputDesc[i];
        ss << "OutputDesc[" << i << "]: ";
        ss << desc->DebugString() << " ";
    }

    if (opAttr != nullptr) {
        ss << "Attr: " << opAttr->DebugString();
    }

    return ss.str();
}

void AclOp::Init(const AclOp& aclOp)
{
    this->isCopyConstructor = true;
    this->opType = aclOp.opType;
    this->numInputs = aclOp.numInputs;
    this->numOutputs = aclOp.numOutputs;
    if ((aclOp.inputDesc != nullptr) && (aclOp.numInputs > 0)) {
        size_t len = aclOp.numInputs * sizeof(aclTensorDesc *);
        aclTensorDesc **desc = static_cast<aclTensorDesc **>(malloc(len));
        ACL_REQUIRES_NOT_NULL_RET_VOID(desc);
        this->inputDesc = static_cast<const aclTensorDesc * const *>(desc);
        for (int i = 0; i < this->numInputs; ++i) {
            if (aclOp.inputDesc[i] != nullptr) {
                desc[i] = new(std::nothrow) aclTensorDesc(*aclOp.inputDesc[i]);
                ACL_REQUIRES_NOT_NULL_RET_VOID(desc[i]);
            } else {
                desc[i] = nullptr;
            }
        }
    }
    if ((aclOp.outputDesc != nullptr) && (aclOp.numOutputs > 0)) {
        size_t len = aclOp.numOutputs * sizeof(aclTensorDesc *);
        aclTensorDesc **desc = static_cast<aclTensorDesc **>(malloc(len));
        ACL_REQUIRES_NOT_NULL_RET_VOID(desc);
        if (memset_s(desc, len, 0, len) != EOK) {
            ACL_LOG_INNER_ERROR("memset failed");
            ACL_FREE(desc);
            return;
        }
        this->outputDesc = static_cast<const aclTensorDesc * const *>(desc);
        for (int i = 0; i < this->numOutputs; ++i) {
            if (aclOp.outputDesc[i] != nullptr) {
                desc[i] = new(std::nothrow) aclTensorDesc(*aclOp.outputDesc[i]);
                ACL_REQUIRES_NOT_NULL_RET_VOID(desc[i]);
            } else {
                desc[i] = nullptr;
            }
        }
    }
    if (aclOp.opAttr != nullptr) {
        this->opAttr = new(std::nothrow) aclopAttr(*aclOp.opAttr);
        ACL_REQUIRES_NOT_NULL_RET_VOID(this->opAttr);
    }
    this->inputs = aclOp.inputs;
    this->outputs = aclOp.outputs;
    this->engineType = aclOp.engineType;
    this->opPath = aclOp.opPath;
    this->compileType = aclOp.compileType;
    this->isCompile = aclOp.isCompile;
    this->exeucteType = aclOp.exeucteType;
    this->isMatched = aclOp.isMatched;
    this->isDynamic = aclOp.isDynamic;
    this->opModel = aclOp.opModel;
}

void AclOp::BackConst() const
{
    for (int32_t i = 0; i < numInputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(inputDesc));
        tmp[i]->BackConst();
    }
        for (int32_t i = 0; i < numOutputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(outputDesc));
        tmp[i]->BackConst();
    }
}

void AclOp::RecoverConst() const
{
    for (int32_t i = 0; i < numInputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(inputDesc));
        tmp[i]->RecoverConst();
    }
        for (int32_t i = 0; i < numOutputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(outputDesc));
        tmp[i]->RecoverConst();
    }
}

void AclOp::BackDimsAndShapeRanges() const
{
    for (int32_t i = 0; i < numInputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(inputDesc));
        tmp[i]->BackDimsAndShapeRanges();
    }
        for (int32_t i = 0; i < numOutputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(outputDesc));
        tmp[i]->BackDimsAndShapeRanges();
    }
}

void AclOp::RecoverdimsAndShaperanges() const
{
    for (int32_t i = 0; i < numInputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(inputDesc));
        tmp[i]->RecoverDimsAndShapeRanges();
    }
        for (int32_t i = 0; i < numOutputs; ++i) {
        aclTensorDesc **tmp = const_cast<aclTensorDesc **>(const_cast<aclTensorDesc *const *>(outputDesc));
        tmp[i]->RecoverDimsAndShapeRanges();
    }
}
} // namespace acl

