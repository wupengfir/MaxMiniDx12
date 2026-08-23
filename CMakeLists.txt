cmake_minimum_required(VERSION 3.20)
project(MinimalDX12 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
# WIN32 = 桌面程序入口（WinMain），不加就是控制台程序
add_executable(MinimalDX12 WIN32
    src/main.cpp
    src/dx12_context.cpp
 "src/mesh.h" "src/mesh.cpp" "src/pipeline.h" "src/pipeline.cpp")

target_link_libraries(MinimalDX12 PRIVATE
    d3d12.lib    # DX12 核心
    dxgi.lib     # SwapChain
    dxguid.lib   # GUID
    user32.lib   # 窗口
    gdi32.lib    # 窗口
    d3dcompiler.lib
)

target_compile_definitions(MinimalDX12 PRIVATE
    UNICODE _UNICODE       # 宽字符
    NOMINMAX               # 禁用 min/max 宏，避免和 std::min 冲突
    WIN32_LEAN_AND_MEAN    # 减少 windows.h 体积
)

target_compile_options(MinimalDX12 PRIVATE /MP)  # 多处理器编译


set_target_properties(MinimalDX12 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:MinimalDX12>
)
# 构建后把 shaders 目录复制到 exe 旁边，这样代码里用相对路径 "shaders/triangle.hlsl" 能找到
add_custom_command(TARGET MinimalDX12 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/shaders
    $<TARGET_FILE_DIR:MinimalDX12>/shaders
)