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
#include "gltfWriter.h"
#include "glslShader.h"

namespace _IOglTF_NS_ {

bool gltfWriter::WriteShaders () {
	//Json::Value buffer  ;
	//FbxString filename =FbxPathUtils::GetFileName (utility::conversions::to_utf8string (_fileName).c_str (), false) ;
	//buffer [("name")] =(utility::conversions::to_string_t (filename.Buffer ())) ;

	//buffer [("uri")] =(utility::conversions::to_string_t ((filename + ".bin").Buffer ())) ;
	//// The Buffer file should be fully completed by now.
	//if ( GetIOSettings ()->GetBoolProp (IOSN_FBX_GLTF_EMBEDMEDIA, false) ) {
	//	// data:[<mime type>][;charset=<charset>][;base64],<encoded data>
	//	buffer [("uri")] =(IOglTF::dataURI (_bin)) ;
	//}

	//if ( _writeDefaults )
	//	buffer [("type")] =(("arraybuffer")) ; ; // default is arraybuffer
	//buffer [("byteLength")] =((int)_bin.tellg ()) ;

	//_json [("buffers")] [utility::conversions::to_string_t (filename.Buffer ())] =buffer ;

	//Json::Value techs =_json [("techniques")] ;
	//for ( auto iter =techs.as_object ().begin () ; iter != techs.as_object ().end () ; iter++ ) {
	//	glslTech test (iter->second) ;

	//	std::cout << test.vertexShader ().source ()
	//		<< std::endl ;
	//}


	Json::Value materials =_json [("materials")] ;
	auto memberNames = materials.getMemberNames();
	for ( auto &name : memberNames ) {
		std::string techniqueName = materials[name] [("technique")].asString () ;
		glslTech tech (
			_json [("techniques")] [techniqueName],
			materials[name] [("values")],
			_json
		) ;

		std::string programName =_json [("techniques")] [techniqueName] [("program")].asString () ;
		std::string vsName =_json [("programs")] [programName] [("vertexShader")].asString () ;
		std::string fsName =_json [("programs")] [programName] [("fragmentShader")].asString () ;
		
		if ( GetIOSettings ()->GetBoolProp (IOSN_FBX_GLTF_EMBEDMEDIA, false) ) {
			// data:[<mime type>][;charset=<charset>][;base64],<encoded data>
			_json [("shaders")] [vsName] [("uri")] =(IOglTF::dataURI (tech.vertexShader ().source ())) ;
			_json [("shaders")] [fsName] [("uri")] =(IOglTF::dataURI (tech.fragmentShader ().source ())) ;
		} else {
			FbxString gltfFilename ((_fileName).c_str ()) ;
			std::string vsFilename =_json [("shaders")] [vsName] [("uri")].asString () ;
			{
				FbxString shaderFilename ((vsFilename).c_str ()) ;
#if defined(_WIN32) || defined(_WIN64)
				shaderFilename =FbxPathUtils::GetFolderName (gltfFilename) + "\\" + shaderFilename ;
#else
				shaderFilename =FbxPathUtils::GetFolderName (vsFilename.c_str()) + "/" + shaderFilename ;
#endif
				std::fstream shaderFile (shaderFilename, std::ios::out | std::ofstream::binary) ;
				//_bin.seekg (0, std::ios_base::beg) ;
				shaderFile.write (tech.vertexShader ().source ().c_str (), tech.vertexShader ().source ().length ()) ;
				shaderFile.close () ;
			}
			std::string fsFilename =_json [("shaders")] [fsName] [("uri")].asString () ;
			{
				FbxString shaderFilename ((fsFilename).c_str ()) ;
#if defined(_WIN32) || defined(_WIN64)
				shaderFilename =FbxPathUtils::GetFolderName (gltfFilename) + "\\" + shaderFilename ;
#else
				shaderFilename =FbxPathUtils::GetFolderName (fsFilename.c_str()) + "/" + shaderFilename ;
#endif
				std::fstream shaderFile (shaderFilename, std::ios::out | std::ofstream::binary) ;
				//_bin.seekg (0, std::ios_base::beg) ;
				shaderFile.write (tech.fragmentShader ().source ().c_str (), tech.fragmentShader ().source ().length ()) ;
				shaderFile.close () ;
			}
		}
	}
	return (true) ;
}

}
