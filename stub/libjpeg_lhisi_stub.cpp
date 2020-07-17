#include <cstdint>
#include <cstdio>
#include "utils/acl_jpeglib.h"

struct jpeg_error_mgr *jpeg_std_error(struct jpeg_error_mgr *err)
{
    return err;
}

void jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize)
{
    cinfo->jpeg_color_space = JCS_RGB565;
}

void jpeg_mem_src(j_decompress_ptr cinfo, const unsigned char *inbuffer, unsigned long insize)
{
}

int jpeg_read_header(j_decompress_ptr cinfo, boolean require_image)
{
    return JPEG_HEADER_OK;
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
