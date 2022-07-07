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
//  to resolve issues discovered in Kodi 19 Matrix
//
//  Modified to use J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------
#include <bits/std_abs.h>
#include <bits/stdint-intn.h>
#include <stddef.h>
#include <unicode/uversion.h>
#include <cctype>
#include <iterator>
#include <locale>
#include <vector>

#include "../commons/ilog.h"
#include "StringUtils.h"

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

#include <unicode/locid.h>
// #include "unicode/uregex.h"
#include <unicode/utf8.h>
#include "utils/log.h"

// DO NOT MOVE or std functions end up in PCRE namespace
// clang-format off
#include "utils/RegExp.h"
// clang-format on
#include "utils/Unicode.h"

void UnicodeUtils::ToUpper(std::string &str, const icu::Locale &locale) {
	if (str.length() == 0)
		return;

	std::string upper = Unicode::ToUpper(str, locale);
	str.swap(upper);
}

void UnicodeUtils::ToUpper(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  return UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToUpper(std::string &str) {
	if (str.length() == 0)
		return;

	icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToUpper(std::wstring &str, const std::locale &locale) {
	if (str.length() == 0)
		return;

  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  UnicodeUtils::ToUpper(str, icuLocale);
}


void UnicodeUtils::ToUpper(std::wstring &str, const icu::Locale &locale) {
	if (str.length() == 0)
		return;

	std::string str_utf8 = Unicode::WStringToUTF8(str);
	const std::string upper = Unicode::ToUpper(str_utf8, locale);
   //std::cout << "ToUpper 1 (wide) in: " << str_utf8 << " out: " << upper << std::endl;
	std::wstring wUpper = Unicode::UTF8ToWString(upper);
	str.swap(wUpper);
}
void UnicodeUtils::ToUpper(std::wstring &str) {
	if (str.length() == 0)
		return;

  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
	UnicodeUtils::ToUpper(str, icuLocale);
}

/**
 * Only these two methods with locale arg are true to-lower.
 * TODO: fix this
 */
void UnicodeUtils::ToLower(std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
	return UnicodeUtils::ToLower(str, icuLocale);
}

/**
 * Only these two methods with locale arg are true to-lower.
 * TODO: fix this
 */
void UnicodeUtils::ToLower(std::string &str, const icu::Locale &locale) {
	if (str.length() == 0)
		return;

	std::string lower = Unicode::ToLower(str, locale);
   //std::cout << "ToLower 1 in: " << str << " out: " << lower << std::endl;
	str.swap(lower);
}

void UnicodeUtils::ToLower(std::wstring &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::ToLower(std::wstring &str, const icu::Locale &locale) {
	if (str.length() == 0)
		return;

	std::string str_utf8 = Unicode::WStringToUTF8(str);
	const std::string lower_utf8 = Unicode::ToLower(str_utf8, locale);
   //std::cout << "ToLower 1 (wide) in: " << str_utf8 << " out: " << lower_utf8 << std::endl;
	std::wstring wLower = Unicode::UTF8ToWString(lower_utf8);
	str.swap(wLower);
}

void UnicodeUtils::ToLower(std::string &str) {
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
	return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::ToLower(std::wstring &str) {
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::FoldCase(std::string &str, const StringOptions opt /*  = StringOptions::FOLD_CASE_DEFAULT */) {
	if (str.length() == 0)
		return;

	std::string result = Unicode::ToFold(str, opt);
}

void UnicodeUtils::FoldCase(std::wstring &str, const StringOptions opt /*  = StringOptions::FOLD_CASE_DEFAULT */) {
	if (str.length() == 0)
		return;

	Unicode::ToFold(str, opt);
	return;
}

void UnicodeUtils::ToCapitalize(std::wstring &str) {
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::ToTitle(str, icuLocale);
#else
	std::wstring result = Unicode::ToCapitalize(str, icuLocale);
	str.swap(result);
#endif
	return;
}

void UnicodeUtils::ToCapitalize(std::string &str) {
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
#ifdef USE_TO_TITLE_FOR_CAPITALIZE
  Unicode::ToTitle(str, icuLocale);
#else
  std::string result = Unicode::ToCapitalize(str, icuLocale);
  str.swap(result);
#endif
	return;
}

std::wstring UnicodeUtils::TitleCase(const std::wstring &str, const std::locale &locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  std::wstring result = Unicode::ToTitle(str, icuLocale);
  return result;
}

std::wstring UnicodeUtils::TitleCase(const std::wstring &str) {
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  std::wstring result = Unicode::ToTitle(str, icuLocale);
  return result;
}

std::string UnicodeUtils::TitleCase(const std::string &str, const std::locale &locale) {
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  std::string result = Unicode::ToTitle(str, icuLocale);
  return result;
}

std::string UnicodeUtils::TitleCase(const std::string &str) {
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  std::string result = Unicode::ToTitle(str, icuLocale);
  return result;
}

const std::wstring UnicodeUtils::Normalize(const std::wstring &src,
		const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const NormalizerType NormalizerType) {
	return Unicode::Normalize(src, opt, NormalizerType);
}

const std::string UnicodeUtils::Normalize(const std::string &src,
		const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const NormalizerType NormalizerType) {
	return Unicode::Normalize(src, opt, NormalizerType);
}

bool UnicodeUtils::Equals(const std::string &str1, const std::string &str2) {
	int8_t rc = Unicode::StrCmp(str1, 0, str1.length(), str2, 0, str2.length());
	return rc == (int8_t) 0;
}

bool UnicodeUtils::Equals(const std::wstring &str1, const std::wstring &str2) {
	int8_t rc = Unicode::StrCmp(str1, 0, str1.length(), str2, 0, str2.length());
	return rc == 0;
}

bool UnicodeUtils::EqualsNoCase(const std::string &str1, const std::string &str2,
		StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool Normalize /* = false */) {
	// before we do the char-by-char comparison, first compare sizes of both strings.
	// This led to a 33% improvement in benchmarking on average. (size() just returns a member of std::string)
	if (str1.size() == 0 and str2.size() == 0)
		return true;

	if (str1.size() == 0 or str2.size() == 0)
		return false;

	int8_t rc = Unicode::StrCaseCmp(str1, str2, opt, Normalize);

	rc = rc == 0;
   //std::cout << "EqualsNoCase 1 str1: " << str1 << " str2: " << str2 << " rc: "
	//		<< (int) rc << std::endl;
	return rc;
}

bool UnicodeUtils::EqualsNoCase(const std::string &str1, const char *s2, StringOptions opt /* StringOptions::FOLD_CASE_DEFAULT */,
		const bool Normalize /* = false */) {
	std::string str2 = std::string(s2);
	return EqualsNoCase(str1, str2, opt, Normalize);
}

bool UnicodeUtils::EqualsNoCase(const char *s1, const char *s2, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
		const bool Normalize /* = false */) {
	std::string str1 = std::string(s1);
	std::string str2 = std::string(s2);
	return EqualsNoCase(str1, str2, opt, Normalize);
}


int UnicodeUtils::Compare(const std::wstring &str1, const std::wstring &str2) {

	int32_t str1_length = str1.length();
	int32_t str2_length = str2.length();
	int32_t str1_start = 0;
	int32_t str2_start = 0;
	int rc = Unicode::StrCmp(str1, str1_start, str1_length,
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
	int rc = Unicode::StrCmp(str1, str1_start, str1_length,
			                     str2, str2_start, str2_length);
	// std::cout << "Compare 1 str1: " << str1 << " str2: " << str2 << " n: " << n
	// 		<< " rc: " << (int) rc << std::endl;

	return rc;
}

int UnicodeUtils::CompareNoCase(const std::wstring &str1, const std::wstring &str2,
		StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool Normalize /* = false */) {

	int rc = Unicode::StrCaseCmp(str1, str2, opt, Normalize);
	return rc;
}
int UnicodeUtils::CompareNoCase(const std::string &str1, const std::string &str2,
    StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool Normalize /* = false */) {
  int rc = Unicode::StrCaseCmp(str1, str2, opt, Normalize);
  return rc;
}

int UnicodeUtils::CompareNoCase(const char *s1, const char *s2, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool Normalize /* = false */) {
  std::string str1 = std::string(s1);
  std::string str2 = std::string(s2);
  return UnicodeUtils::CompareNoCase(str1, str2, opt, Normalize);
}

int UnicodeUtils::CompareNoCase(const std::string &str1, const std::string &str2,
		size_t n, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */ , const bool Normalize /* = false */) {

	// TODO: value of 'n' is questionable in Unicode environment.
	if (n == 0)
	{
		n = std::string::npos;
	}
	else
	{
    if (ContainsNonAscii(str1))
    {
      CLog::Log(LOGWARNING, "UnicodeUtils::CompareNoCase str1 contains non-ASCII: {}", str1);
    }
    if (ContainsNonAscii(str2))
    {
        CLog::Log(LOGWARNING, "UnicodeUtils::CompareNoCase str2 contains non-ASCII: {}", str2);
    }
	}

	int rc = Unicode::StrCaseCmp(str1, str2, n, opt, Normalize);
	// std::cout << "CompareNoCase 1 str1: " << str1 << " str2: " << str2 << " n: " << n
	// 		<< " rc: " << (int) rc << std::endl;

	return rc;
}

int UnicodeUtils::CompareNoCase(const char *s1, const char *s2,
		size_t n /* = 0 */, StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */, const bool Normalize /* = false */) {


	std::string str1 = std::string(s1);
	std::string str2 = std::string(s2);
	return UnicodeUtils::CompareNoCase(str1, str2, n, opt, Normalize);
}

std::string UnicodeUtils::Left(const std::string &str, const size_t charCount, const bool keepLeft)
{
	std::string result = Unicode::Left(str, charCount, Unicode::GetDefaultICULocale(), keepLeft);

	return result;
}

std::string UnicodeUtils::Left(const std::string &str, const size_t charCount,
     const icu::Locale& icuLocale, const bool keepLeft /* = true */) {
  std::string result = Unicode::Left(str, charCount, icuLocale, keepLeft);

  return result;
}

std::string UnicodeUtils::Mid(const std::string &str, const size_t firstCharIndex,
		const size_t charCount /* = std::string::npos */) {
	std::string result = Unicode::Mid(str, firstCharIndex, charCount);
	return result;
}

std::string UnicodeUtils::Right(const std::string &str, const size_t charCount, bool keepRight /* = true */)
{
	std::string result = Unicode::Right(str, charCount, Unicode::GetDefaultICULocale(), keepRight);
	return result;
}

std::string UnicodeUtils::Right(const std::string &str, const size_t charCount,
    const icu::Locale &icuLocale, bool keepRight /* = true */)
{
  std::string result = Unicode::Right(str, charCount, icuLocale, keepRight);
  return result;
}

size_t UnicodeUtils::GetByteIndexForCharacter(const std::string &str, size_t charCount,
   const bool left, const bool keepLeft, const icu::Locale& icuLocale)
{
  size_t byteIndex;
  byteIndex =  Unicode::GetCharPosition(str, charCount, left, keepLeft, icuLocale);
  return byteIndex;
}

size_t UnicodeUtils::GetByteIndexForCharacter(const std::string &str, size_t charCount,
    const bool left, const bool keepLeft, const std::locale& locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  size_t byteIndex;
  byteIndex =  Unicode::GetCharPosition(str, charCount, left, keepLeft, icuLocale);
  return byteIndex;
}

size_t UnicodeUtils::GetByteIndexForCharacter(const std::string &str, size_t charCount,
    const bool left, const bool keepLeft)
{
  size_t byteIndex;
  byteIndex =  Unicode::GetCharPosition(str, charCount, left, keepLeft, Unicode::GetDefaultICULocale());
  return byteIndex;
}

std::string& UnicodeUtils::Trim(std::string &str) {
	std::string result = Unicode::Trim(str);
	str.swap(result);
	return str;
}

std::string& UnicodeUtils::Trim(std::string &str, const char *const chars) {
	std::string delChars = std::string(chars);
	std::string result = Unicode::Trim(str, delChars, true, true);
  str.swap(result);
	return str;
}

std::string& UnicodeUtils::TrimLeft(std::string &str) {
	std::string orig = std::string(str);
	std::string result = Unicode::TrimLeft(str);
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

/*
 * Replaces every occurrence of oldText with newText within the string.
 *
 * Differs from Replace in that it does not return the number of changes
 * made, which makes this version a little more efficient.
 *
 * UnicodeUtils::FindAndReplace(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)'
 *
 */

std::string UnicodeUtils::FindAndReplace(const std::string &str, const std::string oldText,
		const std::string newText) {
	 std::string orig = str;
	 return Unicode::FindAndReplace(str, oldText, newText);
}

std::string UnicodeUtils::FindAndReplace(const std::string &str, const char * oldText,
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
std::string UnicodeUtils::RegexReplaceAll(const std::string &str, const std::string pattern,
		const std::string newStr, const int flags)  {
	 std::string result = Unicode::RegexReplaceAll(str, pattern, newStr, flags);
	 return result;
}


std::string& UnicodeUtils::RemoveDuplicatedSpacesAndTabs(std::string& str)
{
	// Since removing only ASCII spaces and tabs there is no need to convert to/from
	// Unicode.

  // TODO: Write generic remove whitespace function with constants defining different
  //       types of whitespace. Perhaps utilize regex
  //
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

	std::string str_utf8 = Unicode::WStringToUTF8(str);
	std::string str_utf8_save = std::string(str_utf8);
	std::string oldStr_utf8 = Unicode::WStringToUTF8(oldStr);
	std::string newStr_utf8 = Unicode::WStringToUTF8(newStr);
   //std::cout << "Replace (wide) str: " << str_utf8_save << "oldStr: "
		//	<< oldStr_utf8 << " newStr: " << newStr_utf8 << std::endl;

	int changes = UnicodeUtils::Replace(str_utf8, oldStr_utf8, newStr_utf8);

	std::wstring result_w = Unicode::UTF8ToWString(str_utf8);

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
		const std::string &delimiter, const size_t iMaxStrings) {
	if (ContainsNonAscii(delimiter)) {
		CLog::Log(LOGWARNING, "UnicodeUtils::Split delimiter contains non-ASCII: {}", delimiter);
	}
	std::vector < std::string > result = std::vector<std::string>();
	if (not (input.empty() or delimiter.empty())) {
		Unicode::SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
	}
	else {
		if (not input.empty())
			result.push_back(input);  // TODO: Verify that we return empty vector.
	}
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
		if (ContainsNonAscii(delimiters[i])) {
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
	Unicode::SplitTo(std::back_inserter(result), input, delimiters, 0);
	return result;
}

std::vector<std::string> UnicodeUtils::SplitMulti(
		const std::vector<std::string> &input,
		const std::vector<std::string> &delimiters, size_t iMaxStrings /* = 0 */)
{
	return Unicode::SplitMulti(input, delimiters, iMaxStrings);
}

// returns the number of occurrences of strFind in strInput.
int UnicodeUtils::FindNumber(const std::string &strInput,
		const std::string &strFind) {
	int numFound = Unicode::countOccurances(strInput, strFind,
	    to_underlying(RegexpFlag::UREGEX_LITERAL));
	return numFound;
}


bool UnicodeUtils::InitializeCollator(bool Normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::GetDefaultICULocale(), Normalize);
}
bool UnicodeUtils::InitializeCollator(const std::locale &locale, bool Normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::GetICULocale(locale), Normalize);
}

bool UnicodeUtils::InitializeCollator(const icu::Locale &icuLocale, bool Normalize /* = false */)
{
  return Unicode::InitializeCollator(icuLocale, Normalize);
}

void UnicodeUtils::SortCompleted(int sortItems)
{
  Unicode::SortCompleted(sortItems);
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
#else
	int64_t result = StringUtils::AlphaNumericCompare(left, right);
#endif
	return result;
}

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

void UnicodeUtils::RemoveCRLF(std::string &strLine) {
	UnicodeUtils::TrimRight(strLine, "\n\r");
}

/*
size_t UnicodeUtils::FindWords(const char *str, const char *wordLowerCase)
{
  // NOTE: This assumes word is lowercase!
  const unsigned char* s = (const unsigned char*) str;
  do
  {
    // start with a compare
    const unsigned char* c = s;
    const unsigned char* w = (const unsigned char*) wordLowerCase;
    bool same = true;
    while (same && *c && *w)
    {
      unsigned char lc = *c++;
      if (lc >= 'A' && lc <= 'Z')
        lc += 'a' - 'A';

      if (lc != *w++) // different
        same = false;
    }
    if (same && *w == 0)  // only the same if word has been exhausted
      return (const char*) s - str;

    // otherwise, skip current word (composed by latin letters) or number
    int l;
    if (*s >= '0' && *s <= '9')
    {
      ++s;
      while (*s >= '0' && *s <= '9')
        ++s;
    }
    else if ((l = StringUtils::IsUTF8Letter(s)) > 0)
    {
      s += l;
      while ((l = StringUtils::IsUTF8Letter(s)) > 0)
        s += l;
    }
    else
      ++s;
    while (*s && *s == ' ')
      s++;

    // and repeat until we're done
  }
  while (*s);

  return std::string::npos;
}
*/

size_t UnicodeUtils::FindWord(const std::string &str, const std::string &word)
{
  size_t index;
  // Ensure searched for word begins on a "word" boundary.
  // \b matches boundary between word and non-word character
  // std::string pattern = "((\d+)|([a-z]+|.)\s*)(\\Q" + word + "\\E)";  // \\Q does not interpret as regex \\E
#ifdef USE_FINDWORD_REGEX
  std::string pattern = "((\\Q" + word + "\\E)|(((?:(?:\\d++(?=[^\\d]))|(?:[a-z]++(?=[^a-z]))|(?:[^\\d\\sa-z]))\\s*+(?=[^\\s]))(\\Q" + word + "\\E)))";
  int flags = to_underlying(RegexpFlag::UREGEX_CASE_INSENSITIVE);
  index = Unicode::RegexFind(str, pattern, flags);
#else
  index = Unicode::FindWord(str, word);
#endif
	return index;
}

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

std::string UnicodeUtils::Paramify(const std::string &param) {
   //std::cout << "UnicodeUtils.Paramify param: " << param << std::endl;
	// escape backspaces
	std::string result = Unicode::FindAndReplace(param,  "\\", "\\\\");

	// escape double quotes
	result = Unicode::FindAndReplace(result, "\"", "\\\"");

	// add double quotes around the whole string
	return "\"" + result + "\"";
}

