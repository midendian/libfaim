#!/bin/bash
rm -rf aim_meta.o aim_buildcode.h
echo -n "#define AIM_BUILDDATE " > aim_buildcode.h
echo `date +%Y%m%e` >> aim_buildcode.h
echo -n "#define AIM_BUILDTIME " >> aim_buildcode.h
echo `date +%H%M%S` >> aim_buildcode.h
