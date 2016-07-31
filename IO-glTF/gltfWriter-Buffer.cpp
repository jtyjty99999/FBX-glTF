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

bool gltfWriter::WriteBuffer () {
	Json::Value buffer ;
	FbxString filename =FbxPathUtils::GetFileName ((_fileName).c_str (), false) ;
	buffer [("name")] =filename.Buffer () ;

	buffer [("uri")] =(filename + ".bin").Buffer () ;
	// The Buffer file should be fully completed by now.
	if ( GetIOSettings ()->GetBoolProp (IOSN_FBX_GLTF_EMBEDMEDIA, false) ) {
		// data:[<mime type>][;charset=<charset>][;base64],<encoded data>
		buffer [("uri")] =IOglTF::dataURI (_bin) ;
	}

	if ( _writeDefaults )
		buffer [("type")] =("arraybuffer") ; ; // default is arraybuffer
	buffer [("byteLength")] =((int)_bin.tellg ()) ;

	_json [("buffers")] [filename.Buffer ()] =buffer ;
	return (true) ;
}

}
