#include "j2534global.h"
#include "j2534simplelogger.h"
#include <windows.h>
#include <memory>
#include <string>

J2534Global * J2534Global::_global_J2534Global_singletone = nullptr;

J2534Global::J2534Global()
{
    this->_p_factory = new ExJ2534WrapperFactory();
    this->_global_j2534_interface = nullptr;
}

ExJ2534WrapperInterface * J2534Global::getInterface()
{
    if(this->_global_j2534_interface == nullptr)this->setInterface(new ExJ2534SimpleLogger());
    return this->_global_j2534_interface;
}

void J2534Global::setInterface(ExJ2534WrapperInterface * interface)
{
    this->_global_j2534_interface = interface;
}

J2534Global * J2534Global::obj()
{
    if(J2534Global::_global_J2534Global_singletone == nullptr)J2534Global::_global_J2534Global_singletone = new J2534Global();
    return J2534Global::_global_J2534Global_singletone;
}
