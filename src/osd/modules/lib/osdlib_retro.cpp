
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#ifndef _WIN32
#include <sys/mman.h>
#include <signal.h>
#include <dlfcn.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#ifdef __GNUC__
#include <sys/time.h>
#endif
#ifdef SDLMAME_EMSCRIPTEN
#include <emscripten.h>
#endif

#include <cstdio>
#if !defined(_WIN32)
#include <iomanip>
#endif
#include <memory>


// MAME headers
#include "osdcore.h"
#include "osdlib.h"

//============================================================
//  osd_getenv
//============================================================

const char *osd_getenv(const char *name)
{
    return getenv(name);
}

//============================================================
//  osd_setenv
//============================================================

int osd_setenv(const char *name, const char *value, int overwrite)
{
#if defined(_WIN32)
   char *buf;
   int result;

   if (!overwrite)
   {
      if (osd_getenv(name) != NULL)
         return 0;
   }
   buf = (char *) malloc(strlen(name)+strlen(value)+2);
   sprintf(buf, "%s=%s", name, value);
   result = putenv(buf);

   /* will be referenced by environment
    * Therefore it is not freed here
    */

   return result;
#else
   return setenv(name, value, overwrite);
#endif
}

//============================================================
//  osd_process_kill
//============================================================

void osd_process_kill(void)
{
#ifndef _WIN32
    kill(getpid(), SIGKILL);
#else
    TerminateProcess(GetCurrentProcess(), -1);
#endif
}


//============================================================
//  osd_alloc_executable
//
//  allocates "size" bytes of executable memory.  this must take
//  things like NX support into account.
//============================================================

void *osd_alloc_executable(size_t size)
{
#if defined(_WIN32)
   return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
	return (void *)mmap(0, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
#endif
}

//============================================================
//  osd_free_executable
//
//  frees memory allocated with osd_alloc_executable
//============================================================

void osd_free_executable(void *ptr, size_t size)
{
#if defined(_WIN32)
   VirtualFree(ptr, 0, MEM_RELEASE);
#else
	munmap(ptr, size);
#endif
}

//============================================================
//  osd_break_into_debugger
//============================================================

void osd_break_into_debugger(const char *message)
{
	#ifdef MAME_DEBUG
	printf("MAME exception: %s\n", message);
	printf("Attempting to fall into debugger\n");
	kill(getpid(), SIGTRAP);
	#else
	printf("Ignoring MAME exception: %s\n", message);
	#endif
}

//============================================================
//  osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	return NULL;
}

//============================================================
//  osd_getpid
//============================================================

int osd_getpid(void)
{
#if defined(_WIN32)
   return GetCurrentProcessId();
#else
   return getpid();
#endif
}

#if defined(_WIN32)

//============================================================
//  osd_dynamic_bind
//============================================================

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
// for classic desktop applications
#define load_library(filename) LoadLibrary(filename)
#else
// for Windows Store universal applications
#define load_library(filename) LoadPackagedLibrary(filename, 0)
#endif

namespace osd {
class dynamic_module_win32_impl : public dynamic_module
{
public:
	dynamic_module_win32_impl(std::vector<std::string> &libraries)
		: m_module(nullptr)
	{
		m_libraries = libraries;
	}

	virtual ~dynamic_module_win32_impl() override
	{
		if (m_module != nullptr)
			FreeLibrary(m_module);
	};

protected:
	virtual generic_fptr_t get_symbol_address(char const *symbol) override
	{
		/*
		 * given a list of libraries, if a first symbol is successfully loaded from
		 * one of them, all additional symbols will be loaded from the same library
		 */
		if (m_module)
		{
			return reinterpret_cast<generic_fptr_t>(GetProcAddress(m_module, symbol));
		}

		for (auto const &library : m_libraries)
		{
			osd::text::tstring tempstr = osd::text::to_tstring(library);
			HMODULE module = load_library(tempstr.c_str());

			if (module != nullptr)
			{
				generic_fptr_t function = reinterpret_cast<generic_fptr_t>(GetProcAddress(module, symbol));

				if (function != nullptr)
				{
					m_module = module;
					return function;
				}
				else
				{
					FreeLibrary(module);
				}
			}
		}

		return nullptr;
	}

private:
	std::vector<std::string> m_libraries;
	HMODULE                  m_module;
};

dynamic_module::ptr dynamic_module::open(std::vector<std::string> &&names)
{
	return std::make_unique<dynamic_module_win32_impl>(names);
}

#else

//============================================================
//  dynamic_module_posix_impl
//============================================================

namespace osd {
class dynamic_module_posix_impl : public dynamic_module
{
public:
	dynamic_module_posix_impl(std::vector<std::string> &libraries)
		: m_module(nullptr)
	{
		m_libraries = libraries;
	}

	virtual ~dynamic_module_posix_impl() override
	{
		if (m_module != nullptr)
			dlclose(m_module);
	};

protected:
	virtual generic_fptr_t get_symbol_address(char const *symbol) override
	{
		/*
		 * given a list of libraries, if a first symbol is successfully loaded from
		 * one of them, all additional symbols will be loaded from the same library
		 */
		if (m_module)
		{
			return reinterpret_cast<generic_fptr_t>(dlsym(m_module, symbol));
		}

		for (auto const &library : m_libraries)
		{
			void *module = dlopen(library.c_str(), RTLD_LAZY);

			if (module != nullptr)
			{
				generic_fptr_t function = reinterpret_cast<generic_fptr_t>(dlsym(module, symbol));

				if (function != nullptr)
				{
					m_module = module;
					return function;
				}
				else
				{
					dlclose(module);
				}
			}
		}

		return nullptr;
	}

private:
	std::vector<std::string> m_libraries;
	void *                   m_module;
};

dynamic_module::ptr dynamic_module::open(std::vector<std::string> &&names)
{
	return std::make_unique<dynamic_module_posix_impl>(names);
}

} // namespace osd

#endif