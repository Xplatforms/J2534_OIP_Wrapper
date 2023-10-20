#ifndef J2534GLOBAL_H
#define J2534GLOBAL_H

#include <map>
#include <stdexcept>
#include <string>
#include <memory>
#include <functional>
#include "Exj2534WrapperInterface.h"

typedef std::function<ExJ2534WrapperInterface *()> run_func;
// The factory
class ExJ2534WrapperFactory {
public:

    void registerGenerator(const std::string& key, run_func func) {
        m_generators[key] = func;
    }

    ExJ2534WrapperInterface * create(const std::string& key) {
        auto it = m_generators.find(key);
        if (it != m_generators.end()) {
            return it->second();
        }
        return nullptr;
    }

private:
    std::map<std::string, run_func > m_generators;
};


class J2534Global
{
public:
    static J2534Global * obj();

    ExJ2534WrapperInterface * getInterface();
    void setInterface(ExJ2534WrapperInterface *);

    ExJ2534WrapperFactory * factory(){return this->_p_factory;}


private:
    explicit J2534Global();
    ~J2534Global() ;

private:
    static J2534Global *        _global_J2534Global_singletone;
    ExJ2534WrapperInterface *   _global_j2534_interface;

    ExJ2534WrapperFactory *     _p_factory;
};

#endif // J2534GLOBAL_H
