/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
//-----------------------------------------------------------------------
//
//  File:      StringUtils.cpp
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
#include "StringUtils.h"
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
#include "utils/UnicodeUtils.h"

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
const std::string StringUtils::Empty = "";

std::string StringUtils::FormatV(const char *fmt, va_list args) {
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
		 	//CLog::Log(LOGINFO, "StringUtils::FormatV: {}\n", str);
		 	//std::cout << "StringUtils::FormatV: " << str << std::endl;

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

std::wstring StringUtils::FormatV(const wchar_t *fmt, va_list args) {
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
		 	// CLog::Log(LOGINFO, "StringUtils::wFormatV: {}\n", Unicode::wstring_to_utf8(str));
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

void StringUtils::ToCapitalize(std::wstring &str, const icu::Locale &icuLocale) {
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
  std::wstring result = Unicode::toCapitalize(str, icuLocale);
  // TODO: Eliminate swap
  str.swap(result);
#endif
  return;
}

void StringUtils::ToCapitalize(std::wstring &str, const std::locale &locale) {
	icu::Locale icuLocale = Unicode::getICULocale(locale);
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
 return StringUtils::ToCapitalize(str, icuLocale);
#endif
}

void StringUtils::ToCapitalize(std::string &str, const icu::Locale &locale) {
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, locale);
#else
  std::string result = Unicode::toCapitalize(str, locale);
	str.swap(result);
#endif
		return;
}

void StringUtils::ToCapitalize(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::getICULocale(locale);
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
  StringUtils::ToCapitalize(str, icuLocale);
#endif
	return;
}

void StringUtils::ToCapitalize(std::wstring &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
	StringUtils::ToCapitalize(str, icuLocale);
#endif
	return;
}

void StringUtils::ToCapitalize(std::string &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::toTitle(str, icuLocale);
#else
  StringUtils::ToCapitalize(str, icuLocale);
#endif
	return;
}

void StringUtils::TitleCase(std::wstring &str, const std::locale &locale)
{
  icu::Locale icuLocale = Unicode::getICULocale(locale);
  std::wstring result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

void StringUtils::TitleCase(std::wstring &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
  std::wstring result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

void StringUtils::TitleCase(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::getICULocale(locale);
  std::string result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

void StringUtils::TitleCase(std::string &str) {
  icu::Locale icuLocale = Unicode::getDefaultICULocale();
  std::string result = Unicode::toTitle(str, icuLocale);
  str.swap(result);
  return;
}

const std::wstring StringUtils::Normalize(const std::wstring &src,
		const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const NormalizerType normalizerType) {
	return Unicode::normalize(src, opt, normalizerType);
}

const std::string StringUtils::Normalize(const std::string &src,
		const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const NormalizerType normalizerType) {
	return Unicode::normalize(src, opt, normalizerType);
}

bool StringUtils::Equals(const std::string &str1, const std::string &str2) {
	int8_t rc = Unicode::strcmp(str1, 0, str1.length(), str2, 0, str2.length());
	return rc == (int8_t) 0;
}

bool StringUtils::Equals(const std::wstring &str1, const std::wstring &str2) {
	int8_t rc = Unicode::strcmp(str1, 0, str1.length(), str2, 0, str2.length());
	return rc == 0;
}

bool StringUtils::EqualsNoCase(const std::string &str1, const std::string &str2,
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

bool StringUtils::EqualsNoCase(const std::string &str1, const char *s2, StringOptions opt /* StringOptions::FOLD_CASE_DEFAULT */,
		const bool normalize /* = false */) {
	std::string str2 = std::string(s2);
	return EqualsNoCase(str1, str2, opt, normalize);
}

bool StringUtils::EqualsNoCase(const char *s1, const char *s2, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const bool normalize /* = false */) {
	std::string str1 = std::string(s1);
	std::string str2 = std::string(s2);
	return EqualsNoCase(str1, str2, opt, normalize);
}


int StringUtils::Compare(const std::wstring &str1, const std::wstring &str2) {

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

int StringUtils::Compare(const std::string &str1, const std::string &str2) {

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

int StringUtils::CompareNoCase(const std::wstring &str1, const std::wstring &str2,
		StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {

	int rc = Unicode::w_strcasecmp(str1, str2, opt, normalize);
	return rc;
}
int StringUtils::CompareNoCase(const std::string &str1, const std::string &str2,
    StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {
  int rc = Unicode::utf8_strcasecmp(str1, str2, opt, normalize);
  return rc;
}

int StringUtils::CompareNoCase(const char *s1, const char *s2, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {
  std::string str1 = std::string(s1);
  std::string str2 = std::string(s2);
  return StringUtils::CompareNoCase(str1, str2, opt, normalize);
}

int StringUtils::CompareNoCase(const std::string &str1, const std::string &str2,
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
      CLog::Log(LOGWARNING, "StringUtils::CompareNoCase str1 contains non-ASCII: {}", str1);
    }
    if (containsNonAscii(str2))
    {
        CLog::Log(LOGWARNING, "StringUtils::CompareNoCase str2 contains non-ASCII: {}", str2);
    }
	}

	int rc = Unicode::utf8_strcasecmp(str1, str2, n, opt, normalize);
	// std::cout << "CompareNoCase 1 str1: " << str1 << " str2: " << str2 << " n: " << n
	// 		<< " rc: " << (int) rc << std::endl;

	return rc;
}

int StringUtils::CompareNoCase(const char *s1, const char *s2,
		size_t n /* = 0 */, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool normalize /* = false */) {


	std::string str1 = std::string(s1);
	std::string str2 = std::string(s2);
	return StringUtils::CompareNoCase(str1, str2, n, opt, normalize);
}

std::string StringUtils::Left(const std::string &str, const size_t charCount, const bool leftReference)
{
	std::string result = Unicode::Left(str, charCount, leftReference, Unicode::getDefaultICULocale());

	return result;
}

std::string StringUtils::Left(const std::string &str, const size_t charCount, const bool leftReference, const icu::Locale& icuLocale) {
  std::string result = Unicode::Left(str, charCount, leftReference, icuLocale);

  return result;
}

std::string StringUtils::Mid(const std::string &str, const size_t firstCharIndex,
		const size_t charCount /* = std::string::npos */) {
	std::string result = Unicode::Mid(str, firstCharIndex, charCount);
	return result;
}

std::string StringUtils::Right(const std::string &str, const size_t charCount, bool rightReference /* = true */)
{
	std::string result = Unicode::Right(str, charCount, rightReference, Unicode::getDefaultICULocale());
	return result;
}

std::string StringUtils::Right(const std::string &str, const size_t charCount, bool rightReference,
    const icu::Locale &icuLocale)
{
  std::string result = Unicode::Right(str, charCount, rightReference, icuLocale);
  return result;
}

std::string& StringUtils::Trim(std::string &str) {
	std::string result = Unicode::Trim(str);
	str.swap(result);
	return str;
}

std::string& StringUtils::Trim(std::string &str, const char *const chars) {
	std::string delChars = std::string(chars);
	std::string result = Unicode::Trim(str, delChars, true, true);
	 //+CLog::Log(LOGINFO, "StringUtils::Trim str: {} delChars: {} result: {}\n",
	//+		 orig, delChars, str);
  str.swap(result);
	return str;
}

std::string& StringUtils::TrimLeft(std::string &str) {
	std::string orig = std::string(str);
	std::string result = Unicode::TrimLeft(str);
	 //+ CLog::Log(LOGINFO, "StringUtils::TrimLeft str: {} result: {}\n", orig, str);

	str.swap(result);
	 return str;
}

std::string& StringUtils::TrimLeft(std::string &str, const char *const chars) {
	std::string delChars = std::string(chars);
	std::string result = Unicode::Trim(str, delChars, true, false);
	str.swap(result);
	return str;
}

std::string& StringUtils::TrimRight(std::string &str) {
	std::string result = Unicode::TrimRight(str);
  str.swap(result);
	return str;
}

std::string& StringUtils::TrimRight(std::string &str, const char *const chars) {
	std::string delChars = std::string(chars);
	std::string result = Unicode::Trim(str, delChars, false, true);
  str.swap(result);
	return str;
}

int StringUtils::ReturnDigits(const std::string &str)
{
  std::stringstream ss;
  bool digitFound = false;

  //TODO: DELETE ME
  CLog::Log(LOGINFO, "StringUtils::ReturnDigits str: {}\n", str);

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
 *StringUtils::FindAndReplace(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)'
 *
 */

std::string& StringUtils::FindAndReplace(std::string &str, const std::string oldText,
		const std::string newText) {
	std::string orig = str;
	 Unicode::findAndReplace(str, oldText, newText);
		//CLog::Log(LOGINFO, "StringUtils::FindAndReplace\n");

	 //+CLog::Log(LOGINFO, "StringUtils::FindAndReplace str: {} newText: {} result: {}\n",
	 //+		 orig, newText, str);
	 return str;
}

std::string& StringUtils::FindAndReplace(std::string &str, const char * oldText,
		const char * newText) {
	std::string s_oldText = std::string(oldText);
	std::string s_newText = std::string(newText);
	return StringUtils::FindAndReplace(str, s_oldText, s_newText);
}

/**
 * Regex based version of Replace. See:
 * https://unicode-org.github.io/icu/userguide/strings/regexp.html
 *
 */
std::string StringUtils::RegexReplaceAll(std::string &str, const std::string pattern,
		const std::string newStr, const int flags)  {
	 std::string result = Unicode::RegexReplaceAll(str, pattern, newStr, flags);
		//CLog::Log(LOGINFO, "StringUtils::RegexReplaceAll\n");

	 //+CLog::Log(LOGINFO, "StringUtils::RegexReplaceAll str: {} pattern: {} newStr: {} result: {}\n",
	 //+		 str, pattern, newStr, result);
	 return result;
}


std::string& StringUtils::RemoveDuplicatedSpacesAndTabs(std::string& str)
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
int StringUtils::Replace(std::string &str, char oldChar, char newChar) {
	if (not isascii(oldChar)) {
		CLog::Log(LOGWARNING, "StringUtils::Replace oldChar contains non-ASCII: {}\n",
				oldChar);
	}
	if (not isascii(newChar)) {
		CLog::Log(LOGWARNING, "StringUtils::Replace oldChar contains non-ASCII: {}\n",
				newChar);
	}
	std::string oldStr = std::string(1, oldChar);
	std::string newStr = std::string(1, newChar);
	return StringUtils::Replace(str, oldStr, newStr);
}

/**
 * Replaces every occurrence of oldStr with newStr in str. Not regex based.
 */
int StringUtils::Replace(std::string &str, const std::string &oldStr, const std::string &newStr) {
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

int StringUtils::Replace(std::wstring &str, const std::wstring &oldStr,
		const std::wstring &newStr) {
	if (oldStr.empty() or str.empty())
		return 0;

	std::string str_utf8 = Unicode::wstring_to_utf8(str);
	std::string str_utf8_save = std::string(str_utf8);
	std::string oldStr_utf8 = Unicode::wstring_to_utf8(oldStr);
	std::string newStr_utf8 = Unicode::wstring_to_utf8(newStr);
   //std::cout << "Replace (wide) str: " << str_utf8_save << "oldStr: "
		//	<< oldStr_utf8 << " newStr: " << newStr_utf8 << std::endl;

	int changes = StringUtils::Replace(str_utf8, oldStr_utf8, newStr_utf8);

	std::wstring result_w = Unicode::utf8_to_wstring(str_utf8);

	//CLog::Log(LOGINFO, "StringUtils::Replace_w\n");

	//+ CLog::Log(LOGINFO, "StringUtils::Replace_w input: {} #changes: %d oldStr: {} newStr: {}\n", str_utf8_save, changes, oldStr_utf8, newStr_utf8);
	//+ CLog::Log(LOGINFO, "StringUtils::Replace str: {}\n", str_utf8);


	str.swap(result_w);
	return changes;
}

bool StringUtils::StartsWith(const std::string &str1, const std::string &str2) {
	return Unicode::StartsWith(str1, str2);
}

bool StringUtils::StartsWith(const std::string &str1, const char *s2) {
	return StartsWith(str1, std::string(s2));
}

bool StringUtils::StartsWith(const char *s1, const char *s2) {
	return StartsWith(std::string(s1), std::string(s2));
}

bool StringUtils::StartsWithNoCase(const std::string &str1, const std::string &str2, StringOptions opt)
{
	if (str1.length() == 0  and str2.length() == 0)
		return true;

	if (str1.length() == 0 or str2.length() == 0)
		return false;

	// In general, different lengths don't tell you if Unicode strings are
	// different.

	return Unicode::StartsWithNoCase(str1, str2, opt);
}

bool StringUtils::StartsWithNoCase(const std::string &str1, const char *s2, StringOptions opt) {
	return StartsWithNoCase(str1, std::string(s2), opt);
}

bool StringUtils::StartsWithNoCase(const char *s1, const char *s2, StringOptions opt) {
	return StartsWithNoCase(std::string(s1), std::string(s2), opt);
}

bool StringUtils::EndsWith(const std::string &str1, const std::string &str2) {
	const bool result = Unicode::EndsWith(str1, str2);
	return result;
}

bool StringUtils::EndsWith(const std::string &str1, const char *s2) {
	std::string str2 = std::string(s2);
	return EndsWith(str1, str2);
}

bool StringUtils::EndsWithNoCase(const std::string &str1, const std::string &str2, StringOptions opt) {
	bool result = Unicode::EndsWithNoCase(str1, str2, opt);
	return result;
}

bool StringUtils::EndsWithNoCase(const std::string &str1, const char *s2, StringOptions opt) {
	std::string str2 = std::string(s2);
	return EndsWithNoCase(str1, str2, opt);
}

std::vector<std::string> StringUtils::Split(const std::string &input,
		const std::string &delimiter, const unsigned int iMaxStrings) {
	if (containsNonAscii(delimiter)) {
		CLog::Log(LOGWARNING, "StringUtils::Split delimiter contains non-ASCII: {}", delimiter);
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
//  CLog::Log(LOGINFO, "StringUtils::Split\n");

	 //+ CLog::Log(LOGINFO, "StringUtils::Split input: {} delimiter: {} maxStrings: %d\n", orig, delimiter, iMaxStrings);
  //+ for (int i = 0; i < result.size(); i++) {
  //+	CLog::Log(LOGINFO, "StringUtils::    result: {}\n", result[i]);
  //+ }
	return result;
}

std::vector<std::string> StringUtils::Split(const std::string &input,
		const char delimiter, size_t iMaxStrings) {
	if (not isascii(delimiter)) {
		CLog::Log(LOGWARNING, "StringUtils::Split delimiter contains non-ASCII: {}\n", delimiter);
	}
	std::vector < std::string > result;
	std::string sDelimiter = std::string(1, delimiter);
	result = StringUtils::Split(input, sDelimiter, iMaxStrings);

	return result;
}

std::vector<std::string> StringUtils::Split(const std::string &input,
		const std::vector<std::string> &delimiters) {
	for (size_t i = 0; i < delimiters.size(); i++) {
		if (containsNonAscii(delimiters[i])) {
			CLog::Log(LOGWARNING, "StringUtils::Split delimiter contains non-ASCII: {}\n",
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
	 CLog::Log(LOGINFO, "StringUtils::Split input: {}\n", orig);
	 for (int i = 0; i < delimiters.size(); i++) {
		 CLog::Log(LOGINFO, "StringUtils::Split delimiter: {}\n", delimiters[i]);
	 }
	 for (int i = 0; i < result.size(); i++) {
		 CLog::Log(LOGINFO, "StringUtils::Split result: {}\n", result[i]);
	 }
	 +*/

	return result;
}

// TODO: Need test for this
std::vector<std::string> StringUtils::SplitMulti(
		const std::vector<std::string> &input,
		const std::vector<std::string> &delimiters, size_t iMaxStrings /* = 0 */) {
	for (size_t i = 0; i < delimiters.size(); i++) {
		if (containsNonAscii(delimiters[i])) {
			CLog::Log(LOGWARNING, "StringUtils::SplitMulti delimiter contains non-ASCII: {}\n",
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
	 std::vector<std::string> substrings = StringUtils::Split(results[i], delimiters[di]);
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
	 std::vector<std::string> substrings = StringUtils::Split(results[i], delimiters[di], iNew + 1);
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
int StringUtils::FindNumber(const std::string &strInput,
		const std::string &strFind) {
	int numFound = Unicode::countOccurances(strInput, strFind,
	    to_underlying(RegexpFlag::UREGEX_LITERAL));
	return numFound;
}

// Plane maps for MySQL utf8_general_ci (now known as utf8mb3_general_ci) collation
// Derived from https://github.com/MariaDB/server/blob/10.5/strings/ctype-utf8.c

// clang-format off
static const uint16_t plane00[] = {
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
  0x0060, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x039C, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00D7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0053,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00F7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0059
};

static const uint16_t plane01[] = {
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0044, 0x0044,
  0x0110, 0x0110, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0047, 0x0047, 0x0047, 0x0047,
  0x0047, 0x0047, 0x0047, 0x0047, 0x0048, 0x0048, 0x0126, 0x0126, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049,
  0x0049, 0x0049, 0x0132, 0x0132, 0x004A, 0x004A, 0x004B, 0x004B, 0x0138, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x013F,
  0x013F, 0x0141, 0x0141, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x0149, 0x014A, 0x014A, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0152, 0x0152, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053,
  0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0166, 0x0166, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0055, 0x0055, 0x0057, 0x0057, 0x0059, 0x0059, 0x0059, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0053,
  0x0180, 0x0181, 0x0182, 0x0182, 0x0184, 0x0184, 0x0186, 0x0187, 0x0187, 0x0189, 0x018A, 0x018B, 0x018B, 0x018D, 0x018E, 0x018F,
  0x0190, 0x0191, 0x0191, 0x0193, 0x0194, 0x01F6, 0x0196, 0x0197, 0x0198, 0x0198, 0x019A, 0x019B, 0x019C, 0x019D, 0x019E, 0x019F,
  0x004F, 0x004F, 0x01A2, 0x01A2, 0x01A4, 0x01A4, 0x01A6, 0x01A7, 0x01A7, 0x01A9, 0x01AA, 0x01AB, 0x01AC, 0x01AC, 0x01AE, 0x0055,
  0x0055, 0x01B1, 0x01B2, 0x01B3, 0x01B3, 0x01B5, 0x01B5, 0x01B7, 0x01B8, 0x01B8, 0x01BA, 0x01BB, 0x01BC, 0x01BC, 0x01BE, 0x01F7,
  0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C4, 0x01C4, 0x01C4, 0x01C7, 0x01C7, 0x01C7, 0x01CA, 0x01CA, 0x01CA, 0x0041, 0x0041, 0x0049,
  0x0049, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x018E, 0x0041, 0x0041,
  0x0041, 0x0041, 0x00C6, 0x00C6, 0x01E4, 0x01E4, 0x0047, 0x0047, 0x004B, 0x004B, 0x004F, 0x004F, 0x004F, 0x004F, 0x01B7, 0x01B7,
  0x004A, 0x01F1, 0x01F1, 0x01F1, 0x0047, 0x0047, 0x01F6, 0x01F7, 0x004E, 0x004E, 0x0041, 0x0041, 0x00C6, 0x00C6, 0x00D8, 0x00D8
};

static const uint16_t plane02[] = {
  0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x0052, 0x0052, 0x0052, 0x0052, 0x0055, 0x0055, 0x0055, 0x0055, 0x0053, 0x0053, 0x0054, 0x0054, 0x021C, 0x021C, 0x0048, 0x0048,
  0x0220, 0x0221, 0x0222, 0x0222, 0x0224, 0x0224, 0x0041, 0x0041, 0x0045, 0x0045, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0059, 0x0059, 0x0234, 0x0235, 0x0236, 0x0237, 0x0238, 0x0239, 0x023A, 0x023B, 0x023C, 0x023D, 0x023E, 0x023F,
  0x0240, 0x0241, 0x0242, 0x0243, 0x0244, 0x0245, 0x0246, 0x0247, 0x0248, 0x0249, 0x024A, 0x024B, 0x024C, 0x024D, 0x024E, 0x024F,
  0x0250, 0x0251, 0x0252, 0x0181, 0x0186, 0x0255, 0x0189, 0x018A, 0x0258, 0x018F, 0x025A, 0x0190, 0x025C, 0x025D, 0x025E, 0x025F,
  0x0193, 0x0261, 0x0262, 0x0194, 0x0264, 0x0265, 0x0266, 0x0267, 0x0197, 0x0196, 0x026A, 0x026B, 0x026C, 0x026D, 0x026E, 0x019C,
  0x0270, 0x0271, 0x019D, 0x0273, 0x0274, 0x019F, 0x0276, 0x0277, 0x0278, 0x0279, 0x027A, 0x027B, 0x027C, 0x027D, 0x027E, 0x027F,
  0x01A6, 0x0281, 0x0282, 0x01A9, 0x0284, 0x0285, 0x0286, 0x0287, 0x01AE, 0x0289, 0x01B1, 0x01B2, 0x028C, 0x028D, 0x028E, 0x028F,
  0x0290, 0x0291, 0x01B7, 0x0293, 0x0294, 0x0295, 0x0296, 0x0297, 0x0298, 0x0299, 0x029A, 0x029B, 0x029C, 0x029D, 0x029E, 0x029F,
  0x02A0, 0x02A1, 0x02A2, 0x02A3, 0x02A4, 0x02A5, 0x02A6, 0x02A7, 0x02A8, 0x02A9, 0x02AA, 0x02AB, 0x02AC, 0x02AD, 0x02AE, 0x02AF,
  0x02B0, 0x02B1, 0x02B2, 0x02B3, 0x02B4, 0x02B5, 0x02B6, 0x02B7, 0x02B8, 0x02B9, 0x02BA, 0x02BB, 0x02BC, 0x02BD, 0x02BE, 0x02BF,
  0x02C0, 0x02C1, 0x02C2, 0x02C3, 0x02C4, 0x02C5, 0x02C6, 0x02C7, 0x02C8, 0x02C9, 0x02CA, 0x02CB, 0x02CC, 0x02CD, 0x02CE, 0x02CF,
  0x02D0, 0x02D1, 0x02D2, 0x02D3, 0x02D4, 0x02D5, 0x02D6, 0x02D7, 0x02D8, 0x02D9, 0x02DA, 0x02DB, 0x02DC, 0x02DD, 0x02DE, 0x02DF,
  0x02E0, 0x02E1, 0x02E2, 0x02E3, 0x02E4, 0x02E5, 0x02E6, 0x02E7, 0x02E8, 0x02E9, 0x02EA, 0x02EB, 0x02EC, 0x02ED, 0x02EE, 0x02EF,
  0x02F0, 0x02F1, 0x02F2, 0x02F3, 0x02F4, 0x02F5, 0x02F6, 0x02F7, 0x02F8, 0x02F9, 0x02FA, 0x02FB, 0x02FC, 0x02FD, 0x02FE, 0x02FF
};

static const uint16_t plane03[] = {
  0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x0308, 0x0309, 0x030A, 0x030B, 0x030C, 0x030D, 0x030E, 0x030F,
  0x0310, 0x0311, 0x0312, 0x0313, 0x0314, 0x0315, 0x0316, 0x0317, 0x0318, 0x0319, 0x031A, 0x031B, 0x031C, 0x031D, 0x031E, 0x031F,
  0x0320, 0x0321, 0x0322, 0x0323, 0x0324, 0x0325, 0x0326, 0x0327, 0x0328, 0x0329, 0x032A, 0x032B, 0x032C, 0x032D, 0x032E, 0x032F,
  0x0330, 0x0331, 0x0332, 0x0333, 0x0334, 0x0335, 0x0336, 0x0337, 0x0338, 0x0339, 0x033A, 0x033B, 0x033C, 0x033D, 0x033E, 0x033F,
  0x0340, 0x0341, 0x0342, 0x0343, 0x0344, 0x0399, 0x0346, 0x0347, 0x0348, 0x0349, 0x034A, 0x034B, 0x034C, 0x034D, 0x034E, 0x034F,
  0x0350, 0x0351, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x0357, 0x0358, 0x0359, 0x035A, 0x035B, 0x035C, 0x035D, 0x035E, 0x035F,
  0x0360, 0x0361, 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0367, 0x0368, 0x0369, 0x036A, 0x036B, 0x036C, 0x036D, 0x036E, 0x036F,
  0x0370, 0x0371, 0x0372, 0x0373, 0x0374, 0x0375, 0x0376, 0x0377, 0x0378, 0x0379, 0x037A, 0x037B, 0x037C, 0x037D, 0x037E, 0x037F,
  0x0380, 0x0381, 0x0382, 0x0383, 0x0384, 0x0385, 0x0391, 0x0387, 0x0395, 0x0397, 0x0399, 0x038B, 0x039F, 0x038D, 0x03A5, 0x03A9,
  0x0399, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A2, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x0391, 0x0395, 0x0397, 0x0399,
  0x03A5, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A3, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x039F, 0x03A5, 0x03A9, 0x03CF,
  0x0392, 0x0398, 0x03D2, 0x03D2, 0x03D2, 0x03A6, 0x03A0, 0x03D7, 0x03D8, 0x03D9, 0x03DA, 0x03DA, 0x03DC, 0x03DC, 0x03DE, 0x03DE,
  0x03E0, 0x03E0, 0x03E2, 0x03E2, 0x03E4, 0x03E4, 0x03E6, 0x03E6, 0x03E8, 0x03E8, 0x03EA, 0x03EA, 0x03EC, 0x03EC, 0x03EE, 0x03EE,
  0x039A, 0x03A1, 0x03A3, 0x03F3, 0x03F4, 0x03F5, 0x03F6, 0x03F7, 0x03F8, 0x03F9, 0x03FA, 0x03FB, 0x03FC, 0x03FD, 0x03FE, 0x03FF
};

static const uint16_t plane04[] = {
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0460, 0x0460, 0x0462, 0x0462, 0x0464, 0x0464, 0x0466, 0x0466, 0x0468, 0x0468, 0x046A, 0x046A, 0x046C, 0x046C, 0x046E, 0x046E,
  0x0470, 0x0470, 0x0472, 0x0472, 0x0474, 0x0474, 0x0474, 0x0474, 0x0478, 0x0478, 0x047A, 0x047A, 0x047C, 0x047C, 0x047E, 0x047E,
  0x0480, 0x0480, 0x0482, 0x0483, 0x0484, 0x0485, 0x0486, 0x0487, 0x0488, 0x0489, 0x048A, 0x048B, 0x048C, 0x048C, 0x048E, 0x048E,
  0x0490, 0x0490, 0x0492, 0x0492, 0x0494, 0x0494, 0x0496, 0x0496, 0x0498, 0x0498, 0x049A, 0x049A, 0x049C, 0x049C, 0x049E, 0x049E,
  0x04A0, 0x04A0, 0x04A2, 0x04A2, 0x04A4, 0x04A4, 0x04A6, 0x04A6, 0x04A8, 0x04A8, 0x04AA, 0x04AA, 0x04AC, 0x04AC, 0x04AE, 0x04AE,
  0x04B0, 0x04B0, 0x04B2, 0x04B2, 0x04B4, 0x04B4, 0x04B6, 0x04B6, 0x04B8, 0x04B8, 0x04BA, 0x04BA, 0x04BC, 0x04BC, 0x04BE, 0x04BE,
  0x04C0, 0x0416, 0x0416, 0x04C3, 0x04C3, 0x04C5, 0x04C6, 0x04C7, 0x04C7, 0x04C9, 0x04CA, 0x04CB, 0x04CB, 0x04CD, 0x04CE, 0x04CF,
  0x0410, 0x0410, 0x0410, 0x0410, 0x04D4, 0x04D4, 0x0415, 0x0415, 0x04D8, 0x04D8, 0x04D8, 0x04D8, 0x0416, 0x0416, 0x0417, 0x0417,
  0x04E0, 0x04E0, 0x0418, 0x0418, 0x0418, 0x0418, 0x041E, 0x041E, 0x04E8, 0x04E8, 0x04E8, 0x04E8, 0x042D, 0x042D, 0x0423, 0x0423,
  0x0423, 0x0423, 0x0423, 0x0423, 0x0427, 0x0427, 0x04F6, 0x04F7, 0x042B, 0x042B, 0x04FA, 0x04FB, 0x04FC, 0x04FD, 0x04FE, 0x04FF
};

static const uint16_t plane05[] = {
  0x0500, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506, 0x0507, 0x0508, 0x0509, 0x050A, 0x050B, 0x050C, 0x050D, 0x050E, 0x050F,
  0x0510, 0x0511, 0x0512, 0x0513, 0x0514, 0x0515, 0x0516, 0x0517, 0x0518, 0x0519, 0x051A, 0x051B, 0x051C, 0x051D, 0x051E, 0x051F,
  0x0520, 0x0521, 0x0522, 0x0523, 0x0524, 0x0525, 0x0526, 0x0527, 0x0528, 0x0529, 0x052A, 0x052B, 0x052C, 0x052D, 0x052E, 0x052F,
  0x0530, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0557, 0x0558, 0x0559, 0x055A, 0x055B, 0x055C, 0x055D, 0x055E, 0x055F,
  0x0560, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0587, 0x0588, 0x0589, 0x058A, 0x058B, 0x058C, 0x058D, 0x058E, 0x058F,
  0x0590, 0x0591, 0x0592, 0x0593, 0x0594, 0x0595, 0x0596, 0x0597, 0x0598, 0x0599, 0x059A, 0x059B, 0x059C, 0x059D, 0x059E, 0x059F,
  0x05A0, 0x05A1, 0x05A2, 0x05A3, 0x05A4, 0x05A5, 0x05A6, 0x05A7, 0x05A8, 0x05A9, 0x05AA, 0x05AB, 0x05AC, 0x05AD, 0x05AE, 0x05AF,
  0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7, 0x05B8, 0x05B9, 0x05BA, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
  0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05C4, 0x05C5, 0x05C6, 0x05C7, 0x05C8, 0x05C9, 0x05CA, 0x05CB, 0x05CC, 0x05CD, 0x05CE, 0x05CF,
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x05EB, 0x05EC, 0x05ED, 0x05EE, 0x05EF,
  0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4, 0x05F5, 0x05F6, 0x05F7, 0x05F8, 0x05F9, 0x05FA, 0x05FB, 0x05FC, 0x05FD, 0x05FE, 0x05FF
};

static const uint16_t plane1E[] = {
  0x0041, 0x0041, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0043, 0x0043, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044,
  0x0044, 0x0044, 0x0044, 0x0044, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0046, 0x0046,
  0x0047, 0x0047, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0049, 0x0049, 0x0049, 0x0049,
  0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004D, 0x004D,
  0x004D, 0x004D, 0x004D, 0x004D, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0050, 0x0050, 0x0050, 0x0050, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052,
  0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054,
  0x0054, 0x0054, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0056, 0x0056, 0x0056, 0x0056,
  0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0058, 0x0058, 0x0058, 0x0058, 0x0059, 0x0059,
  0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0048, 0x0054, 0x0057, 0x0059, 0x1E9A, 0x0053, 0x1E9C, 0x1E9D, 0x1E9E, 0x1E9F,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045,
  0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x1EFA, 0x1EFB, 0x1EFC, 0x1EFD, 0x1EFE, 0x1EFF
};

static const uint16_t plane1F[] = {
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F16, 0x1F17, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F1E, 0x1F1F,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399,
  0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F46, 0x1F47, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F4E, 0x1F4F,
  0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1F58, 0x03A5, 0x1F5A, 0x03A5, 0x1F5C, 0x03A5, 0x1F5E, 0x03A5,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x1FBB, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0399, 0x1FDB, 0x039F, 0x1FF9, 0x03A5, 0x1FEB, 0x03A9, 0x1FFB, 0x1F7E, 0x1F7F,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FB5, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FBB, 0x0391, 0x1FBD, 0x0399, 0x1FBF,
  0x1FC0, 0x1FC1, 0x0397, 0x0397, 0x0397, 0x1FC5, 0x0397, 0x0397, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0397, 0x1FCD, 0x1FCE, 0x1FCF,
  0x0399, 0x0399, 0x0399, 0x1FD3, 0x1FD4, 0x1FD5, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x1FDB, 0x1FDC, 0x1FDD, 0x1FDE, 0x1FDF,
  0x03A5, 0x03A5, 0x03A5, 0x1FE3, 0x03A1, 0x03A1, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1FEB, 0x03A1, 0x1FED, 0x1FEE, 0x1FEF,
  0x1FF0, 0x1FF1, 0x03A9, 0x03A9, 0x03A9, 0x1FF5, 0x03A9, 0x03A9, 0x039F, 0x1FF9, 0x03A9, 0x1FFB, 0x03A9, 0x1FFD, 0x1FFE, 0x1FFF
};

static const uint16_t plane21[] = {
  0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2105, 0x2106, 0x2107, 0x2108, 0x2109, 0x210A, 0x210B, 0x210C, 0x210D, 0x210E, 0x210F,
  0x2110, 0x2111, 0x2112, 0x2113, 0x2114, 0x2115, 0x2116, 0x2117, 0x2118, 0x2119, 0x211A, 0x211B, 0x211C, 0x211D, 0x211E, 0x211F,
  0x2120, 0x2121, 0x2122, 0x2123, 0x2124, 0x2125, 0x2126, 0x2127, 0x2128, 0x2129, 0x212A, 0x212B, 0x212C, 0x212D, 0x212E, 0x212F,
  0x2130, 0x2131, 0x2132, 0x2133, 0x2134, 0x2135, 0x2136, 0x2137, 0x2138, 0x2139, 0x213A, 0x213B, 0x213C, 0x213D, 0x213E, 0x213F,
  0x2140, 0x2141, 0x2142, 0x2143, 0x2144, 0x2145, 0x2146, 0x2147, 0x2148, 0x2149, 0x214A, 0x214B, 0x214C, 0x214D, 0x214E, 0x214F,
  0x2150, 0x2151, 0x2152, 0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215A, 0x215B, 0x215C, 0x215D, 0x215E, 0x215F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2180, 0x2181, 0x2182, 0x2183, 0x2184, 0x2185, 0x2186, 0x2187, 0x2188, 0x2189, 0x218A, 0x218B, 0x218C, 0x218D, 0x218E, 0x218F,
  0x2190, 0x2191, 0x2192, 0x2193, 0x2194, 0x2195, 0x2196, 0x2197, 0x2198, 0x2199, 0x219A, 0x219B, 0x219C, 0x219D, 0x219E, 0x219F,
  0x21A0, 0x21A1, 0x21A2, 0x21A3, 0x21A4, 0x21A5, 0x21A6, 0x21A7, 0x21A8, 0x21A9, 0x21AA, 0x21AB, 0x21AC, 0x21AD, 0x21AE, 0x21AF,
  0x21B0, 0x21B1, 0x21B2, 0x21B3, 0x21B4, 0x21B5, 0x21B6, 0x21B7, 0x21B8, 0x21B9, 0x21BA, 0x21BB, 0x21BC, 0x21BD, 0x21BE, 0x21BF,
  0x21C0, 0x21C1, 0x21C2, 0x21C3, 0x21C4, 0x21C5, 0x21C6, 0x21C7, 0x21C8, 0x21C9, 0x21CA, 0x21CB, 0x21CC, 0x21CD, 0x21CE, 0x21CF,
  0x21D0, 0x21D1, 0x21D2, 0x21D3, 0x21D4, 0x21D5, 0x21D6, 0x21D7, 0x21D8, 0x21D9, 0x21DA, 0x21DB, 0x21DC, 0x21DD, 0x21DE, 0x21DF,
  0x21E0, 0x21E1, 0x21E2, 0x21E3, 0x21E4, 0x21E5, 0x21E6, 0x21E7, 0x21E8, 0x21E9, 0x21EA, 0x21EB, 0x21EC, 0x21ED, 0x21EE, 0x21EF,
  0x21F0, 0x21F1, 0x21F2, 0x21F3, 0x21F4, 0x21F5, 0x21F6, 0x21F7, 0x21F8, 0x21F9, 0x21FA, 0x21FB, 0x21FC, 0x21FD, 0x21FE, 0x21FF
};

static const uint16_t plane24[] = {
  0x2400, 0x2401, 0x2402, 0x2403, 0x2404, 0x2405, 0x2406, 0x2407, 0x2408, 0x2409, 0x240A, 0x240B, 0x240C, 0x240D, 0x240E, 0x240F,
  0x2410, 0x2411, 0x2412, 0x2413, 0x2414, 0x2415, 0x2416, 0x2417, 0x2418, 0x2419, 0x241A, 0x241B, 0x241C, 0x241D, 0x241E, 0x241F,
  0x2420, 0x2421, 0x2422, 0x2423, 0x2424, 0x2425, 0x2426, 0x2427, 0x2428, 0x2429, 0x242A, 0x242B, 0x242C, 0x242D, 0x242E, 0x242F,
  0x2430, 0x2431, 0x2432, 0x2433, 0x2434, 0x2435, 0x2436, 0x2437, 0x2438, 0x2439, 0x243A, 0x243B, 0x243C, 0x243D, 0x243E, 0x243F,
  0x2440, 0x2441, 0x2442, 0x2443, 0x2444, 0x2445, 0x2446, 0x2447, 0x2448, 0x2449, 0x244A, 0x244B, 0x244C, 0x244D, 0x244E, 0x244F,
  0x2450, 0x2451, 0x2452, 0x2453, 0x2454, 0x2455, 0x2456, 0x2457, 0x2458, 0x2459, 0x245A, 0x245B, 0x245C, 0x245D, 0x245E, 0x245F,
  0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466, 0x2467, 0x2468, 0x2469, 0x246A, 0x246B, 0x246C, 0x246D, 0x246E, 0x246F,
  0x2470, 0x2471, 0x2472, 0x2473, 0x2474, 0x2475, 0x2476, 0x2477, 0x2478, 0x2479, 0x247A, 0x247B, 0x247C, 0x247D, 0x247E, 0x247F,
  0x2480, 0x2481, 0x2482, 0x2483, 0x2484, 0x2485, 0x2486, 0x2487, 0x2488, 0x2489, 0x248A, 0x248B, 0x248C, 0x248D, 0x248E, 0x248F,
  0x2490, 0x2491, 0x2492, 0x2493, 0x2494, 0x2495, 0x2496, 0x2497, 0x2498, 0x2499, 0x249A, 0x249B, 0x249C, 0x249D, 0x249E, 0x249F,
  0x24A0, 0x24A1, 0x24A2, 0x24A3, 0x24A4, 0x24A5, 0x24A6, 0x24A7, 0x24A8, 0x24A9, 0x24AA, 0x24AB, 0x24AC, 0x24AD, 0x24AE, 0x24AF,
  0x24B0, 0x24B1, 0x24B2, 0x24B3, 0x24B4, 0x24B5, 0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF,
  0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5, 0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF,
  0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF, 0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5,
  0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF, 0x24EA, 0x24EB, 0x24EC, 0x24ED, 0x24EE, 0x24EF,
  0x24F0, 0x24F1, 0x24F2, 0x24F3, 0x24F4, 0x24F5, 0x24F6, 0x24F7, 0x24F8, 0x24F9, 0x24FA, 0x24FB, 0x24FC, 0x24FD, 0x24FE, 0x24FF
};

static const uint16_t planeFF[] = {
  0xFF00, 0xFF01, 0xFF02, 0xFF03, 0xFF04, 0xFF05, 0xFF06, 0xFF07, 0xFF08, 0xFF09, 0xFF0A, 0xFF0B, 0xFF0C, 0xFF0D, 0xFF0E, 0xFF0F,
  0xFF10, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17, 0xFF18, 0xFF19, 0xFF1A, 0xFF1B, 0xFF1C, 0xFF1D, 0xFF1E, 0xFF1F,
  0xFF20, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF3B, 0xFF3C, 0xFF3D, 0xFF3E, 0xFF3F,
  0xFF40, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF5B, 0xFF5C, 0xFF5D, 0xFF5E, 0xFF5F,
  0xFF60, 0xFF61, 0xFF62, 0xFF63, 0xFF64, 0xFF65, 0xFF66, 0xFF67, 0xFF68, 0xFF69, 0xFF6A, 0xFF6B, 0xFF6C, 0xFF6D, 0xFF6E, 0xFF6F,
  0xFF70, 0xFF71, 0xFF72, 0xFF73, 0xFF74, 0xFF75, 0xFF76, 0xFF77, 0xFF78, 0xFF79, 0xFF7A, 0xFF7B, 0xFF7C, 0xFF7D, 0xFF7E, 0xFF7F,
  0xFF80, 0xFF81, 0xFF82, 0xFF83, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F,
  0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E, 0xFF9F,
  0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE, 0xFFAF,
  0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
  0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
  0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF,
  0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF,
  0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF
};

static const uint16_t* const planemap[256] = {
    plane00, plane01, plane02, plane03, plane04, plane05, NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, plane1E, plane1F, NULL,
    plane21, NULL,    NULL,    plane24, NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    planeFF
};
// clang-format on

bool StringUtils::InitializeCollator(bool normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::getDefaultICULocale(), normalize);
}
bool StringUtils::InitializeCollator(const std::locale &locale, bool normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::getICULocale(locale), normalize);
}

bool StringUtils::InitializeCollator(const icu::Locale &icuLocale, bool normalize /* = false */)
{
  return Unicode::InitializeCollator(icuLocale, normalize);
}


int32_t StringUtils::Collate(const std::wstring &left, const std::wstring &right)
{
	return Unicode::Collate(left, right);
}

static wchar_t GetCollationWeight(const wchar_t &r) {
	// Lookup the "weight" of a UTF8 char, equivalent lowercase ascii letter, in the plane map,
	// the character comparison value used by using "accent folding" collation utf8_general_ci
	// in MySQL (AKA utf8mb3_general_ci in MariaDB 10)
	auto index = r >> 8;
	if (index > 255)
		return 0xFFFD;
	auto plane = planemap[index];
	if (plane == nullptr)
		return r;
	return static_cast<wchar_t>(plane[r & 0xFF]);
}

// Not sure what the real goal is here. I think it is just to see if the
// strings are the same after case folding. (Normalizing might be a good
// idea too.)
//
// Compares separately the numeric and alphabetic parts of a wide string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical.
// See also the equivalent StringUtils::AlphaNumericCollation() for UFT8 data
int64_t StringUtils::AlphaNumericCompare(const wchar_t *left, const wchar_t *right) {
#ifdef USE_ICU_COLLATOR
	int64_t result = StringUtils::Collate(left, right);
  // CLog::Log(LOGINFO, "StringUtils::AlphaNumericCompare Collate left: {} right: {} result: {}\n",
	//		 Unicode::wstring_to_utf8(left), Unicode::wstring_to_utf8(right), result);
#else
	int64_t result = StringUtils::AlphaNumericCompare_orig(left, right);
  // CLog::Log(LOGINFO, "StringUtils::AlphaNumericCompare_orig left: {} right: {} result: {}\n",
	//		 Unicode::wstring_to_utf8(left), Unicode::wstring_to_utf8(right), result);
#endif
	return result;
}

// Not sure what the real goal is here. I think it is just to see if the
// strings are the same after case folding. (Normalizing might be a good
// idea too.)
//
// Compares separately the numeric and alphabetic parts of a wide string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical.
// See also the equivalent StringUtils::AlphaNumericCollation() for UFT8 data
int64_t StringUtils::AlphaNumericCompare_orig(const wchar_t *left,
		const wchar_t *right) {
	const wchar_t *l = left;
	const wchar_t *r = right;
	const wchar_t *ld, *rd;
	wchar_t lc, rc;
	int64_t lnum, rnum;
	bool lsym, rsym;

	// Normal sort, compare characters until there is a difference....
	// EXCEPT, if you encounter numbers, hold off comparison until you have
	// the entire numeric sequence and then compare the numeric value.
	// If the numbers are the same, then continue with the character
	// comparison.

	while (*l != 0 && *r != 0) {
		// If the current character from both strings are numbers, then
		// we want a numeric comparison
		if (*l >= L'0' && *l <= L'9' && *r >= L'0' && *r <= L'9') {
			ld = l;
			lnum = *ld++ - L'0';
			// First, create an integer numeric value for the left side from
			// up to 15 numeric characters from the left
			while (*ld >= L'0' && *ld <= L'9' && ld < l + 15) { // compare only up to 15 digits
				lnum *= 10;
				lnum += *ld++ - L'0';
			}
			rd = r;
			rnum = *rd++ - L'0';
			// Same for the right side. Lengths don't have to be the same.
			while (*rd >= L'0' && *rd <= L'9' && rd < r + 15) { // compare only up to 15 digits
				rnum *= 10;
				rnum += *rd++ - L'0';
			}
			// do we have numbers?
			if (lnum != rnum) { // yes - and they're different!
				return lnum - rnum;
			}
			// Advance pointers to end of numeric values
			l = ld;
			r = rd;
			continue;
		}

    lc = *l;
    rc = *r;
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ above the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (lc >= 32 && lc < L'0') || (lc > L'9' && lc < L'A') || (lc > L'Z' && lc < L'a') ||
           (lc > L'z' && lc < 128);
    rsym = (rc >= 32 && rc < L'0') || (rc > L'9' && rc < L'A') || (rc > L'Z' && rc < L'a') ||
           (rc > L'z' && rc < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (lc != rc)
        return lc - rc;
      else
      { // Same symbol advance to next wchar
        l++;
        r++;
        continue;
      }
    }
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
    // Do case less comparison, convert ascii upper case to lower case
    if (lc >= L'A' && lc <= L'Z')
      lc += L'a' - L'A';
    if (rc >= L'A' && rc <= L'Z')
      rc += L'a' - L'A';

		if (lc != rc) {
			if (!g_langInfo.UseLocaleCollation()) {
				// Compare unicode (having applied accent folding collation to non-ascii chars).
				int i = wcsncmp(&lc, &rc, 1);
				return i;
			} else {
				// Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not langauge specific but just compares unicode
				const std::collate<wchar_t> &coll =
						std::use_facet<std::collate<wchar_t>>(g_langInfo.GetSystemLocale());
				int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
				if (cmp_res != 0)
					return cmp_res;
			}
		}
		l++;
		r++;
	}
	if (*r) { // r is longer
		return -1;
	} else if (*l) { // l is longer
		return 1;
	}
	return 0; // files are the same
}
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

int StringUtils::AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2)
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

int StringUtils::DateStringToYYYYMMDD(const std::string &dateString) {
	// TODO: I assume this is a fixed format for a database or something?
	std::vector < std::string > days = StringUtils::Split(dateString, '-');
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

std::string StringUtils::ISODateToLocalizedDate(const std::string &strIsoDate) {
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

long StringUtils::TimeStringToSeconds(const std::string &timeString) {
	std::string strCopy(timeString);
	StringUtils::Trim(strCopy);
	if (StringUtils::EndsWithNoCase(strCopy, " min")) {
		// this is imdb format of "XXX min"
		return 60 * atoi(strCopy.c_str());
	} else {
		std::vector < std::string > secs = StringUtils::Split(strCopy, ':');
		int timeInSecs = 0;

		// TODO: This looks odd. Verify

		for (unsigned int i = 0; i < 3 && i < secs.size(); i++) {
			timeInSecs *= 60;
			timeInSecs += atoi(secs[i].c_str());
		}
		return timeInSecs;
	}
}

std::string StringUtils::SecondsToTimeString(long lSeconds, TIME_FORMAT format)
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

bool StringUtils::IsNaturalNumber(const std::string& str)
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
bool StringUtils::IsInteger(const std::string& str)
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

int StringUtils::asciidigitvalue(char chr) {
	if (!isasciidigit(chr))
		return -1;

	return chr - '0';
}

int StringUtils::asciixdigitvalue(char chr) {
	int v = asciidigitvalue(chr);
	if (v >= 0)
		return v;
	if (chr >= 'a' && chr <= 'f')
		return chr - 'a' + 10;
	if (chr >= 'A' && chr <= 'F')
		return chr - 'A' + 10;

	return -1;
}

void StringUtils::RemoveCRLF(std::string &strLine) {
	StringUtils::TrimRight(strLine, "\n\r");
}

std::string StringUtils::SizeToString(int64_t size)
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

std::string StringUtils::BinaryStringToString(const std::string& in)
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

std::string StringUtils::ToHexadecimal(const std::string& in)
{
  std::ostringstream ss;
  ss << std::hex;
  for (unsigned char ch : in) {
    ss << std::setw(2) << std::setfill('0') << static_cast<unsigned long> (ch);
  }
  return ss.str();
}

size_t StringUtils::FindWord(const std::string &str, const std::string &word)
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

/*
 * Guaranteed to work ONLY with ASCII 'brackets'. String may be utf-8.
 */
int StringUtils::FindEndBracket(const std::string &str, char opener, char closer, int startPos)
{
	if (not isascii(opener)) {
	 	CLog::Log(LOGWARNING, "StringUtils::FindEndBracket opener is non-ASCII: {}\n", opener);
	}
	else if (not isascii(closer)) {
	 	CLog::Log(LOGWARNING, "StringUtils::FindEndBracket closer is non-ASCII: {}\n", closer);
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
size_t StringUtils::FindEndBracket(const std::string &str, char opener,
		char closer, size_t startPos) {
	std::cout << "FindEndBracket str: " << str << std::endl;

	size_t result = Unicode::FindEndBracket(str, opener, closer, startPos);
	//CLog::Log(LOGINFO, "StringUtils::FindEndBracket\n");

	 //+CLog::Log(LOGINFO, "StringUtils::FindEndBracket str: {} opener: {} closer: {} startPos: %d result: %d\n",
		//+	 str, opener, closer, startPos, result);

	return result;
}
*/

#include "utils/UnicodeUtils.h"
void StringUtils::WordToDigits(std::string &word)
{

	// Works ONLY with ASCII!

  static const char word_to_letter[] = "22233344455566677778889999";
  word = UnicodeUtils::ToLower(word);
  for (unsigned int i = 0; i < word.size(); ++i)
  { // NB: This assumes ascii, which probably needs extending at some  point.
    char letter = word[i];
		if (letter > 0x7f) {
	    CLog::Log(LOGWARNING, "StringUtils.WordToDigits: Non-ASCII input: {}\n",
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

std::string StringUtils::CreateUUID()
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

bool StringUtils::ValidateUUID(const std::string &uuid) {
	CRegExp guidRE;
	guidRE.RegComp(ADDON_GUID_RE);
	return (guidRE.RegFind(uuid.c_str()) == 0);
}

double StringUtils::CompareFuzzy(const std::string &left,
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

int StringUtils::FindBestMatch(const std::string &str,
		const std::vector<std::string> &strings, double &matchscore) {

	// TODO: NOT Unicode safe

	int best = -1;
	matchscore = 0;

	int i = 0;
	for (std::vector<std::string>::const_iterator it = strings.begin();
			it != strings.end(); ++it, i++) {
		int maxlength = std::max(str.length(), it->length());
		double score = StringUtils::CompareFuzzy(str, *it) / maxlength;
		if (score > matchscore) {
			matchscore = score;
			best = i;
		}
	}
	std::cout << "FindBestMatch score: " << matchscore << " best: " << best << std::endl;
	return best;
}

// Looks for EXACT match (no case folding)

bool StringUtils::ContainsKeyword(const std::string &str,
		const std::vector<std::string> &keywords) {

	// TODO: Needs test

	return Unicode::Contains(str, keywords);
}

size_t StringUtils::utf8_strlen(const char *s) {
	size_t length = 0;
	while (*s) {
		if (U8_IS_LEAD(*s++))
			length ++;
	}

	return length;
}

std::string StringUtils::Paramify(const std::string &param) {
	std::string result = param;
   //std::cout << "StringUtils.Paramify param: " << param << std::endl;
	// escape backspaces
	StringUtils::Replace(result, "\\", "\\\\");
	// escape double quotes
	StringUtils::Replace(result, "\"", "\\\"");

	// add double quotes around the whole string
	return "\"" + result + "\"";
}


/**
 * Note: delimiters is a string of one or more ASCII single-character delimiters.
 *       input may be utf-8.
 */
std::vector<std::string> StringUtils::Tokenize(const std::string &input,
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

void StringUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters)
{
	if (containsNonAscii(delimiters)) {
	 	CLog::Log(LOGWARNING, "StringUtils::Tokenize contains non-ASCII delimiter: {}\n", delimiters);
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
void StringUtils::Tokenize(const std::string &input,
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
std::vector<std::string> StringUtils::Tokenize(const std::string &input,
		const char delimiter) {
	std::vector < std::string > tokens;
	Tokenize(input, tokens, delimiter);
	return tokens;
}

/**
 * Note: delimiter is a single ASCII character that separates tokens.
 *       input may be utf-8.
 */

void StringUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const char delimiter)
{
  tokens.clear();
	if (not isascii(delimiter)) {
	 	CLog::Log(LOGWARNING, "StringUtils::Tokenize contains non-ASCII delimiter: {}s\n", delimiter);
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
void StringUtils::Tokenize(const std::string &input,
		std::vector<std::string> &tokens, const char delimiter) {

	std::string sDelimiter = std::string();
	sDelimiter.append(1, delimiter);
	Unicode::Tokenize(input, tokens, sDelimiter);
  return;
}
*/

uint64_t StringUtils::ToUint64(const std::string &str,
		uint64_t fallback) noexcept {
	std::istringstream iss(str);
	uint64_t result(fallback);
	iss >> result;
	return result;
}

std::string StringUtils::FormatFileSize(uint64_t bytes)
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


std::string StringUtils::CreateFromCString(const char* cstr)
{
  return cstr != nullptr ? std::string(cstr) : std::string();
}
