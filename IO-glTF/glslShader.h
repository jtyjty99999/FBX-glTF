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
#pragma once

// http://stackoverflow.com/questions/13624124/online-webgl-glsl-shader-editor
// http://glslsandbox.com/

namespace _IOglTF_NS_ {

class glslShader {

protected:
	std::string _name ;
	std::string _declarations ;
	std::string _body ;
	std::map<std::string, std::string> _allSymbols ;

public:
	glslShader (const char *glslVersion =nullptr) ;
	virtual ~glslShader () ;

	std::string &name () ;
protected:
	void _addDeclaration (std::string qualifier, std::string symbol, unsigned int type, size_t count, bool forcesAsAnArray =false) ;
	std::string body () ;
public:
	void addAttribute (std::string symbol, unsigned int type, size_t count =1, bool forcesAsAnArray =false) ;
	void addUniform (std::string symbol, unsigned int type, size_t count =1, bool forcesAsAnArray =false) ;
	void addVarying (std::string symbol, unsigned int type, size_t count =1, bool forcesAsAnArray =false) ;
	bool hasSymbol (const std::string &symbol) ;

	void appendCode (const char *format, ...) ;
	//void appendCode (const char *code) ;
	std::string source () ;

	static bool isVertexShaderSemantic (const char *semantic) ;
	static bool isFragmentShaderSemantic (const char *semantic) ;

} ;

class glslTech {
	bool _bHasNormals ;
	bool _bHasJoint ;
	bool _bHasWeight ;
	bool _bHasSkin ;
	bool _bHasTexTangent ;
	bool _bHasTexBinormal ;
	bool _bModelContainsLights ;
	bool _bLightingIsEnabled ;
	bool _bHasAmbientLight ;
	bool _bHasSpecularLight ;
	bool _bHasNormalMap ;

protected:
	glslShader _vertexShader ;
	glslShader _fragmentShader ;

public:
	glslTech (Json::Value technique, Json::Value values, Json::Value gltf, const char *glslVersion =nullptr) ;
	virtual ~glslTech () ;

	glslShader &vertexShader () { return (_vertexShader) ; }
	glslShader &fragmentShader () { return (_fragmentShader) ; }

protected:
	static std::string format (const char *format, ...) ;
	const std::string needsVarying (const char *semantic) ;
	static bool isVertexShaderSemantic (const char *semantic) ;
	static bool isFragmentShaderSemantic (const char *semantic) ;
	void prepareParameters (Json::Value technique) ;
	void hwSkinning () ;
	bool lighting1 (Json::Value technique, Json::Value gltf) ;
	void texcoords (Json::Value technique) ;
	Json::Value lightNodes (Json::Value gltf) ;
	void lighting2 (Json::Value technique, Json::Value gltf) ;
	void finalizingShaders (Json::Value technique, Json::Value gltf) ;

} ;

}
