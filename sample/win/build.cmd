
@rem of course start from msvc developer cmd
@rem debug build -- /Zi
@rem /std:c++17
@rem  /await 
cl  /EHsc /Zi /Fe: example_wins_dbg.exe example_wins.c ../../lfqueue.c
