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
#include "gltfwriterVBO.h"
#include <string.h> // for memcmp

namespace _IOglTF_NS_ {

//-----------------------------------------------------------------------------
Json::Value gltfWriter::WriteMesh (FbxNode *pNode) {
	Json::Value meshDef  ;
	meshDef [("name")] =(nodeId (pNode, true)) ;

	//if ( _json [("meshes")].isMember (meshDef [("name")].asString ()) ) {
	if ( isKnownId (pNode->GetNodeAttribute ()->GetUniqueID ()) ) {
		// The mesh/material/... were already exported, create only the transform node
		Json::Value node =WriteNode (pNode) ;
		Json::Value ret ;
		ret[("nodes")] = node ;
		return (ret) ;
	}

	Json::Value meshPrimitives ;
	Json::Value accessorsAndBufferViews ;
	accessorsAndBufferViews[("accessors")] = Json::Value( Json::objectValue );
	accessorsAndBufferViews[("bufferViews")] = Json::Value( Json::objectValue );

	Json::Value materials ;
	materials["materials"] = Json::Value( Json::objectValue ) ;
	Json::Value techniques ;
	techniques[("techniques")] = Json::Value( Json::objectValue ) ;
	Json::Value programs ;
	programs[("programs")] = Json::Value( Json::objectValue ) ;
	Json::Value images ;
	images[("images")] = Json::Value( Json::objectValue ) ;
	Json::Value samplers ;
	samplers[("samplers")] = Json::Value( Json::objectValue ) ;
	Json::Value textures ;
	textures[("textures")] = Json::Value( Json::objectValue ) ;

	FbxMesh *pMesh =pNode->GetMesh () ; //FbxCast<FbxMesh>(pNode->GetNodeAttribute ()) ;
	pMesh->ComputeBBox () ;

	// FBX Layers works like Photoshop layers
	// - Vertices, Polygons and Edges are not on layers, but every other elements can
	// - Usually Normals are on Layer 0 (FBX default) but can eventually be on other layers
	// - UV, vertex colors, etc... can have multiple layer representation or be on different layers,
	//   the code below will try to retrieve each element recursevily to merge all elements from 
	//   different layer.

	// Ensure we covered all layer elements of layer 0
	// - Normals (all layers... does GLTF support this?)
	// - UVs on all layers
	// - Vertex Colors on all layers.
	// - Materials and Textures are covered when the mesh is exported.
	// - Warnings for unsupported layer element types: polygon groups, undefined

	int nbLayers =pMesh->GetLayerCount () ;
	Json::Value primitive ;
	primitive[("attributes")] = Json::Value( Json::objectValue ) ;
	primitive[("mode")] = IOglTF::TRIANGLES ; // Allowed values are 0 (POINTS), 1 (LINES), 2 (LINE_LOOP), 3 (LINE_STRIP), 4 (TRIANGLES), 5 (TRIANGLE_STRIP), and 6 (TRIANGLE_FAN).

////Json::Value elts =Json::Value::object ({
////	{ ("normals"), Json::Value::object () },
////	{ ("uvs"), Json::Value::object () },
////	{ ("colors"), Json::Value::object () }
////}) ;
	Json::Value localAccessorsAndBufferViews  ;

	gltfwriterVBO vbo (pMesh) ;
	vbo.GetLayerElements (true) ;
	vbo.indexVBO () ;

	std::vector<unsigned short> out_indices =vbo.getIndices () ;
	std::vector<FbxDouble3> out_positions =vbo.getPositions () ;
	std::vector<FbxDouble3> out_normals =vbo.getNormals () ;
	std::vector<FbxDouble2> out_uvs =vbo.getUvs () ;
	std::vector<FbxDouble3> out_tangents =vbo.getTangents () ;
	std::vector<FbxDouble3> out_binormals =vbo.getBinormals () ;
	std::vector<FbxColor> out_vcolors =vbo.getVertexColors () ;

	_uvSets =vbo.getUvSets () ;

	Json::Value vertex =WriteArrayWithMinMax<FbxDouble3, float> (out_positions, pMesh->GetNode (), ("_Positions")) ;
	MergeJsonObjects (localAccessorsAndBufferViews, vertex);
	primitive [("attributes")] [("POSITION")] =(GetJsonFirstKey (vertex [("accessors")])) ;

	if ( out_normals.size () ) {
		std::string st (("_Normals")) ;
		Json::Value ret =WriteArrayWithMinMax<FbxDouble3, float> (out_normals, pMesh->GetNode (), st.c_str ()) ;
		MergeJsonObjects (localAccessorsAndBufferViews, ret) ;
		st=("NORMAL") ;
		primitive [("attributes")] [st] =(GetJsonFirstKey (ret [("accessors")])) ;
	}

	if ( out_uvs.size () ) { // todo more than 1
		std::map<std::string, std::string>::iterator iter =_uvSets.begin () ;
		std::string st (("_") + iter->second) ;
		Json::Value ret =WriteArrayWithMinMax<FbxDouble2, float> (out_uvs, pMesh->GetNode (), st.c_str ()) ;
		MergeJsonObjects (localAccessorsAndBufferViews, ret) ;
		primitive [("attributes")] [iter->second] =(GetJsonFirstKey (ret [("accessors")])) ;
	}

	int nb=(int)out_vcolors.size () ;
	if ( nb ) {
		std::vector<FbxDouble4> vertexColors_ (nb);
		for ( int i =0 ; i < nb ; i++ )
			vertexColors_.push_back (FbxDouble4 (out_vcolors [i].mRed, out_vcolors [i].mGreen, out_vcolors [i].mBlue, out_vcolors [i].mAlpha)) ;
		std::string st (("_Colors0")) ;
		Json::Value ret =WriteArrayWithMinMax<FbxDouble4, float> (vertexColors_, pMesh->GetNode (), st.c_str ()) ;
		MergeJsonObjects (localAccessorsAndBufferViews, ret) ;
		st =("COLOR_0") ;
		primitive [("attributes")] [st] =(GetJsonFirstKey (ret [("accessors")])) ;
	}

	// Get mesh face indices
	Json::Value polygons =WriteArray<unsigned short> (out_indices, 1, pMesh->GetNode (), ("_Polygons")) ;
	primitive [("indices")] =(GetJsonFirstKey (polygons [("accessors")])) ;

	MergeJsonObjects (accessorsAndBufferViews, polygons) ;
	MergeJsonObjects (accessorsAndBufferViews, localAccessorsAndBufferViews) ;

	// Get material
	FbxLayer *pLayer =gltfwriterVBO::getLayer (pMesh, FbxLayerElement::eMaterial) ;
	if ( pLayer == nullptr ) {
		//std::cout << ("Info: (") << utility::conversions::to_string_t (pNode->GetTypeName ())
		//	<< (") ") << utility::conversions::to_string_t (pNode->GetName ())
		//	<< (" no material on Layer: ")
		//	<< iLayer
		//	<< std::endl ;
		// Create default material
		Json::Value ret =WriteDefaultMaterial (pNode) ;
		if ( ret.isString () ) {
			primitive [("material")] =ret ;
		} else {
			primitive [("material")] =(GetJsonFirstKey (ret [("materials")])) ;

			MergeJsonObjects (materials [("materials")], ret [("materials")]) ;

			std::string techniqueName =GetJsonFirstKey (ret [("techniques")]) ;
			Json::Value techniqueParameters =ret [("techniques")] [techniqueName] [("parameters")] ;
			AdditionalTechniqueParameters (pNode, techniqueParameters, out_normals.size () != 0) ;
			TechniqueParameters (pNode, techniqueParameters, primitive [("attributes")], localAccessorsAndBufferViews [("accessors")], false) ;
			ret =WriteTechnique (pNode, nullptr, techniqueParameters) ;
			//MergeJsonObjects (techniques, ret) ;
			techniques [("techniques")] [techniqueName] =ret ;

			std::string programName =ret [("program")].asString () ;
			Json::Value attributes =ret [("attributes")] ;
			ret =WriteProgram (pNode, nullptr, programName, attributes) ;
			MergeJsonObjects (programs, ret) ;
		}
	} else {
		FbxLayerElementMaterial *pLayerElementMaterial =pLayer->GetMaterials () ;
		int materialCount =pLayerElementMaterial ? pNode->GetMaterialCount () : 0 ;
		if ( materialCount > 1 ) {
			_ASSERTE( materialCount > 1 ) ;
			std::cout << ("Warning: (") << (pNode->GetTypeName ())
				<< (") ") << (pNode->GetName ())
				<< (" got more than one material. glTF supports one material per primitive (FBX Layer).")
				<< std::endl ;
		}
		// TODO: need to be revisited when glTF will support more than one material per layer/primitive
		materialCount =materialCount == 0 ? 0 : 1 ;
		for ( int i =0 ; i < materialCount ; i++ ) {
			Json::Value ret =WriteMaterial (pNode, pNode->GetMaterial (i)) ;
			if ( ret.isString () ) {
				primitive [("material")] =ret ;
				continue ;
			}
			primitive [("material")]=(GetJsonFirstKey (ret [("materials")])) ;

			MergeJsonObjects (materials [("materials")], ret [("materials")]) ;
			if ( ret.isMember (("images")) )
				MergeJsonObjects (images [("images")], ret [("images")]) ;
			if ( ret.isMember (("samplers")) )
				MergeJsonObjects (samplers [("samplers")], ret [("samplers")]) ;
			if ( ret.isMember (("textures")) )
				MergeJsonObjects (textures [("textures")], ret [("textures")]) ;

			std::string techniqueName =GetJsonFirstKey (ret [("techniques")]) ;
			Json::Value techniqueParameters =ret [("techniques")] [techniqueName] [("parameters")] ;
			AdditionalTechniqueParameters (pNode, techniqueParameters, out_normals.size () != 0) ;
			TechniqueParameters (pNode, techniqueParameters, primitive [("attributes")], localAccessorsAndBufferViews [("accessors")]) ;
			ret =WriteTechnique (pNode, pNode->GetMaterial (i), techniqueParameters) ;
			//MergeJsonObjects (techniques, ret) ;
			techniques [("techniques")] [techniqueName] =ret ;

			std::string programName =ret [("program")].asString () ;
			Json::Value attributes =ret [("attributes")] ;
			ret =WriteProgram (pNode, pNode->GetMaterial (i), programName, attributes) ;
			MergeJsonObjects (programs, ret) ;
		}
	}
	meshPrimitives [meshPrimitives.size ()] =primitive ;
	meshDef [("primitives")] =meshPrimitives ;

	Json::Value lib ;
	lib[nodeId (pNode, true, true)] = meshDef ;

	Json::Value node =WriteNode (pNode) ;
	//if ( pMesh->GetShapeCount () )
	//	WriteControllerShape (pMesh) ; // Create a controller

	Json::Value ret ;
	ret[("meshes")] = lib ;
	ret[("nodes")] = node ;
	MergeJsonObjects (ret, accessorsAndBufferViews) ;
	MergeJsonObjects (ret, images) ;
	MergeJsonObjects (ret, materials) ;
	MergeJsonObjects (ret, techniques) ;
	MergeJsonObjects (ret, programs) ;
	MergeJsonObjects (ret, samplers) ;
	MergeJsonObjects (ret, textures) ;

	return (ret) ;
}

}
