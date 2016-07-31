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
#ifdef _DEBUG
#include "JsonPrettify.h"
#endif
#include <array>
#include <stdlib.h>
#define _DEBUG_VERBOSE 1

namespace _IOglTF_NS_ {

//-----------------------------------------------------------------------------
Json::Value &MergeJsonObjects (Json::Value &a, const Json::Value &b) {
	if ( b.isNull () || !b.isObject () )
		return (a) ;
	auto memberNames = b.getMemberNames();
	for ( const auto &name : memberNames ) {
		if ( !a.isMember (name) ) {
			a [name] =b[name] ;
		} else {
			MergeJsonObjects (a [name], b[(name)]) ;
		}
	}
	return (a) ;
}

std::string GetJsonObjectKeyAt (Json::Value &a, int i) {
	const auto &b =a.getMemberNames() ;
	return (b.front()) ;
}

//-----------------------------------------------------------------------------
/*static*/ gltfWriter::ExporterRoutes gltfWriter::_routes ={
	{ FbxNodeAttribute::eCamera, &gltfWriter::WriteCamera }, // Camera
	{ FbxNodeAttribute::eLight, &gltfWriter::WriteLight }, // Light
	{ FbxNodeAttribute::eMesh, &gltfWriter::WriteMesh }, // Mesh
//	{ FbxNodeAttribute::eLine, &gltfWriter::WriteLine }, // Mesh
	{ FbxNodeAttribute::eNull, &gltfWriter::WriteNull } // Null

	// Marker // Not implemented in COLLADA / glTF.
	// CameraSwitcher // Not implemented in COLLADA / glTF.
	//FbxNodeAttribute::eNurbs -> converted to Mesh // Not implemented in COLLADA / glTF.
	//FbxNodeAttribute::ePatch -> converted to Mesh // Not implemented in COLLADA / glTF.
	//FbxNodeAttribute::eSkeleton
	//FbxNodeAttribute::eMarker
} ;

//-----------------------------------------------------------------------------
gltfWriter::gltfWriter (FbxManager &pManager, int id)
	: FbxWriter (pManager, id, FbxStatusGlobal::GetRef ()),
	  _fileName(), _writeDefaults(true)
{ 
	_samplingPeriod =1. / 30. ;
}

gltfWriter::~gltfWriter () {
	FileClose () ;
}

bool gltfWriter::FileCreate (char *pFileName) {
	FbxString fileName =FbxPathUtils::Clean (pFileName) ;
	_fileName = (fileName.Buffer ()) ;

	//std::string path =_GLTF_NAMESPACE_::GetModulePath () ;
	std::string path = ((const char *)FbxGetApplicationDirectory ()) ;
	std::ifstream input (path + ("glTF-1-0.json"), std::ios::in) ;
	
	Json::Features features;
	features.allowComments_ = false;
	features.strictRoot_ = true;
	Json::Reader reader( features );
	try {
		reader.parse( input, _json, false );
	}
	catch ( const std::exception &ex ) {
		std::cout << ex.what() << std::endl;
	}
	input.close () ;

	if ( !FbxPathUtils::Create (FbxPathUtils::GetFolderName (fileName)) )
		return (GetStatus ().SetCode (FbxStatus::eFailure, "Cannot create folder!"), false) ;
	//_gltf =utility::ofstream_t (_fileName, std::ios::out) ;
	_gltf.open (_fileName, std::ios::out) ;

	return (IsFileOpen ()) ;
}

bool gltfWriter::FileClose () {
	PrepareForSerialization () ;
#ifdef _DEBUG
	Json::StyledWriter writer;
	writer.write( _json );
#else
	Json::FastWriter writer;
	auto temp = writer.write( _json );
	_gltf.write( temp.c_str(), temp.length() ) ;
#endif
	_gltf.close () ;

	// If media saved in file, gltfWriter::PostprocessScene / gltfWriter::WriteBuffer should have embed the data already
	if ( !GetIOSettings ()->GetBoolProp (IOSN_FBX_GLTF_EMBEDMEDIA, false) ) {
		FbxString fileName ( (_fileName).c_str ()) ;
#if defined(_WIN32) || defined(_WIN64)
		fileName =FbxPathUtils::GetFolderName (fileName) + "\\" + FbxPathUtils::GetFileName (fileName, false) + ".bin" ;
#else
		fileName =FbxPathUtils::GetFolderName (fileName) + "/" + FbxPathUtils::GetFileName (fileName, false) + ".bin" ;
#endif
		std::ofstream binFile (fileName, std::ios::out | std::ofstream::binary) ;
		//_bin.seekg (0, std::ios_base::beg) ;
		binFile.write ((const char *)_bin.rdbuf (), _bin.vec ().size ()) ;
		binFile.close () ;
	}
	return (true) ;
}

bool gltfWriter::IsFileOpen () {
	return (_gltf.is_open ()) ;
}

void gltfWriter::GetWriteOptions () {
}

bool gltfWriter::Write (FbxDocument *pDocument) {
	//_computeDeformations =IOS_REF.GetBoolProp(EXP_GLTF_DEFORMATION, true);
	//_singleMatrix =IOS_REF.GetBoolProp (EXP_GLTF_SINGLEMATRIX, true) ;
	//_samplingPeriod =1. / IOS_REF.GetDoubleProp (EXP_GLTF_FRAME_RATE, 30.0) ;

	FbxScene *pScene =FbxCast<FbxScene>(pDocument) ;
	if ( !pScene )
		return (GetStatus ().SetCode (FbxStatus::eFailure, "Document not supported!"), false) ;

	if ( !PreprocessScene (*pScene) )
		return (false) ;

	FbxDocumentInfo *pSceneInfo =pScene->GetSceneInfo () ;
	if ( !WriteAsset (pSceneInfo) )
		return (false) ;
	if ( !WriteScene (pScene) )
		return (false) ;
	//if ( !ExportAnimation (pScene->GetRootNode ()) )
	//	return (false) ;
	//if ( !ExportLibraries () )
	//	return (false) ;

	if ( !WriteShaders () )
		return (false) ;

	if ( !WriteBuffer () ) // Should be last !!!
		return (false) ;

	if ( !PostprocessScene (*pScene) )
		return (false) ;

	return (true) ;
}

bool gltfWriter::IsGeometryNode (FbxNode *pNode) {
	FbxNodeAttribute *pNodeAttribute =pNode->GetNodeAttribute () ;
	if ( pNodeAttribute == nullptr )
		return (false) ;
	if ( pNodeAttribute->GetAttributeType () == FbxNodeAttribute::eMesh )
		return (true) ;
	else if ( pNodeAttribute->GetAttributeType () == FbxNodeAttribute::eNurbs )
		return (true) ;
	else if ( pNodeAttribute->GetAttributeType () == FbxNodeAttribute::ePatch )
		return (true) ;
	else if ( pNodeAttribute->GetAttributeType () == FbxNodeAttribute::eLine )
		return (true) ;
	return (false) ;
}

bool gltfWriter::PreprocessScene (FbxScene &scene) {
	//FbxSceneRenamer renamer (&pScene) ; // Rename ALL the nodes from FBX to Collada since GLTF is mainly based on Collada
	//renamer.RenameFor (FbxSceneRenamer::eFBX_TO_DAE) ;

	FbxNode *pRootNode =scene.GetRootNode () ;
	PreprocessNodeRecursive (pRootNode) ;

	/*if ( mSingleMatrix ) {
		lRootNode->ResetPivotSetAndConvertAnimation (1. / mSamplingPeriod.GetSecondDouble ());
	}

	// convert to the old material system for now
	FbxMaterialConverter lConv (*pScene.GetFbxManager ());
	lConv.AssignTexturesToLayerElements (pScene);

	FbxString lActiveAnimStackName = pScene.ActiveAnimStackName;
	mAnimStack = pScene.FindMember<FbxAnimStack> (lActiveAnimStackName.Buffer ());
	if ( !mAnimStack ) {
		// the application has an invalid ActiveAnimStackName, we fallback by using the 
		// first animStack.
		mAnimStack = pScene.GetMember<FbxAnimStack> ();
	}

	// If none of the above method succeed in returning an anim stack, we create 
	// a dummy one to avoid crashes. The correctness of the exported values cannot 
	// be guaranteed in this case
	if ( mAnimStack == nullptr ) {
		mAnimStack = FbxAnimStack::Create (&pScene, "dummy");
		mAnimLayer = FbxAnimLayer::Create (&pScene, "dummyL");
		mAnimStack->AddMember (mAnimLayer);
	}

	mAnimLayer = mAnimStack->GetMember<FbxAnimLayer> ();

	// Make sure the scene has a name. If it does not, we try to use the filename
	// and, as last resort a dummy name.
	if ( strlen (pScene.GetName ()) == 0 ) {
		FbxDocumentInfo *lSceneInfo = pScene.GetSceneInfo ();
		FbxString lFilename ("dummy");
		if ( lSceneInfo ) {
			lFilename = lSceneInfo->Original_FileName.Get ();
			if ( lFilename.GetLen () > 0 ) {
				FbxString lFn = FbxPathUtils::GetFileName (lFilename.Buffer (), false);
				if ( lFn.GetLen () > 0 )
					lFilename = lFn;
			}
		}
		pScene.SetName (lFilename.Buffer ());
	}*/
	return (true) ;
}

void gltfWriter::PreprocessNodeRecursive (FbxNode *pNode) {
	FbxVector4 postR ;
	FbxNodeAttribute const *nodeAttribute =pNode->GetNodeAttribute () ;
	// Set PivotState to active to ensure ConvertPivotAnimationRecursive() execute correctly. 
	pNode->SetPivotState (FbxNode::eSourcePivot, FbxNode::ePivotActive) ;
	pNode->SetPivotState (FbxNode::eDestinationPivot, FbxNode::ePivotActive) ;
	// ~
	if ( nodeAttribute ) {
		//// Special transformation conversion cases. If spotlight or directional light, 
		//// rotate node so spotlight is directed at the X axis (was Z axis).
		//if ( nodeAttribute->GetAttributeType () == FbxNodeAttribute::eLight ) {
		//	FbxLight *pLight =FbxCast<FbxLight>(pNode->GetNodeAttribute ()) ;
		//	if (   pLight->LightType.Get () == FbxLight::eSpot
		//		|| pLight->LightType.Get () == FbxLight::eDirectional
		//	) {
		//		postR =pNode->GetPostRotation (FbxNode::eSourcePivot) ;
		//		postR [0] +=90 ;
		//		pNode->SetPostRotation (FbxNode::eSourcePivot, postR) ;
		//	}
		//}
		//// If camera, rotate node so camera is directed at the -Z axis (was X axis).
		//else if ( nodeAttribute->GetAttributeType () == FbxNodeAttribute::eCamera ) {
		//	postR =pNode->GetPostRotation (FbxNode::eSourcePivot) ;
		//	postR [1] +=90 ;
		//	pNode->SetPostRotation (FbxNode::eSourcePivot, postR) ;
		//}
#ifdef _TRIANGULATE_
		FbxNodeAttribute::EType attributeType =nodeAttribute->GetAttributeType () ;
		if ( attributeType == FbxNodeAttribute::eNurbs || attributeType == FbxNodeAttribute::ePatch ) {
			FbxGeometryConverter geometryConverter (&mManager) ;
			//geometryConverter.TriangulateInPlace (pNode) ;
			FbxMesh *pMesh =pNode->GetMesh () ;
			pMesh =FbxCast<FbxMesh>(geometryConverter.Triangulate (pMesh, true)) ;
		}
#endif
	}
	for ( int i =0 ; i < pNode->GetChildCount () ; ++i )
		PreprocessNodeRecursive (pNode->GetChild (i)) ;
}

//FbxNodeAttribute::EType attributeType =nodeAttribute->GetAttributeType () ;
//FbxMesh *pMesh =pNode->GetMesh () ;
//if ( attribute == FbxNodeAttribute::eNurbs || attribute == FbxNodeAttribute::ePatch ) {
//	// FbxGeometryConverter lConverter(pSdkManager);
//	//      lConverter.TriangulateInPlace(pNode);
//
//	FbxGeometryConverter lConverter (pSdkManager);
//	pMesh =FbxCast<FbxMesh> (lConverter.Triangulate (pMesh, true));

bool gltfWriter::PostprocessScene (FbxScene &scene) {
	/*Json::Value val =WriteAmbientLight (pScene) ;
	for ( const auto &iter : val.as_object () )
		_json [("lights")] [iter.first] =iter.second ;
	*/
	//if ( GetIOSettings ()->GetBoolProp (IOSN_FBX_GLTF_COPYMEDIA, false) ) {
	//#if defined(_WIN32) || defined(_WIN64)
	//	FbxString path =FbxPathUtils::GetFolderName (utility::conversions::to_utf8string (_fileName).c_str ()) + "\\" ;
	//#else
	//	FbxString path =FbxPathUtils::GetFolderName (utility::conversions::to_utf8string (_fileName).c_str ()) + "/" ;
	//#endif
	//	for ( const auto &iter : _json [("images")].as_object () ) {
	//	}
	//		
	//}

	return (true) ;
}

// Create our own writer - And your writer will get a pPluginID and pSubID. 
/*static*/ FbxWriter *gltfWriter::Create_gltfWriter (FbxManager &manager, FbxExporter &exporter, int subID, int pluginID) {
	FbxWriter *writer =FbxNew<gltfWriter> (manager, pluginID) ; // Use FbxNew instead of new, since FBX will take charge its deletion
	writer->SetIOSettings (exporter.GetIOSettings ()) ;
	return (writer) ;
}

// Get extension, description or version info about MyOwnWriter
/*static*/ void *gltfWriter::gltfFormatInfo (FbxWriter::EInfoRequest request, int id) {
	return (_IOglTF_NS_::_gltfFormatInfo (request, id)) ;
}

/*static*/ void gltfWriter::FillIOSettings (FbxIOSettings &pIOS) {
	// Here you can write your own FbxIOSettings and parse them.
	// Example at: http://help.autodesk.com/view/FBX/2015/ENU/?guid=__files_GUID_75CD0DC4_05C8_4497_AC6E_EA11406EAE26_htm

	FbxProperty exportGroup =pIOS.GetProperty (IOSN_EXPORT) ;
	if ( !exportGroup.IsValid () )
		return ;

	FbxProperty pluginGroup =pIOS.AddPropertyGroup (exportGroup, GLTF_EXTENTIONS, FbxStringDT, "glTF Export Options") ;
	if ( pluginGroup.IsValid () ) {
		bool defaultValue =false ;
		FbxProperty myOption =pIOS.AddProperty (pluginGroup, GLTF_INVERTTRANSPARENCY, FbxBoolDT, "Invert Transparency [bool]", &defaultValue, true) ;
		myOption =pIOS.AddProperty (pluginGroup, GLTF_DEFAULTLIGHTING, FbxBoolDT, "Enable Default Lighting [bool]", &defaultValue, true) ;
		myOption =pIOS.AddProperty (pluginGroup, GLTF_COPYMEDIA, FbxBoolDT, "Copy Media [bool]", &defaultValue, true) ;
		//myOption =pIOS.AddProperty (pluginGroup, GLTF_EMBEDMEDIA, FbxBoolDT, "Embed all Resources as Data URIs [bool]", &defaultValue, eFbxBool) ;
	}
}

//-----------------------------------------------------------------------------
void gltfWriter::PrepareForSerialization () {
	//for ( auto iter =_json. ; iter != _json.end () ; ++iter ) {
	//	auto k =iter->first ;
	//	auto v =iter->second ;

	//	auto key =k.asString () ;
	//	auto value =v.to_string () ;

	//	wcout << key << L" : " << value << " (" << JsonValueTypeToString (v.type ()) << ")" << endl;
	//}
}

//-----------------------------------------------------------------------------
bool gltfWriter::recordId (FbxUInt64 uniqid, std::string id) {
	if ( !isKnownId (uniqid) ) {
		_IDs [uniqid] =id ;
		return (false) ;
	}
	return (true) ;
}

bool gltfWriter::isKnownId (FbxUInt64 uniqid) {
	return (_IDs.find (uniqid) != _IDs.end ()) ;
}

bool gltfWriter::isKnownId (std::string id) {
	//return (std::find (_registeredIDs.begin (), _registeredIDs.end (), id) != _registeredIDs.end ()) ;
	auto findResult =std::find_if (std::begin (_IDs), std::end (_IDs), [&] (const std::pair<FbxUInt64, std::string> &pair) {
		return (pair.second == id) ;
	}) ;
	return (findResult != std::end (_IDs)) ;
}

std::string gltfWriter::nodeId (FbxNode *pNode, bool bNodeAttribute /*=false*/, bool bRecord /*=false*/) {
	FbxUInt64 id =bNodeAttribute && pNode->GetNodeAttribute () != nullptr ? pNode->GetNodeAttribute ()->GetUniqueID () : pNode->GetUniqueID () ;
	if ( isKnownId (id) )
		return (_IDs [id]) ;
	std::string name = bNodeAttribute && pNode->GetNodeAttribute () != nullptr ? pNode->GetNodeAttribute ()->GetName () : pNode->GetName ();
	if ( name == ("") )
		name =(pNode->GetTypeName ()) ;
	//if ( isKnownId (name) ) // Comment if it should be consistent?
	name +=("_") + std::to_string((int)id) ;
	if ( bRecord )
		recordId (id, name) ;
	return (name) ;
}

std::string gltfWriter::registerName (std::string name) {
	if ( !isNameRegistered (name) )
		_registeredNames.push_back (name) ;
	return (name) ;
}

bool gltfWriter::isNameRegistered (std::string id) {
	return (std::find (_registeredNames.begin (), _registeredNames.end (), id) != _registeredNames.end ()) ;
}

std::string gltfWriter::createUniqueName (std::string type, FbxUInt64 id) {
	for ( ;; id++ ) {
		std::string buffer =utility::conversions::to_string_t ((int)id) ;
		std::string uid (type) ;
		uid +=("_") + buffer ;

		if ( !isNameRegistered (uid) )
			return (registerName (uid)) ;
	}
	_ASSERTE (false) ;
	return (("error")) ;
}

//-----------------------------------------------------------------------------
Json::Value gltfWriter::WriteSceneNodeRecursive (FbxNode *pNode, FbxPose *pPose /*=nullptr*/, bool bRoot /*=false*/) {
	//if ( !WriteSceneNode (pNode, pPose) )
	//	//return (GetStatus ().SetCode (FbxStatus::eFailure, "Could not export node " + pNode->GetName () + "!"), false) ;
	//	return (false) ;
#ifdef _DEBUG_VERBOSE
	std::string name =(pNode->GetNameOnly ().Buffer ()) ;
	//_path.push_back (name) ;
#endif

	Json::Value node =WriteSceneNode (pNode, pPose) ;
	std::string nodeName, meshName ;
	if ( !node.isNull () ) {
		nodeName =GetJsonFirstKey (node [("nodes")]) ;
		static Json::Value sIdentity4Mat( Json::arrayValue ) ;
		static bool sIdentity4MatInitialized = false;
		if( ! sIdentity4MatInitialized ) {
			std::array<double, 16> identity{ 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1. };
			auto i{0};
			for( auto val : identity )
				sIdentity4Mat[i++] = val;
			sIdentity4MatInitialized = true;
		}
		auto &mat = node [("nodes")] [nodeName] [("matrix")];
		if (   !bRoot && node [("meshes")].size ()
			//&& node [("nodes")] [nodeName] [("children")].size () == 0
			&& pNode->GetChildCount () == 0
			&& sIdentity4Mat == mat
		) {
		//	meshName =GetJsonFirstKey (node [("meshes")]) ;
			node [("nodes")]  ;
		}
		MergeJsonObjects (_json, node) ;
		if ( bRoot ) {
			std::string szName ((pNode->GetScene ()->GetName ())) ;
			uint32_t pos =_json [("scenes")] [szName] [("nodes")].size () ;
			_json [("scenes")] [szName] [("nodes")] [pos] =(nodeName) ;
		}
	}
#ifdef _DEBUG_VERBOSE
	//else {
	//	std::cout << (" !!! Null returned ( ") << name << (" ) - ") ;
	//	for ( std::string st : _path )
	//		std::cout << (" << ") << st  ;
	//	std::cout << std::endl ;
	//	return (Json::Value::null ()) ;
	//}
#endif

	FbxNodeAttribute::EType enodeType =nodeType (pNode) ;
	for ( int i =0; i < pNode->GetChildCount () ; i++ ) {
		Json::Value child =WriteSceneNodeRecursive (pNode->GetChild (i), pPose, bRoot && enodeType == FbxNodeAttribute::eUnknown) ;
		if ( !child.isNull () && !node.isNull () ) {
			std::string key (("nodes")), key2 (("children")) ;
			if (   !child.isNull () && child [("nodes")].size () == 0
				&& child [("meshes")].size () != 0
			)
				key =key2 =("meshes") ;
			std::string childName (GetJsonFirstKey (child [key])) ;
			uint32_t pos =_json [("nodes")] [nodeName] [key2].size () ;

			//if ( std::find (
			//		_json [("nodes")] [nodeName] [key2].as_array ().begin (),
			//		_json [("nodes")] [nodeName] [key2].as_array ().end (),
			//		(childName)
			//	) != _json [("nodes")] [nodeName] [key2].as_array ().end ()
			//)
			//	std::cout << (" yes ") << std::endl ;
			_json [("nodes")] [nodeName] [key2] [pos] =(childName) ;


		}
	}

#ifdef _DEBUG_VERBOSE
	//_path.pop_back () ;
#endif
	return (node) ;
}

Json::Value gltfWriter::WriteSceneNode (FbxNode *pNode, FbxPose *pPose) {

	//std::string id =nodeId (pNode) ; 
	//if ( _json [("nodes")].isMember (id) ) {
	//	// This node was already exported, return only its name
	//	return (Json::Value::null ()) ;
	//}

	FbxNodeAttribute::EType enodeType =nodeType (pNode) ;
	if ( _routes.find (enodeType) == _routes.end () ) {
		//if (   FbxString (pNode->GetName ()) != FbxString ("RootNode")
		//	&& pNodeAttribute != nullptr
		//)
			std::cout << ("Warning: (") << (pNode->GetTypeName ())
				  << (" - ") << (int)enodeType
				  << (") ") <<  (pNode->GetName ())
				  << (" not exported!")
				  << std::endl ;
		return (Json::Value( Json::nullValue )) ;
	}

	//FbxProperty cid =pNode->FindProperty ("COLLADA_ID") ;
	//FbxString sid =cid.IsValid () ? cid.Get<FbxString> () : FbxString ("") ;
	//std::string ff =utility::conversions::to_string_t (sid.Buffer ()) ;

	//std::string id =nodeId (pNode) ;
	//if ( isIdRegistered (id) ) {
	////	std::cout << id << (" ( ") << ff << (" ) x") << std::endl ;
	//	return (Json::Value::null ()) ;
	//}
	////std::cout << id << (" ( ") << ff << (" ) ") << std::endl ;

#ifdef _DEBUG_VERBOSE
	//for ( std::string st : _path )
	//	std::cout << (" << ") << st  ;
	//std::cout << std::endl ;
#endif
	ExporterRouteFct fct =(*(_routes.find (enodeType))).second ;
	Json::Value val =(this->*fct) (pNode) ;
//	Json::Value val =WriteNull (pNode) ;

	//for ( auto iter =val.as_object ().cbegin () ; iter != val.as_object ().cend () ; ++iter ) {
	//	//const std::string &str =iter->first ;
	//	const Json::Value &v =iter->second ;
	//	for ( auto iter2 =v.as_object ().cbegin () ; iter2 != v.as_object ().cend () ; ++iter2 ) {
	//		const std::string &id =iter2->first ;
	//		//std::cout << id << std::endl ;
	//		registerId (id) ;
	//	}
	//}

	return (val) ;
}

FbxNodeAttribute::EType gltfWriter::nodeType (FbxNode *pNode) {
	FbxNodeAttribute *pNodeAttribute =pNode->GetNodeAttribute () ;
	FbxNodeAttribute::EType nodeType =pNodeAttribute ? pNodeAttribute->GetAttributeType () : FbxNodeAttribute::eUnknown ;
	// Non paired node
	if ( pNodeAttribute == nullptr ) {
		std::string szType = (pNode->GetTypeName ()) ;
		if ( szType == ("Null") )
			nodeType =FbxNodeAttribute::eNull ;
	}
	return (nodeType) ;
}

Json::Value gltfWriter::GetTransform (FbxNode *pNode) {
	// Export the node's default transforms

	// If the mesh is a skin binded to a skeleton, the bind pose will include its transformations.
	// In that case, do not export the transforms twice.
	//if (   pNode->GetNodeAttribute ()
	//	&& pNode->GetNodeAttribute ()->GetAttributeType () == FbxNodeAttribute::eMesh
	//) {
	//	int deformerCount =FbxCast<FbxMesh>(pNode->GetNodeAttribute ())->GetDeformerCount (FbxDeformer::eSkin) ;
	//	_ASSERTE( deformerCount <= 1 ) ; // "Unexpected number of skin greater than 1") ;
	//	int clusterCount =0 ;
	//	// It is expected for deformerCount to be equal to 1
	//	for ( int i =0 ; i < deformerCount ; ++i )
	//		clusterCount +=FbxCast<FbxSkin>(FbxCast<FbxMesh>(pNode->GetNodeAttribute ())->GetDeformer (i, FbxDeformer::eSkin))->GetClusterCount () ;
	//	if ( clusterCount )
	//		return (true) ;
	//}

	FbxAMatrix identity ;
	identity.SetIdentity () ;
	FbxAMatrix thisLocal ;
	// For Single Matrix situation, obtain transform matrix from eDestinationPivot, which include pivot offsets and pre/post rotations.
	FbxAMatrix &thisGlobal =const_cast<FbxNode *>(pNode)->EvaluateGlobalTransform (FBXSDK_TIME_ZERO, FbxNode::eDestinationPivot) ;
	const FbxNode *pParentNode =pNode->GetParent () ;
	if ( pParentNode ) {
		// For Single Matrix situation, obtain transform matrix from eDestinationPivot, which include pivot offsets and pre/post rotations.
		FbxAMatrix &parentGlobal =const_cast<FbxNode *>(pParentNode)->EvaluateGlobalTransform (FBXSDK_TIME_ZERO, FbxNode::eDestinationPivot) ;
		FbxAMatrix parentInverted =parentGlobal.Inverse () ;
		thisLocal =parentInverted * thisGlobal ;
	} else {
		thisLocal =thisGlobal ;
	}

	FbxAMatrix::kDouble44 &r =thisLocal.Double44 () ;
	Json::Value ar( Json::arrayValue ) ;
	auto k{0};
	for ( int i =0 ; i < 4 ; i++ )
		for ( int j =0 ; j < 4 ; j++ )
			ar[k++] = ((r [i] [j])) ;
	return (ar) ;
}

//-----------------------------------------------------------------------------
Json::Value gltfWriter::WriteNode (FbxNode *pNode) {
	Json::Value nodeDef  ;

	std::string id =nodeId (pNode, false, true) ;
	nodeDef [("name")] =(id) ;

	std::string szType =(pNode->GetTypeName ()) ;
	std::transform (szType.begin (), szType.end (), szType.begin (), ::tolower) ;
	
	// A floating-point 4x4 transformation matrix stored in column-major order.
	// A node will have either a matrix property defined or any combination of rotation, scale, and translation properties defined.
	Json::Value nodeTransform =GetTransform (pNode) ;
	//if ( !nodeTransform.is_boolean () || nodeTransform != true )
		nodeDef [("matrix")] =nodeTransform ;
	//nodeDef [("rotation")] =Json::Value::array ({{ 1., 0., 0., 0. }}) ;
	//nodeDef [("scale")] =Json::Value::array ({{ 1., 1., 1. }}) ;
	//nodeDef [("translation")] =Json::Value::array ({{ 0., 0., 0. }}) ;

	nodeDef [("children")] =Json::Value( Json::arrayValue ) ;
	//nodeDef [("instanceSkin")] = ;

	const FbxNodeAttribute *nodeAttribute =pNode->GetNodeAttribute () ;
	if ( nodeAttribute && nodeAttribute->GetAttributeType () == FbxNodeAttribute::eSkeleton ) {
		// The only difference between a node containing a nullptr and one containing a SKELETON is the property type JOINT.
		nodeDef [("jointName")] =(("JOINT")) ;
	}
	
	//if ( szType == ("mesh") )
	if ( pNode->GetNodeAttribute () && pNode->GetNodeAttribute ()->GetAttributeType () == FbxNodeAttribute::eMesh )
		nodeDef [("meshes")][0] = (nodeId (pNode, true)) ;
	//if ( szType == ("camera") || szType == ("light") )
	if (   pNode->GetNodeAttribute ()
		&& (   pNode->GetNodeAttribute ()->GetAttributeType () == FbxNodeAttribute::eCamera
			|| pNode->GetNodeAttribute ()->GetAttributeType () == FbxNodeAttribute::eLight)
	)
		nodeDef [szType] =(nodeId (pNode, true)) ; //nodeDef [("camera")] nodeDef [("light")]

	Json::Value ret;
	ret[id] = nodeDef;
	return (ret) ;
}


}
