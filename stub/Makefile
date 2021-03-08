stub_path := $(shell pwd)/acl/stub
inc_path := $(shell pwd)/inc/external/acl
out_path := $(shell pwd)/out/acl/stub

mkdir_stub := $(shell mkdir -p $(out_path))
local_stub := $(shell $(HI_PYTHON) $(stub_path)/gen_stubapi.py $(inc_path) $(out_path)/acl_stub.cpp $(out_path)/dvpp_stub.cpp $(out_path)/cblas_stub.cpp $(out_path)/op_compiler_stub.cpp $(out_path)/retr_stub.cpp $(out_path)/tdt_channel_stub.cpp)
