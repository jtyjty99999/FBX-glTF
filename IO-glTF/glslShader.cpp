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
#include "glslShader.h"

namespace _IOglTF_NS_ {

//std::string vs [8] ={
//	("position"), ("normal"), ("joint"), ("jointMat"), ("weight"),
//	("normalMatrix"), ("modelViewMatrix"), ("projectionMatrix")
//} ;
//
//std::string fs [8] ={
//	("position"), ("normal"), ("joint"), ("jointMat"), ("weight"),
//	("normalMatrix"), ("modelViewMatrix"), ("projectionMatrix")
//} ;

glslShader::glslShader (const char *glslVersion /*=nullptr*/) {
	_declarations =("") ;
	if ( glslVersion )
		_declarations +=("#version ") + std::string (glslVersion) + ("\n") ;
	_declarations +=("precision highp float ;\n") ;
	_body =("") ;
}

glslShader::~glslShader () {
}

std::string &glslShader::name () {
	return (_name) ;
}

void glslShader::_addDeclaration (std::string qualifier, std::string symbol, unsigned int type, size_t count /*=1*/, bool forcesAsAnArray /*=false*/) {
	if ( this->hasSymbol (symbol) == false ) {
		std::string szType =IOglTF::glslShaderType (type) ;
		std::string declaration =qualifier + (" ") + szType + (" ") + symbol ;
		if ( count > 1 || forcesAsAnArray == true )
			declaration +=("[") + utility::conversions::to_string_t ((int)count) + ("]") ;
		declaration +=(" ;\n") ;
		_declarations += declaration ;

		_allSymbols [symbol] =symbol ;
	}
}

void glslShader::addAttribute (std::string symbol, unsigned int type, size_t count /*=1*/, bool forcesAsAnArray /*=false*/) {
	symbol =("a_") +  symbol ;
	_addDeclaration (("attribute"), symbol, type, count, forcesAsAnArray) ;
}

void glslShader::addUniform (std::string symbol, unsigned int type, size_t count /*=1*/, bool forcesAsAnArray /*=false*/) {
	symbol =("u_") +  symbol ;
	_addDeclaration (("uniform"), symbol, type, count, forcesAsAnArray) ;
}

void glslShader::addVarying (std::string symbol, unsigned int type, size_t count /*=1*/, bool forcesAsAnArray /*=false*/) {
	symbol =("v_") +  symbol ;
	_addDeclaration (("varying"), symbol, type, count, forcesAsAnArray) ;
}

bool glslShader::hasSymbol (const std::string &symbol) {
	return (_allSymbols.count (symbol) > 0) ;
}

void glslShader::appendCode (const char *format, ...) {
	char buffer [1000] ;
	va_list args ;
	va_start (args, format) ;
#if defined(_WIN32) || defined(_WIN64)
	vsprintf_s (buffer, format, args) ;
#else
	vsprintf (buffer, format, args) ;
#endif
	_body +=buffer ;
	va_end (args) ;
}

//void glslShader::appendCode (const char *code) {
//	_body +=code ;
//}

std::string glslShader::source () {
	return (_declarations + body ()) ;
}

std::string glslShader::body () {
	return (("void main (void) {\n") + _body + ("}\n")) ;
}

//-----------------------------------------------------------------------------
glslTech::glslTech (Json::Value technique, Json::Value values, Json::Value gltf, const char *glslVersion)
	: _vertexShader (glslVersion), _fragmentShader (glslVersion),
	_bHasNormals (false), _bHasJoint (false), _bHasWeight (false), _bHasSkin (false), _bHasTexTangent (false), _bHasTexBinormal (false),
	_bModelContainsLights (false), _bLightingIsEnabled (false), _bHasAmbientLight (false), _bHasSpecularLight (false), _bHasNormalMap (false)
{
	prepareParameters (technique) ;
	hwSkinning () ;
	lighting1 (technique, gltf) ;
	texcoords (technique) ;
	lighting2 (technique, gltf) ;
	finalizingShaders (technique, gltf) ;
}

glslTech::~glslTech () {
}

/*static*/ std::string glslTech::format (const char *format, ...) {
	char buffer [1000] ;
	va_list args ;
	va_start (args, format) ;
#if defined(_WIN32) || defined(_WIN64)
	_vstprintf_s (buffer, format, args) ;
#else
	vsprintf (buffer, format, args) ;
#endif
	va_end (args) ;
	return (buffer) ;
}

const std::string glslTech::needsVarying (const char *semantic) {
	static std::string varyings [] ={
		("^position$"), ("^normal$"),
		("^texcoord([0-9]+)$")
		//("^light([0-9]+)Transform$")
	} ;

	//static std::string varyingsMapping [] ={
	//	("texcoord([0-9]+)"), ("texcoord%s"),
	//	("light([0-9]+)Transform"), ("light%sDirection")
	//} ;

	int nb =sizeof (varyings) / sizeof (std::string) ;
	//int nb2 =sizeof (varyingsMapping) / sizeof (std::string) / 2 ;
	for ( int i =0 ; i < nb ; i++ ) {
		uregex regex (varyings [i], std::regex_constants::ECMAScript | std::regex_constants::icase) ;
		if ( std::regex_search (semantic, regex) ) {
			/*for ( int j =0 ; j < nb2 ; j++ ) {
				if ( varyings [i] == varyingsMapping [j * 2] ) {
					uregex regex2 (varyings [i]) ;
					std::string st (semantic) ;

					//umatch matches ;
					//if ( std::regex_match (st.cbegin (), st.cend (), matches, regex2) )
					//	std::cout << L"The matching text is:" << matches.str () << std::endl ;

					umatch mr ;
					//bool results =std::regex_search (st.cbegin (), st.cend (), regex2, std::regex_constants::match_default) ;
					bool results =std::regex_search (st, mr, regex2) ;
					//for ( size_t k =0 ; k < mr.size () ; ++k )
					//	std::cout << k << (" ") << mr [k].str () << std::endl ;
					std::string st2 =format (varyingsMapping [j * 2 + 1].c_str (), mr [1].str ().c_str ()) ;
					return (st2) ;
				}
			}*/
			return (varyings [i]) ;
		}
	}
	return (("")) ;
}

/*static*/ bool glslTech::isVertexShaderSemantic (const char *semantic) {
	static std::string vs_semantics [] ={
		("^position$"), ("^normal$"), ("^normalMatrix$"), ("^modelViewMatrix$"), ("^projectionMatrix$"),
		("^texcoord([0-9]+)$"), ("^light([0-9]+)Transform$")
	} ;
	int nb =sizeof (vs_semantics) / sizeof (std::string) ;
	for ( int i =0 ; i < nb ; i++ ) {
		uregex regex (vs_semantics [i], std::regex_constants::ECMAScript | std::regex_constants::icase) ;
		if ( std::regex_search (semantic, regex) ) 
			return (true) ;
	}
	return (false) ;
}

/*static*/ bool glslTech::isFragmentShaderSemantic (const char *semantic) {
	static std::string fs_semantics [] ={
		("^ambient$"), ("^diffuse$"), ("^emission$"),  ("^specular$"),  ("^shininess$"),
		("^reflective$"), ("^reflectivity$"), ("^transparent$"), ("^transparency$"),
		("^light([0-9]+)Color$")
	} ;
	int nb =sizeof (fs_semantics) / sizeof (std::string) ;
	for ( int i =0 ; i < nb ; i++ ) {
		uregex regex (fs_semantics [i], std::regex_constants::ECMAScript | std::regex_constants::icase) ;
		if ( std::regex_search (semantic, regex) ) 
			return (true) ;
	}
	return (false) ;
}

void glslTech::prepareParameters (Json::Value technique) {
	// Parameters / attribute - uniforms - varying
	auto &parameters =technique [("parameters")] ;
	auto memberNames = parameters.getMemberNames();
	for ( auto &name : memberNames ) {
		unsigned int iType =parameters[name] [("type")].asInt () ;
		bool bIsAttribute =technique [("attributes")].isMember (("a_") + name) ;
		
		if ( glslTech::isVertexShaderSemantic (name.c_str()) ) {
			if ( bIsAttribute )
				_vertexShader.addAttribute (name, iType) ;
			else
				_vertexShader.addUniform (name, iType) ;
			std::string v =needsVarying (name.c_str ()) ;
			if ( ! v.empty() ) {
				_vertexShader.addVarying (name, iType) ;
				_fragmentShader.addVarying (name, iType) ;
			}
		}
		if ( glslTech::isFragmentShaderSemantic (name.c_str()) ) {
			if ( bIsAttribute )
				_fragmentShader.addAttribute (name, iType) ;
			else
				_fragmentShader.addUniform (name, iType) ;
		}

		if ( parameters[name].isMember (("semantic")) ) {
			std::string semantic =parameters[name] [("semantic")].asString () ;
			_bHasNormals |=semantic == ("NORMAL") ;
			_bHasJoint |=semantic == ("JOINT") ;
			_bHasWeight |=semantic == ("WEIGHT") ;
			_bHasTexTangent |=semantic == ("TEXTANGENT") ;
			_bHasTexBinormal |=semantic == ("TEXBINORMAL") ;
		}
	}
	_bHasSkin =_bHasJoint && _bHasWeight ;
}

void glslTech::hwSkinning () {
	// Handle hardware skinning, for now with a fixed limit of 4 influences
	if ( _bHasSkin ) {
		_vertexShader.appendCode (("mat4 skinMat =a_weight.x * u_jointMat [int(a_joint.x)] ;\n")) ;
		_vertexShader.appendCode (("skinMat +=a_weight.y * u_jointMat [int(a_joint.y)] ;\n")) ;
		_vertexShader.appendCode (("skinMat +=a_weight.z * u_jointMat [int(a_joint.z)] ;\n")) ;
		_vertexShader.appendCode (("skinMat +=a_weight.w * u_jointMat [int(a_joint.w)] ;\n")) ;
		_vertexShader.appendCode (("vec4 pos =u_modelViewMatrix * skinMat * vec4(a_position, 1.0) ;\n")) ;
		if ( _bHasNormals )
			_vertexShader.appendCode (("v_normal =u_normalMatrix * mat3(skinMat) * a_normal ;\n")) ;
	} else {
		_vertexShader.appendCode (("vec4 pos =u_modelViewMatrix * vec4(a_position, 1.0) ;\n")) ;
		if ( _bHasNormals )
			_vertexShader.appendCode (("v_normal =u_normalMatrix * a_normal ;\n")) ;
	}
}

bool glslTech::lighting1 (Json::Value technique, Json::Value gltf) {
	// Lighting
	auto &lights =gltf [("lights")] ;
	auto memberNames = lights.getMemberNames();
	for ( auto &name : memberNames )
		_bModelContainsLights |=lights[name] [("type")].asString () != ("ambient") ;
	_bLightingIsEnabled =_bHasNormals && (_bModelContainsLights || true) ; // todo - default lighting option
	if ( _bLightingIsEnabled ) {
		_fragmentShader.appendCode (("vec3 normal =normalize (v_normal) ;\n")) ;
		bool bDoubledSide =technique [("extras")] [("doubleSided")].asBool () ;
		if ( bDoubledSide )
			_fragmentShader.appendCode ("if ( gl_FrontFacing == false ) normal =-normal ;\n") ;
	} else {
		// https://github.com/KhronosGroup/glTF/issues/121
		// We want to keep consistent the parameter in the instanceTechnique and the ones actually in use in the technique.
		// Given the context, some parameters from instanceTechnique will be removed because they aren't used in the resulting
		// shader. As an example, we may have no light declared and the default lighting disabled == no lighting at all, but 
		// still a specular color and a shininess. in this case specular and shininess won't be used.
	}

	// Color to cumulate all components and light contribution
	_fragmentShader.appendCode (("vec4 color =vec4(0., 0., 0., 0.) ;\n")) ;
	_fragmentShader.appendCode (("vec4 diffuse =vec4(0., 0., 0., 1.) ;\n")) ;
	if ( _bModelContainsLights )
		_fragmentShader.appendCode (("vec3 diffuseLight =vec3(0., 0., 0.) ;\n")) ;
	Json::Value parameters =technique [("parameters")] ;
	if ( parameters.isMember (("emission")) )
		_fragmentShader.appendCode (("vec4 emission ;\n")) ;
	if ( parameters.isMember (("reflective")) )
		_fragmentShader.appendCode (("vec4 reflective ;\n")) ;
	if ( _bLightingIsEnabled && parameters.isMember (("ambient")) )
		_fragmentShader.appendCode (("vec4 ambient ;\n")) ;
	if ( _bLightingIsEnabled && parameters.isMember (("specular")) )
		_fragmentShader.appendCode ("vec4 specular ;\n") ;
	return (_bLightingIsEnabled) ;
}

void glslTech::texcoords (Json::Value technique) {
	Json::Value parameters =technique [("parameters")] ;
	std::string texcoordAttributeSymbol =("a_texcoord") ;
	std::string texcoordVaryingSymbol =("v_texcoord") ;
	std::map<std::string, std::string> declaredTexcoordAttributes ;
	std::map<std::string, std::string> declaredTexcoordVaryings ;
	std::string slots []={ ("ambient"), ("diffuse"), ("emission"), ("reflective"), ("specular"), ("bump") } ;
	const int slotsCount =sizeof (slots) / sizeof (std::string) ;
	for ( size_t slotIndex =0 ; slotIndex < slotsCount ; slotIndex++ ) {
		std::string slot =slots [slotIndex] ;
		if ( parameters.isMember (slot) == false )
			continue ;
		unsigned int slotType =parameters [slot] [("type")].asInt () ;
		if ( (!_bLightingIsEnabled) && ((slot == ("ambient")) || (slot == ("specular"))) ) {
			// As explained in https://github.com/KhronosGroup/glTF/issues/121 export all parameters when details is specified
//todo			addParameter (slot, slotType);
			continue ;
		}
		if ( _bLightingIsEnabled && (slot == ("bump")) ) {
			_bHasNormalMap =slotType == IOglTF::SAMPLER_2D && _bHasTexTangent && _bHasTexBinormal ;
			if ( _bHasNormalMap == true ) {
				_vertexShader.addAttribute (("textangent"), IOglTF::FLOAT_VEC3, 1, true) ;
				_vertexShader.addAttribute (("texbinormal"), IOglTF::FLOAT_VEC3, 1, true) ;
				_vertexShader.appendCode ("v_texbinormal =u_normalMatrix * a_texbinormal ;\n") ;
				_vertexShader.appendCode ("v_textangent =u_normalMatrix * a_textangent ;\n") ;
				_fragmentShader.appendCode ("vec4 bump ;\n") ;
			}
		}

		if ( slotType == IOglTF::FLOAT_VEC4 ) {
			std::string slotColorSymbol =("u_") + slot ;
			_fragmentShader.appendCode (("%s =%s ;\n"), slot.c_str (), slotColorSymbol.c_str ()) ;
			//_fragmentShader.addUniformValue (slotType, 1, slot) ;
		} else if ( slotType == IOglTF::SAMPLER_2D ) {
			std::string semantic =technique [("extras")] [("texcoordBindings")] [slot].asString () ;
			std::string texSymbol, texVSymbol ;
			if ( slot == ("reflective") ) {
				texVSymbol =texcoordVaryingSymbol + utility::conversions::to_string_t ((int)declaredTexcoordVaryings.size ()) ;
				unsigned int reflectiveType =IOglTF::FLOAT_VEC2 ;
			//	_vertexShader.addVarying (texVSymbol, reflectiveType) ;
			//	_fragmentShader.addVarying (texVSymbol, reflectiveType) ;
				// Update Vertex shader for reflection
				std::string normalType =IOglTF::szVEC3 ;
				_vertexShader.appendCode (("%s normalizedVert =normalize (%s(pos)) ;\n"), normalType.c_str (), normalType.c_str ()) ;
				_vertexShader.appendCode (("%s r =reflect (normalizedVert, v_normal) ;\n"), normalType.c_str ()) ;
				_vertexShader.appendCode (("r.z +=1.0 ;\n")) ;
				_vertexShader.appendCode (("float m =2.0 * sqrt (dot (r, r)) ;\n")) ;
				_vertexShader.appendCode (("%s =(r.xy / m) + 0.5 ;\n"), texVSymbol.c_str ()) ;
				declaredTexcoordVaryings [semantic] =texVSymbol ;
			} else {
				if ( declaredTexcoordAttributes.count (semantic) == 0 ) {
					texSymbol =texcoordAttributeSymbol + utility::conversions::to_string_t ((int)declaredTexcoordAttributes.size ()) ;
					texVSymbol =texcoordVaryingSymbol + utility::conversions::to_string_t ((int)declaredTexcoordVaryings.size ()) ;
					unsigned int texType =IOglTF::FLOAT_VEC2 ;
			//		_vertexShader.addAttribute (("texcoord") + utility::conversions::to_string_t ((int)declaredTexcoordAttributes.size ()), texType) ;
			//		_vertexShader.addVarying (texVSymbol, texType) ;
			//		_fragmentShader.addVarying (texVSymbol, texType) ;
					_vertexShader.appendCode (("%s =%s ;\n"), texVSymbol.c_str (), texSymbol.c_str ()) ;

					declaredTexcoordAttributes [semantic] =texSymbol ;
					declaredTexcoordVaryings [semantic] =texVSymbol ;
				} else {
					texSymbol =declaredTexcoordAttributes [semantic] ;
					texVSymbol =declaredTexcoordVaryings [semantic] ;
				}
			}

			std::string textureSymbol =("u_") + slot ;
			Json::Value textureParameter =parameters [slot] ; // get the texture
			//_vertexShader.addUniform (texVSymbol, textureParameter [("type")].asInt ()) ;
			//_fragmentShader.addUniform (texVSymbol, textureParameter [("type")].asInt ()) ;
			if ( _bHasNormalMap == false && slot == ("bump") )
				continue ;
			_fragmentShader.appendCode (("%s =texture2D (%s, %s) ;\n"), slot.c_str (), textureSymbol.c_str (), texVSymbol.c_str ()) ;
		}
	}

	if ( _bHasNormalMap ) {
		//_fragmentShader.appendCode("vec3 binormal = normalize( cross(normal,v_textangent));\n");
		_fragmentShader.appendCode ("mat3 bumpMatrix = mat3(normalize(v_textangent), normalize(v_texbinormal), normal);\n");
		_fragmentShader.appendCode ("normal = normalize(-1.0 + bump.xyz * 2.0);\n");
	}
}

Json::Value glslTech::lightNodes (Json::Value gltf) {
	Json::Value ret( Json::arrayValue ) ;
	auto &nodes = gltf["nodes"];
	auto memberNames = nodes.getMemberNames();
	for ( auto &name : memberNames ) {
		if ( nodes[name].isMember (("light")) )
			ret [ret.size ()] =Json::Value (name) ;
	}
	return (ret) ;
}

void glslTech::lighting2 (Json::Value technique, Json::Value gltf) {
	Json::Value parameters =technique [("parameters")] ;

	std::string lightingModel =technique [("extras")] [("lightingModel")].asString () ;
	bool bHasSpecular =
		   parameters.isMember (("specular")) && parameters.isMember (("shininess"))
		&& (lightingModel == ("Phong") || lightingModel == ("Blinn")) ;

	size_t lightIndex =0 ;
	size_t nbLights =gltf [("lights")].size () ;
	if ( _bLightingIsEnabled && nbLights ) {
		Json::Value lightNodes =glslTech::lightNodes (gltf) ;
		//if ( parameters.isMember (("shininess")) && (!shininessObject) ) {

		//std::vector<std::string> ids ;
		auto &lights = gltf [("lights")];
		auto memberNames = lights.getMemberNames();
		for ( auto name : memberNames ) {
			//ids.push_back (iter.first) ;
		
			std::string lightType =lights[name] [("type")].asString () ;
			// We ignore lighting if the only light we have is ambient
			if ( (lightType == ("ambient")) && nbLights == 1 )
				continue ;

			for ( size_t j =0 ; j < lightNodes.size () ; j++, lightIndex++ ) {
				// Each light needs to be re-processed for each node
				std::string szLightIndex =glslTech::format (("light%d"), (int)lightIndex) ;
				std::string szLightColor =glslTech::format (("%sColor"), szLightIndex.c_str ()) ;
				std::string szLightTransform =glslTech::format (("%sTransform"), szLightIndex.c_str ()) ;
				std::string szLightInverseTransform =glslTech::format (("%sInverseTransform"), szLightIndex.c_str ()) ;
				std::string szLightConstantAttenuation =glslTech::format (("%sConstantAttenuation"), szLightIndex.c_str ()) ;
				std::string szLightLinearAttenuation =glslTech::format (("%sLinearAttenuation"), szLightIndex.c_str ()) ;
				std::string szLightQuadraticAttenuation =glslTech::format (("%sQuadraticAttenuation"), szLightIndex.c_str ()) ;
				std::string szLightFallOffAngle =glslTech::format (("%sFallOffAngle"), szLightIndex.c_str ()) ;
				std::string szLightFallOffExponent =glslTech::format (("%sFallOffExponent"), szLightIndex.c_str ()) ;
				
				if ( bHasSpecular ==true && _bHasSpecularLight == false ) {
					_fragmentShader.appendCode (("vec3 specularLight =vec3(0., 0., 0.) ;\n")) ;
					_bHasSpecularLight =true ;
				}

				if ( lightType == ("ambient") ) {
					if ( _bHasAmbientLight == false ) {
						_fragmentShader.appendCode (("vec3 ambientLight =vec3(0., 0., 0.) ;\n")) ;
						_bHasAmbientLight =true ;
					}

					_fragmentShader.appendCode (("{\n")) ;
					//FIXME: what happens if multiple ambient light ?
					_fragmentShader.appendCode (("ambientLight +=u_%s ;\n"), szLightColor.c_str ()) ;
					_fragmentShader.appendCode (("}\n")) ;
				} else {
					std::string szVaryingLightDirection =glslTech::format (("%sDirection"), szLightIndex.c_str ()) ;
					_vertexShader.addVarying (szVaryingLightDirection, IOglTF::FLOAT_VEC3) ;
					_fragmentShader.addVarying (szVaryingLightDirection, IOglTF::FLOAT_VEC3) ;
					if (   /*_vertexShader.hasSymbol (("v_position")) == false
						&&*/ (lightingModel == ("Phong") || lightingModel == ("Blinn") || lightType == ("spot"))
					) {
						//_vertexShader.addVarying (("v_position"), IOglTF::FLOAT_VEC3) ;
						//_fragmentShader.addVarying (("v_position"), IOglTF::FLOAT_VEC3) ;
						_vertexShader.appendCode (("v_position =pos.xyz ;\n")) ;
					}
					_fragmentShader.appendCode (("{\n")) ;
					_fragmentShader.appendCode (("float specularIntensity =0. ;\n")) ;
					if ( lightType != ("directional") ) {
	//					shared_ptr <JSONObject> lightConstantAttenuationParameter=addValue ("fs", "uniform", floatType, 1, lightConstantAttenuation, asset);
	//					lightConstantAttenuationParameter->setValue ("value", description->getValue ("constantAttenuation"));
	//					shared_ptr <JSONObject> lightLinearAttenuationParameter=addValue ("fs", "uniform", floatType, 1, lightLinearAttenuation, asset);
	//					lightLinearAttenuationParameter->setValue ("value", description->getValue ("linearAttenuation"));
	//					shared_ptr <JSONObject> lightQuadraticAttenuationParameter=addValue ("fs", "uniform", floatType, 1, lightQuadraticAttenuation, asset);
	//					lightQuadraticAttenuationParameter->setValue ("value", description->getValue ("quadraticAttenuation"));
					}
	//				shared_ptr <JSONObject> lightColorParameter=addValue ("fs", "uniform", vec3Type, 1, lightColor, asset);
	//				lightColorParameter->setValue ("value", description->getValue ("color"));
	//				shared_ptr <JSONObject> lightTransformParameter=addValue ("vs", "uniform", mat4Type, 1, lightTransform, asset);
	//				lightTransformParameter->setValue (kNode, nodesIds [j]);
	//				lightTransformParameter->setString (kSemantic, MODELVIEW);

					if ( lightType == ("directional") )
						_vertexShader.appendCode (("v_%s =mat3(u_%s) * vec3(0., 0., 1.) ;\n"), szVaryingLightDirection.c_str (), szLightTransform.c_str ()) ;
					else
						_vertexShader.appendCode (("v_%s =u_%s [3].xyz - pos.xyz ;\n"), szVaryingLightDirection.c_str (), szLightTransform.c_str ()) ;
					_fragmentShader.appendCode (("float attenuation =1.0 ;\n")) ;

					if ( lightType != ("directional") ) {
						// Compute light attenuation from non-normalized light direction
						_fragmentShader.appendCode (("float range =length (%s) ;\n"), szVaryingLightDirection.c_str ()) ;
						_fragmentShader.appendCode (("attenuation =1.0 / (u_%s + (u_%s * range) + (u_%s * range * range)) ;\n"),
							szLightConstantAttenuation.c_str (), szLightLinearAttenuation.c_str (), szLightQuadraticAttenuation.c_str ()
						) ;
					}
					// Compute lighting from normalized light direction
					_fragmentShader.appendCode (("vec3 l =normalize (v_%s) ;\n"), szVaryingLightDirection.c_str ()) ;

					if ( lightType == ("spot") ) {
	//					shared_ptr <JSONObject> lightInverseTransformParameter=addValue ("fs", "uniform", mat4Type, 1, lightInverseTransform, asset);
	//					lightInverseTransformParameter->setValue (kNode, nodesIds [j]);
	//					lightInverseTransformParameter->setString (kSemantic, MODELVIEWINVERSE);

	//					shared_ptr <JSONObject> lightFallOffExponentParameter=addValue ("fs", "uniform", floatType, 1, lightFallOffExponent, asset);
	//					lightFallOffExponentParameter->setValue ("value", description->getValue ("fallOffExponent"));
	//					shared_ptr <JSONObject> lightFallOffAngleParameter=addValue ("fs", "uniform", floatType, 1, lightFallOffAngle, asset);
	//					lightFallOffAngleParameter->setValue ("value", description->getValue ("fallOffAngle"));

						// As in OpenGL ES 2.0 programming guide
						// Raise spec issue about the angle
						// we can test this in the shader generation
						_fragmentShader.appendCode (("vec4 spotPosition =u_%s * vec4(v_position, 1.) ;\n"), szLightInverseTransform.c_str ()) ;
						_fragmentShader.appendCode (("float cosAngle =dot (vec3 (0., 0., -1.), normalize (spotPosition.xyz)) ;\n")) ;
						// doing this cos each pixel is just wrong (for performance)
						// need to find a way to specify that we pass the cos of a value
						_fragmentShader.appendCode (("if ( cosAngle > cos (radians (u_%s * 0.5)) ) {\n"), szLightFallOffAngle.c_str()) ;
						_fragmentShader.appendCode (("attenuation *=max (0., pow (cosAngle, u_%s)) ;\n"), szLightFallOffExponent.c_str()) ;
					}

					// We handle phong, blinn, constant and lambert
					if ( bHasSpecular ) {
						_fragmentShader.appendCode (("vec3 viewDir =-normalize (v_position) ;\n")) ;
						if ( lightingModel == ("Phong") ) {
							if ( _bHasNormalMap ) {
								_fragmentShader.appendCode (("l *=bumpMatrix ;\n")) ;
								_fragmentShader.appendCode (("position *=bumpMatrix ;\n")) ;
							}
							_fragmentShader.appendCode (("float phongTerm =max (0.0, dot (reflect (-l, normal), viewDir)) ;\n")) ;
							_fragmentShader.appendCode (("specularIntensity =max (0., pow (phongTerm , u_shininess)) * attenuation ;\n")) ;
						} else if ( lightingModel == ("Blinn") ) {
							_fragmentShader.appendCode (("vec3 h =normalize (l + viewDir) ;\n")) ;
							if ( _bHasNormalMap ) {
								_fragmentShader.appendCode (("h *=bumpMatrix ;\n")) ;
								_fragmentShader.appendCode (("l *=bumpMatrix ;\n")) ;
							}
							_fragmentShader.appendCode (("specularIntensity =max (0., pow (max (dot (normal, h), 0.), u_shininess)) * attenuation ;\n")) ;
						}
						_fragmentShader.appendCode (("specularLight +=u_%s * specularIntensity ;\n"), szLightColor.c_str ()) ;
					}

					// Write diffuse
					_fragmentShader.appendCode (("diffuseLight +=u_%s * max (dot (normal, l), 0.) * attenuation ;\n"), szLightColor.c_str ()) ;
					if ( lightType == ("spot") ) // Close previous scope beginning with "if (cosAngle > " ...
						_fragmentShader.appendCode (("}\n")) ;
					_fragmentShader.appendCode (("}\n")) ;
				}
			}
		}
	}
}

void glslTech::finalizingShaders (Json::Value technique, Json::Value gltf) {
	Json::Value parameters =technique [("parameters")] ;

	if ( parameters.isMember (("reflective")) )
		_fragmentShader.appendCode (("diffuse.xyz +=reflective.xyz ;\n")) ;
	if ( _bHasAmbientLight && _bLightingIsEnabled && parameters.isMember (("ambient")) ) {
		_fragmentShader.appendCode (("ambient.xyz *=ambientLight ;\n")) ;
		_fragmentShader.appendCode (("color.xyz +=ambient.xyz;\n")) ;
	}
	if ( _bHasSpecularLight && _bLightingIsEnabled && parameters.isMember (("specular")) ) {
		_fragmentShader.appendCode (("specular.xyz *=specularLight ;\n")) ;
		_fragmentShader.appendCode (("color.xyz +=specular.xyz ;\n")) ;
	}
	if ( _bModelContainsLights )
		_fragmentShader.appendCode (("diffuse.xyz *=diffuseLight ;\n")) ;
	else if ( _bHasNormals )
		_fragmentShader.appendCode (("diffuse.xyz *=max (dot (normal, vec3(0., 0., 1.)), 0.) ;\n")) ;
	_fragmentShader.appendCode (("color.xyz +=diffuse.xyz ;\n")) ;

	if ( parameters.isMember (("emission")) )
		_fragmentShader.appendCode (("color.xyz +=emission.xyz ;\n")) ;

	bool hasTransparency =parameters.isMember (("transparency")) ;
	if ( hasTransparency )
		_fragmentShader.appendCode (("color =vec4(color.rgb * diffuse.a, diffuse.a * u_transparency) ;\n")) ;
	else
		_fragmentShader.appendCode (("color =vec4(color.rgb * diffuse.a, diffuse.a) ;\n")) ;

	_fragmentShader.appendCode (("gl_FragColor =color ;\n")) ;
	_vertexShader.appendCode (("gl_Position =u_projectionMatrix * pos ;\n")) ;
}

// NVidia hardware indices are reserved for built-in attributes:
// gl_Vertex			0
// gl_Normal			2
// gl_Color				3
// gl_SecondaryColor	4
// gl_FogCoord			5
// gl_MultiTexCoord0	8
// gl_MultiTexCoord1	9
// gl_MultiTexCoord2	10
// gl_MultiTexCoord3	11
// gl_MultiTexCoord4	12
// gl_MultiTexCoord5	13
// gl_MultiTexCoord6	14
// gl_MultiTexCoord7	15

//unsigned int semanticType (const std::string &semantic) {
//	static std::map<std::string, unsigned int> semanticTypes ;
//	if ( semantic.find (("TEXCOORD")) != std::string::npos )
//		return (IOglTF::FLOAT_VEC2) ;
//	if ( semantic.find (("COLOR")) != std::string::npos )
//		return (IOglTF::FLOAT_VEC4) ;
//	if ( semanticTypes.empty () ) {
//		// attributes
//		semanticTypes [("POSITION")] =IOglTF::FLOAT_VEC3 ; // Babylon.js does not like VEC4
//		semanticTypes [("NORMAL")] =IOglTF::FLOAT_VEC3;
//		semanticTypes [("REFLECTIVE")] =IOglTF::FLOAT_VEC2 ;
//		semanticTypes [("WEIGHT")] =IOglTF::FLOAT_VEC4 ;
//		semanticTypes [("JOINT")] =IOglTF::FLOAT_VEC4 ;
//		semanticTypes [("TEXTANGENT")] =IOglTF::FLOAT_VEC3 ;
//		semanticTypes [("TEXBINORMAL")] =IOglTF::FLOAT_VEC3 ;
//		// uniforms
//		semanticTypes [("MODELVIEWINVERSETRANSPOSE")] =IOglTF::FLOAT_MAT3 ; //typically the normal matrix
//		semanticTypes [("MODELVIEWINVERSE")]=IOglTF::FLOAT_MAT4 ;
//		semanticTypes [("MODELVIEW")] =IOglTF::FLOAT_MAT4 ;
//		semanticTypes [("PROJECTION")] =IOglTF::FLOAT_MAT4 ;
//		semanticTypes [("JOINTMATRIX")] =IOglTF::FLOAT_MAT4 ;
//	}
//	return (semanticTypes [semantic]) ;
//}

//unsigned int semanticAttributeType (const std::string &semantic) {
//	static std::map<std::string, unsigned int> semanticAttributeTypes ;
//	if ( semantic.find (("TEXCOORD")) != std::string::npos )
//		return (IOglTF::FLOAT_VEC2) ;
//	if ( semantic.find (("COLOR")) != std::string::npos )
//		return (IOglTF::FLOAT_VEC4) ;
//	if ( semanticAttributeTypes.empty () ) {
//		semanticAttributeTypes [("POSITION")] =IOglTF::FLOAT_VEC3 ;
//		semanticAttributeTypes [("NORMAL")] =IOglTF::FLOAT_VEC3;
//		semanticAttributeTypes [("REFLECTIVE")] =IOglTF::FLOAT_VEC2 ;
//		semanticAttributeTypes [("WEIGHT")] =IOglTF::FLOAT_VEC4 ;
//		semanticAttributeTypes [("JOINT")] =IOglTF::FLOAT_VEC4 ;
//		semanticAttributeTypes [("TEXTANGENT")] =IOglTF::FLOAT_VEC3 ;
//		semanticAttributeTypes [("TEXBINORMAL")] =IOglTF::FLOAT_VEC3 ;
//	}
//	return (semanticAttributeTypes [semantic]) ;
//}
//
//unsigned int semanticUniformType (const std::string &semantic) {
//	static std::map<std::string, unsigned int> semanticUniformTypes ;
//	if ( semanticUniformTypes.empty () ) {
//		semanticUniformTypes [("MODELVIEWINVERSETRANSPOSE")] =IOglTF::FLOAT_MAT3 ; //typically the normal matrix
//		semanticUniformTypes [("MODELVIEWINVERSE")]=IOglTF::FLOAT_MAT4 ;
//		semanticUniformTypes [("MODELVIEW")] =IOglTF::FLOAT_MAT4 ;
//		semanticUniformTypes [("PROJECTION")] =IOglTF::FLOAT_MAT4 ;
//		semanticUniformTypes [("JOINTMATRIX")] =IOglTF::FLOAT_MAT4 ;
//	}
//	return (semanticUniformTypes [semantic]) ;
//}
//
//enum parameterContext {
//	eAttribute,
//	eUniform,
//	eVarying
//} ;
//
//bool addSemantic (glslShader &shader, parameterContext attributeOrUniform,
//	std::string semantic,
//	std::string parameterID,
//	size_t count,
//	bool includesVarying,
//	bool forcesAsAnArray =false) {
//
//	std::string symbol =(attributeOrUniform ? ("a_") : ("u_")) + parameterID ;
//	unsigned int type =semanticType (semantic) ;
//	shared_ptr <JSONObject> parameter (new GLTF::JSONObject ());
//	parameter->setString (kSemantic, semantic);
//	parameter->setUnsignedInt32 (kType, type);
//	_parameters->setValue (parameterID, parameter);
//	if ( attributeOrUniform == parameterContext::eAttribute )
//		_program->attributes ()->setString (symbol, parameterID);
//	else if ( uniformOrAttribute == parameterContext::eUniform )
//		_program->uniforms ()->setString (symbol, parameterID) ;
//	else
//		return (false) ;
//	if ( attributeOrUniform == parameterContext::eAttribute ) {
//		shader.addAttribute (symbol, type) ;
//		if ( includesVarying )
//			_program->addVarying ("v_" + parameterID, type) ;
//	} else {
//		shader.addUniform (symbol, type, count, forcesAsAnArray) ;
//		if ( (count > 1) || forcesAsAnArray )
//			parameter->setUnsignedInt32 (kCount, count) ;
//	}
//	return (true) ;
//}

}
