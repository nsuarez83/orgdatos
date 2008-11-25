#include "MessageManager.h"
#include <string>

MessageManager* MessageManager:: instance = NULL;
/* -------------------------------------------------------------------------- */

MessageManager::MessageManager(): orgMsg(PATH_MESSAGE_FILE, MsgRegistry::Create),
								  orgListImages(PATH_IMG_LIST_FILE, ListImgRegistry::Create),
								  orgNames(PATH_NAMES_MSG_PATHFILE, PATH_NAMES_MSG_FILE),
								  treeMsg(512, KeyStrFactory(), ValueIntFactory(), PATH_TREE_MSG)
{

}
/* -------------------------------------------------------------------------- */

MessageManager* MessageManager::GetInstance()
{
	if(instance == NULL)
		instance = new MessageManager();
	return instance;
}
/* -------------------------------------------------------------------------- */

MessageManager::~MessageManager(){

}
/* -------------------------------------------------------------------------- */

void MessageManager::Extract(std::string nameMsg, std::string pathMsg, Message msgTarget){

	std::string fullPath(pathMsg + "/" + nameMsg);
	Message msg(fullPath);
	
	//Busco en el arbol el id del mensaje.
	if( this->treeMsg.empty() )
		throw eNotExist(ERR_MSG_NOT_EXIST);
	ID_type idMsg;
	KeyStr key(msg.GetName());
    if(!treeMsg.exists(key)){
		throw eNotExist(ERR_MSG_NOT_EXIST);
	}
	TreeIterator& itTree = this->treeMsg.iterator(key);
	if( itTree.end() )
	{
		treeMsg.deleteIterator(itTree);
		throw eNotExist(ERR_MSG_NOT_EXIST);
	}

	//Verifico si existe el directorio de salida
	if( !FileSystem::ExistDirectory(pathMsg.c_str()) )
	{
		treeMsg.deleteIterator(itTree);
		throw eNotExist(ERR_PATH_NOT_EXIST + pathMsg);
	}
	
	ValueInt* vInt=dynamic_cast<ValueInt*>(itTree.getValue());
	idMsg = vInt->getValue();
	treeMsg.deleteIterator(itTree);
	delete vInt;
	
	try
	{
		//Leo el registro del archivo mensaje
		MsgRegistry *msgRegistry = dynamic_cast<MsgRegistry*>(this->orgMsg.GetRegistry( idMsg));
	
		if( msgRegistry == NULL)
			throw eFile(PATH_MESSAGE_FILE);
		
		ID_type idFirstList = msgRegistry->GetPtrImgList();
		//Extraigo el mensaje de las imagenes
		ExtractMessage(idFirstList, msgTarget);
	}
	catch (char *error)
	{
		throw eFile(PATH_MESSAGE_FILE); 
	}
	Message m1=EncriptationManager::Decrypt(msgTarget);
	CompressionManager::Decompress(m1,msg);
	m1.Delete();
	msgTarget.Delete();
}

/* -------------------------------------------------------------------------- */

void MessageManager::ExtractMessage(ID_type idFirstList, Message &msgTarget)
{
	//Obtengo la lista de espacios de las imagenes en donde esta oculto el mensaje
	try
	{
		std::list<ListRegistry*> *listSpacesImg = this->orgListImages.GetList(idFirstList);
		if( listSpacesImg == NULL)
			throw eFile(PATH_IMG_LIST_FILE);
		
		std::list<ListRegistry*>::iterator it;
		ListImgRegistry* listImgRegistry;
		Space *space;
		Image *image;
		ImageManager *imageManager = ImageManager::GetInstance();
		
		//Por cada elemento de la lista, instancio un Space, una imagen y extraigo la particion del mensaje
		for( it = listSpacesImg->begin(); it != listSpacesImg->end(); it++ )
		{
			listImgRegistry = dynamic_cast<ListImgRegistry*>(*it);
			ID_type idImg = listImgRegistry->GetIDImage();
			unsigned long offsetImg = listImgRegistry->GetOffsetImg();
			unsigned long sizePartitionMsg = listImgRegistry->GetSizePartitionMsg();
			string pathImg = imageManager->GetPathImage(idImg);
			space = new Space(pathImg, offsetImg, sizePartitionMsg);
			image = ImageFactory::GetImage(pathImg.c_str());
			image->Extract(space, &msgTarget);
			delete image;
			delete (*it);
		}
		delete listSpacesImg;
	}
	catch (char* error)	{
		throw eFile(PATH_IMG_LIST_FILE);
	}
}
/* -------------------------------------------------------------------------- */

void MessageManager::Hide(Message msg,Message msgTarget){

	//Verifico si existe un mensaje oculto con el mismo nombre
	KeyStr k( msg.GetName());
	if( !this->treeMsg.empty() )
		if( this->treeMsg.exists(k) )
			throw eExist(ERR_ALREADY_EXIST);
	//Comprimo el mensaje
	Message m1;
	try{
		m1=CompressionManager::Compress(msg);
	}
	catch(eFile &e){
		throw eNotExist(ERR_FILE_OPEN + string(msg.GetFilePath()));
	}

	//Obtengo la lista de espacios libres en donde voy a almacenar el mensaje
	FreeSpaceManager *freeSpaceManager = FreeSpaceManager::GetInstance();
	tListSpaces *spaces = freeSpaceManager->GetFreeSpaces(m1.GetSize());

	//Si la lista es NULL, no hay espacio disponible
	if( spaces == NULL || spaces->empty() )
		throw eExist(ERR_NOT_SPACE);

	//Encripto el mensaje
	EncriptationManager::Encrypt(m1, msgTarget);


	//Oculto el mensaje en las imagenes
	ID_type idFirstList = HideMessage(spaces, msgTarget);
	
	ID_type idName = orgNames.WriteText( string(msg.GetName()) );
	
	//Guardo el registro en la orgMsg
	MsgRegistry regMsg(idName, idFirstList);
	this->orgMsg.WriteRegistry(regMsg);

	//Guardo el registro en el arbol
	ValueInt v(regMsg.GetID());
	this->treeMsg.insert(k,v);

	//Actualizo la lista de mensajes que tiene cada registro en el archivo de imagenes
	UpdateListMessage(regMsg);

	//Elimino los archivos de mensajes
	msg.Delete();
	m1.Delete();
	msgTarget.Delete();
}

/* -------------------------------------------------------------------------- */


ID_type MessageManager::HideMessage(tListSpaces *spaces, Message &msgTarget)
{
	ImageManager *imageManager = ImageManager::GetInstance();
	Space *space;
	Image *image;
	ID_type idImage=0, idFirstList=0;
	unsigned int bitsLsb=0;
	unsigned long sizeHidden = 0, sizeSpace;
	tListSpaces::iterator it = spaces->begin();

	while( it != spaces->end())
	{
		space = dynamic_cast<Space*>(*it);

		//Oculto el mensaje en el esapcio libre
		image = ImageFactory::GetImage( space->GetFilePath());
		image->Hide(space, &msgTarget);

		//Obtengo los datos para alamcenar el registro
		sizeSpace = space->GetSize();
		sizeHidden += sizeSpace;
		idImage = imageManager->GetIDImage(space->GetFilePath());
		bitsLsb = imageManager->GetBitsLsb(space->GetFilePath());

		//Si el space es el primero de la lista, debo crear una lista en la organizacion lista
		if( it == spaces->begin() )
		{
			ListImgRegistry regList( idImage, space->GetInitialPosition(), msgTarget.GetHiddenSize()*(8/bitsLsb) );
			this->orgListImages.CreateList(regList);
			idFirstList = regList.GetID();
			it++;
		}
		else
		{
			/*Verifico si el ultimo space de la lista fue ocupado por completo.
			 * Si no se ocupo por completo, calculo el espacio usado*/
			it++;
			if( (it == spaces->end()) && (sizeHidden > msgTarget.GetSize()) )
				sizeSpace = space->GetSize() - (sizeHidden - msgTarget.GetSize());

			//Guardo el registro en la orgListImages
			ListImgRegistry regList( idImage, space->GetInitialPosition(), sizeSpace*(8/bitsLsb) );
			this->orgListImages.AddToListLast(regList,idFirstList);
		}
		delete image;
		delete space;
	}
	delete spaces;
	return idFirstList;
}
/* -------------------------------------------------------------------------- */

void MessageManager::UpdateListMessage(MsgRegistry &regMsg)
{
	ImageManager *imageManager = ImageManager::GetInstance();
	std::list<ListRegistry*> *listImages = this->orgListImages.GetList( regMsg.GetPtrImgList());
	std::list<ListRegistry*>::iterator itListImg;
	for( itListImg=listImages->begin(); itListImg != listImages->end(); itListImg++ )
	{
		ListImgRegistry* reg = dynamic_cast<ListImgRegistry*>(*itListImg);
		imageManager->AddMessageToImage( reg->GetIDImage(), regMsg.GetID());
		delete reg;
	}
	delete listImages;
}
/* -------------------------------------------------------------------------- */

void MessageManager::DeleteMessage(ID_type idMsg, bool addFreeSpace)
{
	MsgRegistry *msgRegistry = dynamic_cast<MsgRegistry*>(orgMsg.GetRegistry(idMsg));
	if( msgRegistry == NULL )
		throw eFile(PATH_MESSAGE_FILE);

	std::string nameMsg = orgNames.GetText(idMsg);
	if( nameMsg.length() == 0 )
		throw eFile(PATH_NAMES_MSG_FILE);
	KeyStr k(nameMsg);
	ID_type idFirstList = msgRegistry->GetPtrImgList();
	if(addFreeSpace)
		AddFreeSpaces(orgListImages.GetList(idFirstList));
	//Elimino los registros
	orgListImages.DeleteList(idFirstList);
	orgNames.DeleteText(msgRegistry->GetIdName());
	orgMsg.DeleteRegistry(idMsg);
	treeMsg.remove(k);
	delete msgRegistry;
}
/* -------------------------------------------------------------------------- */


void MessageManager::DeleteMessage(std::string nameMessage, bool addFreeSpace)
{
	//Verifico si el mensaje existe
	if( treeMsg.empty() )
		throw eNotExist(ERR_MSG_EMPTY);
	KeyStr k(nameMessage);
	ValueInt *regTree = dynamic_cast<ValueInt*>(treeMsg.find(k));
	if( regTree == NULL )
		throw eNotExist(ERR_MSG_NOT_EXIST);
	ID_type idMsg = regTree->getValue();
	delete regTree;

	MsgRegistry *msgRegistry = dynamic_cast<MsgRegistry*>(orgMsg.GetRegistry(idMsg));
	if( msgRegistry == NULL )
		throw eNotExist(ERR_MSG_NOT_EXIST);

	ID_type idFirstList = msgRegistry->GetPtrImgList();
	if(addFreeSpace)
		AddFreeSpaces(orgListImages.GetList(idFirstList));

	//Elimino los registros
	orgListImages.DeleteList(idFirstList);
	orgNames.DeleteText(msgRegistry->GetIdName());
	orgMsg.DeleteRegistry(idMsg);
	treeMsg.remove(k);
	delete msgRegistry;
}
/* -------------------------------------------------------------------------- */


void MessageManager::DeleteMessages(std::list<ID_type>* listImg) 
{
	std::list<ID_type>::iterator it;
	for( it = listImg->begin(); it != listImg->end(); it++)
		DeleteMessage( *it, false );
}
/* -------------------------------------------------------------------------- */


void MessageManager::AddFreeSpaces(std::list<ListRegistry*> *listImg)
{
	if( listImg == NULL )
		throw eNotExist(PATH_IMG_LIST_FILE);
	std::list<ListRegistry*>::iterator it;
	ListImgRegistry* listImgRegistry;
	tListSpaces* listSpaces = new tListSpaces;
	for( it=listImg->begin(); it != listImg->end(); it++)
	{
		listImgRegistry = dynamic_cast<ListImgRegistry*>(*it);
		ID_type idImg = listImgRegistry->GetIDImage();
		unsigned long offsetImg = listImgRegistry->GetOffsetImg();
		unsigned long size = listImgRegistry->GetSizePartitionMsg();
		listSpaces->push_back(new Space(idImg, offsetImg, size));
		delete (*it);
	}
	//Doy de alta los nuevos espacios libres
	FreeSpaceManager::GetInstance()->AddFreeSpaces(listSpaces);
	
	tListSpaces::iterator itSpaces;
	for(itSpaces = listSpaces->begin(); itSpaces != listSpaces->end(); itSpaces++ )
		delete *itSpaces;
	delete listImg;
}
/* -------------------------------------------------------------------------- */


void MessageManager::ShowMessage()
{
	int hiddenSize = 0;
	if(treeMsg.empty())
	{
		std::cout << std::endl << ERR_MSG_EMPTY << std::endl;
		return;
	}
	TreeIterator &it = treeMsg.first();

	std::cout << std::endl;
	while( !it.end() )
	{
		std::pair<Register*,Register*>keyval= *it;
		KeyStr* key = dynamic_cast<KeyStr*>(keyval.first);
		std::cout << CIRCLE << " " << key->getKey() << std::endl;
		delete key;
		++it;
		hiddenSize++;
	}
	std::cout << std::endl;
	std::cout << LBL_HIDDEN_MSG_SIZE << hiddenSize << std::endl;
	treeMsg.deleteIterator(it);
}
/* -------------------------------------------------------------------------- */

