/**
* @file jpeg_stub.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <cstdint>
#include <cstdio>
#include "jpeg_stub.h"
#include "acl_stub.h"

void aclStub::jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize)
{
    cinfo->jpeg_color_space = JCS_RGB565;
}

int aclStub::jpeg_read_header(j_decompress_ptr cinfo, boolean require_image)
{
    return JPEG_HEADER_OK;
}

void aclStub::jpeg_save_markers(j_decompress_ptr cinfo, int marker_code,
                               unsigned int length_limit)
{

}

void aclStub::jpeg_mem_src(j_decompress_ptr cinfo, const unsigned char *inbuffer, unsigned long insize)
{
}

MockFunctionTest& MockFunctionTest::aclStubInstance()
{
    static MockFunctionTest stub;
    return stub;
};

struct jpeg_error_mgr *jpeg_std_error(struct jpeg_error_mgr *err)
{
    return err;
}

void jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize)
{
    cinfo->jpeg_color_space = JCS_RGB565;
    MockFunctionTest::aclStubInstance().jpeg_CreateDecompress(cinfo, version, structsize);
}

void jpeg_mem_src(j_decompress_ptr cinfo, const unsigned char *inbuffer, unsigned long insize)
{
    MockFunctionTest::aclStubInstance().jpeg_mem_src(cinfo, inbuffer, insize);
}

int jpeg_read_header(j_decompress_ptr cinfo, boolean require_image)
{
    printf("-----------55555---------\n");
    return MockFunctionTest::aclStubInstance().jpeg_read_header(cinfo, require_image);
}

void jpeg_save_markers(j_decompress_ptr cinfo, int marker_code,
                               unsigned int length_limit)
{
    MockFunctionTest::aclStubInstance().jpeg_save_markers(cinfo, marker_code, length_limit);
}

void jpeg_destroy_decompress(j_decompress_ptr cinfo)
{
}

void jpeg_start_compress(j_compress_ptr cinfo, boolean write_all_tables)
{
}

tjhandle tjInitDecompress(void)
{
    tjhandle handle = nullptr;
    return handle;
}

int tjDestroy(tjhandle handle)
{
    return 0;
}

int tjDecompressHeader3(tjhandle handle,
                        const unsigned char *jpegBuf,
                        unsigned long jpegSize, int *width,
                        int *height, int *jpegSubsamp,
                        int *jpegColorspace)
{
    return 0;
}