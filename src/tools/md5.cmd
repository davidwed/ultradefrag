@echo off
md5sum ultradefrag-%1.bin.* > ultradefrag-%1.MD5SUMS
md5sum ultradefrag-%1.src.* >> ultradefrag-%1.MD5SUMS
