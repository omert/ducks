#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <sstream>
#include <boost/filesystem.hpp>   
#include "Identifier.h"

template<class I>
class IteratorRange : public std::pair<I, I> {
public:
    IteratorRange(const std::pair<I, I>& p)
        : std::pair<I, I>(p)
        {}
    IteratorRange(const I& i1, const I& i2)
        : std::pair<I, I>(i1, i2)
        {}
};

template<class I>
class IteratorSequence : public std::vector<IteratorRange<I> > {
};

template<class I>
void
intersect(const IteratorSequence<I>& s1, const IteratorSequence<I>& s2,
          IteratorSequence<I>& rs)
{
    size_t i = 0;
    size_t j = 0;
    while (i < s1.size() && j < s2.size()){
        I begin = max(s1[i].first, s2[j].first);
        I end = min(s1[i].second, s2[j].second);
        if (begin < end)
            rs.push_back(IteratorRange<I>(begin, end));
        if (s1[i].second < s2[j].second)
            ++i;
        else
            ++j;
    }
}

template<class T>
T
fromString(const std::string& s)
{
    std::istringstream iss;
    iss.str(s);
    
    T ret;
    iss >> ret;
    
    return ret;
}

template<class T>
std::string
toString(const T& t)
{
    std::ostringstream oss;
    oss << t;
    
    return oss.str();
}

template<class B>
void
splitString(const std::basic_string<B>& s, 
            const B& seperator,
            std::vector<std::basic_string<B> >& rBits)
{
    rBits.clear();
    
    for (size_t i = 0; i < s.length();)
    {
        size_t j = s.find(seperator, i);
        if (j > i)
            rBits.push_back(s.substr(i, j - i));
        if (j == std::string::npos)
            break;
        i = j + 1;
    }
}

template<class B>
void
splitString(const std::basic_string<B>& s, 
            const std::basic_string<B>& seperators,
            std::vector<std::basic_string<B> >& rBits)
{
    rBits.clear();
    
    for (size_t i = 0; i < s.length();)
    {
        size_t j = s.find_first_of(seperators, i);
        if (j > i)
            rBits.push_back(s.substr(i, j - i));
        if (j == std::string::npos)
            break;
        i = j + 1;
    }
}

template<class Ret, class It>
Ret
perlHash(It begin, It end)
{
    Ret id = 0;
    for (; begin != end; ++begin)
	id = (id + (unsigned char)*begin) * 33;
    return id;
}

class DummyHash32;
typedef Identifier<DummyHash32, 'H', int> Hash32;
template<class It>
Hash32
hash32(It begin, It end)
{
    return Hash32(perlHash<Hash32::BaseType, It>(begin, end));
}

class DummyHash64;
typedef Identifier<DummyHash64, 'H', unsigned long long> Hash64;
template<class It>
Hash64
hash64(It begin, It end)
{
    return Hash64(perlHash<Hash64::BaseType, It>(begin, end));
}


std::string toLower(const std::string& s);


boost::filesystem::path head(const boost::filesystem::path& p);
boost::filesystem::path decapitate(const boost::filesystem::path& p);

#endif //UTILS_HPP
