/**
 * @file retr_ops.cpp
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "single_op/retr/retr_internal.h"
#include "single_op/retr/retr_init.h"
#include "single_op/retr/retr_release.h"
#include "single_op/retr/retr_repo.h"
#include "single_op/retr/retr_search.h"
#include "single_op/retr/retr_accurate.h"
#include "acl/ops/acl_fv.h"

#ifdef __cplusplus
extern "C" {
#endif

aclError aclfvInit(aclfvInitPara *initPara)
{
    return acl::retr::AclFvInit().Init(initPara);
}

aclError aclfvRelease()
{
    return acl::retr::AclFvRelease().Release();
}

aclError aclfvRepoAdd(aclfvSearchType type, aclfvFeatureInfo *featureInfo)
{
    return acl::retr::AclFvRepo().Add(type, featureInfo);
}

aclError aclfvRepoDel(aclfvSearchType type, aclfvRepoRange *repoRange)
{
    return acl::retr::AclFvRepo().Del(type, repoRange);
}

aclError aclfvDel(aclfvFeatureInfo *featureInfo)
{
    return acl::retr::AclFvAccurate().Delete(featureInfo);
}

aclError aclfvModify(aclfvFeatureInfo *featureInfo)
{
    return acl::retr::AclFvAccurate().Modify(featureInfo);
}

aclError aclfvSearch(
    aclfvSearchType type, aclfvSearchInput *searchInput, aclfvSearchResult *searchRst)
{
    return acl::retr::AclFvSearch().Search(type, searchInput, searchRst);
}

#ifdef __cplusplus
}
#endif