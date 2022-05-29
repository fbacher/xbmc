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
#include <stdarg.h>   // others depend on this
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
	 *    standard. What we think of as a character is a Grapheme.
	 *    A Grapheme can be composed of one or more codepoints. The first
	 *    codepoint contains the basic character information, additional
	 *    codepoints add accents, etc.
	 *
	 *    Frequently an accented Grapheme can be represented in multiple
	 *    combinations of codepoints. One such case has five different
	 *    representations of the came Grapheme varying in length from 1
	 *    to five codepoints. Normalization (which is locale specific)
	 *    reorders and possibly combines a Grapheme's codepoints into
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
	 *  * When iterating the graphemes of a string, you frequently have
	 *    to know where the grapheme boundaries are.
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
  static void ToUpper(std::string &str, const std::locale& locale);

  /*!
   * \brief Converts a string to Upper case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   * \param locale controls the conversion rules
   */
  static void ToUpper(std::string &str, const icu::Locale& locale);

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
  static void ToUpper(std::wstring &str, const std::locale& locale);

  /*!
   * \brief Converts a wstring to Upper case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   * \param locale controls the conversion rules
   */

  static void ToUpper(std::wstring &str, const icu::Locale& locale);

  /*!
   * \brief Converts a wstring to Upper case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on
   */
  static void ToUpper(std::wstring &str);


  /*! \brief Converts a string to Lower case using locale.

  Conversion rules determined by locale

  \param str string to change case on
  \param locale
  */
  static void ToLower(std::string &str, const std::locale& locale);


  /*! \brief Converts a string to Lower case using locale.

  Conversion rules determined by locale

  \param str string to change case on
  \param locale
  */
  static void ToLower(std::string &str, const icu::Locale& locale);

  /*! \brief Converts a wstring to Lower case using LangInfo::GetSystemLocale.

  Conversion rules determined by system locale

  \param str string to change case on
  \param locale
  */
  static void ToLower(std::string &str);

  /*! \brief Converts a string to Lower case using locale.

  Conversion rules determined by locale

  \param str string to change case on
  \param locale
  */
  static void ToLower(std::wstring &str, const std::locale& locale);

  /*! \brief Converts a string to Lower case using locale.

  Conversion rules determined by locale

  \param str string to change case on
  \param locale
  */

  static void ToLower(std::wstring &str, const icu::Locale& locale);

  /*! \brief Converts a wstring to Lower case using locale.

  Conversion rules determined by locale

  \param str string to change case on
  \param locale
  */
  static void ToLower(std::wstring &str);

  /*! \brief Folds the case of a string.

  Similar to ToLower except in addition, insignificant accents are stripped
  and other transformations are made (such as German sharp-S is converted to ss).
  The transformation is independent of locale.

  \param str string to fold in place
  \param opt StringOptions to fine-tune behavior. For most purposes, leave at
             default value, FOLD_CASE_DEFAULT

	Note: This function is mostly used here to 'normalize' strings before using
	      them as keywords/map indexes, etc. A particular problem is the behavior
	      of "The Turkic I". FOLD_CASE_DEFAULT is effective at
	      eliminating this problem. Below are the four "I" characters in
	      Turkic and the result of FoldCase for each:

	   *  FOLD_CASE_DEFAULT
     * I (\u0049) -> i (\u0069) // ASCII upper -> ASCII lower
     * İ (\u0130) -> i̇ (\u0069 \u0307) // Dotted upper ->
     * i (\u0069) -> i (\u0069) // ASCII lower -> ASCII lower
     * ı (\u0131) -> ı (\u0131)
		 *
		 *
		 * FOLD_CASE_EXCLUDE_SPECIAL_I
		 * I (\u0049) -> ı (\u0131) // Dotless upper -> Dotless lower
		 * i (\u0069) -> i (\u0069) // Dotted lower -> Dotted lower
		 * İ (\u0130) -> i (\u0069) // Dotted upper -> Dotted lower
		 * ı (\u0131) -> ı (\u0131) // Dotless lower -> Dotless lower

  TODO: Modify to return str (see StartsWithNoCase)
  */
  static void FoldCase(std::wstring &str, const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*! \brief Folds the case of a string.

  Similar to ToLower except in addition, insignificant accents are stripped
  and other transformations are made (such as German sharp-S is converted to ss).
  The transformation is independent of locale.

  \param str string to fold
  \param opt StringOptions to fine-tune behavior. For most purposes, leave at
             default value, FOLD_CASE_DEFAULT

	Note: This function is mostly used here to 'normalize' strings before using
	      them as keywords/map indexes, etc. A particular problem is the behavior
	      of "The Turkic I". FOLD_CASE_DEFAULT is effective at
	      eliminating this problem. Below are the four "I" characters in
	      Turkic and the result of FoldCase for each:

	   *  FOLD_CASE_DEFAULT
     * I (\u0049) -> i (\u0069) // ASCII upper -> ASCII lower
     * İ (\u0130) -> i̇ (\u0069 \u0307) // Dotted upper ->
     * i (\u0069) -> i (\u0069) // ASCII lower -> ASCII lower
     * ı (\u0131) -> ı (\u0131)
		 *
		 *
		 * FOLD_CASE_EXCLUDE_SPECIAL_I
		 * I (\u0049) -> ı (\u0131) // Dotless upper -> Dotless lower
		 * i (\u0069) -> i (\u0069) // Dotted lower -> Dotted lower
		 * İ (\u0130) -> i (\u0069) // Dotted upper -> Dotted lower
		 * ı (\u0131) -> ı (\u0131) // Dotless lower -> Dotless lower

  TODO: Modify to return str (see StartsWithNoCase)
  */
  static void FoldCase(std::string &str, const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*! \brief Capitalizes a wstring using locale.

  Capitalizes using a simplistic approach familiar to English speakers.
  See TitleCase for a more language friendly version.

  \param str string to capitalize
  \param locale
  */
  static void ToCapitalize(std::wstring &str, const icu::Locale &locale);

  /*! \brief Capitalizes a wstring using locale.

  Capitalizes using a simplistic approach familiar to English speakers.
  See TitleCase for a more language friendly version.

  \param str string to capitalize
  \param locale
  */
  static void ToCapitalize(std::wstring &str, const std::locale &locale);

  /*! \brief Capitalizes a wstring using locale.

  Capitalizes using a simplistic approach familiar to English speakers.
  See TitleCase for a more language friendly version.

  \param str string to capitalize
  \param locale
  */
  static void ToCapitalize(std::wstring &str);

  /*! \brief Capitalizes a string using using a locale.

  Capitalizes using a simplistic approach familiar to English speakers.
  See TitleCase for a more language friendly version.

  \param str string to capitalize
  \param locale
  */
  static void ToCapitalize(std::string &str, const icu::Locale &locale);

  /*! \brief Capitalizes a string using using a locale.

  Capitalizes using a simplistic approach familiar to English speakers.
  See TitleCase for a more language friendly version.

  \param str string to capitalize
  \param locale
  */
  static void ToCapitalize(std::string &str, const std::locale &locale);

  /*! \brief Capitalizes a string using using LangInfo::GetSystemLocale.

  Capitalizes using a simplistic approach familiar to English speakers.
  See TitleCase for a more language friendly version.

  \param str string to capitalize
  \param locale
  */
  static void ToCapitalize(std::string &str);

  /*! \brief TitleCase a wstring using locale.

    TitleCases the given wstring according to the rules of the given locale.
    Similar too, but more language friendly version of ToCapitalize. Uses ICU library.

    Best results are when a complete sentence/paragraph is TitleCased rather than
    individual words.

    \param str string to TitleCase
    \param locale
    */
    static void TitleCase(std::wstring &str, const std::locale &locale);

    /*! \brief TitleCase a string using locale.

      TitleCases the given wstring according to the rules of the given locale.
      Similar too, but more language friendly version of ToCapitalize. Uses ICU library.

      Best results are when a complete sentence/paragraph is TitleCased rather than
      individual words.

      \param str string to TitleCase
      \param locale
      */
    static void TitleCase(std::wstring &str);

    /*! \brief TitleCase a wstring using locale.

      TitleCases the given wstring according to the rules of the given locale.
      Similar too, but more language friendly version of ToCapitalize. Uses ICU library.

      Best results are when a complete sentence/paragraph is TitleCased rather than
      individual words.

      \param str string to TitleCase
      \param locale
      */
    static void TitleCase(std::string &str, const std::locale &locale);

    /*! \brief TitleCase a wstring using locale.

      TitleCases the given wstring according to the rules of the given locale.
      Similar too, but more language friendly version of ToCapitalize. Uses ICU library.

      Best results are when a complete sentence/paragraph is TitleCased rather than
      individual words.

      \param str string to TitleCase
      \param locale
      */
    static void TitleCase(std::string &str);

    /*! \brief Normalizes a wstring.

      There are multiple Normalizations that can be performed on Unicode. Fortunately
      normalization is not needed in many situations. An introduction can be found
      at: https://unicode-org.github.io/icu/userguide/transforms/normalization/

      \param str string to normalize in-place
      \param options fine tunes behavior. See StringOptions. Frequently can leave
      at default value.
      \param normalizerType select the appropriate normalizer for the job
      \return src
      */

    static const std::wstring Normalize(std::wstring &src,
    		const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT, const NormalizerType normalizerType = NormalizerType::NFKC);

    /*! \brief Normalizes a string.

    There are multiple Normalizations that can be performed on Unicode. Fortunately
    normalization is not needed in many situations. An introduction can be found
    at: https://unicode-org.github.io/icu/userguide/transforms/normalization/

    \param str string to normalize in-place
    \param options fine tunes behavior. See StringOptions. Frequently can leave
    at default value.
    \param normalizerType select the appropriate normalizer for the job
    \return src
    */

    static const std::string Normalize(std::string &src, const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
    	 const NormalizerType normalizerType = NormalizerType::NFKC);


    /*! \brief Determines if two strings are identical in content.
     *
     * Performs a bitwise comparison of the two strings. Locale is
     * not considered.
     *
     * \param str1 one of the strings to compare
     * \param str2 the other string to compare
     * \return true if both strings are identical, otherwise false
     *
     * Using NormalizerType.NFCKCASEFOLD
     * I (\u0049) -> i (\u0069)
     * İ (\u0130) -> i̇ (\u0069 \u0307)
     * i (\u0069) -> i (\u0069)
     * ı (\u0131) -> ı (\u0131)
     * İİ (\u0130 \u0130) -> i̇i̇ (\u0069 \u0307 \u0069 \u0307)
     * ii (\u0069 \u0069) -> ii (\u0069 \u0069)
     * ıı (\u0131 \u0131) -> ıı (\u0131 \u0131)
     */

  static bool Equals(const std::string& str1, const std::string& str2);

  /*! \brief determines if two wstrings are identical in content.
   *
   * Performs a bitwise comparison of the two strings. Locale is
   * not considered.
   *
   * \param str1 one of the wstrings to compare
   * \param str2 the other wstring to compare
   * \return true if both wstrings are identical, otherwise false
   */
    static bool Equals(const std::wstring &str1, const std::wstring &str2);

    /*! \brief Determines if two strings are the same, after case folding each.
     *
     * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(str2, opt))
     *
     * \param str1 one of the strings to compare
     * \param str2 one of the strings to compare
     * \param opt StringOptions to apply
     * \return true if both strings compare after case folding, otherwise false
     */
  static bool EqualsNoCase(const std::string &str1, const std::string &str2,
  		const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*! \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(str2, opt))
   *
   * \param str1 one of the strings to compare
   * \param s2 one of the (c-style) strings to compare
   * \param opt StringOptions to apply
   * \return true if both strings compare after case folding, otherwise false
   */
   static bool EqualsNoCase(const std::string &str1, const char *s2, const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
  		 const bool normalize = false);

   /*! \brief Determines if two strings are the same, after case folding each.
    *
    * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(str2, opt))
    *
    * \param str1 one of the strings to compare
    * \param s2 one of the (c-style) strings to compare
    * \param opt StringOptions to apply
    * \return true if both strings compare after case folding, otherwise false
    */
   static bool EqualsNoCase(const char *s1, const char *s2, StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
  		 const bool normalize = false);

   static int Compare(const std::wstring &str1, const std::wstring &str2);

   static int Compare(const std::string &str1, const std::string &str2);

   static bool InitializeCollator(bool normalize = false);

   static int32_t Collate(const std::wstring &left, const std::wstring &right);

   static int32_t Collate(const wchar_t *left, const wchar_t *right) {
     return StringUtils::Collate(std::wstring(left), std::wstring(right));
   }

   static int CompareNoCase(const std::wstring &str1, const std::wstring &str2,
   		StringOptions opt = StringOptions::FOLD_CASE_DEFAULT , const bool normalize = false);


   /*! \brief Performs a bit-wise comparison of two strings, after case folding each.
    *
    * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
    *
    * \param str1 one of the strings to compare
    * \param s2 one of the (c-style) strings to compare
    * \param n limits how many bytes are compared. A value of zero compares the
    * entire string.
    * \param opt StringOptions to apply
    * \return The result of bitwise character comparison:
    * < 0 if the characters str1 are bitwise less than the characters in str2,
    * = 0 if str1 contains the same characters as str2,
    * > 0 if the characters in str1 are bitwise greater than the characters in str2.
    */
  static int CompareNoCase(const std::string& str1, const std::string& str2, size_t n = 0,
  		StringOptions opt = StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*! \brief Performs a bit-wise comparison of two substrings, after case folding each.
    *
    * \param str1 one of the strings to compare
    * \param s2 one of the (c-style) strings to compare
    * \param n maximum number of characters to compare
    * \param opt StringOptions to apply
    * \return The result of bitwise character comparison:
    * < 0 if the characters str1 are bitwise less than the characters in str2,
    * = 0 if str1 contains the same characters as str2,
    * > 0 if the characters in str1 are bitwise greater than the characters in str2.
    */
  static int CompareNoCase(const char* s1, const char* s2, size_t n = 0,
  		StringOptions opt = StringOptions::FOLD_CASE_DEFAULT, const bool normalize = false);

  /*! \brief Extracts every digit from string and then returns it's int value
   *
   * This is poorly named and defined. It is highly specialized and probably needs to be fast.
   *
   * It builds a string from every digit in the given string. It then returns the int value
   * of that string. The desired behavior is probably atoi(str.Trim()). Or perhaps it is
   * to skip anything that is not a digit and then convert everything up to the next non-digit
   * into an int.
   *
   * \param str to extract number from
   * \return int value of all concatenated digit in str
   */
  static int ReturnDigits(const std::string &str);

  /*! \brief Get the leftmost side of a string, limited by count graphemes ('characters')
   *
   *  Graphemes (Unicode 'characters') are of variable byte length. This function's
   *  parameters are based on graphemes and NOT bytes.
   *
   * \param str to get a substring of
   * \param count maximum number of characters to get
   * \return leftmost count characters of str, or str if length >= count.
   *
   * TODO: Unicode There are several users which want count to be #bytes (fixed amount of
   * space to write to).
   */
  static std::string Left(const std::string &str, size_t count);

  /*! \brief Get a substring of a string
   *
   * Roughly equivalent to string.substr, but works with Unicode. Note that first and count
   * are related to graphemes (Unicode 'characters') and NOT bytes.
   *
   * \param str to extract substring from
   * \param first character of substring [0 based]
   * \param count maximum number of characters in substring
   * \return substring of str, beginning with grapheme first for min(#num chars in str, count)
   *
   * TODO: Unicode - No apparent users requiring count to be bytes
   */
  static std::string Mid(const std::string &str, size_t first, size_t count = std::string::npos);


  /*! \brief Get the rightmost count graphemes of a string
   *
   * Roughly equivalent to string.substr(string.lenth() - count, count) but works with Unicode. Note that count
   * are related to graphemes (Unicode 'characters') and NOT bytes.
   *
   * \param str to extract substring from
   * \param count maximum rightmost characters to return
   * \return rightmost count characters of str, or str if count >= str.length
   *
   * TODO: Unicode - No apparent users requiring count to be bytes
   */
  static std::string Right(const std::string &str, size_t count);

  /*! \brief Remove all whitespace from beginning and end of str
   *
   * \param str to trim
   * \return trimmed string, same as str argument.
   */
  static std::string& Trim(std::string &str);

  /*! \brief Remove a set of graphemes (characters) from beginning and end of str
   *
   *  Remove any leading or trailing graphemes in chars from str.
   *
   *  Ex: Trim("abc1234bxa", "acb") ==> "1234bx"
   *
   * \param str to trim
   * \param chars (graphemes) to remove from str
   * \return trimmed string, same as str argument.
   *
   * Note: Prior algorithm which will only work if chars is ASCII is probably sufficient
   * for current needs.
   * This implementation allows for chars to be any utf-8 graphemes. (Does NOT normalize).
   */
  static std::string& Trim(std::string &str, const char* const chars);

  /*! \brief Remove all whitespace from beginning of str
   *
   * \param str to trim
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimLeft(std::string &str);

  /*! \brief Remove a set of graphemes (characters) from beginning of str
   *
   *  Remove any leading graphemes in chars from str.
   *
   *  Ex: TrimLeft("abc1234bxa", "acb") ==> "1234bxa"
   *
   * \param str to trim
   * \param chars (graphemes) to remove from str
   * \return trimmed string, same as str argument.
   *
   *
   * Note: Prior algorithm which will only work if chars is ASCII is probably sufficient
   * for current needs.
   * This implementation allows for chars to be any utf-8 graphemes. (Does NOT normalize).
   */
  static std::string& TrimLeft(std::string &str, const char* const chars);

  /*! \brief Remove all whitespace from end of str
   *
   * \param str to trim
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimRight(std::string &str);

  /*! \brief Remove a set of graphemes (characters) from end of str
   *
   *  Remove any trailing graphemes in chars from str.
   *
   *  Ex: trim("abc1234bxa", "acb") ==> "abc1234bx"
   *
   * \param str to trim
   * \param chars (graphemes) to remove from str
   * \return trimmed string, same as str argument.
   *
   * Note: Prior algorithm which will only work if chars is ASCII is probably sufficient
   * for current needs.
   * This implementation allows for chars to be any utf-8 graphemes. (Does NOT normalize).
   */
  static std::string& TrimRight(std::string &str, const char* const chars);

  /*! \brief Remove leading and trailing ASCII space and TAB characters from str
   *
   * \param str to trim
   * \return trimmed string, same as str argument.
   */
  static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str);

  /*! \brief Replaces every occurrence of a char in string.

  Somewhat less efficient than FindAndReplace because this one returns a count
  of the number of changes.

  \param str String to make changes to
  \param oldChar character to be replaced
  \parm newChar character to replace with
  \return Count of the number of changes
  */
  static int Replace(std::string &str, char oldChar, char newChar);

  /*! \brief Replaces every occurrence of a string within another string.

    Somewhat less efficient than FindAndReplace because this one returns a count
    of the number of changes.

    \param str String to make changes to
    \param oldStr string to be replaced
    \parm newStr string to replace with
    \return Count of the number of changes
    */
  static int Replace(std::string &str, const std::string &oldStr, const std::string &newStr);

  /*! \brief Replaces every occurrence of a wstring within another wstring.

     Somewhat less efficient than FindAndReplace because this one returns a count
     of the number of changes.

     \param str String to make changes to
     \param oldStr string to be replaced
     \parm newStr string to replace with
     \return Count of the number of changes
     */
  static int Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr);

  /*! \brief Replaces every occurrence of a string within another string.

     Should be more efficient than Replace since it directly uses an icu library
     routine and does not have to count changes.

     \param str String to make changes to
     \param oldStr string to be replaced
     \parm newStr string to replace with
     \return the modified string (same as str).
     */
  static std::string& FindAndReplace(std::string &str, const std::string oldText,
  		const std::string newText);

  /*! \brief Replaces every occurrence of a string within another string.

       Should be more efficient than Replace since it directly uses an icu library
       routine and does not have to count changes.

       \param str String to make changes to
       \param oldStr string to be replaced
       \parm newStr string to replace with
       \return the modified string (same as str).
       */
  static std::string& FindAndReplace(std::string &str, const char * oldText,
  		const char * newText);

  /*! \brief Replaces every occurrence of a regex pattern with a string in another string.
   *
   * Regex based version of Replace. See:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * \param str string being altered
   * \param pattern regular expression pattern
   * \param newStr new value to replace with
   * \param flags controls behavior of regular expression engine
   * \return result of regular expression
   */
  std::string RegexReplaceAll(std::string &str, const std::string pattern,
  		const std::string newStr, RegexpFlag flags);

  /*! \brief Determines if a string begins with another string
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \return true if str1 starts with str2, otherwise false
   */
  static bool StartsWith(const std::string &str1, const std::string &str2);

  /*! \brief Determines if a string begins with another string
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \return true if str1 starts with s2, otherwise false
   */
  static bool StartsWith(const std::string &str1, const char *s2);

  /*! \brief Determines if a string begins with another string
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of str1
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWith(const char *s1, const char *s2);

  /*! \brief Determines if a string begins with another string, ignoring their case
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

  /*! \brief Determines if a string begins with another string, ignoring their case
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

  /*! \brief Determines if a string begins with another string, ignoring their case
   *
   * Equivalent to StartsWith(FoldCase(s1), FoldCase(s2))
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of s1
   * \param opt controls behavior of case folding
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWithNoCase(const char *s1, const char *s2, StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*! \brief Determines if a string ends with another string
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWith(const std::string &str1, const std::string &str2);

  /*! \brief Determines if a string ends with another string
   *
   * \param str1 string to be searched
   * \param s2 string to find at end of str1
   * \return true if str1 ends with s2, otherwise false
   */
  static bool EndsWith(const std::string &str1, const char *s2);

  /*! \brief Determines if a string ends with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \param opt controls behavior of case folding
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWithNoCase(const std::string &str1, const std::string &str2, StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*! \brief Determines if a string begins with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \param opt controls behavior of case folding
   * \return true if str1 starts with s2, otherwise false
   */
  static bool EndsWithNoCase(const std::string &str1, const char *s2, StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*! \brief Builds a string by appending every string from a container, separated by a delimiter
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

  /*! \brief Splits the given input string into separate strings using the given delimiter.

   If the given input string is empty the result will be an empty array (not
   an array containing an empty string).

   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of split strings
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
   * Scans the str for the occurrence of wordLowerCase. If found, the
   * byte index of the beginning of the occurrence is returned.
   *
   * A "word" is either all letters or all digits. Words must be
   * separated by spaces.
   */
  static size_t FindWords(const std::string &str, const std::string &wordLowerCase);

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
