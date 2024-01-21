#ifndef SPOT_GATEWAY_READINI_H
#define SPOT_GATEWAY_READINI_H

#include <spot/utility/NonCopyAble.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <vector>
const int maxItemNameLen = 30;
const int maxItemValueLen = 500;
namespace spot
{
namespace utility
{
class IniItem
{
public:
	IniItem(char *name, char *value);
	virtual ~IniItem(void);

	char *getName(void);
	char *getValue(void);

private:
	char name_[maxItemNameLen];
	char value_[maxItemValueLen];

};
class ReadIni :public utility::NonCopyable
{
public:
  ReadIni(const char *iniFilename);
	virtual ~ReadIni(void);

    virtual char *getIni(const char *name);
	inline size_t itemSize() { return items_.size(); }
	virtual bool itemExists(const char *name);

private:
  typedef std::vector< std::shared_ptr<IniItem>> ItemVec;
  ItemVec items_;
  
};
class is_equal
{
public:
   is_equal (const char* name)
   {
     memset(name_, 0, sizeof(name_));
     memcpy(name_, name, sizeof(name_));
   }

   bool operator() (std::shared_ptr<IniItem> &item) const
   {
      if(strcmp(item->getName(), name_) == 0)
      {
        return true;
      }
      else
      {
        return false;
      }
   }

private:
   char name_[maxItemNameLen];
};
extern ReadIni *currentIni;
}
}
#endif