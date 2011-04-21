///////////////////////////////////////////////////////////////////////////////
// glInfo.h
// ========
// get GL vendor, version, supported extensions and other states using glGet*
// functions and store them glInfo struct variable
//
// To get valid OpenGL infos, OpenGL rendering context (RC) must be opened
// before calling glInfo::getInfo(). Otherwise it returns false.
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2005-10-04
// UPDATED: 2009-10-07
//
// Copyright (c) 2005 Song Ho Ahn
///////////////////////////////////////////////////////////////////////////////

#ifndef GLINFO_H
#define GLINFO_H

#include <string>
#include <vector>
#include <iostream>

// struct variable to store OpenGL info
typedef struct gl_info
{
    std::string vendor;
    std::string renderer;
    std::string version;
    std::vector <std::string> extensions;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int depthBits;
    int stencilBits;
    int maxTextureSize;
    int maxLights;
    int maxAttribStacks;
    int maxModelViewStacks;
    int maxProjectionStacks;
    int maxClipPlanes;
    int maxTextureStacks;
} gl_info_t;

bool gl_get_info(gl_info_t &info);
bool gl_is_extension_supported(const gl_info_t &info, const std::string& ext);

std::ostream &operator<<(std::ostream &os, const gl_info_t &info);

#endif
