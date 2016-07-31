//
// Copyright (c) Autodesk, Inc. All rights reserved 
//
// C++ glTF FBX converter
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
#include "glTF.h"
#include "gltfPackage.h"
#include <cassert>

//-----------------------------------------------------------------------------
/*static*/ FbxAutoPtr<fbxSdkMgr> fbxSdkMgr::_singleton ;

fbxSdkMgr::fbxSdkMgr () : _sdkManager(FbxManager::Create ()) {
	assert( _sdkManager ) ;
	FbxIOSettings *pIOSettings =FbxIOSettings::Create (_sdkManager, IOSROOT) ;
	//pIOSettings->SetBoolProp (IMP_FBX_CONSTRAINT, true) ;
	//pIOSettings->SetBoolProp (IMP_FBX_CONSTRAINT_COUNT, true) ;
	//pIOSettings->SetBoolProp (IMP_FBX_MATERIAL, true) ;
	//pIOSettings->SetBoolProp (IMP_FBX_TEXTURE, true) ;
	//pIOSettings->SetBoolProp (IMP_FBX_LINK, true) ;
	//pIOSettings->SetBoolProp (IMP_FBX_SHAPE, true) ;
	//pIOSettings->SetBoolProp (IMP_FBX_GOBO, false) ;
	//pIOSettings->SetBoolProp (IMP_FBX_ANIMATION, true) ;
	//pIOSettings->SetBoolProp (IMP_FBX_GLOBAL_SETTINGS, true) ;
	pIOSettings->SetBoolProp (EXP_FBX_EMBEDDED, false) ;
	_sdkManager->SetIOSettings (pIOSettings) ;

	// Load plug-ins from the executable directory
	FbxString path =FbxGetApplicationDirectory () ;
#if defined(_WIN64) || defined(_WIN32)
	FbxString extension ("dll") ;
#elif defined(__APPLE__)
	FbxString extension ("dylib") ;
#else // __linux
	FbxString extension ("so") ;
#endif
	_sdkManager->LoadPluginsDirectory (path.Buffer ()/*, extension.Buffer ()*/) ;
	_sdkManager->FillIOSettingsForReadersRegistered (*pIOSettings) ;
	_sdkManager->FillIOSettingsForWritersRegistered (*pIOSettings) ;
}

fbxSdkMgr::~fbxSdkMgr () {
}

/*static*/ FbxAutoPtr<fbxSdkMgr> &fbxSdkMgr::Instance () {
	if ( !fbxSdkMgr::_singleton )
		fbxSdkMgr::_singleton.Reset (new fbxSdkMgr ()) ;
	return (fbxSdkMgr::_singleton) ;
}

FbxSharedDestroyPtr<FbxManager> &fbxSdkMgr::fbxMgr () {
	assert( _sdkManager ) ;
	return (_sdkManager) ;
}

//-----------------------------------------------------------------------------
gltfPackage::gltfPackage () : _scene(nullptr), _ioSettings ({ ("") }) {
}

gltfPackage::~gltfPackage () {
}

void gltfPackage::ioSettings (
	const char *name /*=nullptr*/,
	bool angleInDegree /*=false*/,
	bool invertTransparency /*=false*/,
	bool defaultLighting /*=false*/,
	bool copyMedia /*=false*/,
	bool embedMedia /*=false*/
) {
	FbxIOSettings *pIOSettings =fbxSdkMgr::Instance ()->fbxMgr ()->GetIOSettings () ;
	_ioSettings._name =name == nullptr ? ("") : name ;
	pIOSettings->SetBoolProp (IOSN_FBX_GLTF_ANGLEINDEGREE, angleInDegree) ;
	pIOSettings->SetBoolProp (IOSN_FBX_GLTF_INVERTTRANSPARENCY, invertTransparency) ;
	pIOSettings->SetBoolProp (IOSN_FBX_GLTF_DEFAULTLIGHTING, defaultLighting) ;
	pIOSettings->SetBoolProp (IOSN_FBX_GLTF_COPYMEDIA, copyMedia) ;
	pIOSettings->SetBoolProp (IOSN_FBX_GLTF_EMBEDMEDIA, embedMedia) ;
	//fbxSdkMgr::Instance ()->fbxMgr ()->SetIOSettings (pIOSettings) ;
}

bool gltfPackage::load (const std::string &fn) {
	assert( !_scene ) ;
	auto pMgr =fbxSdkMgr::Instance ()->fbxMgr () ;
	_scene.Reset (FbxScene::Create (pMgr, (gltfPackage::filename (fn)).c_str ())) ;
	assert( !!_scene ) ;
	bool bRet =LoadScene (fn) ;
	return (bRet) ;
}

bool gltfPackage::import (const std::string &fn) {
	assert( !_scene ) ;
	auto pMgr =fbxSdkMgr::Instance ()->fbxMgr () ;
	_scene.Reset (FbxScene::Create (pMgr, (gltfPackage::filename (fn)).c_str ())) ;
	assert( !!_scene ) ;
	bool bRet =LoadScene (fn) ;
	return (bRet) ;
}

bool gltfPackage::save (const std::string &outdir) {
	assert( !!_scene ) ;
	bool bRet =WriteScene (outdir) ;
	return (bRet) ;
}

/*static*/ std::string gltfPackage::filename (const std::string &path) {
	std::string filename ;
	size_t pos =path.find_last_of (("/\\")) ;
	if ( pos != std::string::npos ) {
		size_t pos2 =path.find (("."), pos) ;
		if ( pos2 != std::string::npos )
			filename.assign (path.begin () + pos + 1, path.begin () + pos2) ;
		else
			filename.assign (path.begin () + pos + 1, path.end ()) ;
	} else {
		filename =path ;
	}
	return (filename) ;
}

/*static*/ std::string gltfPackage::pathname (const std::string &filename) {
	std::string path ;
	size_t pos =filename.find_last_of (("/\\")) ;
	if ( pos != std::string::npos )
		path.assign (filename.begin (), filename.begin () + pos + 1) ;
	else
		path =("") ;
	return (path) ;
}

bool gltfPackage::LoadScene (const std::string &fn) {
	auto pMgr =fbxSdkMgr::Instance ()->fbxMgr () ;
	FbxAutoDestroyPtr<FbxImporter> pImporter (FbxImporter::Create (pMgr, "")) ;

	if ( !pImporter->Initialize ((fn).c_str (), -1, pMgr->GetIOSettings ()) )
		return (false) ;
	if ( pImporter->IsFBX () ) {
		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		// Set the import states. By default, the import states are always set to true.
	}

	bool bStatus =pImporter->Import (_scene) ;
	if ( _ioSettings._name.length () )
		_scene->SetName ((_ioSettings._name).c_str ()) ;
	else if ( _scene->GetName () == FbxString ("") )
		//_scene->SetName ("untitled") ;
		_scene->SetName ((gltfPackage::filename (fn)).c_str ()) ;

	//if ( bStatus == false && pImporter->GetStatus ().GetCode () == FbxStatus::ePasswordError ) {
	//}

	// Get current UpAxis of the FBX file.
	// This have to be done before ConvertiAxisSystem(), cause the function will always change SceneAxisSystem to Y-up.
	// First, however, if we have the ForcedFileAxis activated, we need to overwrite the global settings.
	//switch ( IOS_REF.GetEnumProp (IMP_FILE_UP_AXIS, FbxMayaUtility::eUPAXIS_AUTO) ) {
	//	default:
	//	case FbxMayaUtility::eUPAXIS_AUTO:
	//		break ;
	//	case FbxMayaUtility::eUPAXIS_Y:
	//		_scene->GetGlobalSettings ().SetAxisSystem (FbxAxisSystem::MayaYUp) ;
	//		break ;
	//	case FbxMayaUtility::eUPAXIS_Z:
	//		_scene->GetGlobalSettings ().SetAxisSystem (FbxAxisSystem::MayaZUp) ;
	//	break ;
	//}
	//FbxAxisSystem sceneAxisSystem =_scene->GetGlobalSettings ().GetAxisSystem () ;
	//int lSign =0 ;
	//FbxAxisSystem::EUpVector upVectorFromFile =sceneAxisSystem.GetUpVector (lSign) ;

	FbxAxisSystem::MayaYUp.ConvertScene (_scene) ; // We want the Y up axis for glTF

	FbxSystemUnit sceneSystemUnit =_scene->GetGlobalSettings ().GetSystemUnit () ; // We want meter as default unit for gltTF
	if ( sceneSystemUnit != FbxSystemUnit::m ) {
		//const FbxSystemUnit::ConversionOptions conversionOptions ={
		//	false, // mConvertRrsNodes
		//	true, // mConvertAllLimits
		//	true, // mConvertClusters
		//	false, // mConvertLightIntensity
		//	true, // mConvertPhotometricLProperties
		//	false  // mConvertCameraClipPlanes
		//} ;
		//FbxSystemUnit::m.ConvertScene (_scene, conversionOptions) ;
		FbxSystemUnit::m.ConvertScene (_scene) ;
	}

	FbxGeometryConverter converter (fbxSdkMgr::Instance ()->fbxMgr ()) ;
	converter.Triangulate (_scene, true) ; // glTF supports triangles only
	converter.SplitMeshesPerMaterial (_scene, true) ; // Split meshes per material, so we only have one material per mesh (VBO support)
	
	// Set the current peripheral to be the NULL so FBX geometries that have been imported can be flushed
    _scene->SetPeripheral (NULL_PERIPHERAL) ;

	//int nb =_scene->GetSrcObjectCount<FbxNodeAttribute> () ;
	//int nbTot =5 * nb ;
	//int nbRest =4 * nb ;
	//int nbSteps =nbRest / 8 ;

	//FbxArray<FbxNode *> pBadMeshes =RemoveBadPolygonsFromMeshes (_scene) ;

	return (true) ;
}

//FbxArray<FbxNode *> RemoveBadPolygonsFromMeshes (FbxScene *pScene) {
//	FbxArray<FbxNode *> pAffectedNodes ;
//	FbxNode *pNode =pScene->GetRootNode () ;
//	RemoveBadPolygonsFromMeshesRecursive (pNode, pAffectedNodes) ;
//	return (pAffectedNodes) ;
//}
//
//FbxArray<FbxNode *> RemoveBadPolygonsFromMeshesRecursive (FbxNode *pNode, FbxArray<FbxNode *> &pAffectedNodes) {
//	FbxMesh *pMesh =pNode->GetMesh () ;
//	if ( pMesh ) {
//		if ( pMesh->RemoveBadPolygons () > 0 )
//			pAffectedNodes.Add (pNode) ;
//	}
//	for ( int nChildIndex =0 ; nChildIndex < pNode->GetChildCount () ; nChildIndex++ ) {
//		FbxNode *pChild =pNode->GetChild (nChildIndex) ;
//		RemoveBadPolygonsFromMeshesRecursive (pChild, pAffectedNodes) ;
//	}
//	return (pAffectedNodes) ;
//}

bool gltfPackage::WriteScene (const std::string &outdir) {
	auto pMgr =fbxSdkMgr::Instance ()->fbxMgr () ;
	int iFormat =pMgr->GetIOPluginRegistry ()->FindWriterIDByExtension ("gltf") ;
	if ( iFormat == -1 )
		return (false) ;
	FbxAutoDestroyPtr<FbxExporter> pExporter (FbxExporter::Create (pMgr, "")) ;
	std::string newFn =outdir + (_scene->GetName ()) + (".gltf") ;
	
	bool bRet =pExporter->Initialize ((newFn).c_str (), iFormat, pMgr->GetIOSettings ()) ;
	assert( bRet ) ;
	if ( !bRet )
		return (false) ;

	// The next line will call the exporter
	bRet =pExporter->Export (_scene) ;

	return (bRet) ;
}
