#!bash
cmake -B build \
        -G "Ninja" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
        -DCMAKE_C_COMPILER:FILEPATH=D:/TDM-GCC-64/bin/x86_64-w64-mingw32-gcc.exe \
        -DCMAKE_CXX_COMPILER:FILEPATH=D:/TDM-GCC-64/bin/x86_64-w64-mingw32-g++.exe $@
