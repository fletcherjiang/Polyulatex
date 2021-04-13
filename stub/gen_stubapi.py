import os
import re
import sys

PATTERN_FUNCTION = re.compile(r'ACL_FUNC_VISIBILITY\s+\n+.+\w+\([^();]*\);|.+\w+\([^();]*\);')
PATTERN_RETURN = re.compile(r'([^ ]+[ *])\w+\([^;]+;')

RETURN_STATEMENTS = {
    'aclDataType':          '    return ACL_DT_UNDEFINED;',
    'aclFormat':            '    return ACL_FORMAT_UNDEFINED;',
    'aclError':             '    printf("[ERROR]: stub library cannot be used for execution, please check your environment variables and compilation options to make sure you use the correct ACL library.\\n");\n    return static_cast<aclError>(ACL_ERROR_COMPILING_STUB_MODE);',
    'void':                 '',
    'size_t':               '    return static_cast<size_t>(0);',
    'uint8_t':              '    return static_cast<uint8_t>(0);',
    'int32_t':              '    return static_cast<int32_t>(0);',
    'uint32_t':             '    return static_cast<uint32_t>(0);',
    'int64_t':              '    return static_cast<int64_t>(0);',
    'uint64_t':             '    return static_cast<uint64_t>(0);',
    'aclFloat16':           '    return static_cast<aclFloat16>(0);',
    'float':                '    return 0.0f;',
    'aclvdecCallback':      '    return nullptr;',
    'acldvppPixelFormat':   '    return PIXEL_FORMAT_YUV_400;',
    'bool':                 '    return false;',
    'acldvppStreamFormat':  '    return H265_MAIN_LEVEL;',
    'aclvencCallback':      '    return nullptr;',
    'double':               '    return static_cast<double>(0.0f);',
    'acldvppBorderType':    '    return BORDER_CONSTANT;',
    'acltdtTensorType':     '    return ACL_TENSOR_DATA_TENSOR;'

}

def collect_header_files(path):
    acl_headers = []
    dvpp_headers = []
    cblas_headers = []
    op_compiler_headers = []
    retr_headers = []
    tdt_channel_headers = []
    tdt_queue_headers = []
    for root, dirs, files in os.walk(path):
        files.sort()
        for file in files:
            if file.find("dvpp") >= 0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                dvpp_headers.append(file_path)
            elif file.find("cblas") >= 0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                cblas_headers.append(file_path)
            elif file.find("op_compiler") >=0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                op_compiler_headers.append(file_path)
            elif file.find("acl_fv") >=0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                retr_headers.append(file_path)
            elif file.find("acl_tdt.h") >=0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                tdt_channel_headers.append(file_path)
            elif file.find("acl_tdt_queue.h") >=0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                tdt_channel_headers.append(file_path)
            else:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                acl_headers.append(file_path)
    return acl_headers, dvpp_headers, cblas_headers, op_compiler_headers, retr_headers, tdt_channel_headers, tdt_queue_headers

def collect_functions(file_path):
    signatures = []
    with open(file_path) as f:
        content = f.read()
        matches = PATTERN_FUNCTION.findall(content)
        for signature in matches:
            signatures.append(signature)
    return signatures


def implement_function(func):
    # remove ';'
    function_def = func[:len(func) - 1]
    function_def += '\n'
    function_def += '{\n'
    m = PATTERN_RETURN.search(func)
    if m:
        ret_type = m.group(1).strip()
        if RETURN_STATEMENTS.__contains__(ret_type):
            function_def += RETURN_STATEMENTS[ret_type]
        else:
            if ret_type.endswith('*'):
                function_def += '    return nullptr;'
            else:
                print("Unhandled return type: " + ret_type)
    else:
        function_def += '    return nullptr;'
    function_def += '\n'
    function_def += '}\n'
    return function_def


def generate_stub_file(inc_dir):
    acl_header_files, dvpp_header_files, cblas_header_files, op_compiler_header_files, retr_header_files, tdt_channel_header_files, tdt_queue_header_files = collect_header_files(inc_dir)
    print("header files has been generated")
    acl_content = generate_function(acl_header_files, inc_dir)
    acl_content.append("extern \"C\" {\n");
    acl_content.append("ACL_FUNC_VISIBILITY aclError aclrtSetDeviceWithoutTsdVXX(int32_t deviceId);\n");
    acl_content.append("ACL_FUNC_VISIBILITY aclError aclrtResetDeviceWithoutTsdVXX(int32_t deviceId);\n");
    acl_content.append("}\n");
    acl_content.append("ACL_FUNC_VISIBILITY aclError aclrtSetDeviceWithoutTsdVXX(int32_t deviceId)\n{\n    return ACL_ERROR_COMPILING_STUB_MODE;\n}\n\n");
    acl_content.append("ACL_FUNC_VISIBILITY aclError aclrtResetDeviceWithoutTsdVXX(int32_t deviceId)\n{\n    return ACL_ERROR_COMPILING_STUB_MODE;\n}\n\n");
    print("acl_content has been generated")
    dvpp_content = generate_function(dvpp_header_files, inc_dir)
    print("dvpp_content has been generate")
    cblas_content = generate_function(cblas_header_files, inc_dir)
    print("cblas_content has been generate")
    op_compiler_content = generate_function(op_compiler_header_files, inc_dir)
    retr_content = generate_function(retr_header_files, inc_dir)
    tdt_channel_content = generate_function(tdt_channel_header_files, inc_dir)
    tdt_queue_content = generate_function(tdt_queue_header_files, inc_dir)
    print("retr_content has been generate")
    return acl_content, dvpp_content, cblas_content, op_compiler_content, retr_content, tdt_channel_content, tdt_queue_content

def generate_function(header_files, inc_dir):
    includes = []
    includes.append('#include <stdio.h>\n')
    # generate includes
    for header in header_files:
        if header.find("git") >= 0:
            continue
        if not header.endswith('.h'):
            continue
        include_str = '#include "acl/{}"\n'.format(header[len(inc_dir):])
        includes.append(include_str)

    content = includes
    print("include concent build success")
    total = 0
    content.append('\n')
    # generate implement
    for header in header_files:
        if header.find("git") >= 0:
            continue
        if not header.endswith('.h'):
            continue
        content.append("// stub for {}\n".format(header[len(inc_dir):]))
        functions = collect_functions(header)
        print("inc file:{}, functions numbers:{}".format(header, len(functions)))
        total += len(functions)
        for func in functions:
            content.append("{}\n".format(implement_function(func)))
            content.append("\n")
    print("implement concent build success")
    print('total functions number is {}'.format(total))
    return content

def gen_code(inc_dir, acl_stub_path, dvpp_stub_path, cblas_stub_path, op_compiler_stub_path, retr_stub_path, tdt_channel_stub_path, tdt_queue_stub_path):
    if not inc_dir.endswith('/'):
        inc_dir += '/'
    acl_content, dvpp_content, cblas_content, op_compiler_content, retr_content, tdt_channel_content, tdt_queue_content = generate_stub_file(inc_dir)
    print("acl_content, dvpp_content ,cblas_content and op_compiler_content have been generated")
    with open(acl_stub_path, mode='w') as f:
        f.writelines(acl_content)
    with open(dvpp_stub_path, mode='w') as f:
        f.writelines(dvpp_content)
    with open(cblas_stub_path, mode='w') as f:
        f.writelines(cblas_content)
    with open(op_compiler_stub_path, mode='w') as f:
        f.writelines(op_compiler_content)
    with open(retr_stub_path, mode='w') as f:
        f.writelines(retr_content)
    with open(tdt_channel_stub_path, mode='w') as f:
        f.writelines(tdt_channel_content)
    with open(tdt_queue_stub_path, mode='w') as f:
        f.writelines(tdt_queue_content)

if __name__ == '__main__':
    inc_dir = sys.argv[1]
    acl_stub_path = sys.argv[2]
    dvpp_stub_path = sys.argv[3]
    cblas_stub_path = sys.argv[4]
    op_compiler_stub_path = sys.argv[5]
    retr_stub_path = sys.argv[6]
    tdt_channel_stub_path = sys.argv[7]
    tdt_queue_stub_path = sys.argv[8]
    gen_code(inc_dir, acl_stub_path, dvpp_stub_path, cblas_stub_path, op_compiler_stub_path, retr_stub_path, tdt_channel_stub_path, tdt_queue_stub_path)
