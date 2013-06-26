#ifndef Ne_Database_h__
#define Ne_Database_h__

#include <string>
#include <map>
#include <vector>

class Ne_Database
{
public:
   typedef std::map<std::string, std::string > Values;
   typedef Values::const_iterator const_iterator;

   Ne_Database(const std::string& appName) : appName_(appName) {}
   Ne_Database(const std::string& appName, const std::string& str);

   void set(const std::string& key, const std::string& value);
   void set(const std::string& keyValue);
   void put(const std::string& key, const std::string& value) { set(key, value); }
   bool is_set(const std::string& key) const;

   const std::string& get_value(const std::string& key) const;
   int get_int_value(const std::string& key) const;
   bool get_bool_value(const std::string& key) const;

   void remove(const std::string& key);
   const_iterator find(const std::string& key);

   const_iterator begin() const { return values_.begin(); }
   const_iterator end() const { return values_.end(); }
   
   void merge( const Ne_Database& db );

private:
   Values values_;
   std::string appName_;
};

#endif // Ne_Database_h__
