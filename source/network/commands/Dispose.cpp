#include "network/commands/Dispose.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(Dispose);

Dispose::Dispose(): Command(CommandType::DISPOSE) {}

Dispose::Dispose(std::string who, buuid uuid)
	: Command(CommandType::DISPOSE), _who(std::move(who)), _uuid(std::move(uuid)) {}

std::string Dispose::GetWho() const noexcept { return _who; }
using buuid = boost::uuids::uuid;
buuid Dispose::GetUuid() const noexcept { return _uuid; }

const char* Dispose::GetClassNameW() const noexcept { return "Dispose"; }
