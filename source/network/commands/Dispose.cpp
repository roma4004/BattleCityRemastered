#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../../headers/network/commands/Dispose.h"

BOOST_CLASS_EXPORT_IMPLEMENT(Dispose);

Dispose::Dispose(): Command(CommandType::DISPOSE) {}

Dispose::Dispose(const std::string& who, const int id)
: Command(CommandType::DISPOSE), _who(who), _id(id) {}

const std::string& Dispose::GetWho() const { return _who; }

int Dispose::GetId() const { return _id; }

const char* Dispose::GetClassNameW() const { return "Dispose"; }
