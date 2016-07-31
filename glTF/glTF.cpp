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
#include "getopt.h"
#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include "tchar.h"
#endif

// Tests
// -f $(ProjectDir)\..\models\duck\duck.fbx -o $(ProjectDir)\..\models\duck\out -n test -c
// -f $(ProjectDir)\..\models\au\au3.fbx -o $(ProjectDir)\..\models\au\out -n test -c
// -f $(ProjectDir)\..\models\teapot\teapot.fbx -o $(ProjectDir)\..\models\teapot\out -n test -c
// -f $(ProjectDir)\..\models\wine\wine.fbx -o $(ProjectDir)\..\models\wine\out -n test -c
// -f $(ProjectDir)\..\models\monster\monster.fbx -o $(ProjectDir)\..\models\monster\out -n test -c
// -f $(ProjectDir)\..\models\Carnivorous_plant\Carnivorous_plant.fbx -o $(ProjectDir)\..\models\Carnivorous_plant\out -n test -c

	//{ ("n"), ("a"), required_argument, ("-a -> export animations, argument [bool], default:true") },
	//{ ("n"), ("g"), required_argument, ("-g -> [experimental] GLSL version to output in generated shaders") },
	//{ ("n"), ("d"), no_argument, ("-d -> export pass details to be able to regenerate shaders and states") },
	//{ ("n"), ("c"), required_argument, ("-c -> compression type: available: Open3DGC [string]") },
	//{ ("n"), ("m"), required_argument, ("-m -> compression mode: for Open3DGC can be \"ascii\"(default) or \"binary\" [string]") },
	//{ ("n"), ("n"), no_argument, ("-n -> don't combine animations with the same target") }

void usage () {
	std::cout << std::endl << ("glTF [-h] [-v] [-n] [-d] [-t] [-l] [-c] [-e] [-o <output path>] -f <input file>") << std::endl ;
	std::cout << ("-f/--file \t\t- file to convert to glTF [string]") << std::endl ;
	std::cout << ("-o/--output \t\t- path of output directory [string]") << std::endl ;
	std::cout << ("-n/--name \t\t- override the scene name [string]") << std::endl ;
	std::cout << ("-d/--degree \t\t- output angles in degrees vs radians (default to radians)") << std::endl ;
	std::cout << ("-t/--transparency \t- invert transparency") << std::endl ;
	//std::cout << ("-l/--lighting \t- enable default lighting (if no lights in scene)") << std::endl ;
	std::cout << ("-c/--copy \t\t- copy all media to the target directory (cannot be combined with --embed)") << std::endl ;
	std::cout << ("-e/--embed \t\t- embed all resources as Data URIs (cannot be combined with --copy)") << std::endl ;
	std::cout << ("-h/--help \t\t- this message") << std::endl ;
	std::cout << ("-v/--version \t\t- version") << std::endl ;
}

static struct option long_options [] ={
	{ ("file"), ARG_REQ, 0, ('f') },
	{ ("output"), ARG_REQ, 0, ('o') },
	{ ("name"), ARG_REQ, 0, ('n') },
	{ ("degree"), ARG_NONE, 0, ('d') },
	{ ("transparency"), ARG_NONE, 0, ('t') },
	{ ("lighting"), ARG_NONE, 0, ('l') },
	{ ("copy"), ARG_NONE, 0, ('c') },
	{ ("embed"), ARG_NONE, 0, ('e') },
	{ ("help"), ARG_NONE, 0, ('h') },
	{ ("version"), ARG_NONE, 0, ('v') },

	{ ARG_NULL, ARG_NULL, ARG_NULL, ARG_NULL }
} ;

#if defined(_WIN32) || defined(_WIN64)
int _tmain (int argc, _TCHAR *argv []) {
#else
int main (int argc, char *argv []) {
#endif
	bool bLoop =true ;
	std::string inFile ;
	std::string outDir ;
	std::string name ;
	bool angleInDegree =false ;
	bool reverseTransparency =false ;
	bool defaultLighting =false ;
	bool copyMedia =false ;
	bool embedMedia =false ;
	while ( bLoop ) {
		int option_index =0 ;
		// http://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html
		// http://www.gnu.org/software/libc/manual/html_node/Argp-Examples.html#Argp-Examples
		// http://stackoverflow.com/questions/13251732/c-how-to-specify-an-optstring-in-the-getopt-function
		int c =getopt_long (argc, argv, ("f:o:n:tlcehv"), long_options, &option_index) ;
		// Check for end of operation or error
		if ( c == -1 )
			break ;
		
		// Handle options
		switch ( c ) {
			case 0:
				break ;
			case ('?'):
				// getopt_long already printed an error message.
				break ;
			case (':'): // missing option argument
				std::cout << ("option \'") << optopt << ("\' requires an argument") << std::endl ;
				break ;
			default:
				bLoop =false ;
				break ;
				
			case ('h'): // help message
				usage () ;
				return (0) ;
			case ('f'): // file to convert to glTF [string]
				inFile =optarg ;
				break ;
			case ('o'): // path of output directory argument [string]
				outDir =optarg ;
				break ;
			case ('n'): // override the scene name [string]
				name =optarg ;
				break ;
			case ('d'): // invert transparency
				angleInDegree =true ;
				break ;
			case ('t'): // invert transparency
				reverseTransparency =true ;
				break ;
			case ('l'): // enable default lighting (if no lights in scene)
				defaultLighting =true ;
				break ;
			case ('c'): // copy all media to the target directory (cannot be combined with --embed)
				copyMedia =!embedMedia ;
				break ;
			case ('e'): // embed all resources as Data URIs (cannot be combined with --copy)
				embedMedia =!copyMedia ;
				break ;
		}
	}
#if defined(_WIN32) || defined(_WIN64)
	if ( inFile.length () == 0  || _taccess_s (inFile.c_str (), 0) == ENOENT )
		return (-1) ;
#else
//	if ( inFile.length () == 0  || access (inFile.c_str (), 0) == ENOENT )
//		return (-1) ;
#endif
	if ( outDir.length () == 0 )
		outDir = (FbxPathUtils::GetFolderName ( (inFile).c_str ()).Buffer ()) ;
#if defined(_WIN32) || defined(_WIN64)
	if ( outDir [outDir.length () - 1] != ('\\') )
		outDir +=('\\') ;
#else
	if ( outDir [outDir.length () - 1] != ('/') )
		outDir +=('/') ;
#endif
	
	std::shared_ptr <gltfPackage> asset (new gltfPackage ()) ;
	asset->ioSettings (name.c_str (), angleInDegree, reverseTransparency, defaultLighting, copyMedia, embedMedia) ;

	std::cout << ("Loading file: ") << inFile << ("...") << std::endl ;
	bool bRet =asset->load (inFile) ;
	std::cout << ("Converting to GLTF ...") << std::endl ;
	bRet =asset->save (outDir) ;
	std::cout << ("done!") << std::endl ;
	return (0) ;
}

