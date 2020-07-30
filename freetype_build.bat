cl /c /DFT2_BUILD_LIBRARY /D_WIN64 ^
freetype2/src/base/ftsystem.c ^
freetype2/src/base/ftinit.c ^
freetype2/src/base/ftdebug.c ^
freetype2/src/base/ftbase.c ^
freetype2/src/gzip/ftgzip.c ^
freetype2/src/base/ftbitmap.c ^
freetype2/src/sfnt/sfnt.c ^
freetype2/src/psnames/psnames.c ^
freetype2/src/truetype/truetype.c ^
freetype2/src/smooth/smooth.c ^
/I freetype2/include
lib ftsystem.obj ftinit.obj ftdebug.obj ftbase.obj ftgzip.obj ftbitmap.obj sfnt.obj psnames.obj truetype.obj smooth.obj
