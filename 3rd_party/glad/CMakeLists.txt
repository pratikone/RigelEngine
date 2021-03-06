if (USE_GL_ES)
    set(gl_variant "es2.0")
else()
    set(gl_variant "3.0")
endif()

set(sources
    "${gl_variant}/src/glad.c"
)

set(glad_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/${gl_variant}/include")

add_library(glad STATIC ${sources})
target_include_directories(glad PUBLIC ${glad_include_dir})
target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})

if (MSVC)
    target_compile_options(glad PRIVATE
        /w
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(glad PRIVATE
        -w
        # This is required in order to successfully link the test executable
        # on Mac.
        -fno-common
    )
elseif (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(glad PRIVATE
        -w
    )
endif()
