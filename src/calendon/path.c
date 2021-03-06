#include "path.h"

#include <calendon/string.h>

#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
	#include <calendon/compat-windows.h>
#elif __linux__
	#include <unistd.h>
#endif

bool cnPath_Exists(const char* path)
{
	if (!path) {
		return false;
	}
	struct stat info;
	return stat(path, &info) == 0;
}

bool cnPath_IsDir(const char* path)
{
	if (!path) {
		return false;
	}

	struct stat info;
	if (stat(path, &info) != 0) {
		return false;
	}

#ifdef _WIN32
	return (info.st_mode & S_IFDIR) == S_IFDIR;
#else
	return S_ISDIR(info.st_mode);
#endif
}

bool cnPath_IsFile(const char* path)
{
	if (!path) {
		return false;
	}

	struct stat info;
	if (stat(path, &info) != 0) {
		return false;
	}
#ifdef _WIN32
	return (info.st_mode & S_IFREG) == S_IFREG;
#else
	return !S_ISDIR(info.st_mode);
#endif
}

bool cnPath_Append(const char* toAdd, char* current, uint32_t length)
{
	const size_t currentLength = strlen(current);
	const size_t finalLength = currentLength + strlen(toAdd) + 1 + 1;
	if (finalLength >= length) {
		return false;
	}
	current[currentLength] = '/';
	strcpy(current + currentLength + 1, toAdd);
	return true;
}

void cnPathBuffer_Clear(CnPathBuffer* path)
{
	CN_ASSERT(path != NULL, "Cannot clear a null CnPathBuffer.");
	memset(path->str, 0, CN_MAX_TERMINATED_PATH);
}

bool cnPathBuffer_Set(CnPathBuffer* path, const char* initialPath)
{
	CN_ASSERT(path != NULL, "Cannot assign to a null CnPathBuffer");
	CN_ASSERT(initialPath != NULL, "Cannot assign a null initial path to a CnPathBuffer");
	const size_t initialPathLength = strlen(initialPath);
	if (cnString_FitsWithNull(initialPath, CN_MAX_TERMINATED_PATH)) {
		strcpy(path->str, initialPath);
		return true;
	}
	return false;
}

bool cnPathBuffer_Join(CnPathBuffer* root, const char* suffix)
{
	CN_ASSERT(root != NULL, "Cannot append to a null root path");
	CN_ASSERT(suffix != NULL, "Trying to append a null path");
	return cnPath_Append(suffix, root->str, CN_MAX_TERMINATED_PATH);
}

bool cnPathBuffer_Exists(CnPathBuffer* path)
{
	CN_ASSERT(path != NULL, "Cannot check path existence for a null CnPathBuffer.");
	return cnPath_Exists(path->str);
}

bool cnPathBuffer_IsDir(CnPathBuffer* path)
{
	CN_ASSERT(path != NULL, "Cannot check to see if a null CnPathBuffer is a directory.");
	return cnPath_IsDir(path->str);
}

bool cnPathBuffer_IsFile(CnPathBuffer* path)
{
	CN_ASSERT(path != NULL, "Cannot check to see if a null CnPathBuffer is a file.");
	return cnPath_IsFile(path->str);
}

bool cnPathBuffer_CurrentWorkingDirectory(CnPathBuffer* path)
{
	CN_ASSERT_PTR(path);

#ifdef _WIN32
	//https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory
	return GetCurrentDirectory(CN_MAX_TERMINATED_PATH, path->str) != 0;
#elif __linux__
	return getcwd(path->str, CN_MAX_TERMINATED_PATH) != NULL;
#endif
}

/**
 * The Calendon project home is the root of the git repo.  The environment
 * variable CALENDON_HOME defines the Calendon project home, otherwise it
 * becomes the current working directory.
 */
bool cnPathBuffer_DefaultCalendonHome(CnPathBuffer* path)
{
	CN_ASSERT_PTR(path);

	const char* calendonHomeEnvVar = getenv("CALENDON_HOME");
	if (calendonHomeEnvVar) {
		cnPathBuffer_Set(path, calendonHomeEnvVar);
		return true;
	}

	return cnPathBuffer_CurrentWorkingDirectory(path);
}
