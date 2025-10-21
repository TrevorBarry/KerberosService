cmake_minimum_required(VERSION 3.16)
project(KerberosEchoService)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(KerberosEchoService
    main.cpp
    WindowsService.cpp
    HttpServer.cpp
    KerberosAuth.cpp
)

# Link required libraries
target_link_libraries(KerberosEchoService
    httpapi
    secur32
)

# Set output name
set_target_properties(KerberosEchoService PROPERTIES
    OUTPUT_NAME "KerberosEchoService"
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(KerberosEchoService PRIVATE
        WIN32_LEAN_AND_MEAN
        SECURITY_WIN32
        _CRT_SECURE_NO_WARNINGS
    )
endif()