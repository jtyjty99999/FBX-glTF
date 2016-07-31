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

void gltfWriter::AdditionalTechniqueParameters (FbxNode *pNode, Json::Value &techniqueParameters, bool bHasNormals /*=false*/) {
	if ( bHasNormals ) {
		auto &normalMat = techniqueParameters [("normalMatrix")];
		normalMat[("semantic")] = (("MODELVIEWINVERSETRANSPOSE"));
		normalMat[("type")] =((int)IOglTF::FLOAT_MAT3);
	}
	auto &modelMat = techniqueParameters [("modelViewMatrix")];
	modelMat[("semantic")] = (("MODELVIEW"));
	modelMat[("type")] = ((int)IOglTF::FLOAT_MAT4);
	
	auto &projMat = techniqueParameters [("projectionMatrix")];
	projMat[("semantic")] = (("PROJECTION"));
	projMat[("type")] = ((int)IOglTF::FLOAT_MAT4);
	

//d:\projects\gltf\converter\collada2gltf\shaders\commonprofileshaders.cpp #905
	//if ( hasSkinning ) {
	//	addSemantic ("vs", "attribute",
	//				 "JOINT", "joint", 1, false);
	//	addSemantic ("vs", "attribute",
	//				 "WEIGHT", "weight", 1, false);

	//	assert (techniqueExtras != nullptr);
	//	addSemantic ("vs", "uniform",
	//				 JOINTMATRIX, "jointMat", jointsCount, false, true /* force as an array */);
	//}

	// We ignore lighting if the only light we have is ambient
	int lightCount =pNode->GetScene ()->RootProperty.GetSrcObjectCount<FbxLight> () ;
	for ( int i =0 ; i < lightCount ; i++ ) {
		FbxLight *pLight =pNode->GetScene ()->RootProperty.GetSrcObject<FbxLight> (i) ;
		if ( lightCount == 1 && pLight->LightType.Get () == FbxLight::EType::ePoint )
			return ;
		//std::string name =nodeId (pLight->GetNode ()) ;
		std::string name =(pLight->GetTypeName ()) ;
		std::transform (name.begin (), name.end (), name.begin (), ::tolower) ;
		if ( pLight->LightType.Get () == FbxLight::EType::ePoint ) {
			auto &colorVals =techniqueParameters [name + std::to_string (i) + ("Color")];
			colorVals[("type")] = ((int)IOglTF::FLOAT_VEC3);
			auto &colorValsArr = colorVals[("value")];
			colorValsArr[0] = pLight->Color.Get () [0];
			colorValsArr[1] = pLight->Color.Get () [1];
			colorValsArr[2] = pLight->Color.Get () [2];
		} else {
			std::string light_id =(nodeId (pLight->GetNode (), false)) ;
			auto &colorVal = techniqueParameters [name + std::to_string (i) + ("Color")];
			colorVal[("type")] = ((int)IOglTF::FLOAT_VEC3);
			auto &colorValArr = colorVal["values"];
			colorValArr[0] = pLight->Color.Get () [0];
			colorValArr[1] = pLight->Color.Get () [1];
			colorValArr[2] = pLight->Color.Get () [2];
			
			auto &transVal = techniqueParameters [name + std::to_string(i) + ("Transform")];
			transVal[("semantic")] = ("MODELVIEW");
			transVal[("node")] = (light_id);
			transVal[("type")] = ((int)IOglTF::FLOAT_MAT4);
		
			if ( pLight->LightType.Get () == FbxLight::EType::eDirectional ) {
				Json::Value lightDef  ;
				lightAttenuation (pLight, lightDef) ;
				if ( !lightDef [("constantAttenuation")].isNull () ) {
					auto &attenVal = techniqueParameters [name + std::to_string(i) + ("ConstantAttenuation")];
					attenVal[("type")] = ((int)IOglTF::FLOAT);
					attenVal[("value")] = lightDef [("constantAttenuation")];
				}
				if ( !lightDef [("linearAttenuation")].isNull () ) {
					auto &attenVal = techniqueParameters [name + std::to_string(i) + ("LinearAttenuation")] ;
					attenVal[("type")] = ((int)IOglTF::FLOAT) ;
					attenVal[("value")] = lightDef [("linearAttenuation")];
				}
				if ( !lightDef [("quadraticAttenuation")].isNull () ) {
					auto &attenVal = techniqueParameters [name + std::to_string (i) + ("QuadraticAttenuation")];
					attenVal[("type")] = ((int)IOglTF::FLOAT);
					attenVal[("value")] = lightDef [("quadraticAttenuation")];
				}
			}
			if ( pLight->LightType.Get () == FbxLight::EType::eSpot ) {
				auto &inverseVal = techniqueParameters [name + std::to_string (i) + ("InverseTransform")] ;
				inverseVal[("semantic")] = (("MODELVIEWINVERSE"));
				inverseVal[("node")] = (light_id);
				inverseVal[("type")] = ((int)IOglTF::FLOAT_MAT4);
				
				auto &falloffVal = techniqueParameters [name + std::to_string(i) + ("FallOffAngle")];
				falloffVal[("type")] = ((int)IOglTF::FLOAT);
				falloffVal[("value")] = (DEG2RAD (pLight->OuterAngle));
				
				auto &falloffExpoVal = techniqueParameters [name + std::to_string (i) + ("FallOffExponent")];
				falloffExpoVal[("type")] = ((int)IOglTF::FLOAT);
				falloffExpoVal[("value")] = 0.;
			}
		}
	}
}

void gltfWriter::TechniqueParameters (FbxNode *pNode, Json::Value &techniqueParameters, Json::Value &attributes, Json::Value &accessors, bool bHasMaterial) {
	auto memberNames = attributes.getMemberNames();
	for ( const auto &memberName : memberNames ) {
		auto name = memberName;
		std::transform (name.begin (), name.end (), name.begin (), ::tolower) ;
		//std::replace (name.begin (), name.end (), ('_'), ('x')) ;
		name.erase (std::remove (name.begin (), name.end (), ('_')), name.end ()) ;
		std::string upperName (memberName) ;
		std::transform (upperName.begin (), upperName.end (), upperName.begin (), ::toupper) ;
		Json::Value accessor =accessors [attributes[memberName].asString ()] ;
		if ( !bHasMaterial && utility::details::limitedCompareTo (name, ("texcoord")) == 0 )
			continue ;
		auto &val = techniqueParameters [name];
		val[("semantic")] = (upperName);
		val[("type")] = ((int)IOglTF::techniqueParameters (accessor [("type")].asString ().c_str (), accessor [("componentType")].asInt ()));
	}
}

Json::Value gltfWriter::WriteTechnique (FbxNode *pNode, FbxSurfaceMaterial *pMaterial, Json::Value &techniqueParameters) {
	// The FBX SDK does not have such attribute. At best, it is an attribute of a Shader FX, CGFX or HLSL.
	Json::Value commonProfile ;
	commonProfile[("doubleSided")] = false ;
	if ( pMaterial != nullptr )
		commonProfile [("lightingModel")] =(LighthingModel (pMaterial)) ;
	else
		commonProfile [("lightingModel")] =(("Unknown")) ;
	if ( _uvSets.size () ) {
		commonProfile [("texcoordBindings")]  ;
		for ( auto iter : _uvSets ) {
			std::string key (iter.first) ;
			std::string::size_type i =key.find (("Color")) ;
			if ( i != std::string::npos )
				key.erase (i, 5) ;
			std::transform (key.begin (), key.end (), key.begin (), ::tolower) ;
			commonProfile [("texcoordBindings")] [key] =(iter.second) ;
		}
	}
	/*commonProfile [("parameters")] =Json::Value::array () ;
	for ( const auto &iter : techniqueParameters.as_object () ) {
		if (   (  utility::details::limitedCompareTo (iter.first, ("position")) != 0
			   && utility::details::limitedCompareTo (iter.first, ("normal")) != 0
			   && utility::details::limitedCompareTo (iter.first, ("texcoord")) != 0)
			|| iter.first == ("normalMatrix")
		)
			commonProfile [("parameters")] [commonProfile [("parameters")].size ()] =(iter.first) ;

		Json::Value param =iter.second ;
		if ( param [("type")].asInt () == IOglTF::SAMPLER_2D ) {
			// todo:
		}
	}*/

	Json::Value attributes  ;
	auto memberNames = techniqueParameters.getMemberNames();
	for ( const auto &name : memberNames ) {
		if (   utility::details::limitedCompareTo (name, ("position")) == 0
			|| (utility::details::limitedCompareTo (name, ("normal")) == 0 && name != ("normalMatrix"))
			|| (utility::details::limitedCompareTo (name, ("texcoord")) == 0 && pMaterial != nullptr)
		)
			attributes ["a_" + name] =(name) ;
	}

	Json::Value instanceProgram ;
	instanceProgram [("attributes")] =attributes ;
	instanceProgram [("program")] =(createUniqueName (std::string (("program")), 0)) ; // Start with 0, but will increase based on own many are yet registered
	instanceProgram [("uniforms")]  ;
	for ( const auto &name : memberNames ) {
		if (   (  utility::details::limitedCompareTo (name, ("position")) != 0
			   && utility::details::limitedCompareTo (name, ("normal")) != 0
			   && utility::details::limitedCompareTo (name, ("texcoord")) != 0)
			|| name == ("normalMatrix")
		)
			instanceProgram [("uniforms")] ["u_" + name] =(name) ;
	}

	Json::Value techStatesEnable( Json::arrayValue ) ;
	if ( pNode->mCullingType == FbxNode::ECullingType::eCullingOff )
		techStatesEnable [techStatesEnable.size ()] =((int)IOglTF::CULL_FACE) ;
	// TODO: should it always be this way?
	techStatesEnable [techStatesEnable.size ()] =((int)IOglTF::DEPTH_TEST) ;

	Json::Value techStates  ;
	techStates [("enable")] =techStatesEnable ;
	// TODO: needs to be implemented
	//techStates [("functions")] =
	
	Json::Value technique  ;
	technique [("parameters")] =techniqueParameters ;
	//technique [("passes")] =Json::Value::object ({{ ("defaultPass"), techniquePass }}) ;
	technique [("program")] =instanceProgram [("program")] ;
	technique [("states")] =techStates ;
	technique [("attributes")] =instanceProgram [("attributes")] ;
	technique [("uniforms")] =instanceProgram [("uniforms")] ;

	technique [("extras")] =commonProfile ;

	return (technique) ;
}

}
