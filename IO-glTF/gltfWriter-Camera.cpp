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

#define HFOV2VFOV(h, ar) (2.0 * atan ((ar) * tan ((h * FBXSDK_PI_DIV_180) * 0.5)) * FBXSDK_180_DIV_PI)

double gltfWriter::cameraYFOV (FbxCamera *pCamera) {
	FbxCamera::EApertureMode apertureMode =pCamera->GetApertureMode () ;
	// Camera aperture modes. The aperture mode determines which values drive the camera aperture. 
	// If the aperture mode is eHORIZONTAL_AND_VERTICAL, then the FOVX and FOVY is used. 
	// If the aperture mode is eHORIZONTAL or eVERTICAL, then the FOV is used.
	// if the aperture mode is eFOCAL_LENGTH, then the focal length is used.

	// Get the aperture ratio
	double filmHeight =pCamera->GetApertureHeight () ;
	double filmWidth =pCamera->GetApertureWidth () * pCamera->GetSqueezeRatio () ;
	double apertureRatio =filmHeight / filmWidth ;

	double focalAngle =static_cast<double>(pCamera->FieldOfView.Get ()) ;
	switch ( pCamera->GetApertureMode () ) {
		case FbxCamera::eHorizAndVert: // Fit the resolution gate within the film gate
		{
			focalAngle =pCamera->FieldOfViewY.Get () ;
			break ;
		}
		case FbxCamera::eHorizontal: // Fit the resolution gate horizontally within the film gate
		{
			double focalAngleX =focalAngle ;
			focalAngle =HFOV2VFOV (focalAngleX, apertureRatio) ;
			break ;
		}
		case FbxCamera::eVertical: // Fit the resolution gate vertically within the film gate
		{
			double focalAngleY =focalAngle ;
			break ;
		}
		case FbxCamera::eFocalLength: // Fit the resolution gate according to the focal length
		{
			//double focalLength =static_cast<double>(pCamera->FocalLength.Get ()) ;
			//pCamera->SetApertureMode (FbxCamera::eVertical) ; // ComputeFieldOfView() process the value based on the Aperture mode
			//double computedFOV =pCamera->ComputeFieldOfView (focalLength) ;
			//if ( focalAngle != computedFOV ) {
			//	pCamera->FieldOfView.Set (computedFOV) ;
			//	focalAngle =computedFOV ;
			//}
			double focalAngleX =pCamera->ComputeFieldOfView (static_cast<double>(pCamera->FocalLength.Get ())) ;    //get HFOV
			focalAngle =HFOV2VFOV (focalAngleX, apertureRatio) ;
			break ;
		}
		default:
			_ASSERTE( false ) ;
			break ;
	}
	return (focalAngle) ;
}

Json::Value gltfWriter::WriteCamera (FbxNode *pNode) {
	Json::Value camera;
	Json::Value cameraDef;
	camera [("name")] =nodeId (pNode, true) ;

	if ( isKnownId (pNode->GetNodeAttribute ()->GetUniqueID ()) ) {
		// The camera was already exported, create only the transform node
		Json::Value node =WriteNode (pNode) ;
		Json::Value ret;
		ret[("nodes")] = node ;
		return (ret) ;
	}

	FbxCamera *pCamera =pNode->GetCamera () ; //FbxCast<FbxCamera>(pNode->GetNodeAttribute ()) ;
	//FbxCamera::EAspectRatioMode aspectRatioMode =pCamera->GetAspectRatioMode () ;
	switch ( pCamera->ProjectionType.Get () ) {
		case FbxCamera::EProjectionType::ePerspective:
			camera [("type")] =(("perspective")) ;
			cameraDef [("aspectRatio")] =(pCamera->FilmAspectRatio.Get()) ; // (pCamera->AspectWidth / pCamera->AspectHeight) ;
			//cameraDef [("yfov")] =(DEG2RAD(pCamera->FieldOfView)) ;
			cameraDef [("yfov")] =(GLTF_ANGLE (cameraYFOV (pCamera))) ;
			cameraDef [("zfar")] =(pCamera->FarPlane.Get()) ;
			cameraDef [("znear")] =(pCamera->NearPlane.Get()) ;
			camera [("perspective")] =cameraDef ;
			break ;
		case FbxCamera::EProjectionType::eOrthogonal:
			camera [("type")] =(("orthographic")) ;
			//cameraDef [("xmag")] =(pCamera->_2DMagnifierX) ;
			//cameraDef [("ymag")] =(pCamera->_2DMagnifierY) ;
			cameraDef [("xmag")] =(pCamera->OrthoZoom.Get()) ;
			cameraDef [("ymag")] =(pCamera->OrthoZoom.Get()) ; // FBX Collada reader set OrthoZoom using xmag and ymag each time they appear
			cameraDef [("zfar")] =(pCamera->FarPlane.Get()) ;
			cameraDef [("znear")] =(pCamera->NearPlane.Get()) ;
			camera [("orthographic")] =cameraDef ;
			break ;
		default:
			_ASSERTE (false) ;
			break ;
	}
	Json::Value lib;
	lib[nodeId (pNode, true, true)] = camera ;
	Json::Value node =WriteNode (pNode) ;
	Json::Value ret;
	ret["cameras"] = lib;
	ret["nodes"] = node;
	return ret ;
}

}
