#include <spot/utility/ReadIni.h>
#include <spot/utility/Compatible.h>
#include <stdio.h>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <assert.h>
#include "spot/utility/Logging.h"

using namespace spot::utility;
using namespace std::placeholders;
IniItem::IniItem(char *name, char *value)
{
  
  assert(strlen(name) < maxItemNameLen);
  assert(strlen(value) < maxItemValueLen);
  memcpy(name_,name, sizeof(name_)-1);
  memcpy(value_,value, sizeof(value_)-1);
}

IniItem::~IniItem(void)
{
}

char *IniItem::getName(void)
{
	return name_;
}

char *IniItem::getValue(void)
{
	return value_;
}

ReadIni::ReadIni(const char *iniFilename)
{
	FILE *configFile;
	char *pLast=NULL;

	const int BUF_SIZE = 1024;
	char buffer[BUF_SIZE];

	configFile=fopen(iniFilename,"rt");

	if (configFile==NULL)
	{
		return;
	}

	while (fgets(buffer,BUF_SIZE,configFile)!=NULL)
	{
		char *name,*value;
		if (STRTOK(buffer,"\n\r",&pLast)==NULL)
			continue;
		if (buffer[0]=='#')
			continue;
		name=STRTOK(buffer,"\n\r=",&pLast);
		value=STRTOK(NULL,"\n\r",&pLast);
		if (name==NULL)
		{
			continue;
		}
		items_.push_back(std::shared_ptr<IniItem>(new  IniItem(name,value)));
	}
	fclose(configFile);
}


ReadIni::~ReadIni(void)
{
}

char *ReadIni::getIni(const char *name)
{
  ItemVec::iterator it = std::find_if(items_.begin(), items_.end(), is_equal(name));
  if (it != items_.end())
  {
    return (*it)->getValue();
  }
	std::string errStr = "ReadIni::getIni cann't find " + std::string(name);
	LOG_ERROR << errStr.c_str();
	return nullptr;
}

bool ReadIni::itemExists(const char *name)
{
	ItemVec::iterator it = std::find_if(items_.begin(), items_.end(), is_equal(name));
	if (it != items_.end())
	{
		return true;
	}
	return false;
}