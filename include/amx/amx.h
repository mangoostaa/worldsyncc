#pragma once

#include <cstdint>

#if defined(_WIN32)
#define AMXAPI
#define AMX_NATIVE_CALL
#else
#define AMXAPI
#define AMX_NATIVE_CALL
#endif

typedef int32_t cell;

struct tagAMX;
typedef tagAMX AMX;

typedef cell(AMX_NATIVE_CALL* AMX_NATIVE)(AMX* amx, cell* params);
typedef int(AMXAPI* AMX_CALLBACK)(AMX* amx, cell index, cell* result, const cell* params);
typedef int(AMXAPI* AMX_DEBUG)(AMX* amx);

struct AMX_NATIVE_INFO
{
	const char* name;
	AMX_NATIVE func;
};

enum
{
	AMX_ERR_NONE = 0
};

inline cell amx_ftoc(float value)
{
	union
	{
		float f;
		cell c;
	} data;
	data.f = value;
	return data.c;
}

inline float amx_ctof(cell value)
{
	union
	{
		cell c;
		float f;
	} data;
	data.c = value;
	return data.f;
}
