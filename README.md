Nedit
=====

A port of [Nedit](http://www.nedit.org) to C++ and [FLTK](http://www.fltk.org).

This a [joke project](http://hylvenir.free.fr/nedit/) (with screenshots) to have fun coding in C++ (at this time
only compiling in C++ instead of C), learning FLTK and add some gtests
for [coverage](http://hylvenir.free.fr/nedit/lcov)

This project contains:
 - all the original NEdit source code
 - Some prototypes for some part I have done before porting to C++/FLTK
 - A Visual Studio 2010 project file
 - A Linux Makefile

This project needs FLTK 1.3. You need to change to prop files with your FLTK home.

Basic editing functions are availables (open, save, find).
Copy/paste should work fine, as the rectangular selection.
Syntax highlighting is just *pre-alpha* quality, and crash a lot.
There is no macros, shell commands, and help.

This editor version is pretty harmfull, very unstable and immature. You should use it only for
experimental purpose.

