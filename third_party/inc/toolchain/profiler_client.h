#ifndef PROFILER_CLIENT_H_INCLUDED
#define PROFILER_CLIENT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef WIN32
#ifdef PROFILERCLIENT_EXPORTS
#define MSVP_PROF_CLN_API __declspec(dllexport)
#else
#define MSVP_PROF_CLN_API __declspec(dllimport)
#endif
#else
#define MSVP_PROF_CLN_API
#endif
/**
 * the data structure to write
 */
struct data_chunk {
    char*  relative_file_name;// from subpath begin; For example: subA/subB/example.txt; Note: the begin don't has '/';
    unsigned char*  data_buf;// the pointer to the data
    unsigned int    buf_len;// the len of data_buf
    unsigned int    is_last_chunk;// = 1, the last chunk of the file; != 1, not the last chunk of the file
    long long       offset;// the begin location of the file to write; if the offset is -1, directly append data.
};

struct collect_dev_info_s {
    int dev_id;
};

/** \brief use it to connect the server.
 *
 * \param const unsigned char* target: The engine gets it from ENV. Don't need care about it.
 * \param const unsigned char* engine_name: For example OME;CCE; Runtime;Matrix...
 * \return the return value void* point the client handle
 *
 */

MSVP_PROF_CLN_API extern void* create_collect_client(const char* target, const char* engine_name);

/** \brief notify profiling the device list
 *
 * \param void* handle: the return value of the create_collect_client function
 * \param const char* job_ctx: identifies profiling job
 * \param const collect_dev_info_s* dev_list: pointer to the device list
 * \param int dev_num: the device number
 * \return 0 on success
 */
MSVP_PROF_CLN_API extern int collect_host_sync_dev_list(void* handle, const char* job_ctx, const collect_dev_info_s* dev_list, int dev_num);

/** \brief write data by this function to transfer
 *
 * \param void* handle: the return value of the create_collect_client function
 * \param struct data_chunk* data: record the value to restore the sampling data
 * \param const unsigned char* job_ctx: The engine gets it from ENV. Don't need care about it. Represent the context about profiling job.
 * \return On success, the number of bytes written is returned(zero indicates nothing was written); On error, <0 is  returned, and the value is set appropriately.
 *
 */

MSVP_PROF_CLN_API extern int collect_write(void* handle, const char* job_ctx, struct data_chunk* data);

/** \brief release the handle
 *
 * \param void* handle: the return value of the create_collect_client function
 * \return
 *
 */

MSVP_PROF_CLN_API extern void release_collect_client(void* handle);

/** \brief update job ctx for specific device, colllect_update_job_ctx uses malloc() to allocate a buffer to hold the 
 * new job_ctx and return a pointer to the buffer. The caller should deallocate this buffer using free()
 *
 * \param const char* job_ctx: pointer to current job_ctx
 * \param collect_ctx_info* info: update job_ctx with the info
 * \return pointer to buffer which holds the new job_ctx
 */
MSVP_PROF_CLN_API extern char* collect_dev_update_job_ctx(const char* job_ctx, const collect_dev_info_s * info);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PROFILER_CLIENT_H_INCLUDED


