#if defined(WIN32)
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #if !defined(_WIN32_WCE)
        #include <crtdbg.h>
    #endif
#else
    #include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>


/* Windows 32 */
#if defined(WIN32)
#   include <windows.h>
#   include <time.h>
#   include <direct.h>
#   ifdef getcwd
#   undef getcwd
#       define getcwd(path, sizeOfBuffers) _getcwd(path,sizeOfBuffers)
#   endif
#endif


/* POSIX */
#if defined(_POSIX)
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <dirent.h>
    #include <sys/time.h>
    #include <unistd.h>
#endif

#if defined(WIN32) || defined(_WIN32_WCE)
    #define strncasecmp(x,y,z) _strnicmp(x,y,z)
    #define strcasecmp(x,y) _stricmp(x,y)
#endif


#if defined(_WIN32_WCE)
    #include <Winbase.h>
#endif
#include "FileIoAbstraction.h"

int EndsWith(/*INOUT*/ const char* str, const char* endsWith, int ignoreCase)
{
    int strLen, ewLen, offset;
    int cmp = 0;

    strLen = (int) strlen(str);
    ewLen = (int) strlen(endsWith);
    if (ewLen > strLen) return 0;
    offset = strLen - ewLen;

    if (ignoreCase != 0)
    {
        cmp = strncasecmp(str+offset, endsWith, ewLen);
    }
    else
    {
        cmp = strncmp(str+offset, endsWith, ewLen);
    }

    return cmp == 0?1:0;
}

void ILibFileDir_CloseDir(void* handle)
{
#if defined(WIN32) || defined(_WIN32_WCE)
    FindClose(*((HANDLE*)handle));
    free(handle);
#elif defined(_POSIX)
    DIR* dirObj = (DIR*) handle;
    closedir(dirObj);
#endif
}


void* ILibFileDir_GetDirFirstFile(const char* directory, /*INOUT*/ char* filename, int filenamelength, /*INOUT*/ int* filesize)
{
#if defined(WIN32) || defined(_WIN32_WCE)
    WIN32_FIND_DATA FileData;
    HANDLE* hSearch;
    char* direx;
    #if defined(_WIN32_WCE) || defined(UNICODE)
        wchar_t *tempChar;
        int tempCharLength;
    #endif


    hSearch = malloc(sizeof(HANDLE));
    direx = malloc(filenamelength + 5);

    if (directory[(int) strlen(directory) - 1] == '\\')
    {
        sprintf(direx,"%s*.*",directory);
    }
    else
    {
        sprintf(direx,"%s\\*.*",directory);
    }

    #if defined(_WIN32_WCE) || defined(UNICODE)
        tempCharLength = MultiByteToWideChar(CP_UTF8,0,direx,-1,NULL,0);
        tempChar = (wchar_t*)malloc(sizeof(wchar_t)*tempCharLength);
        MultiByteToWideChar(CP_UTF8,0,direx,-1,tempChar,tempCharLength);
        *hSearch = FindFirstFile(tempChar, &FileData);
        free(direx);
        free(tempChar);
    #else
        *hSearch = FindFirstFile(direx, &FileData);
        free(direx);
    #endif

    if (*hSearch == INVALID_HANDLE_VALUE)
    {
        free(hSearch);
        hSearch = NULL;
    }
    else
    {
        if (filename != NULL)
        {
#if defined(_WIN32_WCE) || defined(UNICODE)
            WideCharToMultiByte(CP_UTF8,0,(LPCWSTR)FileData.cFileName,-1,filename,filenamelength,NULL,NULL);
#else
            memcpy(filename,FileData.cFileName,1+(int)strlen(FileData.cFileName));
#endif
        }

        if (filesize != NULL)
        {
            *filesize = FileData.nFileSizeLow;
        }
    }

    return hSearch;
#elif defined(_POSIX)
    DIR* dirObj;
    struct dirent* dirEntry;    /* dirEntry is a pointer to static memory in the C runtime lib for readdir()*/
    struct stat _si;
    char fullPath[1024];
    
    dirObj = opendir(directory);

    if (dirObj != NULL)
    {
        dirEntry = readdir(dirObj);

        if ((dirEntry != NULL) && ((int) strlen(dirEntry->d_name) < filenamelength))
        {
            if (filename != NULL)
            {
                strcpy(filename,dirEntry->d_name);
                sprintf(fullPath, "%s%s", directory, dirEntry->d_name);

                if (filesize != NULL)
                {
                    if (stat(fullPath, &_si) != -1)
                    {
                        if ((_si.st_mode & S_IFDIR) == S_IFDIR)
                        {
                            *filesize = 0;
                        }
                        else
                        {
                            *filesize = _si.st_size;
                        }
                    }
                }
            }
        }
    }

    return dirObj;
#endif
}




// Windows Version
// 0 = No More Files
// 1 = Next File
int ILibFileDir_GetDirNextFile(void* handle, const char* dirName, char* filename, int filenamelength, int* filesize)
{
#if defined(WIN32) || defined(_WIN32_WCE)
    WIN32_FIND_DATA FileData;
    
    if (FindNextFile(*((HANDLE*)handle), &FileData) == 0) {return 0;}
    #if defined(_WIN32_WCE) || defined(UNICODE)
        WideCharToMultiByte(CP_UTF8,0,(LPCWSTR)FileData.cFileName,-1,filename,filenamelength,NULL,NULL);
    #else
        memcpy(filename,FileData.cFileName,1+(int)strlen(FileData.cFileName));
    #endif
    if (filesize != NULL)
    {
        *filesize = FileData.nFileSizeLow;
    }

    return 1;
#elif defined(_POSIX)
    DIR* dirObj;
    struct dirent* dirEntry;    /* dirEntry is a pointer to static memory in the C runtime lib for readdir()*/
    struct stat _si;
    char fullPath[1024];

    dirObj = (DIR*) handle;
    dirEntry = readdir(dirObj);

    if ((dirEntry != NULL) && ((int) strlen(dirEntry->d_name) < filenamelength))
    {
        strcpy(filename,dirEntry->d_name);
        sprintf(fullPath, "%s%s", dirName, dirEntry->d_name);

        if (filesize != NULL)
        {
            /* ? Cygwin has a memory leak with stat. */
            if (stat(fullPath, &_si) != -1)
            {
                if ((_si.st_mode & S_IFDIR) == S_IFDIR)
                {
                    *filesize = 0;
                }
                else
                {
                    *filesize = _si.st_size;
                }
            }
        }
        return 1;
    }

    return 0;
#endif
}

enum ILibFileDir_Type ILibFileDir_GetType(char* directory)
{
#if defined(WIN32) || defined(_WIN32_WCE)
    DWORD _si;
    int dirLen,dirSize;
    #if defined(_WIN32_WCE) || defined(UNICODE)
        wchar_t *tempChar;
        int tempCharLength;
    #endif

    dirLen = (int) strlen(directory);
    dirSize = dirLen+1;

    #if defined(_WIN32_WCE) || defined(UNICODE)
        tempCharLength = MultiByteToWideChar(CP_UTF8,0,directory,-1,NULL,0);
        tempChar = (wchar_t*)malloc(sizeof(wchar_t)*tempCharLength);
        MultiByteToWideChar(CP_UTF8,0,directory,-1,tempChar,tempCharLength);
        _si = GetFileAttributes(tempChar);
        free(tempChar);
    #else
        _si = GetFileAttributes(directory);
    #endif
    
    if (_si == 0xFFFFFFFF)
    {
        return ILibFileDir_Type_NOT_FOUND_ERROR;
    }

    if ((_si & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        return ILibFileDir_Type_FILE;
    }
    else 
    {
        return ILibFileDir_Type_DIRECTORY;
    }
#elif defined(_POSIX)
    struct stat _si;

    int dirLen,dirSize;
    //char *fullpath;// maybe memory leak. add by leochen
    int pathExists;
    enum ILibFileDir_Type retVal = ILibFileDir_Type_NOT_FOUND_ERROR;

    dirLen = (int) strlen(directory);
    dirSize = dirLen+1;
    //fullpath = (char*) malloc(dirSize);// maybe memory leak. add by leochen

    pathExists = stat (directory, &_si);

    if (pathExists != -1)
    {
        if ((_si.st_mode & S_IFDIR) == S_IFDIR)
        {
            retVal = ILibFileDir_Type_DIRECTORY;
        }
        else
        {
            retVal = ILibFileDir_Type_FILE;
        }
    }
    return retVal;
#endif
}

int ILibFileDir_GetDirEntryCount(const char* fullPath, char *dirDelimiter)
{
    char fn[MAX_PATH_LENGTH];
    void *dirObj;
    int retVal = 0;
    int ewDD;
    int nextFile;

    dirObj = ILibFileDir_GetDirFirstFile(fullPath, fn, MAX_PATH_LENGTH, NULL);

    if (dirObj != NULL)
    {
        ewDD = EndsWith(fullPath, dirDelimiter, 0);

        do
        {
            retVal++;
            nextFile = ILibFileDir_GetDirNextFile(dirObj,fullPath,fn,MAX_PATH_LENGTH, NULL);
        }
        while (nextFile != 0);

        ILibFileDir_CloseDir(dirObj);
    }

    return retVal;
}

char* ILibFileDir_GetWorkingDir(char *path, size_t sizeOfBuf)
{
#if defined(_WIN32_WCE)
    if(path==NULL)
    {
        path = (char*)malloc(2);
    }
    sprintf(path,"/");
    return(path);
#else
    return getcwd(path, (int)sizeOfBuf);
#endif
}
int ILibFileDir_DeleteFile(char *FileName)
{

#if defined(_WIN32_WCE) || (defined(WIN32) && defined(UNICODE))
    int retVal;
    wchar_t* wfile;        /* must MMS_FREE */
    int wFileLen;
    int wFileSize;
    int mbFileLen;
    int mbFileSize;

    mbFileLen = (int) strlen(FileName);
    mbFileSize = mbFileLen+1;
    wFileLen = mbFileLen * 2;
    wFileSize = mbFileSize * 2;
    wfile = (wchar_t*)malloc(wFileSize);

    if (mbstowcs(wfile,FileName,wFileSize) == -1)
    {
        retVal = 0;
    }
    else    
    {
        retVal = DeleteFile(wfile);
    }

    free(wfile);
    return retVal;
#elif defined(WIN32) && !defined(_WIN32_WCE) && !defined(UNICODE)
    return(!DeleteFile((LPCTSTR)FileName));
#elif defined(POSIX)
    return(remove(FileName));
#endif
    return 0;
}
int ILibFileDir_DeleteDir(char *path)
{

#if defined(_WIN32_WCE) || (defined(WIN32) && defined(UNICODE))
    int retVal;
    wchar_t* wdirectory;        /* must MMS_FREE */
    int wDirLen;
    int wDirSize;
    int mbDirLen;
    int mbDirSize;
    DWORD  nError;

    mbDirLen = (int) strlen(path);
    mbDirSize = mbDirLen+1;
    wDirLen = mbDirLen * 2;
    wDirSize = mbDirSize * 2;

    wdirectory = (wchar_t*)malloc(wDirSize);

    if (mbstowcs(wdirectory,path,wDirSize) == -1)
    {
        retVal = 0;
    }
    else    
    {
        retVal = !RemoveDirectory(wdirectory);
        if(retVal==0)
        {
            nError= GetLastError();
        }
    }

    free(wdirectory);
    return retVal;
#elif defined(WIN32) && !defined(_WIN32_WCE) && !defined(UNICODE)
    return(!RemoveDirectory((LPCTSTR)path));
#elif defined(POSIX)
    return(rmdir(path));
#endif
    return 0;
}

int ILibFileDir_Delete(char* path)
{
    int count = 0;
    enum ILibFileDir_Type type = ILibFileDir_GetType(path);
    if( type == ILibFileDir_Type_FILE )
    {
        if(ILibFileDir_DeleteFile(path) == 0)
            count++;
    }
    else if(type == ILibFileDir_Type_DIRECTORY)
    {
        char filename[MAX_PATH_LENGTH];
        int filesize;
        void* handle = ILibFileDir_GetDirFirstFile(path, filename, MAX_PATH_LENGTH, &filesize);
        if(handle == NULL)
        {
            ILibFileDir_DeleteDir(path);
            count++;
        }else if ( !((strcmp(filename, ".") == 0) ||
                    (strcmp(filename, "..") == 0)))
        {
            
            
            char fullpath[MAX_PATH_LENGTH];
            do{
                memset(fullpath,0,MAX_PATH_LENGTH);
                strcpy(fullpath,path);
                if(path[strlen(path) - 1] != '\\')
                    strcat(fullpath,"\\");
                strcat(fullpath,filename);
                count += ILibFileDir_Delete(fullpath);
            }while(ILibFileDir_GetDirNextFile(handle, path, filename, MAX_PATH_LENGTH, &filesize));

            ILibFileDir_CloseDir(handle);
            ILibFileDir_DeleteDir(path);
            count++;
         
        }
    }
    return count;
}



int ILibFileDir_CreateDir(char *path)
{
#if defined(_WIN32_WCE) || (defined(WIN32) && defined(UNICODE))
    int retVal;
    wchar_t* wdirectory;        /* must MMS_FREE */
    int wDirLen;
    int wDirSize;
    int mbDirLen;
    int mbDirSize;
    
    mbDirLen = (int) strlen(path);
    mbDirSize = mbDirLen+1;
    wDirLen = mbDirLen * 2;
    wDirSize = mbDirSize * 2;
    wdirectory = (wchar_t*)malloc(wDirSize);

    if (mbstowcs(wdirectory,path,wDirSize) == -1)
    {
        retVal = 0;
    }
    else    
    {
        retVal = !CreateDirectory(wdirectory, NULL);
    }

    free(wdirectory);
    return retVal;
#elif defined(WIN32) && !defined(_WIN32_WCE) && !defined(UNICODE)
    return(!CreateDirectory((LPCTSTR)path,NULL));
#elif defined(POSIX)
    return(mkdir(path,0));
#endif
    return 0;
}

int ILibFileDir_MoveFile(char *OldFileName, char *NewFileName)
{
#if defined(_WIN32_WCE) || (defined(WIN32) && defined(UNICODE))
    int retVal;
    wchar_t* woldfile;        /* must MMS_FREE */
    int wOldFileLen;
    int wOldFileSize;
    int mbOldFileLen;
    int mbOldFileSize;
    wchar_t* wnewfile;        /* must MMS_FREE */
    int wNewFileLen;
    int wNewFileSize;
    int mbNewFileLen;
    int mbNewFileSize;


    mbOldFileLen = (int) strlen(OldFileName);
    mbOldFileSize = mbOldFileLen+1;
    wOldFileLen = mbOldFileLen * 2;
    wOldFileSize = mbOldFileSize * 2;
    woldfile = (wchar_t*)malloc(wOldFileSize);

    mbNewFileLen = (int) strlen(NewFileName);
    mbNewFileSize = mbNewFileLen+1;
    wNewFileLen = mbNewFileLen * 2;
    wNewFileSize = mbNewFileSize * 2;
    wnewfile = (wchar_t*)malloc(wNewFileSize);

    if (mbstowcs(woldfile,OldFileName,wOldFileSize) == -1)
    {
        retVal = 0;
    }
    else if(mbstowcs(wnewfile,NewFileName,wNewFileSize) == -1)
    {
        retVal = 0;
    }
    else    
    {
        retVal = !MoveFile(woldfile, wnewfile);
    }

    free(woldfile);
    free(wnewfile);
    return retVal;
#elif defined(WIN32) && !defined(_WIN32_WCE) && !defined(UNICODE)
    return(!MoveFile((LPCTSTR)OldFileName, (LPCTSTR)NewFileName));
#elif defined(POSIX)
    return(rename(OldFileName, NewFileName));
#endif
    return 0;
}

long ILibFileDir_GetFileSize(char *FileName)
{
    long SourceFileLength;
    FILE *SourceFile = fopen(FileName,"rb");

    if(SourceFile==NULL)
    {
        return(-1);
    }

    fseek(SourceFile,0,SEEK_END);
    
    SourceFileLength = ftell(SourceFile);
    fclose(SourceFile);
    return(SourceFileLength);
}

time_t ILibFileDir_GetFileTimeStamp(char *FileName)
{    
#if defined(WIN32) || defined(_WIN32_WCE)
    HANDLE* hFile;
    WIN32_FIND_DATA FileData;
    LONGLONG time;

    #if defined(_WIN32_WCE) || (defined(WIN32) && defined(UNICODE))
    wchar_t *tempChar;
    int tempCharLength;

    tempCharLength = MultiByteToWideChar(CP_UTF8,0,FileName,-1,NULL,0);
    tempChar = (wchar_t*)malloc(sizeof(wchar_t)*tempCharLength);
    MultiByteToWideChar(CP_UTF8,0,FileName,-1,tempChar,tempCharLength);
    hFile = FindFirstFile(tempChar, &FileData);
    free(tempChar);
    #else
    hFile = FindFirstFile(FileName, &FileData);
    #endif

    if(hFile == NULL) return -1;
    // Retrieve the last modified timestamp for the file.

    time = (__int64) FileData.ftLastWriteTime.dwHighDateTime << 32;;
    time = ((time + FileData.ftLastWriteTime.dwLowDateTime - 116444736000000000) / 10000000);

    FindClose(hFile);
    return (time_t) time; 
#else
    return -1;
#endif
}
