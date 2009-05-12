/*
 * WordsManager.h
 *
 */

#ifndef WORDSMANAGER_H_
#define WORDSMANAGER_H_

#include "../DataAccess/Organizations/OrgExtensibleRelative.h"
#include "../DataAccess/Registries/WordRegistry.h"
#include "../DataAccess/Organizations/OrgList.h"
#include "../DataAccess/Registries/ListSetsRegistry.h"
#include <list>
#include "Constants.h"
#include "Resource.h"

using std::list;

class WordsManager {
public:

	WordsManager* getInstance();

	~WordsManager();

	/*Agrega una nuevo registro a orgWords*/
	ID_type addWord();

	/*Agrega un nuevo nodo a la lista de sets de una palabra*/
	void addSetToWord(ID_type idWord, ID_type idSet);

	/*Devuelve la lista de sets de una palabra*/
	list<ID_type>* getSets(ID_type idWord);


private:

	/*Constructor Privado*/
	WordsManager();

	/*Puntero a la unica instancia de WordsManager*/
	static WordsManager* instance;

	OrgExtensibleRelative orgWords;
	OrgList orgListSets;

};

#endif /* WORDSMANAGER_H_ */
