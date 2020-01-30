
@rem of course start the visual code from msvc developer cmd
@rem debug build -- /Zi
@rem /std:c++17
@rem  /await 

@rem exe name has to match the one used in .vscode/launch.json

cl  /EHsc /Zi /Fe: windows_sample_dbg.exe windows_sample.c ../../lfqueue.c
