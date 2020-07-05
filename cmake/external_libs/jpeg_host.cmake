if (HAVE_JPEG)
    return()
endif()

if (ACL_PB_PKG)
        set(REQ_URL "${ACL_PB_PKG}/libs/libjpeg/2.0.5.tar.gz")
else()
        set(REQ_URL "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/2.0.5.tar.gz")
        set(MD5 "")
endif()

#message(STATUS ${JPEG_PATH_FILE1})
ExternalProject_Add(jpeg_host
	            URL ${REQ_URL}
		    TLS_VERIFY OFF
                    CONFIGURE_COMMAND  ${CMAKE_COMMAND}
                        -DWITH_JPEG8=ON
                        -DWITH_SIMD=ON
                        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/jpeg <SOURCE_DIR>
                        -DENABLE_SHARED=FALSE
                        -DCMAKE_C_FLAGS=-fPIC\ -fexceptions\ -D_FORTIFY_SOURCE=2\ -O2
                        BUILD_COMMAND ${MAKE}
			INSTALL_COMMAND $(MAKE) install
                        EXCLUDE_FROM_ALL TRUE
)

include(GNUInstallDirs)
set(JPED_PKG_DIR ${CMAKE_INSTALL_PREFIX}/jpeg)

add_library(static_turbojpeg_lib STATIC IMPORTED)
set_target_properties(static_turbojpeg_lib PROPERTIES
                      IMPORTED_LOCATION ${JPED_PKG_DIR}/${CMAKE_INSTALL_LIBDIR}/libturbojpeg.a
)

add_library(static_turbojpeg INTERFACE)

target_link_libraries(static_turbojpeg INTERFACE static_turbojpeg_lib)
target_include_directories(static_turbojpeg INTERFACE ${JPED_PKG_DIR}/include)
add_dependencies(static_turbojpeg jpeg_host)

set(HAVE_JPEG TRUE)
