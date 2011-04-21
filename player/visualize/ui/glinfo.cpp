///////////////////////////////////////////////////////////////////////////////
// glInfo.cpp
// ==========
// get GL vendor, version, supported extensions and other states using glGet*
// functions and store them glInfo struct variable
//
// To get valid OpenGL infos, OpenGL rendering context (RC) must be opened
// before calling glInfo::getInfo(). Otherwise it returns false.
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2005-10-04
// UPDATED: 2009-10-06
//
// Copyright (c) 2005 Song Ho Ahn
///////////////////////////////////////////////////////////////////////////////

#include <GL/gl.h>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include "glinfo.h"

///////////////////////////////////////////////////////////////////////////////
// extract openGL info
// This function must be called after GL rendering context opened.
///////////////////////////////////////////////////////////////////////////////
bool gl_get_info(gl_info_t &info)
{
	char* str = 0;
	char* tok = 0;

	// get vendor string
	str = (char*)glGetString(GL_VENDOR);
	if(str) info.vendor = str;                  // check NULL return value
	else return false;

	// get renderer string
	str = (char*)glGetString(GL_RENDERER);
	if(str) info.renderer = str;                // check NULL return value
	else return false;

	// get version string
	str = (char*)glGetString(GL_VERSION);
	if(str) info.version = str;                 // check NULL return value
	else return false;

	// get all extensions as a string
	str = (char*)glGetString(GL_EXTENSIONS);

	// split extensions
	if(str)
	{
		tok = strtok((char*)str, " ");
		while(tok)
		{
			info.extensions.push_back(tok);    // put a extension into struct
			tok = strtok(0, " ");               // next token
		}
	}
	else
	{
		return false;
	}

	// sort extension by alphabetical order
	std::sort(info.extensions.begin(), info.extensions.end());

	// get number of color bits
	glGetIntegerv(GL_RED_BITS, &info.redBits);
	glGetIntegerv(GL_GREEN_BITS, &info.greenBits);
	glGetIntegerv(GL_BLUE_BITS, &info.blueBits);
	glGetIntegerv(GL_ALPHA_BITS, &info.alphaBits);

	// get depth bits
	glGetIntegerv(GL_DEPTH_BITS, &info.depthBits);

	// get stecil bits
	glGetIntegerv(GL_STENCIL_BITS, &info.stencilBits);

	// get max number of lights allowed
	glGetIntegerv(GL_MAX_LIGHTS, &info.maxLights);

	// get max texture resolution
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);

	// get max number of clipping planes
	glGetIntegerv(GL_MAX_CLIP_PLANES, &info.maxClipPlanes);

	// get max modelview and projection matrix stacks
	glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &info.maxModelViewStacks);
	glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &info.maxProjectionStacks);
	glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &info.maxAttribStacks);

	// get max texture stacks
	glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, &info.maxTextureStacks);

	return true;
}



///////////////////////////////////////////////////////////////////////////////
// check if the video card support a certain extension
///////////////////////////////////////////////////////////////////////////////
bool gl_is_extension_supported(const gl_info_t &info, const std::string& ext)
{
	// search corresponding extension
	std::vector<std::string>::const_iterator iter = info.extensions.begin();
	std::vector<std::string>::const_iterator endIter = info.extensions.end();

	while(iter != endIter)
	{
		if(ext == *iter)
			return true;
		else
			++iter;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// print OpenGL info to screen and save to a file
///////////////////////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &os, const gl_info_t &info)
{
	os << std::endl; // blank line
	os << "OpenGL Driver Info" << std::endl;
	os << "==================" << std::endl;
	os << "Vendor: " << info.vendor << std::endl;
	os << "Version: " << info.version << std::endl;
	os << "Renderer: " << info.renderer << std::endl;

	os << std::endl;
	os << "Color Bits(R,G,B,A): (" << info.redBits << ", " << info.greenBits
	   << ", " << info.blueBits << ", " << info.alphaBits << ")\n";
	os << "Depth Bits: " << info.depthBits << std::endl;
	os << "Stencil Bits: " << info.stencilBits << std::endl;

	os << std::endl;
	os << "Max Texture Size: " << info.maxTextureSize << "x" << info.maxTextureSize << std::endl;
	os << "Max Lights: " << info.maxLights << std::endl;
	os << "Max Clip Planes: " << info.maxClipPlanes << std::endl;
	os << "Max Modelview Matrix Stacks: " << info.maxModelViewStacks << std::endl;
	os << "Max Projection Matrix Stacks: " << info.maxProjectionStacks << std::endl;
	os << "Max Attribute Stacks: " << info.maxAttribStacks << std::endl;
	os << "Max Texture Stacks: " << info.maxTextureStacks << std::endl;

	os << std::endl;
	os << "Total Number of Extensions: " << info.extensions.size() << std::endl;
	os << "==============================" << std::endl;
	return os;
}
