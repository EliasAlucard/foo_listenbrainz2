#pragma once
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

#include "resource.h"
#include <foobar2000/helpers/foobar2000+atl.h>
#include <foobar2000/helpers/atl-misc.h>
#include <json.hpp>
using json = nlohmann::json;

#include "foo_listenbrainz2.h"
#include "preferences.h"
