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

#ifdef _XX_
FbxArray<FbxVector4> GetVertexPositions (FbxLine *pLine, bool bInGeometry =true, bool bExportControlPoints =true) {
	// In an ordinary geometry, export the control points.
	// In a binded geometry, export transformed control points...
	// In a controller, export the control points.
	bExportControlPoints =true ;

	FbxArray<FbxVector4> controlPoints ;
	// Get Control points.
	// Translate a FbxVector4* into FbxArray<FbxVector4>
	FbxVector4 *pTemp =pLine->GetControlPoints () ;
	int nbControlPoints =pLine->GetControlPointsCount () ;
	for ( int i =0 ; i < nbControlPoints ; i++ )
		controlPoints.Add (pTemp [i]) ;

	if ( bExportControlPoints ) {
		if ( !bInGeometry ) {
			FbxAMatrix transform =pLine->GetNode ()->EvaluateGlobalTransform () ;
			for ( int i =0 ; i < nbControlPoints ; i++ )
				controlPoints [i] =transform.MultT (controlPoints [i]) ;
		}
		return (controlPoints) ;
	}

	// Initialize positions
	FbxArray<FbxVector4> positions (nbControlPoints) ;
	// Get the transformed control points.
	int deformerCount =pLine->GetDeformerCount (FbxDeformer::eSkin) ;
	_ASSERTE (deformerCount <= 1); // Unexpected number of skin greater than 1
	// It is expected for deformerCount to be equal to 1
	for ( int i =0 ; i < deformerCount ; i++ ) {
		int clusterCount =FbxCast<FbxSkin> (pLine->GetDeformer (FbxDeformer::eSkin))->GetClusterCount () ;
		for ( int indexLink =0 ; indexLink < clusterCount ; indexLink++ ) {
			FbxCluster *pLink =FbxCast<FbxSkin> (pLine->GetDeformer (FbxDeformer::eSkin))->GetCluster (indexLink) ;
			FbxAMatrix jointPosition =pLink->GetLink ()->EvaluateGlobalTransform () ;
			FbxAMatrix transformLink ;
			pLink->GetTransformLinkMatrix (transformLink) ;
			FbxAMatrix m =transformLink.Inverse () * jointPosition ;
			for ( int j =0 ; j < pLink->GetControlPointIndicesCount () ; j++ ) {
				int index =pLink->GetControlPointIndices () [j] ;
				FbxVector4 controlPoint =controlPoints [index] ;
				double weight =pLink->GetControlPointWeights () [j] ;
				FbxVector4 pos =m.MultT (controlPoint) ;
				pos =pos * weight ;
				positions [index] =positions [index] + pos ;
			}
		}
	}
	return (positions) ;
}

//-----------------------------------------------------------------------------
Json::Value gltfWriter::WriteLine (FbxNode *pNode) {
#ifdef GG
	Json::Value lineDef  ;
	lineDef [("name")] =(nodeId (pNode, true)) ;

	if ( isKnownId (pNode->GetNodeAttribute ()->GetUniqueID ()) ) {
		// The line/material/... were already exported, create only the transform node
		Json::Value node =WriteNode (pNode) ;
		Json::Value ret =Json::Value::object ({ { ("nodes"), node } }) ;
		return (ret) ;
	}

	Json::Value linePrimitives =Json::Value::array () ;
	Json::Value accessorsAndBufferViews =Json::Value::object ({
		{ ("accessors"), Json::Value::object () },
		{ ("bufferViews"), Json::Value::object () }
	}) ;
	Json::Value materials =Json::Value::object ({ { ("materials"), Json::Value::object () } }) ;
	Json::Value techniques =Json::Value::object ({ { ("techniques"), Json::Value::object () } }) ;
	Json::Value programs =Json::Value::object ({ { ("programs"), Json::Value::object () } }) ;
	Json::Value images =Json::Value::object ({ { ("images"), Json::Value::object () } }) ;
	Json::Value samplers =Json::Value::object ({ { ("samplers"), Json::Value::object () } }) ;
	Json::Value textures =Json::Value::object ({ { ("textures"), Json::Value::object () } }) ;

	FbxLine *pLine =pNode->GetLine () ; //FbxCast<FbxLine>(pNode->GetNodeAttribute ()) ;
	pLine->ComputeBBox () ;

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
	// - Materials and Textures are covered when the line is exported.
	// - Warnings for unsupported layer element types: polygon groups, undefined

	int nbLayers =pLine->GetLayerCount () ;
	Json::Value primitive=Json::Value::object ({
		{ ("attributes"), Json::Value::object () },
		{ ("mode"), IOglTF::LINES } // Allowed values are 0 (POINTS), 1 (LINES), 2 (LINE_LOOP), 3 (LINE_STRIP), 4 (TRIANGLES), 5 (TRIANGLE_STRIP), and 6 (TRIANGLE_FAN).
	}) ;

	Json::Value localAccessorsAndBufferViews  ;

	int deformerCount =pLine->GetDeformerCount (FbxDeformer::eSkin) ;
	_ASSERTE (deformerCount <= 1) ; // "Unexpected number of skin greater than 1") ;
	int clusterCount =0 ;
	// It is expected for deformerCount to be equal to 1
	for ( int i =0 ; i < deformerCount ; i++ )
		clusterCount +=FbxCast<FbxSkin> (pLine->GetDeformer (i, FbxDeformer::eSkin))->GetClusterCount () ;
	FbxArray<FbxVector4> positions =GetVertexPositions (pLine, true, (clusterCount == 0)) ;

//	std::vector<FbxDouble3> out_positions =vbo.getPositions () ;

	Json::Value vertex =WriteArrayWithMinMax<FbxDouble3, float> (out_positions, pLine->GetNode (), ("_Positions")) ;
	MergeJsonObjects (localAccessorsAndBufferViews, vertex);
	primitive [("attributes")] [("POSITION")] =(GetJsonFirstKey (vertex [("accessors")])) ;

	// Get line face indices
	Json::Value polygons =WriteArray<unsigned short> (out_indices, 1, pLine->GetNode (), ("_Polygons")) ;
	primitive [("indices")] =(GetJsonFirstKey (polygons [("accessors")])) ;

	MergeJsonObjects (accessorsAndBufferViews, polygons) ;
	MergeJsonObjects (accessorsAndBufferViews, localAccessorsAndBufferViews) ;

	// Get material
	FbxLayer *pLayer =gltfwriterVBO::getLayer (pLine, FbxLayerElement::eMaterial) ;
	if ( pLayer == nullptr ) {
		//std::cout << ("Info: (") << utility::conversions::to_string_t (pNode->GetTypeName ())
		//	<< (") ") << utility::conversions::to_string_t (pNode->GetName ())
		//	<< (" no material on Layer: ")
		//	<< iLayer
		//	<< std::endl ;
		// Create default material
		Json::Value ret =WriteDefaultMaterial (pNode) ;
		if ( ret.is_string () ) {
			primitive [("material")] =ret ;
		} else {
			primitive [("material")]=(GetJsonFirstKey (ret [("materials")])) ;

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
			std::cout << ("Warning: (") << utility::conversions::to_string_t (pNode->GetTypeName ())
				<< (") ") << utility::conversions::to_string_t (pNode->GetName ())
				<< (" got more than one material. glTF supports one material per primitive (FBX Layer).")
				<< std::endl ;
		}
		// TODO: need to be revisited when glTF will support more than one material per layer/primitive
		materialCount =materialCount == 0 ? 0 : 1 ;
		for ( int i =0 ; i < materialCount ; i++ ) {
			Json::Value ret =WriteMaterial (pNode, pNode->GetMaterial (i)) ;
			if ( ret.is_string () ) {
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
	linePrimitives [linePrimitives.size ()] =primitive ;
	lineDef [("primitives")] =linePrimitives ;

	Json::Value lib =Json::Value::object ({ { nodeId (pNode, true, true), lineDef } }) ;

	Json::Value node =WriteNode (pNode) ;
	//if ( pLine->GetShapeCount () )
	//	WriteControllerShape (pLine) ; // Create a controller

	Json::Value ret =Json::Value::object ({ { ("meshes"), lib }, { ("nodes"), node } }) ;
	MergeJsonObjects (ret, accessorsAndBufferViews) ;
	MergeJsonObjects (ret, images) ;
	MergeJsonObjects (ret, materials) ;
	MergeJsonObjects (ret, techniques) ;
	MergeJsonObjects (ret, programs) ;
	MergeJsonObjects (ret, samplers) ;
	MergeJsonObjects (ret, textures) ;
	return (ret) ;
#else
	return (Json::Value::null ()) ;
#endif
}
#endif

}
