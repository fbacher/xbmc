/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//-----------------------------------------------------------------------
//
//  File:      StringUtils.h
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


// TODO: Move to make/cmake file
// USE_ICU_COLLATOR chooses between ICU and legacy Kodi collation

#define USE_ICU_COLLATOR 1
#define STRINGUTILS_STRING_DISABLE 1

/*
 *
 * USE_TO_TITLE_FOR_CAPITALIZE controls whether to use legacy Kodi ToCapitalize
 * algorithm:
 *   Upper case first letter of every "word"; Punctuation as well as spaces
 *   separates words. "n.y-c you pig" results in "N.Y-C You Pig". No characters
 *   lower cased. Only uppercases first codepoint of word, potentially producing
 *   bad Unicode when multiple codepoints are used to represent a letter. Probably
 *   not a severe error. May be difficult to correct with exposed APIs.
 * New Algorithm is to use ICU toTitle. This is locale dependent.
 *   Imperfect, but uses Locale rules to Title Case. Takes context into account.
 *   Does NOT generally use a dictionary of words to NOT uppercases (i.e. to, the...)
 *   Would titlecase "n.y.c you pig" as "N.y.c You Pig", but "n-y-c" becomes "N-Y-C"
 *   (the same as Capitalize).
 *
 *   Properly handles any change of string lengths.
 */
#undef USE_TO_TITLE_FOR_CAPITALIZE

/*
 * TODO: Fix, or remove code for USE_FINDWORD_REGEX
 */
#undef USE_FINDWORD_REGEX

#include <stdarg.h>   // others depend on this
#include <climits>
#include <string>
#include <sstream>
#include <iostream>

// workaround for broken [[deprecated]] in coverity
#if defined(__COVERITY__)
#undef FMT_DEPRECATED
#define FMT_DEPRECATED
#endif
#include <fmt/format.h>

#if FMT_VERSION >= 80000
#include <fmt/xchar.h>
#endif

#include "LangInfo.h"
#include "utils/params_check_macros.h"
#include "Unicode.h"
#include "XBDateTime.h"

/*!
 * \brief  C-processor Token string-ification
 *
 * The following macros can be used to stringify definitions to
 * C style strings.
 *
 * Example:
 *
 * #define foo 4
 * DEF_TO_STR_NAME(foo)  // outputs "foo"
 * DEF_TO_STR_VALUE(foo) // outputs "4"
 *
 */

#define DEF_TO_STR_NAME(x) #x
#define DEF_TO_STR_VALUE(x) DEF_TO_STR_NAME(x)

template<typename T, std::enable_if_t<!std::is_enum<T>::value, int> = 0>
constexpr auto&& EnumToInt(T&& arg) noexcept
{
  return arg;
}
template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
constexpr auto EnumToInt(T&& arg) noexcept
{
  return static_cast<int>(arg);
}

class StringUtils
{
public:

	/**
	 *  A Brief description of some of the Unicode issues: (Work in Progress)
	 *  DO NOT take this as definitive! It is from my current understanding.
	 *
	 *  Working with Unicode and multiple languages greatly complicates
	 *  string operations.
	 *
	 *  Several things to keep in mind: (this is NOT a comprehensive list)
	 *
	 *  In Kodi, all char * and std::string contain UTF-8. For some situations,
	 *  such as the delimiter for "split", only ASCII is used. wchar_t and
	 *  wstrings contain 32-bit unicode codepoints (UChar32). However,
	 *  wchar_t is not guaranteed to be 32-bits. There may be some platforms
	 *  where wchar_t contains UTF-16.
	 *
	 *  TO_LOWER:
	 *
	 *  std::string.toLower is frequently used to 'normalize' strings for use
	 *  as map keys. This WON'T WORK for most languages/character sets.
	 *  	ToLower is locale sensitive. A German sharp-s (looks a bit like an
	 *  	italic B) is equivalent to "ss". There is no upper-case equivalent.
	 *
	 *  	Accent marks frequently change. In particular, Turkic has 4 versions of
	 *  	the letter "I" (English has two). To further complicate matters,
	 *  	in Turkic locale, toUpper of the dotted "i" translates to an upper
	 *  	case dotted i; toLower converts an upper case dottless "I" to a lower
	 *  	case dotless "I".
	 *
	 *  For almost all purposes, you want to use FoldCase instead of ToLower.
	 *
	 *  Character UTF-8, wchar_t, Size, Length, Grapheme, Codepoints:
	 *
	 *  * A Unicode codepoint is unsigned 32 bits. This is specified in the
	 *    standard. Unicode frequently uses the term Grapheme to mean Character.
	 *    A Grapheme can be composed of one or more codepoints. The first
	 *    codepoint contains the basic character information, additional
	 *    codepoints add accents, etc.
	 *
	 *    Frequently an accented character can be represented in multiple
	 *    combinations of codepoints. One such case has five different
	 *    representations of the same character varying in length from 1
	 *    to five codepoints. Normalization (which is locale specific)
	 *    reorders and possibly combines a character's codepoints into
	 *    a canological order so that they can be compared with one another.
	 *
	 *  * UTF-8 is a compressed encoding of Unicode (32-bit).
	 *    UTF-16 is a compressed 16-bit encoding of Unicode
	 *    wchar_t does NOT specify a length, nor any knowledge of Unicode
	 *    although on many g++ systems, it is 32-bits. There are other
	 *    encodings.
	 *
	 *  * C++ support of Unicode is very sparse. Mostly it is only encode/decode
	 *    to/from UTF-8, UTF-16, wchar_t and Unicode. Otherwise there is
	 *    next to no knowledge of Unicode.
	 *
	 *  Implications:
	 *  * You can't compare two strings one byte or even codepoint at a time
	 *  * Two strings of different, non-zero lengths does not tell you if
	 *    strings are not-equal! (Not without knowing that the strings are normalized
	 *    for the same locale).
	 *  * When iterating the characters of a string, you frequently have
	 *    to know where the character boundaries are.
	 *  * You may have to copy or alter strings simply to compare them
	 *
	 *  Don't give up hope yet:
	 *  * The ICU library, while complicated and pretty heavy, can reliably
	 *    handle many localization and Unicode issues.
	 *  * Frequently strings are normalized. Normalizing isn't that expensive.
	 *  * While it is simpler/faster to keep everything in Unicode (32-bit),
	 *    it is not overly expensive to use utf-8, utf-16.
	 *  * ICU tries to be as efficient as possible, assumes the most common,
	 *    cheaper case first, resorting to more expensive solutions only when
	 *    needed.
	 *
	 *  Still, the ICU library is incomplete and aging. It is based on several
	 *  implementations donated from multiple sources and held together with
	 *  hot glue and horse hair. (well, it's not that bad). Still, it is
	 *  widely respected. I would much rather they reimplement using 32-bit
	 *  codepoint characters. It seems supporting utf-8, utf-16, Char32...
	 *  GREATLY complicates the API and implementation, resulting in numerous
	 *  inconsistencies. (Ok, I'll stop preaching. I just want to give you a
	 *  flavor for the state of things.)
   */


  /*!
   * \brief Get a formatted string similar to sprintf
   *
   * \param fmt Format of the resulting string
   * \param ... variable number of value type arguments
   * \return Formatted string
   */
  template<typename... Args>
  static std::string Format(const std::string& fmt, Args&&... args)
  {
  	// Note that there is an ICU version of Format

    // coverity[fun_call_w_exception : FALSE]

    return ::fmt::format(fmt, EnumToInt(std::forward<Args>(args))...);
     // Can NOT call CLog::Log from here due to recursion

  }

  /*!
   *  \brief Get a formatted wstring similar to sprintf
   *
   * \param fmt Format of the resulting string
   * \param ... variable number of value type arguments
   * \return Formatted string
   */
  template<typename... Args>
  static std::wstring Format(const std::wstring& fmt, Args&&... args)
  {
    // coverity[fun_call_w_exception : FALSE]

  	// TODO: Unicode- Which Locale is used? Unicode safe?

    return ::fmt::format(fmt, EnumToInt(std::forward<Args>(args))...);
    // Can NOT call CLog::Log due to recursion!
  }

  static std::string FormatV(PRINTF_FORMAT_STRING const char *fmt, va_list args);
  static std::wstring FormatV(PRINTF_FORMAT_STRING const wchar_t *fmt, va_list args);

#if defined(UNICODE_STRING_DISABLE)
  /*!
   * \brief Converts a string to Upper case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToUpper(std::string &str, const std::locale &locale);

  /*!
   * \brief Converts a string to Upper case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   * \param locale controls the conversion rules
   */
  static void ToUpper(std::string &str, const icu::Locale &locale);

  /*!
   * \brief Converts a string to Upper case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   */
  static void ToUpper(std::string &str);

  /*!
   * \brief Converts a wstring to Upper case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToUpper(std::wstring &str, const std::locale &locale);

  /*!
   * \brief Converts a wstring to Upper case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   * \param locale controls the conversion rules
   */
  static void ToUpper(std::wstring &str, const icu::Locale &locale);

  /*!
   * \brief Converts a wstring to Upper case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   */
  static void ToUpper(std::wstring &str);

  /*!
   * \brief Converts a string to Lower case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToLower(std::string &str, const std::locale &locale);

  /*!
   * \brief Converts a string to Lower case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   * \param locale controls the conversion rules
   */
  static void ToLower(std::string &str, const icu::Locale &locale);

  /*!
   * \brief Converts a string to Lower case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   */
  static void ToLower(std::string &str);

  /*!
   * \brief Converts a wstring to Lower case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToLower(std::wstring &str, const std::locale &locale);

  /*!
   * \brief Converts a wstring to Lower case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   * \param locale controls the conversion rules
   */
  static void ToLower(std::wstring &str, const icu::Locale &locale);

  /*!
   * \brief Converts a wstring to Lower case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   */
  static void ToLower(std::wstring &str);

  /*!
   *  \brief Folds the case of a string. Locale Independent.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * In particular, when FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
   * i follow the "en" locale rules for ToLower.
   *
   * DEVELOPERS who use non-ASCII keywords that will use FoldCase
   * should be aware that it may not always work as expected. Testing is important.
   * Changes will have to be made to keywords that don't work as expected. One solution is
   * to try to always use lower-case in the first place.
   *
   * \param str string to fold in place
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, FOLD_CASE_DEFAULT
   *
   * Note: This function serves a similar purpose that "ToLower/ToUpper" is
   *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does
   *       NOT work to 'normalize' Unicode to a consistent value regardless of
   *       the case variations of the input string. A particular problem is the behavior
   *       of "The Turkic I". FOLD_CASE_DEFAULT is effective at
   *       eliminating this problem. Below are the four "I" characters in
   *       Turkic and the result of FoldCase for each:
   *
	 * Locale                    Unicode                                      Unicode
   *                           codepoint                                    (hex 32-bit codepoint(s))
   * en_US I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
   * tr_TR I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
   * FOLD1 I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
   * FOLD2 I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
   *
   * en_US i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * tr_TR i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * FOLD1 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * FOLD2 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   *
   * en_US İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069
   * FOLD1 İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * FOLD2 İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069
   *
   * en_US ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   * tr_TR ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   * FOLD1 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   * FOLD2 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   *
   * FOLD_CASE_DEFAULT causes FoldCase to behave similar to ToLower for the "en" locale
   * FOLD_CASE_SPECIAL_I causes FoldCase to behave similar to ToLower for the "tr_TR" locale.
   *
   * Case folding also ignores insignificant differences in strings (some accent marks,
   * etc.).
   *
   * TODO: Modify to return str (see StartsWithNoCase)
   */
  static void FoldCase(std::wstring &str,
      const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   *  \brief Folds the case of a string.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param str string to fold
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, FOLD_CASE_DEFAULT
   *
   * Note: For more details, see
   *       FoldCase(std::wstring &str, const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
   *
   *
   * TODO: Modify to return str (see StartsWithNoCase)
   */
  static void FoldCase(std::string &str,
      const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
#endif

  /*!
   *  \brief Capitalizes a wstring using locale.
   *
   * If USE_TO_TITLE_FOR_CAPITALIZE is set, then uses ICU:toTitle to
   * convert string to "Title Case" following Locale specific rules.
   *
   * Otherwise, uses a simplistic approach familiar to English speakers.
   * See comments for USE_TO_TITLE_FOR_CAPITALIZE for more information.
   *
   * \param str string to capitalize in place
   * \param locale Use capitalization rules of Locale (ignored when
   *        USE_TO_TITLE_FOR_CAPITALIZE not set)
   * \return str converted to "Title Case"
   */
  static void ToCapitalize(std::wstring &str, const icu::Locale &locale);

  /*!
   *  \brief Capitalizes a wstring using locale.
   *
   * If USE_TO_TITLE_FOR_CAPITALIZE is set, then uses ICU:toTitle to
   * convert string to "Title Case" following Locale specific rules.
   *
   * Otherwise, uses a simplistic approach familiar to English speakers.
   * See comments for USE_TO_TITLE_FOR_CAPITALIZE for more information.
   *
   * \param str string to capitalize in place
   * \param locale Use capitalization rules of Locale (ignored when
   *        USE_TO_TITLE_FOR_CAPITALIZE not set)
   * \return str converted to "Title Case"
   */
  static void ToCapitalize(std::wstring &str, const std::locale &locale);

  /*!
   *  \brief Capitalizes a wstring using locale.
   *
   * If USE_TO_TITLE_FOR_CAPITALIZE is set, then uses ICU:toTitle to
   * convert string to "Title Case" following Locale specific rules, using
   * LangInfo::GetSystemLocale.
   *
   * Otherwise, uses a simplistic approach familiar to English speakers.
   * See comments for USE_TO_TITLE_FOR_CAPITALIZE for more information.
   *
   * \param str string to capitalize in place
   * \return str converted to "Title Case"
   */
  static void ToCapitalize(std::wstring &str);

  /*!
   *  \brief Capitalizes a string using locale.
   *
   * If USE_TO_TITLE_FOR_CAPITALIZE is set, then uses ICU:toTitle to
   * convert string to "Title Case" following Locale specific rules.
   *
   * Otherwise, uses a simplistic approach familiar to English speakers.
   * See comments for USE_TO_TITLE_FOR_CAPITALIZE for more information.
   *
   * \param str string to capitalize in place
   * \param locale Use capitalization rules of Locale (ignored when
   *        USE_TO_TITLE_FOR_CAPITALIZE not set)
   * \return str converted to "Title Case"
   */
  static void ToCapitalize(std::string &str, const icu::Locale &locale);

  /*!
   *  \brief Capitalizes a string using locale.
   *
   * If USE_TO_TITLE_FOR_CAPITALIZE is set, then uses ICU:toTitle to
   * convert string to "Title Case" following Locale specific rules.
   *
   * Otherwise, uses a simplistic approach familiar to English speakers.
   * See comments for USE_TO_TITLE_FOR_CAPITALIZE for more information.
   *
   * \param str string to capitalize in place
   * \param locale Use capitalization rules of Locale (ignored when
   *        USE_TO_TITLE_FOR_CAPITALIZE not set)
   * \return str converted to "Title Case"
   */
  static void ToCapitalize(std::string &str, const std::locale &locale);

  /*!
   *  \brief Capitalizes a string using LangInfo::GetSystemLocale.
   *
   * If USE_TO_TITLE_FOR_CAPITALIZE is set, then uses ICU:toTitle to
   * convert string to "Title Case" following Locale specific rules.
   *
   * Otherwise, uses a simplistic approach familiar to English speakers.
   * See comments for USE_TO_TITLE_FOR_CAPITALIZE for more information.
   *
   * \param str string to capitalize in place
   * \return str converted to "Title Case"
   */
  static void ToCapitalize(std::string &str);

  /*!
   *  \brief TitleCase a wstring using locale.
   *
   *  TitleCases the given wstring according to the rules of the given locale.
   *  Similar too, but more language friendly version of ToCapitalize. Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   */
  static void TitleCase(std::wstring &str, const std::locale &locale);

  /*!
   *  \brief TitleCase a string using LangInfo::GetSystemLocale.
   *
   *  TitleCases the given wstring according to the locale.
   *  Similar too, but more language friendly version of ToCapitalize with
   *  USE_TO_TITLE_FOR_CAPITALIZE not defined. Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   */
  static void TitleCase(std::wstring &str);

  /*!
   *  \brief TitleCase a wstring using locale.
   *
   *  TitleCases the given wstring according to the rules of the given locale.
   *  Similar too, but more language friendly version of ToCapitalize when
   *  USE_TO_TITLE_FOR_CAPITALIZE is not defined. Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   */
  static void TitleCase(std::string &str, const std::locale &locale);

  /*!
   *  \brief TitleCase a wstring using LangInfo::GetSystemLocale.
   *
   *  TitleCases the given wstring according to the rules of the locale.
   *  Similar too, but more language friendly version of ToCapitalize when
   *  USE_TO_TITLE_FOR_CAPITALIZE is not defined. Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   */
  static void TitleCase(std::string &str);

  /*!
   *  \brief Normalizes a wstring. Not expected to be used outside of StringUtils.
   *
   *  Made public to facilitate testing.
   *
   *  There are multiple Normalizations that can be performed on Unicode. Fortunately
   *  normalization is not needed in many situations. An introduction can be found
   *  at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
   *
   *  \param str string to normalize.
   *  \param options fine tunes behavior. See StringOptions. Frequently can leave
   *         at default value.
   *  \param normalizerType select the appropriate normalizer for the job
   *  \return normalized string
   */

  static const std::wstring Normalize(const std::wstring &src, const StringOptions opt =
      StringOptions::FOLD_CASE_DEFAULT, const NormalizerType normalizerType = NormalizerType::NFKC);

  /*!
   *  \brief Normalizes a string. Not expected to be used outside of StringUtils.
   *
   *  Made public to facilitate testing.
   *
   * There are multiple Normalizations that can be performed on Unicode. Fortunately
   * normalization is not needed in many situations. An introduction can be found
   * at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
   *
   * \param str string to normalize.
   * \param options fine tunes behavior. See StringOptions. Frequently can leave
   *        at default value.
   * \param normalizerType select the appropriate normalizer for the job
   * \return normalized string
   */

  static const std::string Normalize(const std::string &src, const StringOptions opt =
      StringOptions::FOLD_CASE_DEFAULT, const NormalizerType normalizerType = NormalizerType::NFKC);

  /*!
   *  \brief Determines if two strings are identical in content.
   *
   * Performs a bitwise comparison of the two strings. Locale is
   * not considered.
   *
   * \param str1 one of the strings to compare
   * \param str2 the other string to compare
   * \return true if both strings are identical, otherwise false
   */

  static bool Equals(const std::string &str1, const std::string &str2);

  /*!
   * \brief determines if two wstrings are identical in content.
   *
   * Performs a bitwise comparison of the two strings. Locale is
   * not considered.
   *
   * \param str1 one of the wstrings to compare
   * \param str2 the other wstring to compare
   * \return true if both wstrings are identical, otherwise false
   */
  static bool Equals(const std::wstring &str1, const std::wstring &str2);

  // TODO: Add wstring version of EqualsNoCase

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if normalize == true: Equals(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param opt StringOptions to apply. Generally leave at default value.
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return true if both strings compare after case folding, otherwise false
   */
  static bool EqualsNoCase(const std::string &str1, const std::string &str2,
      const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(s2, opt))
   * or, if normalize == true: Equals(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param str1 one of the strings to compare
   * \param s2 one of the (c-style) strings to compare
   * \param opt StringOptions to apply. Generally leave at default value.
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return true if both strings compare after case folding, otherwise false
   */
  static bool EqualsNoCase(const std::string &str1, const char *s2, const StringOptions opt =
      StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(s1, opt)), FoldCase(s2, opt))
   * or, if normalize == true: Equals(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param s1 one of the (c-style) strings to compare
   * \param s2 one of the (c-style) strings to compare
   * \param opt StringOptions to apply. Generally leave at default value.
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return true if both strings compare after case folding, otherwise false
   */
  static bool EqualsNoCase(const char *s1, const char *s2, StringOptions opt =
      StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*!
   * \brief Compares two wstrings using codepoint order. Locale does not matter.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \return <0 or 0 or >0 as usual for string comparisons
   */
  static int Compare(const std::wstring &str1, const std::wstring &str2);

  /*!
   * \brief Compares two strings using codepoint order. Locale does not matter.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \return <0 or 0 or >0 as usual for string comparisons
   */
  static int Compare(const std::string &str1, const std::string &str2);

  /*!
   * \brief Initializes the Collator for this thread, such as before sorting a
   * table.
   *
   * Assumes that all collation will occur in this thread.
   *
   * \param icuLocale Collation order will be based on the given locale.
   * \param normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true of initialization was successful, otherwise false.
   */
  static bool InitializeCollator(const icu::Locale &icuLocale, bool normalize /* = false */);

  /*!
   * \brief Initializes the Collator for this thread, such as before sorting a
   * table.
   *
   * Assumes that all collation will occur in this thread.
   *
   * \param locale Collation order will be based on the given locale.
   * \param normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true of initialization was successful, otherwise false.
   */
  static bool InitializeCollator(const std::locale &locale, bool normalize /* = false */);

  /*!
   * \brief Initializes the Collator for this thread using LangInfo::GetSystemLocale,
   * such as before sorting a table.
   *
   * Assumes that all collation will occur in this thread.
   *
   * \param normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true of initialization was successful, otherwise false.
   */
  static bool InitializeCollator(bool normalize = false);

  /*!
   * \brief Performs locale sensitive string comparison.
   *
   * Must be run in the same thread that InitializeCollator that configured the Collator
   * for this was run.
   *
   * \param left string to compare
   * \param right string to compare
   * \return  < 0 if left collates < right
   *         == 0 if left collates the same as right
   *          > 0 if left collates > right
   */
  static int32_t Collate(const std::wstring &left, const std::wstring &right);

  /*!
   * \brief Performs locale sensitive wstring comparison.
   *
   * Must be run in the same thread that InitializeCollator that configured the Collator
   * for this was run.
   *
   * \param left string to compare
   * \param right string to compare
   * \return  < 0 if left collates < right
   *         == 0 if left collates the same as right
   *          > 0 if left collates > right
   */
  static int32_t Collate(const wchar_t *left, const wchar_t *right)
  {
    return StringUtils::Collate(std::wstring(left), std::wstring(right));
  }

  /*!
   * \brief Performs a bit-wise comparison of two wstrings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param str1 one of the wstrings to compare
   * \param str2 one of the wstrings to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters str1 are bitwise less than the characters in str2,
   * = 0 if str1 contains the same characters as str2,
   * > 0 if the characters in str1 are bitwise greater than the characters in str2.
   */
  static int CompareNoCase(const std::wstring &str1, const std::wstring &str2, StringOptions opt =
      StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters str1 are bitwise less than the characters in str2,
   * = 0 if str1 contains the same characters as str2,
   * > 0 if the characters in str1 are bitwise greater than the characters in str2.
   */
  static int CompareNoCase(const std::string &str1, const std::string &str2, StringOptions opt =
      StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  // TODO: Review issues with truncating length of multi-byte strings.

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(s1, opt)), FoldCase(s2, opt))
   * or, if normalize == true: Compare(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * NOTE: Limiting the number of bytes to compare via the option n may produce
   *       unexpected results for multi-byte characters.
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param s1 one of the strings to compare
   * \param s2 one of the strings to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if s1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int CompareNoCase(const char *s1, const char *s2, StringOptions opt =
      StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: Use of the byte-length argument n is STRONGLY discouraged since
   * it can easily result in malformed Unicode.
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param n maximum number of bytes to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters str1 are bitwise less than the characters in str2,
   * = 0 if str1 contains the same characters as str2,
   * > 0 if the characters in str1 are bitwise greater than the characters in str2.
   */
  [[deprecated("StartsWith/EndsWith may be better choices. Multibyte characters, case folding and byte lengths don't mix.") ]] static int CompareNoCase(
      const std::string &str1, const std::string &str2, size_t n, StringOptions opt =
          StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(s1, opt)), FoldCase(s2, opt))
   * or, if normalize == true: Compare(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * NOTE: Limiting the number of bytes to compare via the option n may produce
   *       unexpected results for multi-byte characters.
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using normalize
   * should yield better results for those cases.
   *
   * \param s1 one of the strings to compare
   * \param s2 one of the strings to compare
   * \param n maximum number of bytes to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if s1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */

  [[deprecated("StartsWith/EndsWith may be better choices. Multibyte characters, case folding and byte lengths don't mix.") ]] static int CompareNoCase(
      const char *s1, const char *s2, size_t n,
      StringOptions opt = StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  // TODO: consider renaming to ParseInt

  /*!
   * \brief Returns the int value of the first series of digits found in the string
   *
   *  Ignores any non-digits in string
   *
   * \param str to extract number from
   * \return int value of found string
   */
  static int ReturnDigits(const std::string &str);

  /*!
   *  \brief Remove all whitespace from beginning and end of str
   *
   *  TODO:  Create TrimCopy versions which do not modify the original string.
   *         If, in order to avoid complications with redefining StringUtils methods,
   *         a replacement class, UnicodeUtils or some such is created. Then more
   *         latitude will exist for naming, etc.
   *
   * \param str to trim in place
   * \return trimmed string, same as str argument.
   */
  static std::string& Trim(std::string &str);

  /*!
   *  \brief Remove a set of characters from beginning and end of str
   *
   *  Ex: Trim("abc1234bxa", "acb") ==> "1234bx"
   *
   * \param str to trim in place
   * \param chars (characters) to remove from str
   * \return trimmed string, same as str argument.
   */
  static std::string& Trim(std::string &str, const char* const chars);

  /*!
   * \brief Remove all whitespace from beginning of str
   *
   * \param str to trim in place
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimLeft(std::string &str);

  /*!
   * \brief Remove a set of characters from beginning of str
   *
   *  Ex: TrimLeft("abc1234bxa", "acb") ==> "1234bxa"
   *
   * \param str to trim in place
   * \param chars (characters) to remove from str
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimLeft(std::string &str, const char* const chars);

  /*!
   * \brief Remove all whitespace from end of str
   *
   * \param str to trim in place
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimRight(std::string &str);

  /*! \brief Remove a set of characters from end of str
   *
   *  Remove any trailing characters from str that are in the set chars.
   *
   *  Ex: TrimRight("abc1234bxa", "acb") ==> "abc1234bx"
   *
   * \param str to trim in place
   * \param chars (characters) to remove from str
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimRight(std::string &str, const char* const chars);

  /*! \brief Remove leading and trailing ASCII space and TAB characters from str
   *
   * TODO: See if can be eliminated. At least rename.
   *
   * \param str to trim in place
   * \return trimmed string, same as str argument.
   */
  static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str);

  /*!
   *  \brief Replaces every occurrence of a char in string.
   *
   *  Somewhat less efficient than FindAndReplace because this one returns a count
   *  of the number of changes, which requires extra calls to Unicode code.
   *
   *  \param str String to make changes to in place
   *  \param oldChar character to be replaced
   *  \parm newChar character to replace with
   *  \return Count of the number of changes
   */
  static int Replace(std::string &str, char oldChar, char newChar);

  /*!
   *  \brief Replaces every occurrence of a string within another string.
   *
   *  Somewhat less efficient than FindAndReplace because this one returns a count
   *  of the number of changes, which requires extra calls to Unicode lib.
   *
   *  \param str String to make changes to in place
   *  \param oldStr string to be replaced
   *  \parm newStr string to replace with
   *  \return Count of the number of changes
   */
  static int Replace(std::string &str, const std::string &oldStr, const std::string &newStr);

  /*!
   * \brief Replaces every occurrence of a wstring within another wstring
   *
   * Somewhat less efficient than FindAndReplace because this one returns a count
   * of the number of changes.
   *
   * \param str String to make changes to in place
   * \param oldStr string to be replaced
   * \parm newStr string to replace with
   * \return Count of the number of changes
   */
  static int Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr);

  /*!
   *  \brief Replaces every occurrence of a string within another string
   *
   * Should be more efficient than Replace since it directly uses an icu library
   * routine and does not have to count changes.
   *
   * \param str String to make changes to in place
   * \param oldStr string to be replaced
   * \parm newStr string to replace with
   * \return the modified string (same as str)
   */
  static std::string& FindAndReplace(std::string &str, const std::string oldText,
  		const std::string newText);

  /*!
   * \brief Replaces every occurrence of a string within another string.
   *
   * Should be more efficient than Replace since it directly uses an icu library
   * routine and does not have to count changes.
   *
   * \param str String to make changes to in place
   * \param oldStr string to be replaced
   * \parm newStr string to replace with
   * \return the modified string (same as str)
   */
  static std::string& FindAndReplace(std::string &str, const char * oldText,
  		const char * newText);

  /*!
   * \brief Replaces every occurrence of a regex pattern with a string in another string.
   *
   * Regex based version of Replace. See:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * \param str string being altered in place
   * \param pattern regular expression pattern
   * \param newStr new value to replace with
   * \param flags controls behavior of regular expression engine
   * \return result of regular expression
   */
  std::string RegexReplaceAll(std::string &str, const std::string pattern,
  		const std::string newStr, const int flags);

  /*!
   * \brief Determines if a string begins with another string
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \return true if str1 starts with str2, otherwise false
   */
  static bool StartsWith(const std::string &str1, const std::string &str2);

  /*!
   *  \brief Determines if a string begins with another string
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \return true if str1 starts with s2, otherwise false
   */
  static bool StartsWith(const std::string &str1, const char *s2);

  /*!
   *  \brief Determines if a string begins with another string
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of str1
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWith(const char *s1, const char *s2);

  /*!
   * \brief Determines if a string begins with another string, ignoring their case
   *
   * Equivalent to StartsWith(FoldCase(str1), FoldCase(str2))
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \param opt controls behavior of case folding
   * \return true if str1 starts with str2, otherwise false
   */
  static bool StartsWithNoCase(const std::string &str1, const std::string &str2,
  		StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string begins with another string, ignoring their case
   *
   * Equivalent to StartsWith(FoldCase(str1), FoldCase(s2))
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \param opt controls behavior of case folding
   * \return true if str1 starts with s2, otherwise false
   */
  static bool StartsWithNoCase(const std::string &str1, const char *s2,
  		StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string begins with another string, ignoring their case
   *
   * Equivalent to StartsWith(FoldCase(s1), FoldCase(s2))
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of s1
   * \param opt controls behavior of case folding
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWithNoCase(const char *s1, const char *s2, StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string ends with another string
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWith(const std::string &str1, const std::string &str2);

  /*!
   * \brief Determines if a string ends with another string
   *
   * \param str1 string to be searched
   * \param s2 string to find at end of str1
   * \return true if str1 ends with s2, otherwise false
   */
  static bool EndsWith(const std::string &str1, const char *s2);

  /*!
   * \brief Determines if a string ends with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \param opt controls behavior of case folding
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWithNoCase(const std::string &str1, const std::string &str2, StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string begins with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \param opt controls behavior of case folding
   * \return true if str1 starts with s2, otherwise false
   */
  static bool EndsWithNoCase(const std::string &str1, const char *s2, StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Builds a string by appending every string from a container, separated by a delimiter
   *
   * \param strings a container of a number of strings
   * \param delimiter will separate each member of strings
   * \return the concatenation of every string in the container, separated by the delimiter
   *
   * Note: This looks like it is Unicode safe, but more research required to be sure.
   *       Most likely this is okay if the delimiter is a simple separator (space, comma,
   *       etc.).
   */
  template<typename CONTAINER>
  static std::string Join(const CONTAINER &strings, const std::string& delimiter)
  {
    std::string result;
    for (const auto& str : strings)
      result += str + delimiter;

    if (!result.empty())
      result.erase(result.size() - delimiter.size());
    return result;
  }

  /*!
   * \brief Splits the given input string into separate strings using the given delimiter.
   *
   * If the given input string is empty the result will be an empty array (not
   * an array containing an empty string)
   *
   * \param input Input string to be split
   * \param delimiter Delimiter to be used to split the input string
   * \param iMaxStrings (optional) Maximum number of split strings
   */
  static std::vector<std::string> Split(const std::string& input, const std::string& delimiter, unsigned int iMaxStrings = 0);

  /*! \brief Splits the given input string into separate strings using the given delimiter.

   If the given input string is empty the result will be an empty array (not
   an array containing an empty string).

   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of split strings
   */
  static std::vector<std::string> Split(const std::string& input, const char delimiter, size_t iMaxStrings = 0);

  /*! \brief Splits the given input string into separate strings using the given delimiters.

   First, for all but the first delimiter, each occurrence of the delimiter in input will be
   replaced by the first delimiter.

   example: input = "abcdefg" delimiters = "a", "de", "e"
            input => "abcag"  // Substitute "de" with "a"
            input => "abcag" // No occurrence of "e"
            output => "", "bc", "g"

   Finally, the first delimiter will be used to split input, as split using a single delimiter

   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of split strings

   TODO: Need test case for this, including example
   */
  static std::vector<std::string> Split(const std::string& input, const std::vector<std::string>& delimiters);


  /*! \brief Splits the given input strings using the given delimiters into further separate strings.

  If an input vector element is empty the result will be an empty vector element (not
  an empty string).

  Delimiter strings are applied in order, so once the (optional) maximum number of
  items is produced no other delimiters are applied. This produces different results
  to applying all delimiters at once e.g. "a/b#c/d" becomes "a", "b#c", "d" rather
  than "a", "b", "c/d"

  \param input Input vector of strings each to be split
  \param delimiters Delimiter strings to be used to split the input strings
  \param iMaxStrings (optional) Maximum number of resulting split strings

  TODO: Need Testcase!
  */
  static std::vector<std::string> SplitMulti(const std::vector<std::string>& input,
                                             const std::vector<std::string>& delimiters,
                                             size_t iMaxStrings = 0);

  /*! \brief Counts the occurrences of strFind in strInput
   *
   * \param strInput string to be searched
   * \param strFind string to count occurrences in strInput
   * \return count of the number of occurrences found
   */
  static int FindNumber(const std::string& strInput, const std::string &strFind);

  /*! \brief Compares two strings based on the rules of the given locale
   *
   *  TODO: DRAFT
   *
   *  This is a complex comparison. Broadly speaking, it tries to compare
   *  numbers using math rules and Alphabetic characters as words in a caseless
   *  manner. Punctuation characters, etc. are also included in comparison.
   *
   * \param left string to compare
   * \param right other string to compare
   * \param locale supplies rules for comparison
   * \return < 0 if left < right based upon comparison based on comparison rules
   */
  static int64_t AlphaNumericCompare(const wchar_t *left, const wchar_t *right, const std::locale& locale);

  /*! \brief Compares two strings based on the rules of LocaleInfo.GestSystemLocale
   *
   *  TODO: DRAFT
   *
   *  This is a complex comparison. Broadly speaking, it tries to compare
   *  numbers using math rules and Alphabetic characters as words in a caseless
   *  manner. Punctuation characters, etc. are also included in comparison.
   *
   * \param left string to compare
   * \param right other string to compare
   * \param locale supplies rules for comparison
   * \return < 0 if left < right based upon comparison based on comparison rules
   */
  static int64_t AlphaNumericCompare(const wchar_t *left, const wchar_t *right);

  /*!
     * SQLite collating function, see sqlite3_create_collation
     * The equivalent of AlphaNumericCompare() but for comparing UTF8 encoded data using
     * LangInfo::GetSystemLocale
     *
     * This only processes enough data to find a difference, and avoids expensive data conversions.
     * When sorting in memory item data is converted once to wstring in advance prior to sorting, the
     * SQLite callback function can not do that kind of preparation. Instead, in order to use
     * AlphaNumericCompare(), it would have to repeatedly convert the full input data to wstring for
     * every pair comparison made. That approach was found to be 10 times slower than using this
     * separate routine.
     *
     * /param nKey1 byte-length of first UTF-8 string
     * /param pKey1 pointer to byte array for the first UTF-8 string to compare
     * /param nKey2 byte-length of second UTF-8 string
  	 * /param pKey2 pointer to byte array for the second UTF-8 string to compare
  	 * /return 0 if strings are the same,
  	 *       < 0 if first string should come before the second
  	 *       > 0 if the first string should come after the second
     */
  static int AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2);

  /*! \brief converts timeString (hh:mm:ss or nnn min) to seconds.
   *
   * timeString is expected to be of the form:
   *  <opt whitespace>hh:mm:ss<opt whitespace> | <opt_whitespace>number min
   *
   *  ex: " 14:57 " or "  23 min"
   *
   * \param timeString string to convert to seconds may be in "hh:mm:ss" or "nnn min" format
   *                   missing values are assumed zero. Whitespace is trimmed first.
   * \return parsed value in seconds
   */
  static long TimeStringToSeconds(const std::string &timeString);

  /*! \brief Strip any trailing \n or \r characters.
   *
   * \param strLine
   */
  static void RemoveCRLF(std::string& strLine);

  /*! \brief utf8 version of strlen - skips any non-starting bytes in the count, thus returning the number of utf8 characters
   \param s c-string to find the length of.
   \return the number of utf8 characters in the string.
   */
  static size_t utf8_strlen(const char *s);

  /*! \brief convert a time in seconds to a string based on the given time format
   \param seconds time in seconds
   \param format the format we want the time in.
   \return the formatted time
   \sa TIME_FORMAT
   */
  static std::string SecondsToTimeString(long seconds, TIME_FORMAT format = TIME_FORMAT_GUESS);

  /*! \brief check whether a string is a natural number.
   Matches [ \t]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is a natural number, false otherwise.
   */
  static bool IsNaturalNumber(const std::string& str);

  /*! \brief check whether a string is an integer.
   Matches [ \t]*[\-]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is an integer, false otherwise.
   */
  static bool IsInteger(const std::string& str);

  inline static bool containsNonAscii(std::string str)
  {
  	for (size_t i = 0; i < str.length(); i++) {
  		if (not isascii(str[i])) {
  			return true;
  		}
  	}
  	return false;
  }

  inline static bool containsNonAscii(std::wstring str)
  {
    for (size_t i = 0; i < str.length(); i++) {
      if (not isascii(str[i])) {
        return true;
      }
    }
    return false;
  }
  /* The next several isasciiXX and asciiXXvalue functions are locale independent (US-ASCII only),
   * as opposed to standard ::isXX (::isalpha, ::isdigit...) which are locale dependent.
   * Next functions get parameter as char and don't need double cast ((int)(unsigned char) is required for standard functions). */
  inline static bool isasciidigit(char chr) // locale independent
  {
    return chr >= '0' && chr <= '9';
  }
  inline static bool isasciixdigit(char chr) // locale independent
  {
    return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F');
  }
  static int asciidigitvalue(char chr); // locale independent
  static int asciixdigitvalue(char chr); // locale independent
  inline static bool isasciiuppercaseletter(char chr) // locale independent
  {
    return (chr >= 'A' && chr <= 'Z');
  }
  inline static bool isasciilowercaseletter(char chr) // locale independent
  {
    return (chr >= 'a' && chr <= 'z');
  }
  inline static bool isasciialphanum(char chr) // locale independent
  {
    return isasciiuppercaseletter(chr) || isasciilowercaseletter(chr) || isasciidigit(chr);
  }
  static std::string SizeToString(int64_t size);
  static const std::string Empty;

  /**
   * Scans the str for the occurrence of word.
   *
   * Word is either all letters or all digits. Words must be
   * separated by spaces.
   */
  static size_t FindWord(const std::string &str, const std::string &word);

  /*!
   * \brief Starting at a point after an opening bracket, scans a string for it's matching
   * close bracket.
   *
   * Note: While the string can be utf-8, the open & close brackets must be ASCII.
   *
   * \param str A utf-8 string to scan for 'brackets'
   * \param opener The 'open-bracket' ASCII character used in the string
   * \param closer The 'close-bracket' ASCII character used in the string
   * \param startPos Offset in str, past the open-bracket that the function is to find
   * the closing bracket for.
   *
   * \return the index of the matching close-bracket, or std::string::npos if not found.
   *
   */
  static int FindEndBracket(const std::string &str, char opener, char closer, int startPos = 0);

  static int DateStringToYYYYMMDD(const std::string &dateString);
  static std::string ISODateToLocalizedDate (const std::string& strIsoDate);
  static void WordToDigits(std::string &word);
  static std::string CreateUUID();
  static bool ValidateUUID(const std::string &uuid); // NB only validates syntax
  static double CompareFuzzy(const std::string &left, const std::string &right);
  static int FindBestMatch(const std::string &str, const std::vector<std::string> &strings, double &matchscore);
  static bool ContainsKeyword(const std::string &str, const std::vector<std::string> &keywords);

  /*! \brief Convert the string of binary chars to the actual string.

  Convert the string representation of binary chars to the actual string.
  For example \1\2\3 is converted to a string with binary char \1, \2 and \3

  \param param String to convert
  \return Converted string
  */
  static std::string BinaryStringToString(const std::string& in);
  /**
   * Convert each character in the string to its hexadecimal
   * representation and return the concatenated result
   *
   * example: "abc\n" -> "6162630a"
   */
  static std::string ToHexadecimal(const std::string& in);
  /*! \brief Format the string with locale separators.

  Format the string with locale separators.
  For example 10000.57 in en-us is '10,000.57' but in italian is '10.000,57'

  \param param String to format
  \return Formatted string
  */
  template<typename T>
  static std::string FormatNumber(T num)
  {
    std::stringstream ss;
// ifdef is needed because when you set _ITERATOR_DEBUG_LEVEL=0 and you use custom numpunct you will get runtime error in debug mode
// for more info https://connect.microsoft.com/VisualStudio/feedback/details/2655363
#if !(defined(_DEBUG) && defined(TARGET_WINDOWS))
    ss.imbue(g_langInfo.GetSystemLocale());
#endif
    ss.precision(1);
    ss << std::fixed << num;
    return ss.str();
  }

  /*! \brief Escapes the given string to be able to be used as a parameter.

   Escapes backslashes and double-quotes with an additional backslash and
   adds double-quotes around the whole string.

   \param param String to escape/paramify
   \return Escaped/Paramified string
   */
  static std::string Paramify(const std::string &param);

  // TODO: Could rewrite to use Unicode delimiters using icu::UnicodeSet::spanUTF8(), and friends
  /*!
   *  \brief Splits a string using one or more delimiting ASCII characters,
   *  ignoring empty tokens.
   *
   *  Missing delimiters results in a match of entire string.
   *
   *  Differs from Split() in two ways:
   *    1. The delimiters are treated as individual ASCII characters, rather than a single
   *       delimiting string.
   *    2. Empty tokens are ignored.
   *
   * \param input string to split into tokens
   * \param delimiters one or more ASCII characters to use as token separators
   * \return a vector of non-empty tokens.
   *
   */
  static std::vector<std::string> Tokenize(const std::string& input, const std::string& delimiters);

  /*!
   *  \brief Splits a string using one or more delimiting ASCII characters,
   *  ignoring empty tokens.
   *
   *  Instead of returning the tokens as a return value, they are returned via the first
   *  argument, 'tokens'.
   *
   * \param tokens Vector that found, non-empty tokens are returned by
   * \param input string to split into tokens
   * \param delimiters one or more ASCII characters to use as token separators
   *
   */
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters);

  /*!
   * \brief Splits a string into tokens delimited by a single ASCII character.
   *
   * \param input string to split into tokens
   * \param delimiter an ASCII character to use as a token separator
   * \return a vector of non-empty tokens.
   *
   */
  static std::vector<std::string> Tokenize(const std::string& input, const char delimiter);

  /*!
   * \brief Splits a string into tokens delimited by a single ASCII character.
   *
   * \param tokens Vector that found, non-empty tokens are returned by
   * \param input string to split into tokens
   * \param delimiter an ASCII character to use as a token separator
   *
   */
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const char delimiter);


  static uint64_t ToUint64(const std::string& str, uint64_t fallback) noexcept;

  /*!
   * \brief Formats a file-size into human a human-friendly form
   * Returns bytes in a human readable format using the smallest unit that will fit `bytes` in at
   * most three digits. The number of decimals are adjusted with significance such that 'small'
   * numbers will have more decimals than larger ones.
   *
   * For example: 1024 bytes will be formatted as "1.00kB", 10240 bytes as "10.0kB" and
   * 102400 bytes as "100kB". See TestStringUtils for more examples.
   */
  static std::string FormatFileSize(uint64_t bytes);

  /*!
   * \brief Creates a std::string from a char*. A null pointer results in an empty string.
   *
   * \param cstr pointer to C null-terminated byte array
   * \return the resulting std::string or ""
   */
  static std::string CreateFromCString(const char* cstr);

private:
  static int64_t AlphaNumericCompare_orig(const wchar_t *left, const wchar_t *right);

};

struct sortstringbyname
{
  bool operator()(const std::string& strItem1, const std::string& strItem2) const
  {
    return StringUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
};
