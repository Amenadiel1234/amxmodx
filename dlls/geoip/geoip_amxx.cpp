<<<<<<< HEAD
<<<<<<< HEAD
// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

//
// GeoIP Module
//

#include "geoip_amxx.h"
#include <am-string.h>
#include <am-vector.h>

MMDB_s HandleDB;
ke::Vector<ke::AString> LangList;

char *stripPort(char *ip)
{
	char *tmp = strchr(ip, ':');

	if (tmp)
	{
		*tmp = '\0';
	}

	return ip;
}

const char* stristr(const char* str, const char* substr)
{
	register char *needle = (char *)substr;
	register char *prevloc = (char *)str;
	register char *haystack = (char *)str;

	while (*haystack)
	{
		if (tolower(*haystack) == tolower(*needle))
		{
			haystack++;

			if (!*++needle)
			{
				return prevloc;
			}
		}
		else 
		{
			haystack = ++prevloc;
			needle = (char *)substr;
		}
	}

	return NULL;
}

bool lookupByIp(const char *ip, const char **path, MMDB_entry_data_s *result)
{
	int gai_error = 0, mmdb_error = 0;
	MMDB_lookup_result_s lookup = MMDB_lookup_string(&HandleDB, ip, &gai_error, &mmdb_error);

	if (gai_error != 0 || MMDB_SUCCESS != mmdb_error || !lookup.found_entry)
	{
		return false;
	}

	MMDB_entry_data_s entry_data;
	MMDB_aget_value(&lookup.entry, &entry_data, path);

	if (!entry_data.has_data)
	{
		return false;
	}

	*result = entry_data;

	return true;
}

const char *lookupString(const char *ip, const char **path, int *length = NULL)
{
	static char buffer[64];
	MMDB_entry_data_s result;

	if (!lookupByIp(ip, path, &result))
	{
		return NULL;
	}

	if (length)
	{
		*length = result.data_size;
	}

	memcpy(buffer, result.utf8_string, result.data_size);
	buffer[result.data_size] = '\0';

	return buffer;
}

double lookupDouble(const char *ip, const char **path)
{
	MMDB_entry_data_s result;

	if (!lookupByIp(ip, path, &result))
	{
		return NULL;
	}

	return result.double_value;
}

int getContinentId(const char *code)
{
	#define CONTINENT_UNKNOWN        0
	#define CONTINENT_AFRICA         1    
	#define CONTINENT_ASIA           2
	#define CONTINENT_EUROPE         3
	#define CONTINENT_NORTH_AMERICA  4
	#define CONTINENT_OCEANIA        5
	#define CONTINENT_SOUTH_AMERICA  6

	int index = CONTINENT_UNKNOWN;

	if (code)
	{
		switch (code[0])
		{
			case 'A':
			{
				switch (code[1])
				{
					case 'F': index = CONTINENT_AFRICA; break;
					case 'S': index = CONTINENT_ASIA; break;
				}
			}
			case 'E': index = CONTINENT_EUROPE; break;
			case 'O': index = CONTINENT_OCEANIA; break;
			case 'N': index = CONTINENT_NORTH_AMERICA; break;
			case 'S': index = CONTINENT_SOUTH_AMERICA; break;
		}
	}

	return index;
}

const char *getLang(int playerIndex)
{
	static cvar_t *amxmodx_language = NULL;
	static cvar_t *amxmodx_cl_langs = NULL;

	if (!amxmodx_language)
		 amxmodx_language = CVAR_GET_POINTER("amx_language");

	if (!amxmodx_cl_langs)
		 amxmodx_cl_langs = CVAR_GET_POINTER("amx_client_languages");

	if (playerIndex >= 0 && amxmodx_cl_langs && amxmodx_language)
	{
		const char *value;
		const char *lang;

		if (playerIndex == 0 || amxmodx_cl_langs->value <= 0 || !MF_IsPlayerIngame(playerIndex))
		{
			value = amxmodx_language->string;
		}
		else
		{
			value = ENTITY_KEYVALUE(MF_GetPlayerEdict(playerIndex), "lang");
		}

		if (value && *value)
		{
			for (size_t i = 0; i < LangList.length(); ++i)
			{
				lang = LangList.at(i).chars();

				if (stristr(lang, value) != NULL)
				{
					return lang;
				}
			}
		}
	}

	return "en";
}


// native geoip_code2(const ip[], ccode[3]);
// Deprecated.
static cell AMX_NATIVE_CALL amx_geoip_code2(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "country", "iso_code", NULL };
	const char *code = lookupString(ip, path);

	return MF_SetAmxString(amx, params[2], code ? code : "error", 3);
}

// native geoip_code3(const ip[], result[4]);
// Deprecated.
static cell AMX_NATIVE_CALL amx_geoip_code3(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "country", "iso_code", NULL };
	const char *code = lookupString(ip, path);

	for (size_t i = 0; i < ARRAYSIZE(GeoIPCountryCode); ++i)
	{
		if (!strncmp(code, GeoIPCountryCode[i], 2))
		{
			code = GeoIPCountryCode3[i];
			break;
		}
	}

	return MF_SetAmxString(amx, params[2], code ? code : "error", 4);
}

// native bool:geoip_code2_ex(const ip[], result[3]);
static cell AMX_NATIVE_CALL amx_geoip_code2_ex(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "country", "iso_code", NULL };
	const char *code = lookupString(ip, path);

	if (!code)
	{
		return 0;
	}

	MF_SetAmxString(amx, params[2], code, 2);

	return 1;
}

// native bool:geoip_code3_ex(const ip[], result[4]);
static cell AMX_NATIVE_CALL amx_geoip_code3_ex(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "country", "iso_code", NULL };
	const char *code = lookupString(ip, path, &length);

	if (!code)
	{
		return 0;
	}

	for (size_t i = 0; i < ARRAYSIZE(GeoIPCountryCode); ++i)
	{
		if (!strncmp(code, GeoIPCountryCode[i], 2))
		{
			code = GeoIPCountryCode3[i];
			break;
		}
	}

	MF_SetAmxString(amx, params[2], code, 3);

	return 1;
}

// native geoip_country(const ip[], result[], len, id = -1);
static cell AMX_NATIVE_CALL amx_geoip_country(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "country", "names", getLang(params[4]), NULL };
	const char *country = lookupString(ip, path, &length);

	if (!country)
	{
		return 0;
	}

	return MF_SetAmxString(amx, params[2], country, length >= params[3] ? params[3] : length); // TODO: make this utf8 safe.
}

// native geoip_city(const ip[], result[], len, id = -1);
static cell AMX_NATIVE_CALL amx_geoip_city(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "city", "names", getLang(params[4]), NULL };
	const char *city = lookupString(ip, path, &length);

	return MF_SetAmxString(amx, params[2], city ? city : "", length >= params[3] ? params[3] : length); // TODO: make this utf8 safe.
}

// native geoip_region_code(const ip[], result[], len);
static cell AMX_NATIVE_CALL amx_geoip_region_code(AMX *amx, cell *params)
{
	int length;
	int finalLength = 0;
	char code[12]; // This should be largely enough to hold xx-yyyy and more if needed.

	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *pathCountry[] = { "country", "iso_code", NULL };
	const char *countryCode = lookupString(ip, pathCountry, &length);

	if (countryCode)
	{
		finalLength = length + 1; // + 1 for dash.
		snprintf(code, finalLength + 1, "%s-", countryCode); // + EOS.

		const char *pathRegion[] = { "subdivisions",  "0", "iso_code", NULL }; // First result.
		const char *regionCode = lookupString(ip, pathRegion, &length);

		if (regionCode)
		{
			finalLength += length;
			strncat(code, regionCode, length);
		}
		else
		{
			finalLength = 0;
		}
	}

	return MF_SetAmxString(amx, params[2], finalLength ? code : "", finalLength >= params[3] ? params[3] : finalLength);
}

// native geoip_region_name(const ip[], result[], len, id = -1);
static cell AMX_NATIVE_CALL amx_geoip_region_name(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "subdivisions", "0", "names", getLang(params[4]), NULL }; // First result.
	const char *region = lookupString(ip, path, &length);

	return MF_SetAmxString(amx, params[2], region ? region : "", length >= params[3] ? params[3] : length); // TODO: make this utf8 safe.
}

// native geoip_timezone(const ip[], result[], len);
static cell AMX_NATIVE_CALL amx_geoip_timezone(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "location", "time_zone", NULL };
	const char *timezone = lookupString(ip, path, &length);

	return MF_SetAmxString(amx, params[2], timezone ? timezone : "", length >= params[3] ? params[3] : length);
}

// native geoip_latitude(const ip[]);
static cell AMX_NATIVE_CALL amx_geoip_latitude(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "location", "latitude", NULL };
	double latitude = lookupDouble(ip, path);

	return amx_ftoc(latitude);
}

// native geoip_longitude(const ip[]);
static cell AMX_NATIVE_CALL amx_geoip_longitude(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "location", "longitude", NULL };
	double longitude = lookupDouble(ip, path);

	return amx_ftoc(longitude);
}

// native Float:geoip_distance(Float:lat1, Float:lon1, Float:lat2, Float:lon2, system = SYSTEM_METRIC);
static cell AMX_NATIVE_CALL amx_geoip_distance(AMX *amx, cell *params)
{
	float earthRadius = params[5] ? 3958.0 : 6370.997; // miles / km

	float lat1 = amx_ctof(params[1]) * (M_PI / 180);
	float lon1 = amx_ctof(params[2]) * (M_PI / 180);
	float lat2 = amx_ctof(params[3]) * (M_PI / 180);
	float lon2 = amx_ctof(params[4]) * (M_PI / 180);

	return amx_ftoc(earthRadius * acos(sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon2 - lon1)));
}

// native Continent:geoip_continent_code(const ip[], result[3] = "");
static cell AMX_NATIVE_CALL amx_geoip_continent_code(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "continent", "code", NULL };
	const char *code = lookupString(ip, path, &length);

	MF_SetAmxString(amx, params[2], code ? code : "", code ? 2 : 0);

	return getContinentId(code);
}

// native geoip_continent_name(const ip[], result[], len, id = -1);
static cell AMX_NATIVE_CALL amx_geoip_continent_name(AMX *amx, cell *params)
{
	int length;
	char *ip = stripPort(MF_GetAmxString(amx, params[1], 0, &length));

	const char *path[] = { "continent", "names", getLang(params[4]), NULL };
	const char *continent = lookupString(ip, path, &length);

	return MF_SetAmxString(amx, params[2], continent ? continent : "", length >= params[3] ? params[3] : length); // TODO: make this utf8 safe.
}


void OnAmxxAttach()
{
	const char *databases[] =
	{
		"City",
		"Country" // is the default shipped database with AMXX.
	};

	const char *modName = MF_GetModname();
	const char *dataDir = MF_GetLocalInfo("amxx_datadir", "addons/amxmodx/data");

	char file[255];
	int status = -1;

	for (size_t i = 0; i < ARRAYSIZE(databases); ++i)
	{
		sprintf(file, "%s/%s/GeoLite2-%s.mmdb", modName, dataDir, databases[i]); // MF_BuildPathname not used because backslash
                                                                                 // makes CreateFileMapping failing under windows.
		status = MMDB_open(file, MMDB_MODE_MMAP, &HandleDB);

		if (status == MMDB_SUCCESS)
		{
			break;
		}
		else if (status != MMDB_FILE_OPEN_ERROR)
		{
			MF_Log("Could not open %s - %s", file, MMDB_strerror(status));

			if (status == MMDB_IO_ERROR)
			{
				MF_Log("    IO error: %s", strerror(errno));
			}
		}
	}

	if (status != MMDB_SUCCESS)
	{
		MF_Log("Could not find GeoIP2 databases. Disabled natives.");
		return;
	}

	MF_Log("Database info: %s %i.%i",
		HandleDB.metadata.description.descriptions[0]->description,
		HandleDB.metadata.binary_format_major_version,
		HandleDB.metadata.binary_format_minor_version);

	// Retrieve supported languages.
	for (size_t i = 0; i < HandleDB.metadata.languages.count; i++) 
	{
		LangList.append(ke::AString(HandleDB.metadata.languages.names[i]));
	}

	MF_AddNatives(geoip_natives);
}

void OnAmxxDetach()
{
	MMDB_close(&HandleDB);

	LangList.clear();
}


AMX_NATIVE_INFO geoip_natives[] =
{
	{ "geoip_code2", amx_geoip_code2 },
	{ "geoip_code3", amx_geoip_code3 },

	{ "geoip_code2_ex", amx_geoip_code2_ex },
	{ "geoip_code3_ex", amx_geoip_code3_ex },

	{ "geoip_country", amx_geoip_country },
	{ "geoip_city"   , amx_geoip_city },

	{ "geoip_region_code", amx_geoip_region_code },
	{ "geoip_region_name", amx_geoip_region_name },

	{ "geoip_timezone" , amx_geoip_timezone  },
	{ "geoip_latitude" , amx_geoip_latitude  },
	{ "geoip_longitude", amx_geoip_longitude },
	{ "geoip_distance" , amx_geoip_distance  },

	{ "geoip_continent_code", amx_geoip_continent_code },
	{ "geoip_continent_name", amx_geoip_continent_name },

	{ NULL, NULL },
};


/**
* GEOIP2 DATA EXAMPLE:
*
* {
*   "city":  {
*       "confidence":  25,
*       "geoname_id": 54321,
*       "names":  {
*           "de":    "Los Angeles",
*           "en":    "Los Angeles",
*           "es":    "Los Ángeles",
*           "fr":    "Los Angeles",
*           "ja":    "ロサンゼルス市",
*           "pt-BR":  "Los Angeles",
*           "ru":    "Лос-Анджелес",
*           "zh-CN": "洛杉矶"
*       }
*   },
*   "continent":  {
*       "code":       "NA",
*       "geoname_id": 123456,
*       "names":  {
*           "de":    "Nordamerika",
*           "en":    "North America",
*           "es":    "América del Norte",
*           "fr":    "Amérique du Nord",
*           "ja":    "北アメリカ",
*           "pt-BR": "América do Norte",
*           "ru":    "Северная Америка",
*           "zh-CN": "北美洲"
*
*       }
*   },
*   "country":  {
*       "confidence":  75,
*       "geoname_id": "6252001",
*       "iso_code":    "US",
*       "names":  {
*           "de":     "USA",
*           "en":     "United States",
*           "es":     "Estados Unidos",
*           "fr":     "États-Unis",
*           "ja":     "アメリカ合衆国",
*           "pt-BR":  "Estados Unidos",
*           "ru":     "США",
*           "zh-CN":  "美国"
*       }
*   },
*   "location":  {
*       "accuracy_radius":   20,
*       "latitude":          37.6293,
*       "longitude":         -122.1163,
*       "metro_code":        807,
*       "time_zone":         "America/Los_Angeles"
*   },
*   "postal": {
*       "code":       "90001",
*       "confidence": 10
*   },
*   "registered_country":  {
*       "geoname_id": "6252001",
*       "iso_code":    "US",
*       "names":  {
*           "de":     "USA",
*           "en":     "United States",
*           "es":     "Estados Unidos",
*           "fr":     "États-Unis",
*           "ja":     "アメリカ合衆国",
*           "pt-BR":  "Estados Unidos",
*           "ru":     "США",
*           "zh-CN":  "美国"
*       }
*   },
*   "represented_country":  {
*      "geoname_id": "6252001",
*       "iso_code":    "US",
*       "names":  {
*           "de":     "USA",
*           "en":     "United States",
*           "es":     "Estados Unidos",
*           "fr":     "États-Unis",
*           "ja":     "アメリカ合衆国",
*           "pt-BR":  "Estados Unidos",
*           "ru":     "США",
*           "zh-CN":  "美国"
*       },
*       "type": "military"
*   },
*   "subdivisions":  [
*       {
*           "confidence":  50,
*           "geoname_id": 5332921,
*           "iso_code":    "CA",
*           "names":  {
*               "de":    "Kalifornien",
*               "en":    "California",
*               "es":    "California",
*               "fr":    "Californie",
*               "ja":    "カリフォルニア",
*               "ru":    "Калифорния",
*               "zh-CN": "加州"
*           }
*       }
*   ],
*   "traits": {
*       "autonomous_system_number":      "1239",
*       "autonomous_system_organization": "Linkem IR WiMax Network",
*       "domain":                        "example.com",
*       "is_anonymous_proxy":            true,
*       "is_transparent_proxy":          true,
*       "isp":                           "Linkem spa",
*       "ip_address":                    "1.2.3.4",
*       "organization":                  "Linkem IR WiMax Network",
*       "user_type":                     "traveler",
*   },
*   "maxmind": {
*       "queries_remaining":            "54321"
*   }
* }
*/
