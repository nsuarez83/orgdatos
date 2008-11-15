#include "FileSystem.h"
#include "Constant.h"
#include "Exception/eFile.h"
#include "Date.h"
#include "MessageManager.h"
#include "../DataAccess/Registries/ImgRegistry.h"
#include "../DataAccess/Files/ExtensibleRelativeFile.h"

using namespace std;

vector<string> FileSystem::GetFiles(const char* path, FSMode mode)
{
	 DIR *pdir;
	 struct dirent *pent;
	 struct stat stFileInfo;
	 char szFullName[256];
	 vector<string> fileList;

	 pdir=opendir(path);
	 if (!pdir)
	 {
		 cerr << ERR_PATH_NOT_EXIST << "\"" << path << "\"" << "\n" << flush;
		 return fileList;
	 }
	 errno=0;
	 while ((pent=readdir(pdir)))
	 {
		  sprintf(szFullName, "%s/%s", path, pent->d_name);

		  if (lstat(szFullName, &stFileInfo) < 0)
			  perror ( szFullName );

		  if (S_ISREG(stFileInfo.st_mode) && (mode==All || mode==File ))
		  {
			  string file(pent->d_name);
			  fileList.push_back(file);
		  }

		  if (S_ISDIR(stFileInfo.st_mode) && (mode==All || mode==Dir ))
  		  {
  			  string file(pent->d_name);
  			  fileList.push_back(file);
  		  }

		  if (S_ISLNK(stFileInfo.st_mode) && mode==All)
		  {
			  string file(pent->d_name);
			  fileList.push_back(file);
		  }
	 }
	 if (errno){
		 cerr << ERR_FILE_ERROR << path << "\n" << flush;
		 closedir(pdir);
		 return fileList;
	 }
	 closedir(pdir);
	 return fileList;
}

bool FileSystem::IsValidStructure(){
	return true;
}

void FileSystem::CreatePass(const Message& msg){
	MessageManager::Hide(msg,Message(Constant::PassFile.c_str()));
	ImgRegistry reg;
	ExtensibleRelativeFile fImg(Constant::ImgFile.c_str(), ImgRegistry::RegCreate);
	struct tm* clock;				// create a time structure
	struct stat attrib;			// create a file attribute structure
	stat(Constant::PassFile.c_str(), &attrib);		// get the attributes of PassFile
	clock = gmtime(&(attrib.st_mtime));	// Get the last modified time and put it into the time structure
	Date d(clock->tm_year,clock->tm_mon,clock->tm_mday,clock->tm_hour,clock->tm_min,clock->tm_sec);
	reg.setDate(d);
	fImg.Open(ExtensibleRelativeFile::WRITE);
	fImg.Write(reg);
	fImg.Close();
}

void FileSystem::CreateStruture(const Message& pass){
	ImgRegistry reg;
	ExtensibleRelativeFile fImg(Constant::ImgFile.c_str(), ImgRegistry::RegCreate);
	fImg.Create(reg.GetSize());

	FileSystem::CreatePass(pass);
}
