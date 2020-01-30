
@rem of course Windows VCode has to start from msvc developer cmd
@rem debug build on/off -- include/exclude argument -- /Zi

cl  /std:c++17 /EHsc /Zi /Fe: cppsample_dbg.exe cppsample.cpp ../../lfqueue.c