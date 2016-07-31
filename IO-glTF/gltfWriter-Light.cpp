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

void gltfWriter::lightAttenuation (FbxLight *pLight, Json::Value &lightDef) {
	float attenuation [3] = {
		1.f, // Constant
		0.f, // Linear
		0.f // Quadratic
	} ;
	switch ( pLight->DecayType.Get () ) {
		case FbxLight::EDecayType::eLinear:
			attenuation [0] =0.0f ;
			attenuation [1] =1.0f ;
			break ;
		case FbxLight::EDecayType::eCubic:
			attenuation [0] =0.0f ;
			attenuation [2] =1.0f ; // OpenGL doesn't support cubic( so default to quadratic
			break ;
		case FbxLight::EDecayType::eQuadratic:
			attenuation [0] =0.0f ;
			attenuation [2] =1.0f ;
			break ;
		case FbxLight::EDecayType::eNone:
		default:
			attenuation [0] =1.0f ;
			break ;
	}
	if ( attenuation [0] != 1.0f )
		lightDef [("constantAttenuation")] =(attenuation [0]) ;
	if ( attenuation [1] != 0.0f )
		lightDef [("linearAttenuation")] =(attenuation [1]) ;
	if ( attenuation [2] != 0.0f )
		lightDef [("quadraticAttenuation")] =(attenuation [2]) ;
}

Json::Value gltfWriter::WriteLight (FbxNode *pNode) {
	Json::Value light ;
	Json::Value lightDef ;
	light [("name")] =nodeId (pNode, true) ;

	if ( isKnownId (pNode->GetNodeAttribute ()->GetUniqueID ()) ) {
		// The light was already exported, create only the transform node
		Json::Value node =WriteNode (pNode) ;
		Json::Value ret;
		ret["nodes"] = node ;
		return (ret) ;
	}

	FbxLight *pLight =pNode->GetLight () ; //FbxCast<FbxLight>(pNode->GetNodeAttribute ()) ;
	static const FbxDouble3 defaultLightColor (1., 1., 1.) ;
	auto color =pLight->Color ;
	if ( _writeDefaults || color.Get () != defaultLightColor ) {
		auto &lightColor = lightDef["color"];
		lightColor[0] =	static_cast<float>(color.Get () [0]);
		lightColor[1] =	static_cast<float>(color.Get () [1]);
		lightColor[2] =	static_cast<float>(color.Get () [2]);
	}
	switch ( pLight->LightType.Get () ) {
		case FbxLight::EType::ePoint:
			light [("type")] =(("point")) ;
			lightAttenuation (pLight, lightDef) ;
			light [("point")] =lightDef ;
			break ;
		case FbxLight::EType::eSpot:
			light [("type")] =(("spot")) ;
			lightAttenuation (pLight, lightDef) ;
			if ( pLight->OuterAngle.Get () != 180.0 ) // default is PI
				lightDef [("fallOffAngle")] =DEG2RAD (pLight->OuterAngle) ;
			if ( _writeDefaults )
				lightDef [("fallOffExponent")] =((double)0.) ;
			light [("spot")] =lightDef ;
			break ;
		case FbxLight::EType::eDirectional:
			light [("type")] =(("directional")) ;
			light [("directional")] =lightDef ;
			break ;
		case FbxLight::EType::eArea:
		case FbxLight::EType::eVolume:
		default: // ambient
			_ASSERTE (false) ;
			return (Json::Value()) ;
			break ;
	}

	Json::Value lib ;
	lib[nodeId (pNode, true, true)] = light ;
	Json::Value node =WriteNode (pNode) ;
	
	Json::Value ret;
	ret["lights"] = lib;
	ret["nodes"] = node;
	return ret ;
}

Json::Value gltfWriter::WriteAmbientLight (FbxScene &pScene) {
	Json::Value light ;
	Json::Value lightDef ;
	static const FbxDouble3 defaultLightColor (1., 1., 1.) ;
	FbxColor color (pScene.GetGlobalSettings ().GetAmbientColor ()) ;
	if ( !color.mRed && !color.mGreen && !color.mBlue )
		return (Json::Value()) ;
	if ( !_writeDefaults && color == defaultLightColor )
		return (Json::Value()) ;
	lightDef [("color")] [0] =static_cast<float> (color.mRed) ;
	lightDef [("color")] [1] =static_cast<float> (color.mGreen) ;
	lightDef [("color")] [2] =static_cast<float> (color.mBlue) ;
	light [("type")] =(("ambient")) ;
	light [("ambient")] =lightDef ;

	std::string buffer =utility::conversions::to_string_t ((int)0) ;
	std::string uid (("defaultambient")) ;
	uid +=("_") + buffer ;
	Json::Value ret;
	ret[uid] = light;
	return ret ;
	//return (Json::Value::object ({ { nodeId (("defaultambient"), 0x00), light } })) ;
}

}