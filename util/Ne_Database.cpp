#include "Ne_Database.h"

#include "../util/utils.h"

#include <cstdlib>
#include <sstream>
#include <iostream>

namespace
{
   const std::string EmptyString;
}

std::vector<std::string> Split(const std::string& str)
{
   std::vector<std::string> tmp;
   std::string::size_type pos = 0;
   std::string::size_type begin = 0;
   std::string::size_type escape = 0;
   while ((pos = str.find("\\n", escape)) != std::string::npos)
   {
      if (pos != 0 && (str[pos-1]=='\\'))
      {
         escape = pos + 2;
         continue;
      }
      std::string value=str.substr(begin, pos - begin);
      tmp.push_back(value);
      begin = pos + 2;
      escape = begin;
   }
   std::string value = str.substr(begin);
   if (!value.empty())
      tmp.push_back(value);
   return tmp;
}

Ne_Database::Ne_Database(const std::string& appName, const std::string& str)
   : appName_(appName)
{
   set(str);
}

void Ne_Database::set( const std::string& key, const std::string& value )
{
   values_[key] = value;
}

void Ne_Database::set(const std::string& str)
{
   std::istringstream iss(str);

   enum Mode {KEYVALUE, VALUE};
   Mode mode = KEYVALUE;
   std::string key;
   std::string value;
   for (std::string line; std::getline(iss, line, '\n');)
   {
      if (line.empty() || line[0] == '!')
         continue;
      if (mode == KEYVALUE)
      {
         if (!key.empty() && !value.empty())
         {
            for(std::string::size_type i = 0; i < value.size(); ++i)
            {
               if (value[i] == '\\')
               {
                  if (value[i+1] == 'n') value[i] = '\n';
                  else if (value[i+1] == '\\') value[i] = '\\';
                  else throw "Invalid escape sequence [" + value + "]";

                  value.erase(i+1, 1);
               }
            }
            values_[key] = value;
         }

         std::string::size_type keySeparatorPos = line.find(":");
         if (keySeparatorPos == std::string::npos)
         {
            std::cerr << "Invalid file format : a KeyValue without : [" << line << "]"<< std::endl;
            break;
         }

         key = Trim(line.substr(0, keySeparatorPos));
         value = Trim(line.substr(keySeparatorPos + 1), " ");
         if (!value.empty() && *value.rbegin() == '\\')
         {
            value.erase(value.size()-1);
            mode = VALUE;
         }
      }
      else if (mode==VALUE)
      {
         value += line;
         if (!value.empty() && *value.rbegin() == '\\')
            value.erase(value.size()-1);
         else
            mode = KEYVALUE;
      }
   }

   if (!key.empty() && !value.empty())
   {
      for(std::string::size_type i = 0; i < value.size(); ++i)
      {
         if (value[i] == '\\')
         {
            if (value[i+1] == 'n') value[i] = '\n';
            else if (value[i+1] == '\\') value[i] = '\\';
            else throw "Invalid escape sequence [" + value + "]";

            value.erase(i+1, 1);
         }
      }
      values_[key] = value;
   }

      values_[key] = value;
}

bool Ne_Database::is_set(const std::string& key) const
{
   return (values_.find(key) != values_.end());
}

const std::string& Ne_Database::get_value(const std::string& key) const
{
   Values::const_iterator found = values_.find(key);
   if (found == values_.end())
      return EmptyString;
   return found->second;
}

int Ne_Database::get_int_value(const std::string& key) const
{
   return std::atoi(Trim(get_value(key)).c_str());
}

bool Ne_Database::get_bool_value(const std::string& key) const
{
   return (Trim(get_value(key)) == "true");
}

void Ne_Database::remove(const std::string& key)
{
   values_.erase(key);
}

Ne_Database::const_iterator Ne_Database::find(const std::string& key)
{
   return values_.find(key);
}

void Ne_Database::merge( const Ne_Database& db )
{
   for( Values::const_iterator kp = db.begin(); kp != db.end(); ++kp)
      this->set(kp->first, kp->second);
}
