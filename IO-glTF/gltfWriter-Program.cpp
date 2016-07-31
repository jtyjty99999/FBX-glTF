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

namespace _IOglTF_NS_ {

Json::Value gltfWriter::WriteProgram (FbxNode *pNode, FbxSurfaceMaterial *pMaterial, std::string programName, Json::Value &attributes) {
	Json::Value programAttributes ;
	auto memberNames = attributes.getMemberNames();
	for ( const auto &name : memberNames )
		programAttributes [programAttributes.size ()] =(name) ;

	// Get the implementation to see if it's a hardware shader.
	if ( pMaterial != nullptr ) {
		//const FbxImplementation *pImplementation =GetImplementation (pMaterial, FBXSDK_IMPLEMENTATION_HLSL) ;
		//if ( !pImplementation )
		//	pImplementation =GetImplementation (pMaterial, FBXSDK_IMPLEMENTATION_CGFX) ;
		//if ( pImplementation ) {
		//	const FbxBindingTable *pRootTable =pImplementation->GetRootTable () ;
		//	FbxString fileName =pRootTable->DescAbsoluteURL.Get () ;
		//	FbxString pTechniqueName =pRootTable->DescTAG.Get () ;
		//	const FbxBindingTable *pTable =pImplementation->GetRootTable () ;
		//	size_t entryNum =pTable->GetEntryCount () ;
		//	for ( size_t i =0 ; i < entryNum ; i++ ) {
		//		const FbxBindingTableEntry &entry =pTable->GetEntry (i) ;
		//		const char *pszEntrySrcType =entry.GetEntryType (true) ;
		//	}
		//} else {
		//	int nb =pNode->GetImplementationCount () ;
		//	//pImplementation =pNode->GetImplementation (0) ;
		//}
	}
	FbxString filename =FbxPathUtils::GetFileName ((_fileName).c_str (), false) ;

	Json::Value program ;
	program[("attributes")] = programAttributes ;
	program[("name")] = (programName) ;
	program[("fragmentShader")] = (programName + ("FS")) ;
	program[("vertexShader")] = (programName + ("VS")) ;

	Json::Value lib ;
	lib[programName] = program ;

	Json::Value shaders =WriteShaders (pNode, program) ;
	Json::Value ret;
	ret["programs"] = lib;
	ret["shaders"] = shaders;
	return ret ;
}

Json::Value gltfWriter::WriteShaders (FbxNode *pNode, Json::Value &program) {
	Json::Value ret ;
	auto &fragment = ret[program [("fragmentShader")].asString ()];
	fragment[("type")] = ((int)IOglTF::FRAGMENT_SHADER) ;
	fragment[("uri")] = (program [("fragmentShader")].asString () + (".glsl")) ;
	
	auto &vertex = ret[program [("vertexShader")].asString ()];
	vertex[("type")] = ((int)IOglTF::VERTEX_SHADER) ;
	vertex[("uri")] = (program [("vertexShader")].asString () + (".glsl")) ;

	return ret;
}

}
