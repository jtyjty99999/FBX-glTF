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

// FBX does not support Blinn yet, it would become Phong by default.
std::string gltfWriter::LighthingModel (FbxSurfaceMaterial *pMaterial) {
	if ( pMaterial->Is<FbxSurfacePhong> () ) {
		return (("Phong")) ;
	} else if ( pMaterial->Is<FbxSurfaceLambert> () ) {
		return (("Lambert")) ;
	} else { // use shading model
		FbxString szShadingModel =pMaterial->ShadingModel.Get () ;
		// shading models supported here are: "constant", "blinn"
		// note that "lambert" and "phong" should have been treated above
		// all others default to "phong"
		if ( szShadingModel == "constant" )
			return (("Constant")) ;
		else if ( szShadingModel == "blinn" )
			return (("Blinn")) ;
		else
			return (("Phong")) ;
	}
}

Json::Value gltfWriter::WriteMaterial (FbxNode *pNode, FbxSurfaceMaterial *pMaterial) {
	std::string materialName =pMaterial->GetNameWithoutNameSpacePrefix ().Buffer () ; // Material do not support namespaces.

	// Look if this material is already in the materials library.
	//if ( _json [("materials")].isMember (materialName) )
	if ( isNameRegistered (materialName) )
		return (Json::Value(materialName)) ;

	Json::Value material ;
	material [("name")] =materialName ;

	Json::Value ret ;
	
		// Use Cg shaders in WebGL?
		// Usually you don't want to.
		//
		// You could compile your shaders into GLSL with cgc (-profile glslv or - profile glslf), then load them 
		// anywhere you want.There is, however, slight difference between desktop and ES GLSL (WebGL likely using
		// ES specification), so it may require adding a little hints at the beginning of shader 
		// (like precision mediump float; , could easily be #ifdef'd/).
		//
		// Of course you can't use some Cg functionality in this case - if it is missing in GLSL, cgc can do nothing.
		// E.g. mapping uniforms to specified registers or settings varyings to specified interpolator.

		// If the material has a CGFX implementation, export the bind table
		const FbxImplementation *pImpl =GetImplementation (pMaterial, FBXSDK_IMPLEMENTATION_HLSL) ;
		if ( pImpl == nullptr )
			pImpl =GetImplementation (pMaterial, FBXSDK_IMPLEMENTATION_CGFX) ;
		if ( pImpl ) {
			const FbxBindingTable *pTable =pImpl->GetRootTable () ;
			size_t entryCount =pTable->GetEntryCount () ;
			for ( size_t entryIndex =0 ; entryIndex < entryCount ; entryIndex++ ) {
				const FbxBindingTableEntry &bindTableEntry =pTable->GetEntry (entryIndex) ;
				const char *pszDest =bindTableEntry.GetDestination () ;
				FbxProperty pSourceProperty =pMaterial->FindPropertyHierarchical (bindTableEntry.GetSource ()) ;
				_ASSERTE( pSourceProperty.IsValid () ) ;
				//TODO: - hlsl and CGFX support
			}
		}

		// IMPORTANT NOTE:
		// Always check for the most complex class before the less one. In this case, Phong inherit from Lambert,
		// so if we would be testing for lambert classid before phong, we would never enter the phong case.
		// eh, Blinn–Phong shading model - The Blinn–Phong reflection model (also called the modified Phong reflection model) 
		// is a modification to the Phong reflection model
		if ( pMaterial->Is<FbxSurfacePhong> () ) {
			ret =WritePhongMaterial (pNode, pMaterial) ;
		} else if ( pMaterial->Is<FbxSurfaceLambert> () ) {
			ret =WriteLambertMaterial (pNode, pMaterial) ;
		} else { // use shading model
			FbxString szShadingModel =pMaterial->ShadingModel.Get () ;
			// shading models supported here are: "constant", "blinn"
			// note that "lambert" and "phong" should have been treated above
			// all others default to "phong"
			if ( szShadingModel == "constant" ) {
				ret =WriteConstantShadingModelMaterial (pNode, pMaterial) ;
			} else if ( szShadingModel == "blinn" ) {
				ret =WriteBlinnShadingModelMaterial (pNode, pMaterial) ;
			} else {
				// If has a CGFX implementation, export constant in profile common;
				// And set path with NVidia extension
				FbxImplementation *pImpl =pMaterial->GetDefaultImplementation () ;
				if ( pImpl && pImpl->Language.Get () == FBXSDK_SHADING_LANGUAGE_CGFX )
					ret =WriteDefaultShadingModelWithCGFXMaterial (pNode, pMaterial) ;
				else
					ret =WriteDefaultShadingModelMaterial (pNode, pMaterial) ;
			}
		}
	

	Json::Value techniqueParameters ;
	if ( !ret.isNull () ) {
		material [("values")] =ret [("values")] ;
		techniqueParameters =ret [("techniqueParameters")] ;
	}

	std::string techniqueName =createUniqueName (materialName + ("_technique"), 0) ; // Start with 0, but will increase based on own many are yet registered
	material [("technique")] =(techniqueName) ; // technique name already registered

	registerName (materialName) ; // Register material name to avoid duplicates
	Json::Value lib;
	lib[materialName] = material ;

	Json::Value technique ;
	technique["parameters"] = techniqueParameters ;
	Json::Value techniques ;
	techniques[techniqueName] = technique ;

	Json::Value full ;
	full[("materials")] = lib ;
	full[("techniques")] = techniques ;
	if ( !ret.isNull () ) {
		auto memberNames = ret.getMemberNames();
		for ( auto &name : memberNames ) {
			if ( name != ("values") && name != ("techniqueParameters") )
				full [name] =ret[name] ;
		}
	}
	return (full) ;
}

Json::Value gltfWriter::WriteDefaultMaterial (FbxNode *pNode) {
	std::string materialName (("defaultMaterial")) ;

	// Look if this material is already in the materials library.
	//if ( _json [("materials")].isMember (materialName) )
	if ( isNameRegistered (materialName) )
		return (Json::Value(materialName)) ;

	Json::Value material ;
	material [("name")] =(materialName) ;

	// Look if this material is already in the materials library.
	Json::Value ret ;
	if ( !_json [("materials")].isMember (materialName) ) {
		FbxNode::EShadingMode shadingMode =pNode->GetShadingMode () ;
		ret =WriteDefaultShadingModelMaterial (pNode) ;
	}

	Json::Value techniqueParameters ;
	if ( !ret.isNull () ) {
		material [("values")] =ret [("values")] ;
		techniqueParameters =ret [("techniqueParameters")] ;
	}

	std::string techniqueName =createUniqueName (materialName + ("_technique"), 0) ; // Start with 0, but will increase based on own many are yet registered
	material [("technique")] =(techniqueName) ; // technique name already registered

	registerName (materialName) ; // Register material name to avoid duplicates
	Json::Value lib ;
	lib[materialName] = material ;

	Json::Value technique ;
	technique[("parameters")] = techniqueParameters ;
	Json::Value techniques ;
	techniques[techniqueName] = technique ;

	Json::Value full ;
	full[("materials")] = lib ;
	full[("techniques")] = techniques ;
	if ( !ret.isNull () ) {
		auto memberNames = ret.getMemberNames();
		for ( auto & name : memberNames ) {
			if ( name != ("values") && name != ("techniqueParameters") )
				full [name] =ret[name] ;
		}
	}
	return (full) ;
}

#define MultiplyDouble3By(a,b) a [0] *= b ; a [1] *= b ; a [2] *= b

// The transparency factor is usually in the range [0,1]. Maya does always export a transparency factor of 1.0, which implies full transparency.

Json::Value gltfWriter::WriteMaterialTransparencyParameter (
	const char *pszName, 
	FbxPropertyT<FbxDouble> &property, FbxPropertyT<FbxDouble3> &propertyColor, FbxProperty &propertyOpaque,
	Json::Value &values, Json::Value &techniqueParameters
) {
	Json::Value ret ;
	double value =1. ;
	if ( propertyOpaque.IsValid () ) {
		value =1.0 - propertyOpaque.Get<double> () ;
	} else {
		if ( !property.IsValid () )
			return (ret) ;
		value =property.Get () ;
		if ( propertyColor.IsValid () ) {
			FbxDouble3 color =propertyColor.Get () ;
			value =(color [0] * value + color [1] * value + color [2] * value) / 3.0 ;
		}
	}
	if ( !GetIOSettings ()->GetBoolProp (IOSN_FBX_GLTF_INVERTTRANSPARENCY, false) )
		value =1.0 - value ;
	values [pszName] =(value) ;
	techniqueParameters [pszName][("type")] = IOglTF::FLOAT ;
	return (ret) ;
}

//Json::Value gltfWriter::WriteMaterialTransparencyParameter (
//	const char *pszName, 
//	FbxPropertyT<FbxDouble> &property, 
//	Json::Value &values, Json::Value &techniqueParameters
//) {
//	Json::Value ret ;
//	if ( !property.IsValid () )
//		return (ret) ;
//	double value =property.Get () ;
//	if ( !GetIOSettings ()->GetBoolProp (IOSN_FBX_GLTF_INVERTTRANSPARENCY, false) )
//		value =1.0 - value ;
//	values [pszName] =(value) ;
//	techniqueParameters [pszName] =Json::Value::object ({ { ("type"), IOglTF::FLOAT } }) ;
//	return (ret) ;
//}

Json::Value gltfWriter::WriteMaterialParameter (const char *pszName, FbxPropertyT<FbxDouble3> &property, double factor, Json::Value &values, Json::Value &techniqueParameters) {
	Json::Value ret ;
	if ( !property.IsValid () )
		return (ret) ;
	if ( property.GetSrcObjectCount<FbxTexture> () != 0 ) {
		FbxTexture *pTexture =property.GetSrcObject<FbxTexture> (0) ;
		values [pszName] =(createTextureName (pTexture->GetName ())) ;
		techniqueParameters [pszName] [("type")] = IOglTF::SAMPLER_2D ;
		ret =WriteTexture (pTexture) ;
	} else {
		FbxDouble3 color =property.Get () ;
		MultiplyDouble3By (color, factor) ;
		auto &colorVals = values [pszName];
		colorVals[0] = color [0];
		colorVals[1] = color [1];
		colorVals[2] = color [2];
		colorVals[3] = 1. ;
		techniqueParameters [pszName] [("type")] = IOglTF::FLOAT_VEC4 ;
	}
	return (ret) ;
}

Json::Value gltfWriter::WriteMaterialParameter (const char *pszName, FbxPropertyT<FbxDouble> &property, Json::Value &values, Json::Value &techniqueParameters) {
	Json::Value ret ;
	if ( !property.IsValid () )
		return (ret) ;
	if ( property.GetSrcObjectCount<FbxTexture> () != 0 ) {
		FbxTexture *pTexture =property.GetSrcObject<FbxTexture> (0) ;
		values [pszName] =(createTextureName (pTexture->GetName ())) ;
		techniqueParameters [pszName] [("type")] = IOglTF::SAMPLER_2D ;
		ret =WriteTexture (pTexture) ;
	} else {
		double value =property.Get () ;
		values [pszName] =(value) ;
		techniqueParameters [pszName] [("type")] = IOglTF::FLOAT ;
	}
	return (ret) ;
}

Json::Value gltfWriter::WriteMaterialParameter (const char *pszName, FbxSurfaceMaterial *pMaterial, const char *propertyName, const char *factorName, Json::Value &values, Json::Value &techniqueParameters) {
	Json::Value ret ;
	FbxProperty property =pMaterial->FindProperty (propertyName, FbxColor3DT, false) ;
	if ( property.IsValid () && property.GetSrcObjectCount<FbxTexture> () != 0 ) {
		FbxTexture *pTexture =property.GetSrcObject<FbxTexture> (0) ;
		values [pszName] =(createTextureName (pTexture->GetName ())) ;
		techniqueParameters [pszName] [("type")] = IOglTF::SAMPLER_2D ;
		ret =WriteTexture (pTexture) ;
	} else {
		FbxProperty factorProperty =pMaterial->FindProperty (factorName, FbxDoubleDT, false) ;
		double factor =factorProperty.IsValid () ? factorProperty.Get<FbxDouble> () : 1. ;
		FbxDouble3 color =property.IsValid () ? property.Get<FbxDouble3> () : FbxDouble3 (1., 1., 1.) ;
		if ( !property.IsValid () && !factorProperty.IsValid () )
			factor =0. ;
		MultiplyDouble3By (color, factor) ;
		auto &colorVals = values [pszName] ;
		colorVals[0] = color [0];
		colorVals[1] = color [1];
		colorVals[2] = color [2];
		colorVals[3] = 1. ;
		techniqueParameters [pszName] [("type")] = IOglTF::FLOAT_VEC4 ;
	}
	return (ret) ;
}

Json::Value gltfWriter::WritePhongMaterial (FbxNode *pNode, FbxSurfaceMaterial *pMaterial) {
	Json::Value values  ;
	Json::Value techniqueParameters  ;
	FbxSurfacePhong *pPhongSurface =FbxCast<FbxSurfacePhong> (pMaterial) ;

	Json::Value ret  ;
	MergeJsonObjects (ret, WriteMaterialParameter (("ambient"), pPhongSurface->Ambient, pPhongSurface->AmbientFactor.Get (), values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("diffuse"), pPhongSurface->Diffuse, pPhongSurface->DiffuseFactor.Get (), values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("emission"), pPhongSurface->Emissive, pPhongSurface->EmissiveFactor.Get (), values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("specular"), pPhongSurface->Specular, pPhongSurface->SpecularFactor.Get (), values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("shininess"), pPhongSurface->Shininess, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("reflective"), pPhongSurface->Reflection, 1., values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("reflectivity"), pPhongSurface->ReflectionFactor, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("transparent"), pPhongSurface->TransparentColor, 1., values, techniqueParameters)) ;

	// gltf - opaque is 1. / transparency is 0.
	// pPhongSurface->TransparentColor // Transparent color property.
	// pPhongSurface->TransparencyFactor // Transparency factor property. This property is used to make a surface more or less opaque (0 = opaque, 1 = transparent).
	auto opacity = pPhongSurface->FindProperty ("Opacity");
	MergeJsonObjects (ret, WriteMaterialTransparencyParameter (
		("transparency"),
		pPhongSurface->TransparencyFactor, pPhongSurface->TransparentColor, opacity,
		values, techniqueParameters
	)) ;

	// todo bumpScale? https://github.com/zfedoran/convert-to-threejs-json/blob/master/convert_to_threejs.py #364
	// 		eTextureNormalMap,
	//		eTextureBump,
	//		eTextureDisplacement,
	//		eTextureDisplacementVector,

	// Note: 
	// INDEXOFREFRACTION is not supported by FBX.
	Json::Value temp;
	temp["values"] = values;
	temp["techniqueParameters"] = techniqueParameters;
	MergeJsonObjects (ret, temp) ;
	return (ret) ;
}

Json::Value gltfWriter::WriteLambertMaterial (FbxNode *pNode, FbxSurfaceMaterial *pMaterial) {
	Json::Value values  ;
	Json::Value techniqueParameters  ;
	FbxSurfaceLambert *pLambertSurface =FbxCast<FbxSurfaceLambert> (pMaterial) ;

	Json::Value ret  ;
	MergeJsonObjects (ret, WriteMaterialParameter (("ambient"), pLambertSurface->Ambient, pLambertSurface->AmbientFactor.Get (), values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("diffuse"), pLambertSurface->Diffuse, pLambertSurface->DiffuseFactor.Get (), values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("emission"), pLambertSurface->Emissive, pLambertSurface->EmissiveFactor.Get (), values, techniqueParameters)) ;
	values [("reflectivity")] =(1.) ;
	techniqueParameters [("reflectivity")] ["type"] = IOglTF::FLOAT ;
	MergeJsonObjects (ret, WriteMaterialParameter (("transparent"), pLambertSurface->TransparentColor, 1., values, techniqueParameters)) ;

	// gltf - opaque is 1. / transparency is 0.
	// pPhongSurface->TransparentColor // Transparent color property.
	// pPhongSurface->TransparencyFactor // Transparency factor property. This property is used to make a surface more or less opaque (0 = opaque, 1 = transparent).
	auto opacity = pLambertSurface->FindProperty ("Opacity");
	MergeJsonObjects (ret, WriteMaterialTransparencyParameter (
		("transparency"),
		pLambertSurface->TransparencyFactor, pLambertSurface->TransparentColor, opacity,
		values, techniqueParameters
	)) ;

	// Note: 
	// REFLECTIVITY, INDEXOFREFRACTION are not supported by FBX.

	//return (Json::Value::object ({ { ("values"), values }, { ("techniqueParameters"), techniqueParameters } })) ;
	Json::Value temp;
	temp["values"] = values;
	temp["techniqueParameters"] = techniqueParameters;
	MergeJsonObjects (ret, temp) ;
	return (ret) ;
}

Json::Value gltfWriter::WriteConstantShadingModelMaterial (FbxNode *pNode, FbxSurfaceMaterial *pMaterial) {
	Json::Value values  ;
	Json::Value techniqueParameters  ;

	Json::Value ret  ;
	MergeJsonObjects (ret, WriteMaterialParameter (("emission"), pMaterial, FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, values, techniqueParameters)) ;
	FbxPropertyT<FbxDouble> factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sReflectionFactor, FbxDoubleDT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("reflectivity"), factorProperty, values, techniqueParameters)) ;
	FbxPropertyT<FbxDouble3> colorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sTransparentColor, FbxDouble3DT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("transparent"), colorProperty, 1., values, techniqueParameters)) ;
	
	// gltf - opaque is 1. / transparency is 0.
	// pPhongSurface->TransparentColor // Transparent color property.
	// pPhongSurface->TransparencyFactor // Transparency factor property. This property is used to make a surface more or less opaque (0 = opaque, 1 = transparent).
	factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sTransparencyFactor, FbxDoubleDT, false) ;
	auto opacity = pMaterial->FindProperty ("Opacity");
	MergeJsonObjects (ret, WriteMaterialTransparencyParameter (
		("transparency"),
		factorProperty, colorProperty, opacity,
		values, techniqueParameters
	)) ;

	// Note: 
	// REFLECTIVE, INDEXOFREFRACTION are not supported by FBX.
	Json::Value temp;
	temp["values"] = values;
	temp["techniqueParameters"] = techniqueParameters;
	MergeJsonObjects (ret, temp) ;
	return (ret) ;
}

Json::Value gltfWriter::WriteBlinnShadingModelMaterial (FbxNode *pNode, FbxSurfaceMaterial *pMaterial) {
	Json::Value values  ;
	Json::Value techniqueParameters  ;

	Json::Value ret  ;
	MergeJsonObjects (ret, WriteMaterialParameter (("ambient"), pMaterial, FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("diffuse"), pMaterial, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("emission"), pMaterial, FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("specular"), pMaterial, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, values, techniqueParameters)) ;
	FbxPropertyT<FbxDouble> factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sShininess, FbxDoubleDT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("shininess"), factorProperty, values, techniqueParameters)) ;
	FbxPropertyT<FbxDouble3> colorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sReflection, FbxDouble3DT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("reflective"), colorProperty, 1., values, techniqueParameters)) ;
	factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sReflectionFactor, FbxDoubleDT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("reflectivity"), factorProperty, values, techniqueParameters)) ;
	colorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sTransparentColor, FbxDouble3DT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("transparent"), colorProperty, 1., values, techniqueParameters)) ;

	// gltf - opaque is 1. / transparency is 0.
	// pPhongSurface->TransparentColor // Transparent color property.
	// pPhongSurface->TransparencyFactor // Transparency factor property. This property is used to make a surface more or less opaque (0 = opaque, 1 = transparent).
	factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sTransparencyFactor, FbxDoubleDT, false) ;
	auto opacity = pMaterial->FindProperty ("Opacity");
	MergeJsonObjects (ret, WriteMaterialTransparencyParameter (
		("transparency"),
		factorProperty, colorProperty, opacity,
		values, techniqueParameters
	)) ;

	// Note: 
	// INDEXOFREFRACTION is not supported by FBX.
	Json::Value temp;
	temp["values"] = values;
	temp["techniqueParameters"] = techniqueParameters;
	MergeJsonObjects (ret, temp) ;
	return (ret) ;
}

Json::Value gltfWriter::WriteDefaultShadingModelWithCGFXMaterial (FbxNode *pNode, FbxSurfaceMaterial *pMaterial) {
	// Set default material to constant, in case when the reader doesn't support NVidia extension.
	//TODO: hlsl and CGFX support

	_ASSERTE( false ) ;
	return (Json::Value()) ;
}

Json::Value gltfWriter::WriteDefaultShadingModelMaterial (FbxNode *pNode, FbxSurfaceMaterial *pMaterial) {
	Json::Value values  ;
	Json::Value techniqueParameters  ;

	Json::Value ret  ;
	MergeJsonObjects (ret, WriteMaterialParameter (("ambient"), pMaterial, FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("diffuse"), pMaterial, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("emission"), pMaterial, FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, values, techniqueParameters)) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("specular"), pMaterial, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, values, techniqueParameters)) ;
	//FbxProperty<FbxDouble> factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sShininess, FbxDoubleDT, false) ;
	FbxPropertyT<FbxDouble> factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sShininess, FbxDoubleDT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("shininess"), factorProperty, values, techniqueParameters)) ;
	FbxPropertyT<FbxDouble3> colorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sReflection, FbxDouble3DT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("reflective"), colorProperty, 1., values, techniqueParameters)) ;
	factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sReflectionFactor, FbxDoubleDT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("reflectivity"), factorProperty, values, techniqueParameters)) ;
	colorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sTransparentColor, FbxDouble3DT, false) ;
	MergeJsonObjects (ret, WriteMaterialParameter (("transparent"), colorProperty, 1., values, techniqueParameters)) ;
	
	// gltf - opaque is 1. / transparency is 0.
	// pPhongSurface->TransparentColor // Transparent color property.
	// pPhongSurface->TransparencyFactor // Transparency factor property. This property is used to make a surface more or less opaque (0 = opaque, 1 = transparent).
	factorProperty =pMaterial->FindProperty (FbxSurfaceMaterial::sTransparencyFactor, FbxDoubleDT, false) ;
	auto opacity = pMaterial->FindProperty ("Opacity");
	MergeJsonObjects (ret, WriteMaterialTransparencyParameter (
		("transparency"),
		factorProperty, colorProperty, opacity,
		values, techniqueParameters
	)) ;

	// Note: 
	// INDEXOFREFRACTION is not supported by FBX.
	Json::Value temp;
	temp["values"] = values;
	temp["techniqueParameters"] = techniqueParameters;
	MergeJsonObjects (ret, temp) ;
	return (ret) ;
}

#define AssignDefaultColor(pszName, defaultColor) \
	{ \
		auto &colorValues = values [pszName]; \
		colorValues[0] = defaultColor [0]; \
		colorValues[1] = defaultColor [1]; \
		colorValues[2] = defaultColor [2]; \
		colorValues[3] = 1. ; \
		techniqueParameters [pszName] [("type")] = IOglTF::FLOAT_VEC4 ; \
	}

Json::Value gltfWriter::WriteDefaultShadingModelMaterial (FbxNode *pNode) {
	Json::Value values  ;
	Json::Value techniqueParameters  ;

	Json::Value ret  ;
	//FbxScene *pScene =pNode->GetScene () ;
	//FbxColor ambient (pScene->GetGlobalSettings ().GetAmbientColor ()) ;
	//AssignDefaultColor (("ambient"), ambient) ;

	FbxProperty property =pNode->GetNodeAttribute ()->Color ;
	FbxDouble3 color =property.IsValid () ? property.Get<FbxDouble3> () : FbxNodeAttribute::sDefaultColor ;
	AssignDefaultColor (("diffuse"), color) ;
	Json::Value temp;
	temp["values"] = values;
	temp["techniqueParameters"] = techniqueParameters;
	MergeJsonObjects (ret, temp) ;
	return (ret) ;
}

}
