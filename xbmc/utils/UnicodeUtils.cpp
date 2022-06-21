/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
//-----------------------------------------------------------------------
//
//  File:      UnicodeUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Major modifications by Frank Feuerbacher to utilize icu4c Unicode library
//  to resolve issues discovered in Kodi 19 Matrix (Unicode and Python 3 support)
//
//  Modified to use J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------

#ifdef HAVE_NEW_CROSSGUID
#include <crossguid/guid.hpp>
#else
#include <guid.h>
#endif

#if defined(TARGET_ANDROID)
#include <androidjni/JNIThreading.h>
#endif

#include "CharsetConverter.h"
#include "LangInfo.h"
#include "UnicodeUtils.h"
#include <tuple>
#include "Util.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <functional>
#include <inttypes.h>
#include <iomanip>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fstrcmp.h>

#include <unicode/utf8.h>
#include "utils/log.h"

// don't move or std functions end up in PCRE namespace
// clang-format off
#include "utils/RegExp.h"
// clang-format on
#include "utils/Unicode.h"
#include "unicode/uregex.h"

#define FORMAT_BLOCK_SIZE 512 // # of bytes for initial allocation for printf

static constexpr const char *ADDON_GUID_RE =
		"^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";

/* empty string for use in returns by ref */
const std::string UnicodeUtils::Empty = "";

#if not defined(USE_STRINGUTILS_FORMAT)
std::string UnicodeUtils::FormatV(const char *fmt, va_list args) {
	if (!fmt || !fmt[0])
		return "";

	int size = FORMAT_BLOCK_SIZE;
	va_list argCopy;

	while (true) {
		char *cstr = reinterpret_cast<char*>(malloc(sizeof(char) * size));
		if (!cstr)
			return "";

		va_copy(argCopy, args);
		int nActual = vsnprintf(cstr, size, fmt, argCopy);
		va_end(argCopy);

		if (nActual > -1 && nActual < size) // We got a valid result
				{
			std::string str(cstr, nActual);
			free(cstr);
		 	//CLog::Log(LOGINFO, "UnicodeUtils::FormatV: {}\n", str);
		 	//std::cout << "UnicodeUtils::FormatV: " << str << std::endl;

			return str;
		}
		free(cstr);
#ifndef TARGET_WINDOWS
		if (nActual > -1)                   // Exactly what we will need (glibc 2.1)
			size = nActual + 1;
		else
			// Let's try to double the size (glibc 2.0)
			size *= 2;
#else  // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return "";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
	}

	return ""; // unreachable
}

std::wstring UnicodeUtils::FormatV(const wchar_t *fmt, va_list args) {
	if (!fmt || !fmt[0])
		return L"";

	int size = FORMAT_BLOCK_SIZE;
	va_list argCopy;

	while (true) {
		wchar_t *cstr = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * size));
		if (!cstr)
			return L"";

		va_copy(argCopy, args);
		int nActual = vswprintf(cstr, size, fmt, argCopy);
		va_end(argCopy);

		if (nActual > -1 && nActual < size) // We got a valid result
				{
			std::wstring str(cstr, nActual);
			free(cstr);
		 	// CLog::Log(LOGINFO, "UnicodeUtils::wFormatV: {}\n", Unicode::wstring_to_utf8(str));
			return str;
		}
		free(cstr);

#ifndef TARGET_WINDOWS
		if (nActual > -1)                   // Exactly what we will need (glibc 2.1)
			size = nActual + 1;
		else
			// Let's try to double the size (glibc 2.0)
			size *= 2;
#else  // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscwprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return L"";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
	}

	return L"";
}
#endif

void UnicodeUtils::ToUpper(std::string &str, const icu::Locale &locale) {
	// std::cout << "ToUpper in: " << str << std::endl;

	if (str.length() == 0)
		return;

	std::string upper = Unicode::toUpper(str, locale);
   //std::cout << "ToUpper in: " << str << " out: " << upper << std::endl;
	str.swap(upper);
}

void UnicodeUtils::ToUpper(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::getICULocale(locale);
  return UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToUpper(std::string &str) {
	if (str.length() == 0)
		return;

	icu::Locale icuLocale = Unicode::getDefaultICULocale();
  UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToUpper(std::wstring &str, const std::locale &locale) {
	if (str.length() == 0)
		return;

  icu::Locale icuLocale = Unicode::getICULocale(locale);
  UnicodeUtils::ToUpper(str, icuLocale);
}


void UnicodeUtils::ToUpper(std::wstring &str, const icu::Locale &locale) {
	if (str.length() == 0)
		return;

	std::string str_utf8 = Unicode::wstring_to_utf8(str);
	const std::string upper = Unicode::toUpper(str_utf8, locale);
   //std::cout << "ToUpper 1 (wide) in: " << str_utf8 << " out: " << upper << std::endl;
	std::wstring wUpper = Unicode::utf8_to_wstring(upper);
	str.swap(wUpper);
}
void UnicodeUtils::ToUpper(std::wstring &str) {
	if (str.length() == 0)
		return;

  icu::Locale icuLocale = Unicode::getDefaultICULocale();
	UnicodeUtils::ToUpper(str, icuLocale);
}

/**
 * Only these two methods with locale arg are true to-lower.
 * TODO: fix this
 */
void UnicodeUtils::ToLower(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::getICULocale(locale);
	return UnicodeUtils::ToLower(str, icuLocale);
}

/**
 * Only these two methods with locale arg are true to-lower.
 * TODO: fix this
 */
void UnicodeUtils::ToLower(std::string &str, const icu::Locale &locale) {
	if (str.length() == 0)
		return;

	std::string lower = Unicode::toLower(str, locale);
   //std::cout << "ToLower 1 in: " << str << " out: " << lower << std::endl;
	str.swap(lower);
}

void UnicodeUtils::ToLower(std::wstring &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::getICULocale(locale);
  return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::ToLower(std::wstring &str, const icu::Locale &locale) {
	if (str.length() == 0)
		return;

	std::string str_utf8 = Unicode::wstring_to_utf8(str);
	const std::string lower_utf8 = Unicode::toLower(str_utf8, locale);
   //std::cout << "ToLower 1 (wide) in: " << str_utf8 << " out: " << lower_utf8 << std::endl;
	std::wstring wLower = Unicode::utf8_to_wstring(lower_utf8);
	str.swap(wLower);
}

void UnicodeUtils::ToLower(std::string &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
	return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::ToLower(std::wstring &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
  return UnicodeUtils::ToLower(str, icuLocale);}

void UnicodeUtils::FoldCase(std::string &str, const StringOptions opt /*  = StringOptions::FOLD_CASE_DEFAULT */) {
	if (str.length() == 0)
		return;

	std::string result = Unicode::toFold(str, opt);
}

void UnicodeUtils::FoldCase(std::wstring &str, const StringOptions opt /*  = StringOptions::FOLD_CASE_DEFAULT */) {
	if (str.length() == 0)
		return;

	Unicode::toFold(str, opt);
	return;
}

void UnicodeUtils::ToCapitalize(std::wstring &str, const icu::Locale &icuLocale) {
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
  std::wstring result = Unicode::toCapitalize(str, icuLocale);
  // TODO: Eliminate swap
  str.swap(result);
#endif
  return;
}

void UnicodeUtils::ToCapitalize(std::wstring &str, const std::locale &locale) {
	icu::Locale icuLocale = Unicode::getICULocale(locale);
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
 return UnicodeUtils::ToCapitalize(str, icuLocale);
#endif
}

void UnicodeUtils::ToCapitalize(std::string &str, const icu::Locale &locale) {
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, locale);
#else
  std::string result = Unicode::toCapitalize(str, locale);
	str.swap(result);
#endif
		return;
}

void UnicodeUtils::ToCapitalize(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::getICULocale(locale);
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
  UnicodeUtils::ToCapitalize(str, icuLocale);
#endif
	return;
}

void UnicodeUtils::ToCapitalize(std::wstring &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
	UnicodeUtils::ToCapitalize(str, icuLocale);
#endif
	return;
}

void UnicodeUtils::ToCapitalize(std::string &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
  UnicodeUtils::ToCapitalize(str, icuLocale);
#endif
	return;
}

void UnicodeUtils::TitleCase(std::wstring &str, const std::locale &locale)
{
  icu::Locale icuLocale = Unicode::getICULocale(locale);
  std::wstring result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

void UnicodeUtils::TitleCase(std::wstring &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
  std::wstring result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

void UnicodeUtils::TitleCase(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::getICULocale(locale);
  std::string result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

void UnicodeUtils::TitleCase(std::string &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
  std::string result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

const std::wstring UnicodeUtils::Normalize(const std::wstring &src,
		const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const NormalizerType normalizerType) {
	return Unicode::normalize(src, opt, normalizerType);
}

const std::string UnicodeUtils::Normalize(const std::string &src,
		const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const NormalizerType normalizerType) {
	return Unicode::normalize(src, opt, normalizerType);
}

bool UnicodeUtils::Equals(const std::string &str1, const std::string &str2) {
	int8_t rc = Unicode::strcmp(str1, 0, str1.length(), str2, 0, str2.length());
	return rc == (int8_t) 0;
}

bool UnicodeUtils::Equals(const std::wstring &str1, const std::wstring &str2) {
	int8_t rc = Unicode::strcmp(str1, 0, str1.length(), str2, 0, str2.length());
	return rc == 0;
}

bool UnicodeUtils::EqualsNoCase(const std::string &str1, const std::string &str2,
		StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {
	// before we do the char-by-char comparison, first compare sizes of both strings.
	// This led to a 33% improvement in benchmarking on average. (size() just returns a member of std::string)
	if (str1.size() == 0 and str2.size() == 0)
		return true;

	if (str1.size() == 0 or str2.size() == 0)
		return false;

	int8_t rc = Unicode::utf8_strcasecmp(str1, str2, opt, normalize);

	rc = rc == 0;
   //std::cout << "EqualsNoCase 1 str1: " << str1 << " str2: " << str2 << " rc: "
	//		<< (int) rc << std::endl;
	return rc;
}

bool UnicodeUtils::EqualsNoCase(const std::string &str1, const char *s2, StringOptions opt /* StringOptions::FOLD_CASE_DEFAULT */,
		const bool normalize /* = false */) {
	std::string str2 = std::string(s2);
	return EqualsNoCase(str1, str2, opt, normalize);
}

bool UnicodeUtils::EqualsNoCase(const char *s1, const char *s2, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const bool normalize /* = false */) {
	std::string str1 = std::string(s1);
	std::string str2 = std::string(s2);
	return EqualsNoCase(str1, str2, opt, normalize);
}


int UnicodeUtils::Compare(const std::wstring &str1, const std::wstring &str2) {

	int32_t str1_length = str1.length();
	int32_t str2_length = str2.length();
	int32_t str1_start = 0;
	int32_t str2_start = 0;
	int rc = Unicode::strcmp(str1, str1_start, str1_length,
			                     str2, str2_start, str2_length);
	// std::cout << "Compare 1 str1: " << str1 << " str2: " << str2 << " n: " << n
	// 		<< " rc: " << (int) rc << std::endl;

	return rc;
}

int UnicodeUtils::Compare(const std::string &str1, const std::string &str2) {

	int32_t str1_length = str1.length();
	int32_t str2_length = str2.length();
	int32_t str1_start = 0;
	int32_t str2_start = 0;
	int rc = Unicode::strcmp(str1, str1_start, str1_length,
			                     str2, str2_start, str2_length);
	// std::cout << "Compare 1 str1: " << str1 << " str2: " << str2 << " n: " << n
	// 		<< " rc: " << (int) rc << std::endl;

	return rc;
}

int UnicodeUtils::CompareNoCase(const std::wstring &str1, const std::wstring &str2,
		StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {

	int rc = Unicode::w_strcasecmp(str1, str2, opt, normalize);
	return rc;
}
int UnicodeUtils::CompareNoCase(const std::string &str1, const std::string &str2,
    StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {
  int rc = Unicode::utf8_strcasecmp(str1, str2, opt, normalize);
  return rc;
}

int UnicodeUtils::CompareNoCase(const char *s1, const char *s2, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {
  std::string str1 = std::string(s1);
  std::string str2 = std::string(s2);
  return UnicodeUtils::CompareNoCase(str1, str2, opt, normalize);
}

int UnicodeUtils::CompareNoCase(const std::string &str1, const std::string &str2,
		size_t n, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */ , const bool normalize /* = false */) {

	// TODO: value of 'n' is questionable in Unicode environment.
	if (n == 0)
	{
		n = std::string::npos;
	}
	else
	{
    if (containsNonAscii(str1))
    {
      CLog::Log(LOGWARNING, "UnicodeUtils::CompareNoCase str1 contains non-ASCII: {}", str1);
    }
    if (containsNonAscii(str2))
    {
        CLog::Log(LOGWARNING, "UnicodeUtils::CompareNoCase str2 contains non-ASCII: {}", str2);
    }
	}

	int rc = Unicode::utf8_strcasecmp(str1, str2, n, opt, normalize);
	// std::cout << "CompareNoCase 1 str1: " << str1 << " str2: " << str2 << " n: " << n
	// 		<< " rc: " << (int) rc << std::endl;

	return rc;
}

int UnicodeUtils::CompareNoCase(const char *s1, const char *s2,
		size_t n /* = 0 */, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {


	std::string str1 = std::string(s1);
	std::string str2 = std::string(s2);
	return UnicodeUtils::CompareNoCase(str1, str2, n, opt, normalize);
}

std::string UnicodeUtils::Left(const std::string &str, const size_t charCount, const bool leftReference)
{
	std::string result = Unicode::Left(str, charCount, leftReference, Unicode::getDefaultICULocale());

	return result;
}

std::string UnicodeUtils::Left(const std::string &str, const size_t charCount, const bool leftReference, const icu::Locale& icuLocale) {
  std::string result = Unicode::Left(str, charCount, leftReference, icuLocale);

  return result;
}

std::string UnicodeUtils::Mid(const std::string &str, const size_t firstCharIndex,
		const size_t charCount /* = std::string::npos */) {
	std::string result = Unicode::Mid(str, firstCharIndex, charCount);
	return result;
}

std::string UnicodeUtils::Right(const std::string &str, const size_t charCount, bool rightReference /* = true */)
{
	std::string result = Unicode::Right(str, charCount, rightReference, Unicode::getDefaultICULocale());
	return result;
}

std::string UnicodeUtils::Right(const std::string &str, const size_t charCount, bool rightReference,
    const icu::Locale &icuLocale)
{
  std::string result = Unicode::Right(str, charCount, rightReference, icuLocale);
  return result;
}

std::string& UnicodeUtils::Trim(std::string &str) {
	std::string result = Unicode::Trim(str);
	str.swap(result);
	return str;
}

std::string& UnicodeUtils::Trim(std::string &str, const char *const chars) {
	std::string delChars = std::string(chars);
	std::string result = Unicode::Trim(str, delChars, true, true);
	 //+CLog::Log(LOGINFO, "UnicodeUtils::Trim str: {} delChars: {} result: {}\n",
	//+		 orig, delChars, str);
  str.swap(result);
	return str;
}

std::string& UnicodeUtils::TrimLeft(std::string &str) {
	std::string orig = std::string(str);
	std::string result = Unicode::TrimLeft(str);
	 //+ CLog::Log(LOGINFO, "UnicodeUtils::TrimLeft str: {} result: {}\n", orig, str);

	str.swap(result);
	 return str;
}

std::string& UnicodeUtils::TrimLeft(std::string &str, const char *const chars) {
	std::string delChars = std::string(chars);
	std::string result = Unicode::Trim(str, delChars, true, false);
	str.swap(result);
	return str;
}

std::string& UnicodeUtils::TrimRight(std::string &str) {
	std::string result = Unicode::TrimRight(str);
  str.swap(result);
	return str;
}

std::string& UnicodeUtils::TrimRight(std::string &str, const char *const chars) {
	std::string delChars = std::string(chars);
	std::string result = Unicode::Trim(str, delChars, false, true);
  str.swap(result);
	return str;
}

int UnicodeUtils::ReturnDigits(const std::string &str)
{
  std::stringstream ss;
  bool digitFound = false;

  //TODO: DELETE ME
  CLog::Log(LOGINFO, "UnicodeUtils::ReturnDigits str: {}\n", str);

  for (const auto& character : str)
  {
    if (isdigit(character))
    {
      digitFound = true;
      ss << character;
    }
    else if (digitFound)
    {
      break;
    }
  }
  return atoi(ss.str().c_str());
}


/*
 * Replaces every occurrence of oldText with newText within the string.
 *
 * Differs from Replace in that it does not return the number of changes
 * made, which makes this version a little more efficient.
 *
 *UnicodeUtils::FindAndReplace(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)'
 *
 */

std::string& UnicodeUtils::FindAndReplace(std::string &str, const std::string oldText,
		const std::string newText) {
	std::string orig = str;
	 Unicode::findAndReplace(str, oldText, newText);
		//CLog::Log(LOGINFO, "UnicodeUtils::FindAndReplace\n");

	 //+CLog::Log(LOGINFO, "UnicodeUtils::FindAndReplace str: {} newText: {} result: {}\n",
	 //+		 orig, newText, str);
	 return str;
}

std::string& UnicodeUtils::FindAndReplace(std::string &str, const char * oldText,
		const char * newText) {
	std::string s_oldText = std::string(oldText);
	std::string s_newText = std::string(newText);
	return UnicodeUtils::FindAndReplace(str, s_oldText, s_newText);
}

/**
 * Regex based version of Replace. See:
 * https://unicode-org.github.io/icu/userguide/strings/regexp.html
 *
 */
std::string UnicodeUtils::RegexReplaceAll(std::string &str, const std::string pattern,
		const std::string newStr, const int flags)  {
	 std::string result = Unicode::RegexReplaceAll(str, pattern, newStr, flags);
		//CLog::Log(LOGINFO, "UnicodeUtils::RegexReplaceAll\n");

	 //+CLog::Log(LOGINFO, "UnicodeUtils::RegexReplaceAll str: {} pattern: {} newStr: {} result: {}\n",
	 //+		 str, pattern, newStr, result);
	 return result;
}


std::string& UnicodeUtils::RemoveDuplicatedSpacesAndTabs(std::string& str)
{
	// Since removing only ASCII spaces and tabs there is no need to convert to/from
	// Unicode.

  std::string::iterator it = str.begin();
  bool onSpace = false;
  while(it != str.end())
  {
    if (*it == '\t')
      *it = ' ';

    if (*it == ' ')
    {
      if (onSpace)
      {
        it = str.erase(it);
        continue;
      }
      else
        onSpace = true;
    }
    else
      onSpace = false;

    ++it;
  }
  return str;
}

/**
 * Replaces every occurrence of oldchar with newChar in str.
 *
 */
int UnicodeUtils::Replace(std::string &str, char oldChar, char newChar) {
	if (not isascii(oldChar)) {
		CLog::Log(LOGWARNING, "UnicodeUtils::Replace oldChar contains non-ASCII: {}\n",
				oldChar);
	}
	if (not isascii(newChar)) {
		CLog::Log(LOGWARNING, "UnicodeUtils::Replace oldChar contains non-ASCII: {}\n",
				newChar);
	}
	std::string oldStr = std::string(1, oldChar);
	std::string newStr = std::string(1, newChar);
	return UnicodeUtils::Replace(str, oldStr, newStr);
}

/**
 * Replaces every occurrence of oldStr with newStr in str. Not regex based.
 */
int UnicodeUtils::Replace(std::string &str, const std::string &oldStr, const std::string &newStr) {
	std::string orig = str;
	if (oldStr.empty() or str.empty())
		return 0;

   // Called by Log.CLog!
   
	std::tuple<std::string, int> result = Unicode::FindCountAndReplace(str, oldStr, newStr);

	std::string resultStr = std::get<0> (result);
	str.swap(resultStr);
	int changes = std::get<1> (result);
	return changes;
}

int UnicodeUtils::Replace(std::wstring &str, const std::wstring &oldStr,
		const std::wstring &newStr) {
	if (oldStr.empty() or str.empty())
		return 0;

	std::string str_utf8 = Unicode::wstring_to_utf8(str);
	std::string str_utf8_save = std::string(str_utf8);
	std::string oldStr_utf8 = Unicode::wstring_to_utf8(oldStr);
	std::string newStr_utf8 = Unicode::wstring_to_utf8(newStr);
   //std::cout << "Replace (wide) str: " << str_utf8_save << "oldStr: "
		//	<< oldStr_utf8 << " newStr: " << newStr_utf8 << std::endl;

	int changes = UnicodeUtils::Replace(str_utf8, oldStr_utf8, newStr_utf8);

	std::wstring result_w = Unicode::utf8_to_wstring(str_utf8);

	//CLog::Log(LOGINFO, "UnicodeUtils::Replace_w\n");

	//+ CLog::Log(LOGINFO, "UnicodeUtils::Replace_w input: {} #changes: %d oldStr: {} newStr: {}\n", str_utf8_save, changes, oldStr_utf8, newStr_utf8);
	//+ CLog::Log(LOGINFO, "UnicodeUtils::Replace str: {}\n", str_utf8);


	str.swap(result_w);
	return changes;
}

bool UnicodeUtils::StartsWith(const std::string &str1, const std::string &str2) {
	return Unicode::StartsWith(str1, str2);
}

bool UnicodeUtils::StartsWith(const std::string &str1, const char *s2) {
	return StartsWith(str1, std::string(s2));
}

bool UnicodeUtils::StartsWith(const char *s1, const char *s2) {
	return StartsWith(std::string(s1), std::string(s2));
}

bool UnicodeUtils::StartsWithNoCase(const std::string &str1, const std::string &str2, StringOptions opt)
{
	if (str1.length() == 0  and str2.length() == 0)
		return true;

	if (str1.length() == 0 or str2.length() == 0)
		return false;

	// In general, different lengths don't tell you if Unicode strings are
	// different.

	return Unicode::StartsWithNoCase(str1, str2, opt);
}

bool UnicodeUtils::StartsWithNoCase(const std::string &str1, const char *s2, StringOptions opt) {
	return StartsWithNoCase(str1, std::string(s2), opt);
}

bool UnicodeUtils::StartsWithNoCase(const char *s1, const char *s2, StringOptions opt) {
	return StartsWithNoCase(std::string(s1), std::string(s2), opt);
}

bool UnicodeUtils::EndsWith(const std::string &str1, const std::string &str2) {
	const bool result = Unicode::EndsWith(str1, str2);
	return result;
}

bool UnicodeUtils::EndsWith(const std::string &str1, const char *s2) {
	std::string str2 = std::string(s2);
	return EndsWith(str1, str2);
}

bool UnicodeUtils::EndsWithNoCase(const std::string &str1, const std::string &str2, StringOptions opt) {
	bool result = Unicode::EndsWithNoCase(str1, str2, opt);
	return result;
}

bool UnicodeUtils::EndsWithNoCase(const std::string &str1, const char *s2, StringOptions opt) {
	std::string str2 = std::string(s2);
	return EndsWithNoCase(str1, str2, opt);
}

std::vector<std::string> UnicodeUtils::Split(const std::string &input,
		const std::string &delimiter, const unsigned int iMaxStrings) {
	if (containsNonAscii(delimiter)) {
		CLog::Log(LOGWARNING, "UnicodeUtils::Split delimiter contains non-ASCII: {}", delimiter);
	}
	std::string orig = input;
	std::vector < std::string > result = std::vector<std::string>();
	if (not (input.empty() or delimiter.empty())) {
		Unicode::SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings,
		    to_underlying(RegexpFlag::UREGEX_LITERAL));
	}
	else {
		if (not input.empty()) // delimiter empty, so just return input, if
			result.push_back(input);     // any. TODO: Verify that we return empty vector.
	}
//  CLog::Log(LOGINFO, "UnicodeUtils::Split\n");

	 //+ CLog::Log(LOGINFO, "UnicodeUtils::Split input: {} delimiter: {} maxStrings: %d\n", orig, delimiter, iMaxStrings);
  //+ for (int i = 0; i < result.size(); i++) {
  //+	CLog::Log(LOGINFO, "UnicodeUtils::    result: {}\n", result[i]);
  //+ }
	return result;
}

std::vector<std::string> UnicodeUtils::Split(const std::string &input,
		const char delimiter, size_t iMaxStrings) {
	if (not isascii(delimiter)) {
		CLog::Log(LOGWARNING, "UnicodeUtils::Split delimiter contains non-ASCII: {}\n", delimiter);
	}
	std::vector < std::string > result;
	std::string sDelimiter = std::string(1, delimiter);
	result = UnicodeUtils::Split(input, sDelimiter, iMaxStrings);

	return result;
}

std::vector<std::string> UnicodeUtils::Split(const std::string &input,
		const std::vector<std::string> &delimiters) {
	for (size_t i = 0; i < delimiters.size(); i++) {
		if (containsNonAscii(delimiters[i])) {
			CLog::Log(LOGWARNING, "UnicodeUtils::Split delimiter contains non-ASCII: {}\n",
					delimiters[i]);
		}
	}
	std::vector < std::string > result = std::vector<std::string>();

	if (input.length() == 0)
		return result;

	if (delimiters.size() == 0) {
		result.push_back(std::string(input)); // Send back a copy
	  return result;
}

	std::string orig = input;
	Unicode::SplitTo(std::back_inserter(result), input, delimiters);
	/*+
	 CLog::Log(LOGINFO, "UnicodeUtils::Split input: {}\n", orig);
	 for (int i = 0; i < delimiters.size(); i++) {
		 CLog::Log(LOGINFO, "UnicodeUtils::Split delimiter: {}\n", delimiters[i]);
	 }
	 for (int i = 0; i < result.size(); i++) {
		 CLog::Log(LOGINFO, "UnicodeUtils::Split result: {}\n", result[i]);
	 }
	 +*/

	return result;
}

// TODO: Need test for this
std::vector<std::string> UnicodeUtils::SplitMulti(
		const std::vector<std::string> &input,
		const std::vector<std::string> &delimiters, size_t iMaxStrings /* = 0 */) {
	for (size_t i = 0; i < delimiters.size(); i++) {
		if (containsNonAscii(delimiters[i])) {
			CLog::Log(LOGWARNING, "UnicodeUtils::SplitMulti delimiter contains non-ASCII: {}\n",
					delimiters[i]);
		}
	}
	return Unicode::SplitMulti(input, delimiters, iMaxStrings);

	/**
	 std::cout << "SplitMulti: #inputs: " << input.size() << " #delims: " << delimiters.size()
	 << " maxStrings: " << iMaxStrings << std::endl;
	 if (input.empty())
	 return std::vector<std::string>();

	 std::vector<std::string> results(input);

	 if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
	 return results;

	 std::vector<std::string> strings1;
	 if (iMaxStrings == 0)
	 {
	 for (size_t di = 0; di < delimiters.size(); di++)
	 {
	 // For each delimiter given us, split(remaining delimiter)

	 for (size_t i = 0; i < results.size(); i++)
	 {
	 // for each input string, split(<input_string>, current_delimiter)
	 //
	 std::vector<std::string> substrings = UnicodeUtils::Split(results[i], delimiters[di]);
	 for (size_t j = 0; j < substrings.size(); j++)
	 // Copy every substring (found by above split) to a new vector.
	 strings1.push_back(substrings[j]);
	 }
	 // Move all of the substrings (produced by the split) is copied to the vector of all
	 // of the substrings found so far. This means results has:
	 // original input strings + substrings from first delimiter ... + substrings from last delimiter
	 //

	 results = strings1;
	 strings1.clear();
	 }
	 return results;
	 }

	 // Control the number of strings input is split into, keeping the original strings.
	 // Note iMaxStrings > input.size()
	 int64_t iNew = iMaxStrings - results.size();
	 for (size_t di = 0; di < delimiters.size(); di++)
	 {
	 for (size_t i = 0; i < results.size(); i++)
	 {
	 if (iNew > 0)
	 {
	 std::vector<std::string> substrings = UnicodeUtils::Split(results[i], delimiters[di], iNew + 1);
	 iNew = iNew - substrings.size() + 1;
	 for (size_t j = 0; j < substrings.size(); j++)
	 strings1.push_back(substrings[j]);
	 }
	 else
	 strings1.push_back(results[i]);
	 }
	 results = strings1;
	 iNew = iMaxStrings - results.size();
	 strings1.clear();
	 if ((iNew <= 0))
	 break;  //Stop trying any more delimiters
	 }
	 return results;
	 */
}

// returns the number of occurrences of strFind in strInput.
int UnicodeUtils::FindNumber(const std::string &strInput,
		const std::string &strFind) {
	int numFound = Unicode::countOccurances(strInput, strFind,
	    to_underlying(RegexpFlag::UREGEX_LITERAL));
	return numFound;
}


bool UnicodeUtils::InitializeCollator(bool normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::getDefaultICULocale(), normalize);
}
bool UnicodeUtils::InitializeCollator(const std::locale &locale, bool normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::getICULocale(locale), normalize);
}

bool UnicodeUtils::InitializeCollator(const icu::Locale &icuLocale, bool normalize /* = false */)
{
  return Unicode::InitializeCollator(icuLocale, normalize);
}


int32_t UnicodeUtils::Collate(const std::wstring &left, const std::wstring &right)
{
	return Unicode::Collate(left, right);
}


// Not sure what the real goal is here. I think it is just to see if the
// strings are the same after case folding. (Normalizing might be a good
// idea too.)
//
// Compares separately the numeric and alphabetic parts of a wide string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical.
// See also the equivalent UnicodeUtils::AlphaNumericCollation() for UFT8 data
int64_t UnicodeUtils::AlphaNumericCompare(const wchar_t *left, const wchar_t *right) {
#ifdef USE_ICU_COLLATOR
	int64_t result = UnicodeUtils::Collate(left, right);
  // CLog::Log(LOGINFO, "UnicodeUtils::AlphaNumericCompare Collate left: {} right: {} result: {}\n",
	//		 Unicode::wstring_to_utf8(left), Unicode::wstring_to_utf8(right), result);
#else
	int64_t result = StringUtils::AlphaNumericCompareOrig(left, right);
  // CLog::Log(LOGINFO, "UnicodeUtils::AlphaNumericCompare_orig left: {} right: {} result: {}\n",
	//		 Unicode::wstring_to_utf8(left), Unicode::wstring_to_utf8(right), result);
#endif
	return result;
}

#if defined(STRINGUTILS_UNICODE_ENABLE)

/*
 Convert the UTF8 character to which z points into a 31-bit Unicode point.
 Return how many bytes (0 to 3) of UTF8 data encode the character.
 This only works right if z points to a well-formed UTF8 string.
 Byte-0    Byte-1    Byte-2    Byte-3     Value
 0xxxxxxx                                 00000000 00000000 0xxxxxxx
 110yyyyy  10xxxxxx                       00000000 00000yyy yyxxxxxx
 1110zzzz  10yyyyyy  10xxxxxx             00000000 zzzzyyyy yyxxxxxx
 11110uuu  10uuzzzz  10yyyyyy  10xxxxxx   000uuuuu zzzzyyyy yyxxxxxx
 */
static uint32_t UTF8ToUnicode(const unsigned char *z, int nKey,
		unsigned char &bytes) {
	// Lookup table used decode the first byte of a multi-byte UTF8 character
	// clang-format off
	static const unsigned char utf8Trans1[] = { 0x00, 0x01, 0x02, 0x03, 0x04,
			0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
			0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
			0x1d, 0x1e, 0x1f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04,
			0x05, 0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00, };
	// clang-format on

	uint32_t c;
	bytes = 0;
	c = z[0];
	if (c >= 0xc0) {
		c = utf8Trans1[c - 0xc0];
		int index = 1;
		while (index < nKey && (z[index] & 0xc0) == 0x80) {
			c = (c << 6) + (0x3f & z[index]);
			index++;
		}
		if (c < 0x80 || (c & 0xFFFFF800) == 0xD800 || (c & 0xFFFFFFFE) == 0xFFFE)
			c = 0xFFFD;
		bytes = static_cast<unsigned char>(index - 1);
	}
	return c;
}

/*
 SQLite collating function, see sqlite3_create_collation
 The equivalent of AlphaNumericCompare() but for comparing UTF8 encoded data

 This only processes enough data to find a difference, and avoids expensive data conversions.
 When sorting in memory item data is converted once to wstring in advance prior to sorting, the
 SQLite callback function can not do that kind of preparation. Instead, in order to use
 AlphaNumericCompare(), it would have to repeatedly convert the full input data to wstring for
 every pair comparison made. That approach was found to be 10 times slower than using this
 separate routine.

 TODO: length is in bytes. What happens when not on character boundary?
       Is normalization necessary?
 */

int UnicodeUtils::AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2)
{
  // Get exact matches of shorter text to start of larger test fast
  int n = std::min(nKey1, nKey2);
  int r = memcmp(pKey1, pKey2, n);
  if (r == 0)
    return nKey1 - nKey2;

  //Not a binary match, so process character at a time
  const unsigned char* zA = static_cast<const unsigned char*>(pKey1);
  const unsigned char* zB = static_cast<const unsigned char*>(pKey2);
  wchar_t lc, rc;
  unsigned char bytes;
  int64_t lnum, rnum;
  bool lsym, rsym;
  int ld, rd;
  int i = 0;
  int j = 0;
  // Looping Unicode point at a time through potentially 1 to 4 multi-byte encoded UTF8 data
  while (i < nKey1 && j < nKey2)
  {
    // Check if we have numerical values, compare only up to 15 digits
    if (isdigit(zA[i]) && isdigit(zB[j]))
    {
      lnum = zA[i] - '0';
      ld = i + 1;
      while (ld < nKey1 && isdigit(zA[ld]) && ld < i + 15)
      {
        lnum *= 10;
        lnum += zA[ld] - '0';
        ld++;
      }
      rnum = zB[j] - '0';
      rd = j + 1;
      while (rd < nKey2 && isdigit(zB[rd]) && rd < j + 15)
      {
        rnum *= 10;
        rnum += zB[rd] - '0';
        rd++;
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return static_cast<int>(lnum - rnum);
      }
      // Advance to after digits
      i = ld;
      j = rd;
      continue;
    }
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ before the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (zA[i] >= 32 && zA[i] < '0') || (zA[i] > '9' && zA[i] < 'A') ||
           (zA[i] > 'Z' && zA[i] < 'a') || (zA[i] > 'z' && zA[i] < 128);
    rsym = (zB[j] >= 32 && zB[j] < '0') || (zB[j] > '9' && zB[j] < 'A') ||
           (zB[j] > 'Z' && zB[j] < 'a') || (zB[j] > 'z' && zB[j] < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (zA[i] != zB[j])
        return zA[i] - zB[j];
      else
      { // Same symbol advance to next
        i++;
        j++;
        continue;
      }
    }
    //Decode single (1 to 4 bytes) UTF8 character to Unicode
    lc = UTF8ToUnicode(&zA[i], nKey1 - i, bytes);
    i += bytes;
    rc = UTF8ToUnicode(&zB[j], nKey2 - j, bytes);
    j += bytes;
    if (!g_langInfo.UseLocaleCollation())
    {
      // Apply case sensitive accent folding collation to non-ascii chars.
      // This mimics utf8_general_ci collation, and provides simple collation of LATIN-1 chars
      // for any platform that doesn't have a language specific collate facet implemented
      if (lc > 128)
        lc = GetCollationWeight(lc);
      if (rc > 128)
        rc = GetCollationWeight(rc);
    }
    // Caseless comparison so convert ascii upper case to lower case
    if (lc >= 'A' && lc <= 'Z')
      lc += 'a' - 'A';
    if (rc >= 'A' && rc <= 'Z')
      rc += 'a' - 'A';

    if (lc != rc)
    {
      if (!g_langInfo.UseLocaleCollation() || (lc <= 128 && rc <= 128))
        // Compare unicode (having applied accent folding collation to non-ascii chars).
        return lc - rc;
      else
      {
        // Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not language specific but just compares unicode
        const std::collate<wchar_t>& coll =
            std::use_facet<std::collate<wchar_t>>(g_langInfo.GetSystemLocale());
        int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
        if (cmp_res != 0)
          return cmp_res;
      }
    }
    i++;
    j++;
  }
  // Compared characters of shortest are the same as longest, length determines order
  return (nKey1 - nKey2);
}
#endif

int UnicodeUtils::DateStringToYYYYMMDD(const std::string &dateString) {
	// TODO: I assume this is a fixed format for a database or something?
	std::vector < std::string > days = UnicodeUtils::Split(dateString, '-');
	if (days.size() == 1)
		return atoi(days[0].c_str());
	else if (days.size() == 2)
		return atoi(days[0].c_str()) * 100 + atoi(days[1].c_str());
	else if (days.size() == 3)
		return atoi(days[0].c_str()) * 10000 + atoi(days[1].c_str()) * 100
				+ atoi(days[2].c_str());
	else
		return -1;
}
#if defined(STRINGUTILS_UNICODE_ENABLE)

std::string UnicodeUtils::ISODateToLocalizedDate(const std::string &strIsoDate) {
	// Convert ISO8601 date strings YYYY, YYYY-MM, or YYYY-MM-DD to (partial) localized date strings
	// TODO: ICU has date formatters for many locales
	CDateTime date;
	std::string formattedDate = strIsoDate;
	if (formattedDate.size() == 10) {
		date.SetFromDBDate(strIsoDate);
		formattedDate = date.GetAsLocalizedDate();
	} else if (formattedDate.size() == 7) {
		std::string strFormat = date.GetAsLocalizedDate(false);
		std::string tempdate;
		// find which date separator we are using.  Can be -./
		size_t pos = strFormat.find_first_of("-./");
		if (pos != std::string::npos) {
			bool yearFirst = strFormat.find("1601") == 0; // true if year comes first
			std::string sep = strFormat.substr(pos, 1);
			if (yearFirst) { // build formatted date with year first, then separator and month
				tempdate = formattedDate.substr(0, 4);
				tempdate += sep;
				tempdate += formattedDate.substr(5, 2);
			} else {
				tempdate = formattedDate.substr(5, 2);
				tempdate += sep;
				tempdate += formattedDate.substr(0, 4);
			}
			formattedDate = tempdate;
		}
		// return either just the year or the locally formatted version of the ISO date
	}
	return formattedDate;
}
#endif

long UnicodeUtils::TimeStringToSeconds(const std::string &timeString) {
	std::string strCopy(timeString);
	UnicodeUtils::Trim(strCopy);
	if (UnicodeUtils::EndsWithNoCase(strCopy, " min")) {
		// this is imdb format of "XXX min"
		return 60 * atoi(strCopy.c_str());
	} else {
		std::vector < std::string > secs = UnicodeUtils::Split(strCopy, ':');
		int timeInSecs = 0;

		// TODO: This looks odd. Verify

		for (unsigned int i = 0; i < 3 && i < secs.size(); i++) {
			timeInSecs *= 60;
			timeInSecs += atoi(secs[i].c_str());
		}
		return timeInSecs;
	}
}

std::string UnicodeUtils::SecondsToTimeString(long lSeconds, TIME_FORMAT format)
{
  bool isNegative = lSeconds < 0;
  lSeconds = std::abs(lSeconds);

  std::string strHMS;
  if (format == TIME_FORMAT_SECS)
    strHMS = std::to_string(lSeconds);
  else if (format == TIME_FORMAT_MINS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 60.0f));
  else if (format == TIME_FORMAT_HOURS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 3600.0f));
  else if (format & TIME_FORMAT_M)
    strHMS += std::to_string(lSeconds % 3600 / 60);
  else
  {
    int hh = lSeconds / 3600;
    lSeconds = lSeconds % 3600;
    int mm = lSeconds / 60;
    int ss = lSeconds % 60;

    if (format == TIME_FORMAT_GUESS)
      format = (hh >= 1) ? TIME_FORMAT_HH_MM_SS : TIME_FORMAT_MM_SS;
    if (format & TIME_FORMAT_HH)
      strHMS += StringUtils::Format("{:02}", hh);
    else if (format & TIME_FORMAT_H)
      strHMS += std::to_string(hh);
    if (format & TIME_FORMAT_MM)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", mm);
    if (format & TIME_FORMAT_SS)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", ss);
  }

  if (isNegative)
    strHMS = "-" + strHMS;

  return strHMS;
}

bool UnicodeUtils::IsNaturalNumber(const std::string& str)
{
	// Since this function is only looking for whitespace and
	// digits, it is reasonably safe to assume single byte.
	// If by some reason, multibyte utf-8 is passed, then
	// it is highly unlikely (probably impossible) for
	// this to return true.

  size_t i = 0, n = 0;
  // allow whitespace,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

/*
 * Returns true if string contains an optional 'minus'
 * followed by a series of consecutive
 * digits [0-9] surrounded by nothing but 'whitespace.
 *
 * Like isNaturalNumber, above, this likely does not require
 * Unicode conversion.
 *
 * C++ defines isspace to be at least: [ \t\n\v\f\r], depending
 * upon locale.
 *
 * Should be nearly Equivalent to the ICU string matching
 * regex: '[\\h]*=-?[0-9]+[-\\s]*'
 * Where \\s matches whitespace characters: [\t\n\f\r\p{Z}]
 *
 * TODO: Is there any demand for a leading '+'?
 */
bool UnicodeUtils::IsInteger(const std::string& str)
{
  size_t i = 0, n = 0;
  // allow whitespace,-,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  if (i < str.size() && str[i] == '-')
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

int UnicodeUtils::asciidigitvalue(char chr) {
	if (!isasciidigit(chr))
		return -1;

	return chr - '0';
}

int UnicodeUtils::asciixdigitvalue(char chr) {
	int v = asciidigitvalue(chr);
	if (v >= 0)
		return v;
	if (chr >= 'a' && chr <= 'f')
		return chr - 'a' + 10;
	if (chr >= 'A' && chr <= 'F')
		return chr - 'A' + 10;

	return -1;
}

void UnicodeUtils::RemoveCRLF(std::string &strLine) {
	UnicodeUtils::TrimRight(strLine, "\n\r");
}

std::string UnicodeUtils::SizeToString(int64_t size)
{
  std::string strLabel;
  constexpr std::array<char, 9> prefixes = {' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
  unsigned int i = 0;
  double s = (double)size;
  while (i < prefixes.size() && s >= 1000.0)
  {
    s /= 1024.0;
    i++;
  }

  if (!i)
    strLabel = StringUtils::Format("{:.2f} B", s);
  else if (i == prefixes.size())
  {
    if (s >= 1000.0)
      strLabel = StringUtils::Format(">999.99 {}B", prefixes[i - 1]);
    else
      strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i - 1]);
  }
  else if (s >= 100.0)
    strLabel = StringUtils::Format("{:.1f} {}B", s, prefixes[i]);
  else
    strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i]);

  return strLabel;
}

#if defined(STRINGUTILS_UNICODE_ENABLE)
std::string UnicodeUtils::BinaryStringToString(const std::string& in)
{
  std::string out;
  out.reserve(in.size() / 2);
  for (const char *cur = in.c_str(), *end = cur + in.size(); cur != end; ++cur) {
    if (*cur == '\\') {
      ++cur;
      if (cur == end) {
        break;
      }
      if (isdigit(*cur)) {
        char* end;
        unsigned long num = strtol(cur, &end, 10);
        cur = end - 1;
        out.push_back(num);
        continue;
      }
    }
    out.push_back(*cur);
  }
  return out;
}

std::string UnicodeUtils::ToHexadecimal(const std::string& in)
{
  std::ostringstream ss;
  ss << std::hex;
  for (unsigned char ch : in) {
    ss << std::setw(2) << std::setfill('0') << static_cast<unsigned long> (ch);
  }
  return ss.str();
}

size_t UnicodeUtils::FindWord(const std::string &str, const std::string &word)
{
  size_t index;
  // Ensure searched for word begins on a "word" boundary.
  // \b matches boundary between word and non-word character
  // std::string pattern = "((\d+)|([a-z]+|.)\s*)(\\Q" + word + "\\E)";  // \\Q does not interpret as regex \\E
#ifdef USE_FINDWORD_REGEX
  std::string pattern = "((\\Q" + word + "\\E)|(((?:(?:\\d++(?=[^\\d]))|(?:[a-z]++(?=[^a-z]))|(?:[^\\d\\sa-z]))\\s*+(?=[^\\s]))(\\Q" + word + "\\E)))";
  int flags = to_underlying(RegexpFlag::UREGEX_CASE_INSENSITIVE);
  index = Unicode::regexFind(str, pattern, flags);
#else
  index = Unicode::FindWord(str, word);
#endif
	return index;
}
#endif

/*
 * Guaranteed to work ONLY with ASCII 'brackets'. String may be utf-8.
 */
int UnicodeUtils::FindEndBracket(const std::string &str, char opener, char closer, int startPos)
{
	if (not isascii(opener)) {
	 	CLog::Log(LOGWARNING, "UnicodeUtils::FindEndBracket opener is non-ASCII: {}\n", opener);
	}
	else if (not isascii(closer)) {
	 	CLog::Log(LOGWARNING, "UnicodeUtils::FindEndBracket closer is non-ASCII: {}\n", closer);
	}
  int blocks = 1;
  for (unsigned int i = startPos; i < str.size(); i++)
  {
    if (str[i] == opener)
      blocks++;
    else if (str[i] == closer)
    {
      blocks--;
      if (!blocks)
        return i;
    }
  }

  return (int)std::string::npos;
}

/*
// assumes it is called from after the first open bracket is found
// TODO: Should 'brackets' (opener/closer) be constrained to a single
// byte?
//
// TODO: Is startPos in bytes?
size_t UnicodeUtils::FindEndBracket(const std::string &str, char opener,
		char closer, size_t startPos) {
	std::cout << "FindEndBracket str: " << str << std::endl;

	size_t result = Unicode::FindEndBracket(str, opener, closer, startPos);
	//CLog::Log(LOGINFO, "UnicodeUtils::FindEndBracket\n");

	 //+CLog::Log(LOGINFO, "UnicodeUtils::FindEndBracket str: {} opener: {} closer: {} startPos: %d result: %d\n",
		//+	 str, opener, closer, startPos, result);

	return result;
}
*/
void UnicodeUtils::WordToDigits(std::string &word)
{

	// Works ONLY with ASCII!

  static const char word_to_letter[] = "22233344455566677778889999";
  UnicodeUtils::ToLower(word);
  for (unsigned int i = 0; i < word.size(); ++i)
  { // NB: This assumes ascii, which probably needs extending at some  point.
    char letter = word[i];
		if (letter > 0x7f) {
	    CLog::Log(LOGWARNING, "UnicodeUtils.WordToDigits: Non-ASCII input: {}\n",
	    		word);
		}

    if ((letter >= 'a' && letter <= 'z')) // assume contiguous letter range
    {
      word[i] = word_to_letter[letter-'a'];
    }
    else if (letter < '0' || letter > '9') // We want to keep 0-9!
    {
      word[i] = ' ';  // replace everything else with a space
    }
  }
}

#if defined(STRINGUTILS_UNICODE_ENABLE)
std::string UnicodeUtils::CreateUUID()
{
#ifdef HAVE_NEW_CROSSGUID
#ifdef TARGET_ANDROID
  JNIEnv* env = xbmc_jnienv();
  return xg::newGuid(env).str();
#else
  return xg::newGuid().str();
#endif /* TARGET_ANDROID */
#else
  static GuidGenerator guidGenerator;
  auto guid = guidGenerator.newGuid();

  std::stringstream strGuid; strGuid << guid;
  return strGuid.str();
#endif
}

bool UnicodeUtils::ValidateUUID(const std::string &uuid) {
	CRegExp guidRE;
	guidRE.RegComp(ADDON_GUID_RE);
	return (guidRE.RegFind(uuid.c_str()) == 0);
}

double UnicodeUtils::CompareFuzzy(const std::string &left,
		const std::string &right) {
	/*  TODO: Unicode. Examine if this works with Unicode. Does it need locale?
	 *        Should strings be case folded and normalized first?
	 *        Other packages exist (mostly python) that take locale
	 *        most are vague about Unicode.
	 *        Google's BERT and TensorFlow are interesting (mostly python).
	 */

	std::cout << "CompareFuzzy left: " << left << " right: " << right << std::endl;
	return (0.5
			+ fstrcmp(left.c_str(), right.c_str()) * (left.length() + right.length()))
			/ 2.0;
}

int UnicodeUtils::FindBestMatch(const std::string &str,
		const std::vector<std::string> &strings, double &matchscore) {

	// TODO: NOT Unicode safe

	int best = -1;
	matchscore = 0;

	int i = 0;
	for (std::vector<std::string>::const_iterator it = strings.begin();
			it != strings.end(); ++it, i++) {
		int maxlength = std::max(str.length(), it->length());
		double score = UnicodeUtils::CompareFuzzy(str, *it) / maxlength;
		if (score > matchscore) {
			matchscore = score;
			best = i;
		}
	}
	std::cout << "FindBestMatch score: " << matchscore << " best: " << best << std::endl;
	return best;
}

// Looks for EXACT match (no case folding)

bool UnicodeUtils::ContainsKeyword(const std::string &str,
		const std::vector<std::string> &keywords) {

	// TODO: Needs test

	return Unicode::Contains(str, keywords);
}

size_t UnicodeUtils::utf8_strlen(const char *s) {
	size_t length = 0;
	while (*s) {
		if (U8_IS_LEAD(*s++))
			length ++;
	}

	return length;
}
#endif

std::string UnicodeUtils::Paramify(const std::string &param) {
	std::string result = param;
   //std::cout << "UnicodeUtils.Paramify param: " << param << std::endl;
	// escape backspaces
	UnicodeUtils::Replace(result, "\\", "\\\\");
	// escape double quotes
	UnicodeUtils::Replace(result, "\"", "\\\"");

	// add double quotes around the whole string
	return "\"" + result + "\"";
}

#if defined(STRINGUTILS_UNICODE_ENABLE)
/**
 * Note: delimiters is a string of one or more ASCII single-character delimiters.
 *       input may be utf-8.
 */
std::vector<std::string> UnicodeUtils::Tokenize(const std::string &input,
		const std::string &delimiters) {
	// TODO:  Need Tests!!

	std::vector < std::string > tokens = std::vector<std::string>();
	Tokenize(input, tokens, delimiters);
	return tokens;
}

/**
 * Note: delimiters is a string of one or more ASCII single-character delimiters.
 *       input may be utf-8.
 */

void UnicodeUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters)
{
	if (containsNonAscii(delimiters)) {
	 	CLog::Log(LOGWARNING, "UnicodeUtils::Tokenize contains non-ASCII delimiter: {}\n", delimiters);
	}
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiters);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find_first_of(delimiters, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiters, nextDelimPos);
  }
}

/*
void UnicodeUtils::Tokenize(const std::string &input,
		std::vector<std::string> &tokens, const std::string &delimiters) {
	// TODO: Need Tests!

	Unicode::Tokenize(input, tokens, delimiters);
  return;
}
*/

/**
 * Note: delimiter is a single ASCII character that separates tokens.
 *       input may be utf-8.
 */
std::vector<std::string> UnicodeUtils::Tokenize(const std::string &input,
		const char delimiter) {
	std::vector < std::string > tokens;
	Tokenize(input, tokens, delimiter);
	return tokens;
}

/**
 * Note: delimiter is a single ASCII character that separates tokens.
 *       input may be utf-8.
 */

void UnicodeUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const char delimiter)
{
  tokens.clear();
	if (not isascii(delimiter)) {
	 	CLog::Log(LOGWARNING, "UnicodeUtils::Tokenize contains non-ASCII delimiter: {}s\n", delimiter);
	}
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiter);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find(delimiter, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiter, nextDelimPos);
  }
}

/*
void UnicodeUtils::Tokenize(const std::string &input,
		std::vector<std::string> &tokens, const char delimiter) {

	std::string sDelimiter = std::string();
	sDelimiter.append(1, delimiter);
	Unicode::Tokenize(input, tokens, sDelimiter);
  return;
}
*/
#endif

#if defined(STRINGUTILS_UNICODE_ENABLE)
uint64_t UnicodeUtils::ToUint64(const std::string &str,
		uint64_t fallback) noexcept {
	std::istringstream iss(str);
	uint64_t result(fallback);
	iss >> result;
	return result;
}

std::string UnicodeUtils::FormatFileSize(uint64_t bytes)
{
  const std::array<std::string, 6> units{{"B", "kB", "MB", "GB", "TB", "PB"}};
  if (bytes < 1000)
    return Format("{}B", bytes);

  size_t i = 0;
  double value = static_cast<double>(bytes);
  while (i + 1 < units.size() && value >= 999.5)
  {
    ++i;
    value /= 1024.0;
  }
  unsigned int decimals = value < 9.995 ? 2 : (value < 99.95 ? 1 : 0);
  return Format("{:.{}f}{}", value, decimals, units[i]);
}
#endif

#if defined(STRINGUTILS_UNICODE_ENABLE)

std::string UnicodeUtils::CreateFromCString(const char* cstr)
{
  return cstr != nullptr ? std::string(cstr) : std::string();
}
#endif
