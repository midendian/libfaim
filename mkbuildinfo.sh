#!/bin/bash
rm -rf aim_meta.o aim_buildcode.h

echo -n "#define AIM_BUILDDATE \"" > aim_buildcode.h
echo -n `date +%Y%m%d` >> aim_buildcode.h
echo "\"" >> aim_buildcode.h

echo -n "#define AIM_BUILDTIME \"" >> aim_buildcode.h
echo -n `date +%H%M%S` >> aim_buildcode.h
echo "\"" >> aim_buildcode.h
