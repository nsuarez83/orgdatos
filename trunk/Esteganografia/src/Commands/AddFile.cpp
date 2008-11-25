#include "AddFile.h"

using std::cout;

AddFile::AddFile(string cmd) : Command(cmd)
{

}

AddFile::AddFile() : Command(" ")
{

}

AddFile::~AddFile()
{
}

/**
 * Muestra la ayuda del comando.
 */
void AddFile::ShowHelpCommand()
{
	cout << HLP_ADD_FILE << "\n";
}

/**
 * Ejecuta el comando correspondiente.
 */
bool AddFile::InternalProcess(tVecStr params)
{
	std::string pathMessage = params[1];
	MessageManager *messageManager = MessageManager::GetInstance();
	try
	{
		std::cout << PROCESS_COMMAND;
		if( FileSystem::IsDirectory(pathMessage.c_str()) )
			throw eNotExist(MSG_NOT_MSG);
		messageManager->Hide(pathMessage);
		std::cout << MSG_HIDE_SUCCESS;
	}
	catch(eFile &e){
		cout << EXC_EFILE << e.what() << "\n";
	}
	catch(exception &e){
		cout << e.what() << "\n";
	}
	return true;
}
