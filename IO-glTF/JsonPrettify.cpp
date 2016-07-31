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
#include "JsonPrettify.h"
#include <array>

namespace _IOglTF_NS_ {

///*static*/ Json::Value JsonPrettify::sNull ;
//
//void JsonPrettify::serialize (utility::ostream_t &stream) {
//	serialize (_json, stream) ;
//}
//
//void JsonPrettify::serialize (Json::Value &val, utility::ostream_t &stream) {
//	formatValue (val, stream) ;
//}
//
//void JsonPrettify::indent (utility::ostream_t &stream) {
//	stream << std::string (_level, '\t') ;
//}
//
//void JsonPrettify::format_string (const std::string &st, utility::ostream_t &stream) {
//    stream << ('"') ;
//	std::string escaped ;
//	JsonPrettify::append_escape_string<char> (escaped, st) ;
//	stream << escaped ;
//    stream << ('"') ;
//}
//
//void JsonPrettify::format_boolean (const bool val, utility::ostream_t &stream) {
//	stream << val ? ("true") : ("false") ;
//}
//
//void JsonPrettify::format_integer (const int val, utility::ostream_t &stream) {
//	stream << val ;
//}
//
//void JsonPrettify::format_double (const double val, utility::ostream_t &stream) {
//	stream << val ;
//}
//
//void JsonPrettify::format_null (utility::ostream_t &stream) {
//	stream << ("null") ;
//}
//
//void JsonPrettify::formatValue (Json::Value &val, utility::ostream_t &stream) {
//	if ( val.is_array () )
//		formatArray (val.as_array (), stream) ;
//	else if ( val.is_object () )
//		formatObject (val.as_object (), stream) ;
//	else if ( val.is_string () )
//		format_string (val.asString (), stream) ;
//	else if ( val.is_boolean () )
//		format_boolean (val.as_bool (), stream) ;
//	else if ( val.is_integer () )
//		format_integer (val.asInt (), stream) ;
//	else if ( val.is_double () )
//		format_double (val.as_double (), stream) ;
//	else if ( val.isNull () )
//		format_null (stream) ;
//}
//
//void JsonPrettify::formatPair (std::string &key, Json::Value &val, utility::ostream_t &stream) {
//	indent (stream) ;
//	format_string (key, stream) ;
//	stream << (": ") ;
//	formatValue (val, stream) ;
//}
//
//void JsonPrettify::formatArray (web::json::array &arr, utility::ostream_t &stream) {
//	stream << ("[\n") ;
//	if ( arr.size () ) {
//		_level++ ;
//		auto lastElement =arr.end () - 1 ;
//		for ( auto iter =arr.begin () ; iter != lastElement ; ++iter ) {
//			indent (stream) ;
//			formatValue (*iter, stream) ;
//			stream << (",\n") ;
//		}
//		indent (stream) ;
//		formatValue (*lastElement, stream) ;
//		stream << ("\n") ;
//		_level-- ;
//	}
//	indent (stream) ;
//	stream << ("]") ;
//}
//
//void JsonPrettify::formatObject (web::json::object &obj, utility::ostream_t &stream) {
//	stream << ("{\n") ;
//	if ( !obj.empty () ) {
//		_level++ ;
//		auto lastElement =obj.end () - 1 ;
//		for ( auto iter =obj.begin () ; iter != lastElement ; ++iter ) {
//			formatPair (iter->first, iter->second, stream) ;
//			stream << (",\n") ;
//		}
//		formatPair (lastElement->first, lastElement->second, stream) ;
//		stream << ("\n") ;
//		_level-- ;
//	}
//	indent (stream) ;
//	stream << ("}") ;
//}
//
//// Copied from Casablanca REST SDK - json_serialization.cpp #75
//template<typename CharType>
//void JsonPrettify::append_escape_string (std::basic_string<CharType> &str, const std::basic_string<CharType> &escaped) {
//    for ( const auto &ch : escaped ) {
//        switch ( ch ) {
//            case '\"':
//                str +='\\' ;
//                str +='\"' ;
//                break ;
//            case '\\':
//                str +='\\' ;
//                str +='\\' ;
//                break ;
//            case '\b':
//                str +='\\' ;
//                str +='b' ;
//                break ;
//            case '\f':
//                str +='\\' ;
//                str +='f' ;
//                break ;
//            case '\r':
//                str +='\\' ;
//                str +='r' ;
//                break ;
//            case '\n':
//                str +='\\' ;
//                str +='n' ;
//                break ;
//            case '\t':
//                str +='\\' ;
//                str +='t' ;
//                break ;
//            default:
//                // If a control character then must unicode escaped.
//                if ( ch >= 0 && ch <= 0x1F ) {
//                    static const std::array<CharType, 16> intToHex ={ { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' } } ;
//                    str +='\\' ;
//                    str +='u' ;
//                    str +='0' ;
//                    str +='0' ;
//                    str +=intToHex [(ch & 0xF0) >> 4] ;
//                    str +=intToHex [ch & 0x0F] ;
//                } else {
//                    str +=ch ;
//                }
//        }
//    }
//}

}
