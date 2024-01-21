#ifndef __JSONDECODE_H__
#define __JSONDECODE_H__
#include <spot/base/MqStruct.h>
using namespace spot::base;
namespace spot
{
	namespace base
	{
		static void decodeFields(spotrapidjson::Value &arrayValue, map<string, setMethod> &mapMeth)
		{
			for (auto bodyiter = arrayValue.MemberBegin(); bodyiter != arrayValue.MemberEnd(); ++bodyiter)
			{
				auto map_find = mapMeth.find(bodyiter->name.GetString());
				if (map_find != mapMeth.end())
				{
					if (bodyiter->value.IsString())
					{
						const char *value = bodyiter->value.GetString();
						map_find->second((void *)value, bodyiter->value.GetStringLength());
					}
					else if (bodyiter->value.IsDouble())
					{
						double value = bodyiter->value.GetDouble();
						map_find->second(static_cast<void *>(&value), 8);
					}
					else if (bodyiter->value.IsInt())
					{
						int value = bodyiter->value.GetInt();
						map_find->second(static_cast<void *>(&value), 0);
					}
					else if (bodyiter->value.IsUint64())
					{
						uint64_t value = bodyiter->value.GetUint64();
						map_find->second(static_cast<void *>(&value), 4);
					}
				}
			}
		}
	}
}
#endif