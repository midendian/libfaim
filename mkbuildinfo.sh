#!/bin/sh
rm -rf aim_meta.o aim_buildcode.h

echo "#define AIM_BUILDDATE \"`date +%Y%m%d`\"" > aim_buildcode.h
echo "#define AIM_BUILDTIME \"`date +%H%M%S`\"" >> aim_buildcode.h
