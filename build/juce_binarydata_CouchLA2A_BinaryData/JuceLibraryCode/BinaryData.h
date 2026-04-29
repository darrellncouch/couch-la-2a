/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   knob_filmstrip_png;
    const int            knob_filmstrip_pngSize = 1414954;

    extern const char*   panel_metal_png;
    const int            panel_metal_pngSize = 34569;

    extern const char*   toggle_up_png;
    const int            toggle_up_pngSize = 118517;

    extern const char*   toggle_down_png;
    const int            toggle_down_pngSize = 118581;

    extern const char*   window_png;
    const int            window_pngSize = 473964;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 5;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
