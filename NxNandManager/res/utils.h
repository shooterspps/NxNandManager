#ifndef __utils_h__
#define __utils_h__

extern bool isdebug;
extern bool isGUI;

#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <windows.h>
#include <winioctl.h>
#include <Wincrypt.h>
#include "types.h"
#include <sys/stat.h>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <wchar.h>
#include <algorithm>
#include <sstream>
#include <tchar.h>
#include <locale>
#include <codecvt>
#include <vector>
#include "win_ioctl.h"
#include "types.h"
#include "../gui/gui.h"


#if defined(ENABLE_GUI)
#include "../gui/debug.h"
#endif


void app_printf (const char *format, ...);
void app_wprintf (const wchar_t *format, ...);
#define printf(f_, ...) app_printf((f_), ##__VA_ARGS__)
#define wprintf(f_, ...) app_wprintf((f_), ##__VA_ARGS__)

// MinGW
#if defined(__MINGW32__) || defined(__MINGW64__) || defined(__MSYS__)
#define strcpy_s strcpy
#define sprintf_s snprintf
#endif

//CRYPTO
#define ENCRYPT 1
#define DECRYPT 2

#define SUCCESS 0
// ERRORS
#define ERR_WRONG_USE			   -1001
#define ERR_INVALID_INPUT		   -1002
#define ERR_INVALID_OUTPUT		   -1003
#define ERR_INVALID_PART		   -1004
#define ERR_IO_MISMATCH			   -1005
#define ERR_INPUT_HANDLE		   -1006
#define ERR_OUTPUT_HANDLE		   -1007
#define ERR_CRYPTO_MD5			   -1008
#define ERR_NO_LIST_DISK		   -1009
#define ERR_NO_SPACE_LEFT		   -1010
#define ERR_COPY_SIZE			   -1011
#define ERR_MD5_COMPARE			   -1012
#define ERR_INIT_GUI			   -1013
#define ERR_WORK_RUNNING		   -1014
#define ERR_WHILE_COPY             -1015
#define NO_MORE_BYTES_TO_COPY      -1016
#define ERR_RESTORE_TO_SPLIT	   -1017
#define ERR_DECRYPT_CONTENT		   -1018
#define ERR_RESTORE_CRYPTO_MISSING -1019
#define ERR_CRYPTO_KEY_MISSING	   -1020
#define ERR_CRYPTO_GENERIC  	   -1021
#define ERR_CRYPTO_NOT_ENCRYPTED   -1022
#define ERR_CRYPTO_ENCRYPTED_YET   -1023
#define ERR_CRYPTO_DECRYPTED_YET   -1024
#define ERR_RESTORE_CRYPTO_MISSIN2 -1025
#define ERROR_DECRYPT_FAILED	   -1026
#define ERR_RESTORE_UNKNOWN_DISK   -1027
#define ERR_IN_PART_NOT_FOUND      -1028
#define ERR_OUT_PART_NOT_FOUND     -1029
#define ERR_KEYSET_NOT_EXISTS      -1030
#define ERR_KEYSET_EMPTY           -1031
#define ERR_FILE_ALREADY_EXISTS    -1032
#define ERR_CRYPTO_RAW_COPY        -1033
#define ERR_NX_TYPE_MISSMATCH      -1034
#define ERR_OUTPUT_NOT_MMC         -1035
#define ERR_OUT_DISMOUNT_VOL       -1036
#define ERR_WHILE_WRITE			   -1037
#define ERR_PART_CREATE_FAILED	   -1038
#define ERR_USER_ABORT             -1039
#define ERR_BAD_CRYPTO             -1040
#define ERR_VOL_MOUNT_FAILED       -1041
#define ERR_INVALID_NAND           -1042
#define ERR_INVALID_BOOT0          -1043
#define ERR_INVALID_BOOT1          -1044
#define ERR_OUTPUT_NOT_DRIVE       -1045
#define ERR_CREATE_DIR_FAILED      -1046
#define ERR_CREATE_ZIP             -1047
#define ERR_CREATE_FILE_FAILED     -1048
#define ERR_FORMAT_BAD_PART        -1049
#define ERR_INPUT_READ_ONLY        -1050
#define ERR_OUTPUT_READY_ONLY      -1051
#define ERR_MOUNTED_VIRTUAL_FS     -1052
#define ERR_DOKAN_DRIVER_NOT_FOUND -1053
#define ERR_FAILED_TO_MOUNT_FS     -1054
#define ERR_FAILED_TO_POPULATE_VFS -1055
#define ERR_DRIVER_FILE_NOT_FOUND  -1056
#define ERR_PARTITION_NO_FATFS     -1057
#define ERR_FAILED_TO_UNMOUNT_FS   -1058


typedef struct ErrorLabel ErrorLabel;
struct ErrorLabel {
	int error;
	const char* label;
};

static ErrorLabel ErrorLabelArr[] =
{
    { ERR_PARTITION_NO_FATFS, "分区上没有发现 FAT FS" },
    { ERR_DRIVER_FILE_NOT_FOUND, "未能找到驱动文件" },
    { ERR_FAILED_TO_POPULATE_VFS, "填充虚拟文件系统失败" },
    { ERR_FAILED_TO_MOUNT_FS, "挂载文件系统失败" },
    { ERR_FAILED_TO_UNMOUNT_FS, "卸载文件系统失败" },
    { ERR_DOKAN_DRIVER_NOT_FOUND, "未找到 Dokan 驱动" },
    { ERR_INPUT_READ_ONLY, "输入为只读" },
    { ERR_OUTPUT_READY_ONLY, "输出为只读" },
    { ERR_MOUNTED_VIRTUAL_FS, "Nx 存储已挂载为虚拟磁盘. 必须先弹出" },
	{ ERR_WORK_RUNNING, "已在进行的任务"},
	{ ERR_INPUT_HANDLE, "获得导入文件/磁盘句柄失败"},
	{ ERR_OUTPUT_HANDLE, "获取导出文件/磁盘句柄失败"},
	{ ERR_NO_SPACE_LEFT, "导出磁盘 : 空间不足 !"},
	{ ERR_CRYPTO_MD5, "提供的加密错误"},
	{ ERR_MD5_COMPARE, "数据完整性错误 : 未通过校验.\n复制过程中发生了错误"},
	{ ERR_RESTORE_TO_SPLIT, "不支持还原分卷备份"},
    { ERR_WHILE_COPY, "复制过程中发生错误"},
	{ ERR_IO_MISMATCH, "导入类型/大小与导出大小/类型不匹配"},
    { ERR_NX_TYPE_MISSMATCH, "导入类型与导出类型不匹配"},
	{ ERR_INVALID_INPUT, "导入不是有效的 NX 存储"},
    { ERR_INVALID_NAND, "导入丢失或不是有效的 NAND (FULL NAND 或 RAWNAND"},
    { ERR_INVALID_BOOT0, "BOOT0 缺失或无效"},
    { ERR_INVALID_BOOT1, "BOOT0 缺失或无效"},
	{ ERR_INVALID_OUTPUT, "导出不是有效的 NX 存储"},
	{ ERR_DECRYPT_CONTENT, "解密验证内容失败 (密钥错误?)"},
    { ERR_RESTORE_CRYPTO_MISSING, "尝试将解密内容还原为加密内容"},
	{ ERR_RESTORE_CRYPTO_MISSIN2, "尝试将加密内容还原为解密内容"},
    { ERR_CRYPTO_KEY_MISSING, "试图解密/加密内容但丢失了一些密钥 (配置密钥)"},
	{ ERROR_DECRYPT_FAILED, "解密验证失败 (密钥错误?)"},
	{ ERR_CRYPTO_NOT_ENCRYPTED, "导入的文件未加密"},
	{ ERR_CRYPTO_ENCRYPTED_YET, "导入的文件已经加密"},
	{ ERR_CRYPTO_DECRYPTED_YET, "导入的文件已经解密"},
	{ ERR_IN_PART_NOT_FOUND, "未找到分区 \"导入\""},
	{ ERR_OUT_PART_NOT_FOUND, "未找到分区 \"导出\""},
	{ ERR_RESTORE_UNKNOWN_DISK, "无法还原到未知磁盘"},
    { ERR_FILE_ALREADY_EXISTS, "导出的文件/目录已存在"},
	{ ERR_OUTPUT_NOT_MMC, "导出必须是可移动媒体 (MMC)"},
	{ ERR_OUT_DISMOUNT_VOL, "拆除导出驱动器中的卷失败"},
	{ ERR_WHILE_WRITE, "导出文件/磁盘写入失败"},
    { ERR_PART_CREATE_FAILED, "创建新分区失败"},
    { ERR_USER_ABORT, "用户任务中止"},
    { ERR_BAD_CRYPTO, "密钥验证失败 (密钥错误?)"},
    { ERR_VOL_MOUNT_FAILED, "挂载新 FAT32 分区失败. 无法创建 CFW (大气层) 所需文件"},
    { ERR_OUTPUT_NOT_DRIVE, "导出的不是驱动器/卷"},
    { ERR_CREATE_DIR_FAILED, "导出驱动器/卷中创建目录失败"},
    { ERR_CREATE_FILE_FAILED, "导出驱动器/卷中创建文件失败"},
    { ERR_CREATE_ZIP, "创建压缩 (zip) 时发生错误"},
    { ERR_FORMAT_BAD_PART, "格式化分区只适用于 USER 和 SYSTEM"}
};


struct GenericKey {
    std::string name;
    std::string key;
};

typedef struct KeySet KeySet;
struct KeySet {
	char crypt0[33];
	char tweak0[33];
	char crypt1[33];
	char tweak1[33];
	char crypt2[33];
	char tweak2[33];
	char crypt3[33];
	char tweak3[33];
    std::vector<GenericKey> other_keys;
};
std::string GetGenericKey(KeySet* keyset, std::string name);
bool HasGenericKey(KeySet* keyset, std::string name);

void dbg_printf (const char *format, ...);
void dbg_wprintf (const wchar_t *format, ...);

wchar_t *convertCharArrayToLPCWSTR(const char* charArray);
LPWSTR convertCharArrayToLPWSTR(const char* charArray);
u64 GetFilePointerEx (HANDLE hFile);
unsigned long sGetFileSize(std::string filename);
std::string GetLastErrorAsString();
std::string hexStr(unsigned char *data, int len);
BOOL AskYesNoQuestion(const char* question, void* p_arg1 = NULL, void* p_arg2 = NULL);
std::string GetReadableSize(u64 size);
std::string GetReadableElapsedTime(std::chrono::duration<double> elapsed_seconds);
void throwException(int rc, const char* errorStr=NULL);
void throwException(const char* errorStr=NULL, void* p_arg1 = NULL, void* p_arg2 = NULL);
char * flipAndCodeBytes(const char * str, int pos, int flip, char * buf);
std::string ExePath();
std::wstring ExePathW();
HMODULE GetCurrentModule();
bool file_exists(const wchar_t *fileName);
int digit_to_int(char d);
int hex_to_int(char c);
int hex_to_ascii(char c, char d);
std::string hexStr_to_ascii(const char* hexStr);
void hexStrPrint(unsigned char *data, int len);

static DWORD crc32table[256];
static bool crc32Intalized = false;
DWORD crc32Hash(const void *data, DWORD size);

std::vector<std::string> explode(std::string const & s, char delim);

template<class T>
T base_name(T const & path, T const & delims = "/\\")
{
	return path.substr(path.find_last_of(delims) + 1);
}

template<class T>
T base_nameW(T const & path, T const & delims = L"/\\")
{
	return path.substr(path.find_last_of(delims) + 1);
}
template<class T>
T remove_extension(T const & filename)
{
    typename T::size_type const pmin(filename.find_last_of(L"/\\"));
	typename T::size_type const p(filename.find_last_of('.'));
    return p > 0 && (pmin == T::npos || p > pmin) && p != T::npos ? filename.substr(0, p) : filename;
}
template<class T>
T remove_extensionW(T const & filename)
{
    typename T::size_type const pmin(filename.find_last_of(L"/\\"));
	typename T::size_type const p(filename.find_last_of(L'.'));
    return p > 0 && (pmin == T::npos || p > pmin) && p != T::npos ? filename.substr(0, p) : filename;
}
template<class T>
T get_extension(T const & filename)
{
    typename T::size_type const pmin(filename.find_last_of("/\\"));
	typename T::size_type const p(filename.find_last_of('.'));
    return p > 0 && (pmin == T::npos || p > pmin) && p != T::npos ? filename.substr(p, T::npos) : filename;
}
template<class T>
T get_extensionW(T const & filename)
{
    typename T::size_type const pmin(filename.find_last_of(L"/\\"));
	typename T::size_type const p(filename.find_last_of(L'.'));
    return p > 0 && (pmin == T::npos || p > pmin) && p != T::npos ? filename.substr(p, T::npos) : filename;
}
template<class T>
static bool endsWith(T const & str, T const & suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}
template<class T>
static bool startsWith(T const & str, T const & prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

template< typename T >
std::string int_to_hex(T i)
{
	std::stringstream stream;
	stream << "0x"
		<< std::setfill('0') //<< std::setw(sizeof(T) * 2)
		<< std::uppercase
		<< std::hex << i;

	return stream.str();
}
template<typename T, size_t ARR_SIZE>
size_t array_countof(T(&)[ARR_SIZE]) { return ARR_SIZE; }
template <typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1) {
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i<hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

template <typename T>
bool is_in(const T& v, std::initializer_list<T> lst)
{
    return std::find(std::begin(lst), std::end(lst), v) != std::end(lst);
}

template <typename T>
bool not_in(const T& v, std::initializer_list<T> lst)
{
    return std::find(std::begin(lst), std::end(lst), v) == std::end(lst);
}

std::string ltrim(const std::string& s);
std::string rtrim(const std::string& s);
std::string trim(const std::string& s);
std::wstring rtrimW(const std::wstring& s);

bool is_file(const char* path);
bool is_dir(const char* path);
int parseKeySetFile(const char *keyset_file, KeySet *biskeys);
u32 u32_val(u8* buf);

template<typename M> inline void* GetMethodPointer(M ptr)
{
    return *reinterpret_cast<void**>(&ptr);
}

unsigned random(unsigned n);
DWORD randomDWORD();
int IsWow64();
void flipBytes(u8* buf, size_t len);
bool parse_hex_key(unsigned char *key, const char *hex, unsigned int len);
#endif
