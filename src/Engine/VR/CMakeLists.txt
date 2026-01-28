set(VR_SOURCES
    VRManager.cpp
)

set(VR_HEADERS
    VRManager.h
)

add_library(engine_vr STATIC ${VR_SOURCES} ${VR_HEADERS})

# Linkar com OpenXR Loader (baixado via FetchContent na raiz)
target_link_libraries(engine_vr PUBLIC OpenXR::openxr_loader)

# Adicionar diretórios de inclusão
target_include_directories(engine_vr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Se precisar de GLM
find_package(glm REQUIRED)
target_link_libraries(engine_vr PUBLIC glm::glm)

# SDL3 para pegar handles nativos
target_link_libraries(engine_vr PUBLIC SDL3::SDL3)
