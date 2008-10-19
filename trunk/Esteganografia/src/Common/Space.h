///////////////////////////////////////////////////////////
//  Space.h
//  Implementation of the Class Space
//  Created on:      13-Oct-2008 2:49:36 AM
//  Original author: zebas
///////////////////////////////////////////////////////////

#if !defined(EA_B792C72C_98EA_11dd_B49B_001B2425640C__INCLUDED_)
#define EA_B792C72C_98EA_11dd_B49B_001B2425640C__INCLUDED_
#include <string>
#include <fstream>
#include <iostream>
using namespace std;
/**
 * Unidad de espacio del cual se puede recuperar o almacenar infomacion.
 */
class Space
{

public:
	Space(string filePath, string format, long initialPosition, long size);
	Space(string filePath);
	virtual ~Space();

	const char* GetFilePath() const;
	string GetFormat() const;
	long GetInitialPosition() const;
	long GetSize() const;
	long GetTotalSize() const;
	void SetSize(long size);
	void SetInitialPosition(long position);

private:
	string filePath;
	string format;
	long initialPosition;
	long size;

};
#endif // !defined(EA_B792C72C_98EA_11dd_B49B_001B2425640C__INCLUDED_)
