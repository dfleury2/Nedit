#!/bin/sh

for name in $(ls *.c); do
 tmp=$( basename $name .c )
 svn mv $name ${tmp}.cpp
done
