//
// Copyright (c) Autodesk, Inc. All rights reserved 
//
// C++ glTF FBX importer/exporter plug-in
// by Cyrille Fauvel - Autodesk Developer Network (ADN)
// January 2015
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
#include "StdAfx.h"
#include "IOglTF.h"

// FBX Interface
extern "C" {
	// The DLL is owner of the plug-in
	static _IOglTF_NS_::IOglTF *pPlugin =nullptr ;

	// This function will be called when an application will request the plug-in
#if defined(_WIN64) || defined (_WIN32)
	__declspec(dllexport) void FBXPluginRegistration (FbxPluginContainer &pContainer, FbxModule pLibHandle) {
#else
	void FBXPluginRegistration (FbxPluginContainer &pContainer, FbxModule pLibHandle) {
#endif
		if ( pPlugin == nullptr ) {
			// Create the plug-in definition which contains the information about the plug-in
			FbxPluginDef pluginDef ;
			pluginDef.mName =FbxString (_IOglTF_NS_::IOglTF::PLUGIN_NAME) ;
			pluginDef.mVersion =FbxString (_IOglTF_NS_::IOglTF::PLUGIN_VERSION) ;

			// Create an instance of the plug-in.  The DLL has the ownership of the plug-in
			pPlugin =_IOglTF_NS_::IOglTF::Create (pluginDef, pLibHandle) ;

			// Register the plug-in
			pContainer.Register (*pPlugin) ;
		}
	}
}

namespace _IOglTF_NS_ {

FBXSDK_PLUGIN_IMPLEMENT(IOglTF) ;

bool IOglTF::SpecificInitialize () {
	int registeredCount =0 ;
	int gltfReaderId =0, gltfWriterId =0 ;
	GetData ().mSDKManager->GetIOPluginRegistry ()->RegisterWriter (gltfWriter::Create_gltfWriter, gltfWriter::gltfFormatInfo, gltfWriterId, registeredCount, gltfWriter::FillIOSettings) ;
	GetData ().mSDKManager->GetIOPluginRegistry ()->RegisterReader (gltfReader::Create_gltfReader, gltfReader::gltfFormatInfo, gltfReaderId, registeredCount, gltfReader::FillIOSettings) ;
	return (true) ;
}

//-----------------------------------------------------------------------------

/*static*/ const char *IOglTF::PLUGIN_NAME ="IO-glTF" ;
/*static*/ const char *IOglTF::PLUGIN_VERSION ="1.0.0" ;

#ifdef __APPLE__
/*static*/ const unsigned int IOglTF::BYTE ;
/*static*/ const unsigned int IOglTF::UNSIGNED_BYTE ;
/*static*/ const unsigned int IOglTF::SHORT ;
/*static*/ const unsigned int IOglTF::UNSIGNED_SHORT ;
/*static*/ const unsigned int IOglTF::INT ;
/*static*/ const unsigned int IOglTF::UNSIGNED_INT ;
/*static*/ const unsigned int IOglTF::FLOAT ;
/*static*/ const unsigned int IOglTF::UNSIGNED_SHORT_5_6_5 ;
/*static*/ const unsigned int IOglTF::UNSIGNED_SHORT_4_4_4_4 ;
/*static*/ const unsigned int IOglTF::UNSIGNED_SHORT_5_5_5_1 ;
/*static*/ const unsigned int IOglTF::FLOAT_VEC2 ;
/*static*/ const unsigned int IOglTF::FLOAT_VEC3 ;
/*static*/ const unsigned int IOglTF::FLOAT_VEC4 ;
/*static*/ const unsigned int IOglTF::INT_VEC2 ;
/*static*/ const unsigned int IOglTF::INT_VEC3 ;
/*static*/ const unsigned int IOglTF::INT_VEC4 ;
/*static*/ const unsigned int IOglTF::BOOL ;
/*static*/ const unsigned int IOglTF::BOOL_VEC2 ;
/*static*/ const unsigned int IOglTF::BOOL_VEC3 ;
/*static*/ const unsigned int IOglTF::BOOL_VEC4 ;
/*static*/ const unsigned int IOglTF::FLOAT_MAT2 ;
/*static*/ const unsigned int IOglTF::FLOAT_MAT3 ;
/*static*/ const unsigned int IOglTF::FLOAT_MAT4 ;
/*static*/ const unsigned int IOglTF::SAMPLER_2D ;
/*static*/ const unsigned int IOglTF::SAMPLER_CUBE ;
/*static*/ const unsigned int IOglTF::FRAGMENT_SHADER ;
/*static*/ const unsigned int IOglTF::VERTEX_SHADER ;
#endif
/*static*/ const char *IOglTF::szSCALAR =("SCALAR") ;
/*static*/ const char *IOglTF::szFLOAT =("FLOAT") ;
/*static*/ const char *IOglTF::szVEC2 =("VEC2") ;
/*static*/ const char *IOglTF::szVEC3 =("VEC3") ;
/*static*/ const char *IOglTF::szVEC4 =("VEC4") ;
/*static*/ const char *IOglTF::szINT =("INT") ; ;
/*static*/ const char *IOglTF::szIVEC2 =("IVEC2") ; ;
/*static*/ const char *IOglTF::szIVEC3 =("IVEC3") ; ;
/*static*/ const char *IOglTF::szIVEC4 =("IVEC4") ; ;
/*static*/ const char *IOglTF::szBOOL =("BOOL") ; ;
/*static*/ const char *IOglTF::szBVEC2 =("BVEC2") ; ;
/*static*/ const char *IOglTF::szBVEC3 =("BVEC3") ; ;
/*static*/ const char *IOglTF::szBVEC4 =("BVEC4") ; ;
/*static*/ const char *IOglTF::szMAT2 =("MAT2") ;
/*static*/ const char *IOglTF::szMAT3 =("MAT3") ;
/*static*/ const char *IOglTF::szMAT4 =("MAT4") ;
/*static*/ const char *IOglTF::szSAMPLER_2D =("sampler2D") ;
#ifdef __APPLE__
/*static*/ const unsigned int IOglTF::ARRAY_BUFFER ;
/*static*/ const unsigned int IOglTF::ELEMENT_ARRAY_BUFFER ;
/*static*/ const unsigned int IOglTF::POINTS ;
/*static*/ const unsigned int IOglTF::LINES ;
/*static*/ const unsigned int IOglTF::LINE_LOOP ;
/*static*/ const unsigned int IOglTF::LINE_STRIP ;
/*static*/ const unsigned int IOglTF::TRIANGLES ;
/*static*/ const unsigned int IOglTF::TRIANGLE_STRIP ;
/*static*/ const unsigned int IOglTF::TRIANGLE_FAN ;
/*static*/ const unsigned int IOglTF::CULL_FACE ;
/*static*/ const unsigned int IOglTF::DEPTH_TEST ;
/*static*/ const unsigned int IOglTF::BLEND ;
/*static*/ const unsigned int IOglTF::POLYGON_OFFSET_FILL ;
/*static*/ const unsigned int IOglTF::SAMPLE_ALPHA_TO_COVERAGE ;
/*static*/ const unsigned int IOglTF::SCISSOR_TEST ;
/*static*/ const unsigned int IOglTF::TEXTURE_2D ;
/*static*/ const unsigned int IOglTF::ALPHA ;
/*static*/ const unsigned int IOglTF::RGB ;
/*static*/ const unsigned int IOglTF::RGBA ;
/*static*/ const unsigned int IOglTF::LUMINANCE ;
/*static*/ const unsigned int IOglTF::LUMINANCE_ALPHA ;
/*static*/ const unsigned int IOglTF::NEAREST ;
/*static*/ const unsigned int IOglTF::LINEAR ;
/*static*/ const unsigned int IOglTF::NEAREST_MIPMAP_NEAREST ;
/*static*/ const unsigned int IOglTF::LINEAR_MIPMAP_NEAREST ;
/*static*/ const unsigned int IOglTF::NEAREST_MIPMAP_LINEAR ;
/*static*/ const unsigned int IOglTF::LINEAR_MIPMAP_LINEAR ;
/*static*/ const unsigned int IOglTF::CLAMP_TO_EDGE ;
/*static*/ const unsigned int IOglTF::MIRRORED_REPEAT ;
/*static*/ const unsigned int IOglTF::REPEAT ;
#endif
/*static*/ const Json::Value IOglTF::Identity2 ;//=Json::Value::array ({ { 1., 0., 0., 1. } }) ;
/*static*/ const Json::Value IOglTF::Identity3 ;// =Json::Value::array ({ { 1., 0., 0., 0., 1., 0., 0., 0., 1. } }) ;
/*static*/ const Json::Value IOglTF::Identity4 ;// =Json::Value::array ({{ 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1. }}) ;

// Get extension, description or version info about gltfWriter
void *_gltfFormatInfo (FbxWriter::EInfoRequest pRequest, int pId) {
	static const char *sExt [] = { "gltf", 0 } ;
	static const char *sDesc [] = { "glTF for WebGL (*.gltf)", 0 } ;
	static const char *sVersion [] = { "0.8", 0 } ;
	static const char *sInfoCompatible [] = { "-", 0 } ;
	static const char *sInfoUILabel [] = { "-", 0 } ;

	switch ( pRequest ) {
		case FbxWriter::eInfoExtension:
			return (sExt) ;
		case FbxWriter::eInfoDescriptions:
			return (sDesc) ;
		case FbxWriter::eInfoVersions:
			return (sVersion) ;
		case FbxWriter::eInfoCompatibleDesc:
			return (sInfoCompatible) ;
		case FbxWriter::eInfoUILabel:
			return (sInfoUILabel) ;
		default:
			return (0) ;
	}
}

//-----------------------------------------------------------------------------
/*static*/ unsigned int IOglTF::techniqueParameters (const char *szType, int compType /*=FLOAT*/) {
	std::string st (szType) ;
	if ( st == szSCALAR )
		return (compType) ;
	if ( st == szVEC2 )
		return (compType == FLOAT ? FLOAT_VEC2 : (compType == INT ? INT_VEC2 : BOOL_VEC2)) ;
	if ( st == szVEC3 )
		return (compType == FLOAT ? FLOAT_VEC3 : (compType == INT ? INT_VEC3 : BOOL_VEC3)) ;
	if ( st == szVEC4 )
		return (compType == FLOAT ? FLOAT_VEC4 : (compType == INT ? INT_VEC4 : BOOL_VEC4)) ;
	if ( st == szMAT2 )
		return (FLOAT_MAT2) ;
	if ( st == szMAT3 )
		return (FLOAT_MAT3) ;
	if ( st == szMAT4 )
		return (FLOAT_MAT4) ;
	return (0) ;
}

/*static*/ const char *IOglTF::glslAccessorType (unsigned int glType) {
	switch ( glType ) {
		case FLOAT:
		case INT:
		case BOOL:
		case UNSIGNED_SHORT:
		case UNSIGNED_INT:
			return (szSCALAR) ;
		case FLOAT_VEC2:
		case INT_VEC2:
		case BOOL_VEC2:
		case SAMPLER_2D:
			return (szVEC2) ;
		case FLOAT_VEC3:
		case INT_VEC3:
		case BOOL_VEC3:
		case SAMPLER_CUBE:
			return (szVEC3) ;
		case FLOAT_VEC4:
		case INT_VEC4:
		case BOOL_VEC4:
			return (szVEC4) ;
		case FLOAT_MAT2:
			return (szMAT2) ;
		case FLOAT_MAT3:
			return (szMAT3) ;
		case FLOAT_MAT4:
			return (szMAT4) ;
	}
	return (("")) ;
}

/*static*/ const std::string IOglTF::glslShaderType (unsigned int glType) {
	std::string szType ;
	switch ( glType ) {
		case FLOAT:
			szType =szFLOAT ;
			break ;
		case INT:
		case BOOL:
		case UNSIGNED_SHORT:
		case UNSIGNED_INT:
			szType =szSCALAR ;
			break ;
		case FLOAT_VEC2:
		case INT_VEC2:
		case BOOL_VEC2:
			szType =szVEC2 ;
			break ;
		case FLOAT_VEC3:
		case INT_VEC3:
		case BOOL_VEC3:
		case SAMPLER_CUBE:
			szType =szVEC3 ;
			break ;
		case FLOAT_VEC4:
		case INT_VEC4:
		case BOOL_VEC4:
			szType =szVEC4 ;
			break ;
		case FLOAT_MAT2:
			szType =szMAT2 ;
			break ;
		case FLOAT_MAT3:
			szType =szMAT3 ;
			break ;
		case FLOAT_MAT4:
			szType =szMAT4 ;
			break ;
		case SAMPLER_2D:
			return (szSAMPLER_2D) ;
		default:
			break ;
	}
	std::transform (szType.begin (), szType.end (), szType.begin (), ::tolower) ;
	return (szType) ;
}

/*static*/ const char *IOglTF::mimeType (const char *szFilename) {
	std::string contentType =("application/octet-stream") ;
	FbxString ext =FbxPathUtils::GetExtensionName (szFilename) ;
	ext =ext.Lower () ;
	if ( ext == "png" )
		return (("image/png")) ;
	if ( ext == "gif" )
		return (("image/gif")) ;
	if ( ext == "jpg" || ext == "jpeg" )
		return (("image/jpeg")) ;
	return (("application/octet-stream")) ;
}
	
/*
base64.cpp and base64.h

Copyright (C) 2004-2008 René Nyffenegger

This source code is provided 'as-is', without any express or implied
warranty. In no event will the author be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this source code must not be misrepresented; you must not
claim that you wrote the original source code. If you use this source code
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original source code.

3. This notice may not be removed or altered from any source distribution.

René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/
	
static const std::string base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";
	
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];
		
  while (in_len--) {
	  char_array_3[i++] = *(bytes_to_encode++);
	  if (i == 3) {
		  char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		  char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		  char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		  char_array_4[3] = char_array_3[2] & 0x3f;
		  
		  for(i = 0; (i <4) ; i++)
			  ret += base64_chars[char_array_4[i]];
		  i = 0;
	  }
  }
		
  if (i)
  {
	  for(j = i; j < 3; j++)
		  char_array_3[j] = '\0';
	  
	  char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
	  char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
	  char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
	  char_array_4[3] = char_array_3[2] & 0x3f;
	  
	  for (j = 0; (j < i + 1); j++)
		  ret += base64_chars[char_array_4[j]];
	  
	  while((i++ < 3))
		  ret += '=';
	  
  }
		
  return ret;
		
}

/*static*/ const std::string IOglTF::dataURI (const std::string fileName) {
	// data:[<mime type>][;charset=<charset>][;base64],<encoded data>
	std::string st (("data:")) ;
	std::fstream fileStream( fileName );
	
	try {
		fileStream.seekg (0, std::ios::end) ;
		auto length =static_cast<size_t> (fileStream.tellg ()) ;
		fileStream.seekg (0, std::ios_base::beg) ;

		std::vector<uint8_t> v8 ;
		v8.resize (length) ;
		fileStream.get (reinterpret_cast<char*>(v8.data()), length) ;

		st +=IOglTF::mimeType (fileName.c_str ()) ;
		//st +="[;charset=<charset>]" ;
		st +=(";base64") ;
		st +=(",") +  base64_encode( v8.data(), static_cast<uint32_t>(length) ) ;
	} catch ( ... ) {
		//std::cout << ("Error: ") << e.what () << std::endl ;
	}
	return (st) ;
}

/*static*/ const std::string IOglTF::dataURI (memoryStream<uint8_t> &stream) {
	// data:[<mime type>][;charset=<charset>][;base64],<encoded data>
	std::string st (("data:")) ;
	st +=IOglTF::mimeType (("")) ;
	//st +="[;charset=<charset>]" ;
	st +=(";base64") ;
	auto &vec = stream.vec();
	st +=(",") + base64_encode( vec.data(), static_cast<uint32_t>(vec.size()) ) ;
	return (st) ;
}

}
