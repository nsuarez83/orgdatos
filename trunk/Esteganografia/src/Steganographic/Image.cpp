///////////////////////////////////////////////////////////
//  Image.cpp
//  Implementation of the Class Image
//  Created on:      13-Oct-2008 2:49:31 AM
//  Original author: zebas
///////////////////////////////////////////////////////////

#include "Image.h"


Image::Image(){

}

Image::Image(const char* filePath)
{
	this->filePath = filePath;
}

Image::~Image(){

}

bool Image::Load()
{
	return false;
}
