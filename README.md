PE Resource Dump
================

Dumps all resources from a PE file such as EXE, DLL, MUI, CPL, OCX, and others.

The command line two takes 2 arguments: the PE file and the directory to output
to. The output directory will be set up with a tree where the first level is
the resource type (common types are converted to string names). If a resource
has more than one language there will be another directory level with the
language numerical ID. The files then are the resources themselves with the
name of the resource (either the numerical identifier or the string) followed
by an extension.

The default dumper simply dumps each resource as a binary file directly copied
from the PE file and gives a .bin extension.

`BITMAP` resources are converted into BMP images. `ICON` and `ICON_GROUP`
resources are converted into ICO files. `CURSOR` and `CURSOR_GROUP` resources
are converted into CUR files.

`MANIFEST` resources that start with a `<` are saved with an .xml extension.
Any resources that seem to be PNG, GIF, or JPEG images are saved with the
appropiate extension. All of these work without conversion, just with a
different file extension.

New dumpers can be added by creating a function similar to `dump_binary` in
`PEResourceDump.cpp` and adding it to the `dumpers` array before `dump_binary`.
The functions in this array are tried in order until one of them returns
`true`. The signature of the function must match the `dump_func` type.
