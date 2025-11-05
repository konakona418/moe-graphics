@echo off
setlocal enabledelayedexpansion

set GLSLC=glslc.exe

echo === Compiling GLSL shaders to SPIR-V ===

for %%f in (*.vert) do (
    echo [VERT] %%f -> %%f.spv
    "%GLSLC%" "%%f" -fshader-stage=vert --target-env=vulkan1.3 -o "%%f.spv"
)

for %%f in (*.frag) do (
    echo [FRAG] %%f -> %%f.spv
    "%GLSLC%" "%%f" -fshader-stage=frag --target-env=vulkan1.3 -o "%%f.spv"
)

for %%f in (*.comp) do (
    echo [COMP] %%f -> %%f.spv
    "%GLSLC%" "%%f" -fshader-stage=comp --target-env=vulkan1.3 -o "%%f.spv"
)

for %%f in (*.geom) do (
    echo [COMP] %%f -> %%f.spv
    "%GLSLC%" "%%f" -fshader-stage=geom --target-env=vulkan1.3 -o "%%f.spv"
)

echo === Done! ===
pause
