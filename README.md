# ManyTypes

C/C++ type parsing library based on libclang.

This project was intended to parse windows headers and phnt.h in order to generate type data which is parsable by x64dbg.

## x64dbg usage
When starting a x64dbg debug session with the plugin enabled, ManyTypes will create a folder in the root of x64dbg called `ManyTypes/[ImageName]`.
This folder will contain a `project.h` file. 

When the file is updated, ManyTypes will attempt to **only parse the header `project.h`**
Other headers in `ManyTypes/[ImageName]` will be ignored, but they may be included in `project.h`

TLDR:
1. Start debug session
2. Open `x64dbg/ManyTypes/[ImageName]`
3. Edit and save `project.h`
4. Use the newly created type in struct window.

## Missing Features
- Template parsing