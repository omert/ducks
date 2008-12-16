#include <string>
#include "Utils.hpp"

#include <boost/filesystem.hpp>   
#include <boost/filesystem/fstream.hpp>


using namespace std;
using namespace boost::filesystem;

string
pathFrom(const string& full, const string& from)
{
    path fullPath(full);
    path retPath;

    bool foundFrom = false;
    for (path::const_iterator it = fullPath.begin(); 
         it != fullPath.end(); ++it)
    {
        if (*it == path(from))
            foundFrom = true;
        if (foundFrom)
            retPath /= *it;
    }
    return toString(retPath);
}

path
head(const path& p)
{
    return *p.begin();
}
 
path
decapitate(const path& p)
{
    path ret;
    for (path::const_iterator it = p.begin(); it != p.end(); ++it)
        if (it != p.begin())
            ret /= *it;
    return ret;
}

string
toLower(const string& s)
{
    string r(s);
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = tolower(r[i]);
    return r;
}

