#include "../../../headers/network/commands/Dispose.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(Dispose);

Dispose::Dispose(): Command(CommandType::DISPOSE) {}

Dispose::Dispose(const std::string& who, const boost::uuids::uuid uuid)
	: Command(CommandType::DISPOSE), _who(who), _uuid(uuid) {}

const std::string& Dispose::GetWho() const { return _who; }

boost::uuids::uuid Dispose::GetUuid() const { return _uuid; }

const char* Dispose::GetClassNameW() const { return "Dispose"; }
