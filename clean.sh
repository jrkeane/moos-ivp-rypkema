#!/bin/bash
#--- simple clean script ---#

#remove all files in build directory
rm -rf ./build/*
#remove all files in lib directory
rm -rf ./lib/*
#remove all files in bin directory (except for original binaries)
find ./bin/ -mindepth 1 ! -name "MOOSup.py" ! -name "MyGenMOOSApp" ! -name "README_MOOSup.py" -delete
#remove all MOOSLog* folders
find ./ -mindepth 1 -name "MOOSLog*" -type d -exec rm -rfv {} \;
#remove all files of type .LastOpenedMOOSLogDirectory
find ./ -mindepth 1 -name "*.LastOpenedMOOSLogDirectory" -delete
#remove all files of type .moos++
find ./ -mindepth 1 -name "*.moos++" -delete
