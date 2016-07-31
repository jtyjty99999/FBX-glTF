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

bool gltfWriter::WriteAsset (FbxDocumentInfo *pSceneInfo) {
	Json::Value asset ;

	// unit - <meter> and <name>. In FBX we always work in centimeters, but we already converted to meter here
	//double scale =_scene->GetGlobalSettings ().GetSystemUnit ().GetScaleFactor () / 100. ;

	// Up axis - Y up axis. The scene already got converted to Maya Y up axis
	//FbxScene *pScene =sceneInfo->GetScene () ;
	//FbxAxisSystem axisSystem =pScene->GetGlobalSettings ().GetAxisSystem () ;

	// FBX uses author and comments, not authoring_tool(i.e. generator), and copyright.
	asset [("copyright")] =(pSceneInfo->mAuthor.Buffer ()) ;
	asset [("generator")] =(FBX_GLTF_EXPORTER) ;
	if ( _writeDefaults )
		asset [("premultipliedAlpha")] =(false) ;
	if (_writeDefaults) {
		asset[("profile")]["api"] =(PROFILE_API) ; // default "WebGL"
		asset[("profile")]["version"] =(PROFILE_VERSION) ; // default 1.0.3
	}

	asset [("version")] =(GLTF_VERSION) ;

	_json [("asset")] =asset ;
	return (true) ;
}

}
