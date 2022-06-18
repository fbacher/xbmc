#pragma once

//#include <locale>

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <stddef.h>
#include <tuple>
#include <unicode/locid.h>
#include <unicode/platform.h>
#include <unicode/stringpiece.h>
#include <unicode/umachine.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>
#include <climits>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

template<typename E>
constexpr auto to_underlying(E e) noexcept
{
  return static_cast<std::underlying_type_t<E>>(e);
}

/**
 *
 * local version of RegexpFlag from uregex.h. Avoids import and
 */

enum class StringOptions : uint32_t
{

  /**
   * Normalization see: https://unicode.org/reports/tr15/#Norm_Forms
   *
   * Fold character case according to ICU rules:
   * Strip insignificant accents, translate chars to more simple equivalents (German sharp-s),
   * etc.
   *
   * FOLD_CASE_DEFAULT causes FoldCase to behave similar to ToLower for the "en" locale
   * FOLD_CASE_SPECIAL_I causes FoldCase to behave similar to ToLower for the "tr_TR" locale.
   *
   * Case folding also ignores insignificant differences in strings (some accent marks,
   * etc.).
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
   */
  FOLD_CASE_DEFAULT = 0, // U_FOLD_CASE_DEFAULT,
  FOLD_CASE_EXCLUDE_SPECIAL_I = 1, // U_FOLD_CASE_EXCLUDE_SPECIAL_I

  /**
   * Titlecase the string as a whole rather than each word.
   * (Titlecase only the character at index 0, possibly adjusted.)
   * Option bits value for titlecasing APIs that take an options bit set.
   *
   * It is an error to specify multiple titlecasing iterator options together,
   * including both an options bit and an explicit BreakIterator.
   *
   * @see U_TITLECASE_ADJUST_TO_CASED
   * @stable ICU 60
   */
  TITLECASE_WHOLE_STRING = 0x20,
  /**
   * Titlecase sentences rather than words.
   * (Titlecase only the first character of each sentence, possibly adjusted.)
   * Option bits value for titlecasing APIs that take an options bit set.
   *
   * It is an error to specify multiple titlecasing iterator options together,
   * including both an options bit and an explicit BreakIterator.
   *
   * @see U_TITLECASE_ADJUST_TO_CASED
   * @stable ICU 60
   */

  TITLE_CASE_SENTENCES = 0X40, // U_TITLECASE_SENTENCES

  /**
   * Do not lowercase non-initial parts of words when titlecasing.
   * Option bit for titlecasing APIs that take an options bit set.
   *
   * By default, titlecasing will titlecase the character at each
   * (possibly adjusted) BreakIterator index and
   * lowercase all other characters up to the next iterator index.
   * With this option, the other characters will not be modified.
   *
   * @see U_TITLECASE_ADJUST_TO_CASED
   * @see UnicodeString::toTitle
   * @see CaseMap::toTitle
   * @see ucasemap_setOptions
   * @see ucasemap_toTitle
   * @see ucasemap_utf8ToTitle
   * @stable ICU 3.8
   */

  TITLE_CASE_NO_LOWERCASE = 0x100, // U_TITLECASE_NO_LOWERCASE

  /**
   * Do not adjust the titlecasing BreakIterator indexes;
   * titlecase exactly the characters at breaks from the iterator.
   * Option bit for titlecasing APIs that take an options bit set.
   *
   * By default, titlecasing will take each break iterator index,
   * adjust it to the next relevant character (see U_TITLECASE_ADJUST_TO_CASED),
   * and titlecase that one.
   *
   * Other characters are lowercased.
   *
   * It is an error to specify multiple titlecasing adjustment options together.
   *
   * @see U_TITLECASE_ADJUST_TO_CASED
   * @see U_TITLECASE_NO_LOWERCASE
   * @see UnicodeString::toTitle
   * @see CaseMap::toTitle
   * @see ucasemap_setOptions
   * @see ucasemap_toTitle
   * @see ucasemap_utf8ToTitle
   * @stable ICU 3.8
   */

  TITLE_CASE_NO_BREAK_ADJUSTMENT = 0x200, // U_TITLECASE_NO_BREAK_ADJUSTMENT

  /**
   * Adjust each titlecasing BreakIterator index to the next cased character.
   * (See the Unicode Standard, chapter 3, Default Case Conversion, R3 toTitlecase(X).)
   * Option bit for titlecasing APIs that take an options bit set.
   *
   * This used to be the default index adjustment in ICU.
   * Since ICU 60, the default index adjustment is to the next character that is
   * a letter, number, symbol, or private use code point.
   * (Uncased modifier letters are skipped.)
   * The difference in behavior is small for word titlecasing,
   * but the new adjustment is much better for whole-string and sentence titlecasing:
   * It yields "49ers" and "«丰(abc)»" instead of "49Ers" and "«丰(Abc)»".
   *
   * It is an error to specify multiple titlecasing adjustment options together.
   *
   * @see U_TITLECASE_NO_BREAK_ADJUSTMENT
   * @stable ICU 60
   */

  TITLE_CASE_ADJUST_TO_CASED = 0X400, // U_TITLECASE_ADJUST_TO_CASED

  /**
   * Option for string transformation functions to not first reset the Edits object.
   * Used for example in some case-mapping and normalization functions.
   *
   * @see CaseMap
   * @see Edits
   * @see Normalizer2
   * @stable ICU 60
   */

  EDITS_NO_RESET = 0X2000, // U_EDITS_NO_RESET

  /**
   * Omit unchanged text when recording how source substrings
   * relate to changed and unchanged result substrings.
   * Used for example in some case-mapping and normalization functions.
   *
   * @see CaseMap
   * @see Edits
   * @see Normalizer2
   * @stable ICU 60
   */
  OMIT_UNCHANGED_TEXT = 0x4000, // U_OMIT_UNCHANGED_TEXT

  /**
   * Option bit for u_strCaseCompare, u_strcasecmp, unorm_compare, etc:
   * Compare strings in code point order instead of code unit order.
   * @stable ICU 2.2
   */
  COMPARE_CODE_POINT_ORDER = 0x8000, // U_COMPARE_CODE_POINT_ORDER
  /**
   * Option bit for unorm_compare:
   * Perform case-insensitive comparison.
   * @stable ICU 2.2
   */
  COMPARE_IGNORE_CASE = 0x10000, // U_COMPARE_IGNORE_CASE

  /**
   * Option bit for unorm_compare:
   * Both input strings are assumed to fulfill FCD conditions.
   * @stable ICU 2.2
   */
  NORM_INPUT_IS_FCD = 0X20000 // UNORM_INPUT_IS_FCD

// Related definitions elsewhere.
// Options that are not meaningful in the same functions
// can share the same bits.
//
// Public:
// unicode/unorm.h #define UNORM_COMPARE_NORM_OPTIONS_SHIFT 20
//
// Internal: (may change or be removed)
// ucase.h #define _STRCASECMP_OPTIONS_MASK 0xffff
// ucase.h #define _FOLD_CASE_OPTIONS_MASK 7
// ucasemap_imp.h #define U_TITLECASE_ITERATOR_MASK 0xe0
// ucasemap_imp.h #define U_TITLECASE_ADJUSTMENT_MASK 0x600
// ustr_imp.h #define _STRNCMP_STYLE 0x1000
// unormcmp.cpp #define _COMPARE_EQUIV 0x80000

};

inline StringOptions operator|(StringOptions a, StringOptions b)
{
  return static_cast<StringOptions>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/*
 inline StringOptions& operator |=(StringOptions& a, StringOptions& b)
 {
 return static_cast<StringOptions>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
 } */
/**
 * From uregex.h RegexpFlag. It uses older non-scoped enum.
 */
enum class RegexpFlag : uint32_t
{
  /**  Enable case insensitive matching.  @stable ICU 2.4 */
  UREGEX_CASE_INSENSITIVE = 2,

  /**  Allow white space and comments within patterns  @stable ICU 2.4 */
  UREGEX_COMMENTS = 4,

  /**  If set, '.' matches line terminators,  otherwise '.' matching stops at line end.
   *  @stable ICU 2.4 */
  UREGEX_DOTALL = 32,

  /**  If set, treat the entire pattern as a literal string.
   *  Metacharacters or escape sequences in the input sequence will be given
   *  no special meaning.
   *
   *  The flag UREGEX_CASE_INSENSITIVE retains its impact
   *  on matching when used in conjunction with this flag.
   *  The other flags become superfluous.
   *
   * @stable ICU 4.0
   */
  UREGEX_LITERAL = 16,

  /**   Control behavior of "$" and "^"
   *    If set, recognize line terminators within string,
   *    otherwise, match only at start and end of input string.
   *   @stable ICU 2.4 */
  UREGEX_MULTILINE = 8,

  /**   Unix-only line endings.
   *   When this mode is enabled, only \\u000a is recognized as a line ending
   *    in the behavior of ., ^, and $.
   *   @stable ICU 4.0
   */
  UREGEX_UNIX_LINES = 1,

  /**  Unicode word boundaries.
   *     If set, \b uses the Unicode TR 29 definition of word boundaries.
   *     Warning: Unicode word boundaries are quite different from
   *     traditional regular expression word boundaries.  See
   *     http://unicode.org/reports/tr29/#Word_Boundaries
   *     @stable ICU 2.8
   */
  UREGEX_UWORD = 256,

  /**  Error on Unrecognized backslash escapes.
   *     If set, fail with an error on patterns that contain
   *     backslash-escaped ASCII letters without a known special
   *     meaning.  If this flag is not set, these
   *     escaped letters represent themselves.
   *     @stable ICU 4.0
   */
  UREGEX_ERROR_ON_UNKNOWN_ESCAPES = 512

};
inline RegexpFlag operator|(RegexpFlag a, RegexpFlag b)
{
  return static_cast<RegexpFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class NormalizerType : int
{
  NFC = 0, NFD = 1, NFKC = 2, NFKD = 3, NFCKCASEFOLD = 4
};

class Unicode
{
public:

  static std::wstring utf8_to_wstring(const std::string &str);
  static std::string wstring_to_utf8(const std::wstring &str);
  static icu::Locale getDefaultICULocale();
  static icu::Locale getICULocale(const std::locale &locale);

  /*
   * A simple wrapper around icu::Locale
   *
   * Construct a locale from language, country, variant.
   * If an error occurs, then the constructed object will be "bogus"
   * (isBogus() will return true).
   *
   * @param language Lowercase two-letter or three-letter ISO-639 code.
   *  This parameter can instead be an ICU style C locale (e.g. "en_US"),
   *  but the other parameters must not be used.
   *  This parameter can be NULL; if so,
   *  the locale is initialized to match the current default locale.
   *  (This is the same as using the default constructor.)
   *
   * @param country  Uppercase two-letter ISO-3166 code. (optional)
   * @param variant  Uppercase vendor and browser specific code. See class
   *                 description. (optional)
   * @param keywordsAndValues A string consisting of keyword/values pairs, such as
   *                 "collation=phonebook;currency=euro
   *
   * Note: More information can be found in unicode/locid.h (part of ICU library).
   */

  static icu::Locale getICULocale(const char *language, const char *country = 0,
      const char *variant = 0, const char *keywordsAndValues = 0);

  static const std::string getICULocaleId(icu::Locale locale);

  /*!
   *  \brief Folds the case of a string, independent of Locale.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param str string to fold (not modified).
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, 0 (same as FOLD_CASE_DEFAULT)
   * \return UTF-8 folded string
   *
   * Notes: length and number of bytes in string may change during folding/normalization
   *
   * When FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
   * i follow the "en" locale rules for ToLower.
   *
   * DEVELOPERS who use non-ASCII keywords should be aware that it may not
   * always work as expected. Testing is important.
   *
   * Changes will have to be made to keywords that don't work as expected. One solution is
   * to try to always use lower-case in the first place.
   *
   * See StringOptions for the numeric values to use for options as well as descriptions.
   * StringOptions is not used directly so that this API can be by used by Python.
   */

  static const std::string utf8Fold(const std::string &src, const int32_t options);

  /*!
    *  \brief Folds the case of a string, independent of Locale.
    *
    * Similar to ToLower except in addition, insignificant accents are stripped
    * and other transformations are made (such as German sharp-S is converted to ss).
    * The transformation is independent of locale.
    *
    * \param str string to fold in place.
    * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
    *            default value, 0 (same as FOLD_CASE_DEFAULT)
    * \return UTF-8 src.
    *
    * Notes: length and number of bytes in string may change during folding/normalization
    *
    * When FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
    * i follow the "en" locale rules for ToLower.
    *
    * DEVELOPERS who use non-ASCII keywords should be aware that it may not
    * always work as expected. Testing is important.
    *
    * Changes will have to be made to keywords that don't work as expected. One solution is
    * to try to always use lower-case in the first place.
    *
    * See StringOptions for the more details.
    */

  static const std::wstring toFold(const std::wstring &src, const StringOptions options);

  /*!
   *  \brief Folds the case of a wstring, independent of Locale.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param str string to fold in place.
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, 0 (same as FOLD_CASE_DEFAULT)
   * \return UTF-8 src.
   *
   * Notes: length and number of bytes in string may change during folding/normalization
   *
   * When FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
   * i follow the "en" locale rules for ToLower.
   *
   * DEVELOPERS who use non-ASCII keywords should be aware that it may not
   * always work as expected. Testing is important.
   *
   * Changes will have to be made to keywords that don't work as expected. One solution is
   * to try to always use lower-case in the first place.
   *
   * See StringOptions for the more details.
   */

  static const std::string toFold(const std::string &src, const StringOptions options);

  static const std::wstring normalize(const std::wstring &src, const StringOptions option,
      const NormalizerType normalizerType = NormalizerType::NFKC);

  static const std::string normalize(const std::string &src, const StringOptions options,
      const NormalizerType normalizerType = NormalizerType::NFKC);

  static const std::string ToUpper(const std::string &src, const icu::Locale &locale);
  static const std::string toLower(const std::string &src, const icu::Locale &locale);
  static const std::string ToCapitalize(const std::string &src, const icu::Locale &locale);
  static const std::wstring ToCapitalize(const std::wstring &src, const icu::Locale &locale);
  static const std::wstring toTitle(const std::wstring &src, const icu::Locale &locale);
  static const std::string toTitle(const std::string &src, const icu::Locale &locale);

  static int8_t strcmp(const std::wstring &s1, size_t s1_start, size_t s1_length,
      const std::wstring &s2, size_t s2_start, size_t s2_length, const bool normalize = false);

  static int8_t strcmp(const std::string &s1, size_t s1_start, size_t s1_length,
      const std::string &s2, size_t s2_start, size_t s2_length, const bool normalize = false);

  // Go with default Normalization (off). Some free normalization
  // is still performed. Even with it on, you should do some
  // extra normalization up-front to handle certain
  // locales/codepoints. See the documentation for UCOL_NORMALIZATION_MODE
  // for a hint.

  static bool InitializeCollator(std::locale locale, bool normalize = false);

  static bool InitializeCollator(icu::Locale icuLocale, bool normalize = false);

  static int32_t Collate(const std::wstring &left, const std::wstring &right);

  static int w_strcasecmp(const std::wstring &s1, const std::wstring &s2,
      const StringOptions options, const bool normalize = false);

  static int utf8_strcasecmp(const std::string &s1, const std::string &s2,
      const StringOptions options, const bool normalize = false);

  /*!
   * \param n Limits number of bytes to compare. A value of string:npos
   * causes the entire strings to be compared.
   */
  static int utf8_strcasecmp(const std::string &s1, const std::string &s2, size_t n,
      const StringOptions options, const bool normalize = false);

  static int utf8_strcasecmp(const std::string &s1, size_t s1_start, size_t s1_length,
      const std::string &s2, size_t s2_start, size_t s2_length, const StringOptions options,
      const bool normalize = false);

  static bool StartsWith(const std::string &s1, const std::string &s2);
   static bool StartsWithNoCase(const std::string &s1, const std::string &s2,
       const StringOptions options);

  static bool EndsWith(const std::string &s1, const std::string &s2);
  static bool EndsWithNoCase(const std::string &s1, const std::string &s2,
      const StringOptions options);

  /*!
   * \brief Get the leftmost side of a UTF-8 string, limited by character count
   *
   * Unicode characters are of variable byte length. This function's
   * parameters are based on characters and NOT bytes.
   *
   * \param str to get a substring of
   * \param charCount if leftReference: charCount is number of characters to
   *                  copy from left end (limited by str length)
   *                  if ! leftReference: number of characters to omit from right end
   * \param leftReference controls how charCount is interpreted
   * \return leftmost characters of string, length determined by charCount
   *
   */
  static std::string Left(const std::string &str, const size_t charCount, const bool leftReference,
      const icu::Locale icuLocale);

  /*!
   *  \brief Get a substring of a UTF-8 string
   *
   * Unicode characters are of variable byte length. This function's
   * parameters are based on characters and NOT bytes.
   *
   * \param str string to extract substring from
   * \param startCharIndex leftmost character of substring [0 based]
   * \param charCount maximum number of characters to include in substring
   * \return substring of str, beginning with character 'firstCharIndex',
   *         length determined by charCount
   */
  static std::string Mid(const std::string &str, const size_t startCharIndex,
      const size_t charCount = std::string::npos);

  /*!
   * \brief Get the rightmost side of a UTF-8 string, using character boundary
   * rules defined by the given locale.
   *
   * Unicode characters may consist of multiple codepoints. This function's
   * parameters are based on characters and NOT bytes. Due to normalization,
   * the byte-length of the strings may change, although the character counts
   * will not.
   *
   * \param str to get a substring of
   * \param charCount if rightReference: charCount is number of characters to
   *                  copy from right end (limited by str length)
   *                  if ! rightReference: number of characters to omit from left end
   * \param rightReference controls how charCount is interpreted
   * \param icuLocale determines how character breaks are made
   * \return rightmost characters of string, length determined by charCount
   *
   * Ex: Copy all but the leftmost two characters from str:
   *
   * std::string x = Right(str, 2, false, Unicode::getDefaultICULocale());
   */
  static std::string Right(const std::string &str, const size_t charCount, bool rightReference,
      const icu::Locale &icuLocale);

  static size_t getCodeUnitIndex(const std::string &str, size_t startOffset, size_t charCount,
      const bool forward, const bool left, icu::Locale icuLocale = nullptr);
  /*!
   * \brief return a substring of a string
   *
   * \param str string to extract substring from
   * \param firstCharIndex character of substring [0 based]
   *                  if > 0 then index measured from beginning of string
   *                  if < 0 then index measured from end of string
   * \param charCount if > 0 maximum number of characters to keep left end
   *                  if < 0 number of characters to remove from right end
   * \return substring of str, beginning with character 'first', length determined by charCount
   * /
   static const std::string substr(const std::string &str, int32_t startCharIndex,
   int32_t numChars);
   */
  static std::string Trim(const std::string &str);
  static std::string TrimLeft(const std::string &str);
  static std::string TrimLeft(const std::string &str, const std::string deleteChars);

  static std::string TrimRight(const std::string &str);
  static std::string TrimRight(const std::string &str, const std::string deleteChars);

  static std::string Trim(const std::string &str, const std::string &deleteChars, const bool trimStart,
      const bool trimEnd);

  static std::vector<std::string> SplitMulti(const std::vector<std::string> &input,
      const std::vector<std::string> &delimiters, const size_t iMaxStrings, const int flags =
          to_underlying(RegexpFlag::UREGEX_LITERAL));

  static std::tuple<std::string, int> FindCountAndReplace(const std::string &src, const std::string &oldText,
      const std::string &newText);

  /**
   * Returns true of the given word is find in str, using a caseless search.
   */
  static size_t FindWord(const std::string &str, const std::string &word);
  /*
   * Replaces every occurrence of oldText with newText within the string.
   * Does not return account. Should be more efficient than
   * FindCountAndReplace.
   *
   */

  static std::string& findAndReplace(std::string &str, const std::string oldText,
      const std::string newText);

  /**
   * Not required at this time since 'brackets' are confined to ASCII characters.
   *
   * Finds the position of the open bracket in a substring.
   *
   * The startPos is already advanced after the open bracket which we
   * are looking for the close bracket for.
   *
   * str string to scan
   * opener the open-bracket character to look for
   * closer the close-bracket character to look for
   * startPos offset into string, after the first opener  to begin searching.
   *
   * return If the brackets are paired, then the position of the last
   *        close-bracket will be returned.
   *        Otherwise, string.np is returned.
   *
   static size_t FindEndBracket(const std::string& str,
   char opener,
   char closer,
   size_t startPos = 0);
   */
  /*
   * Regular expression patterns for this lib can be found at:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * flags:  See enum RegexpFlag in uregex.h
   *
   */

  static size_t regexFind(const std::string &str, const std::string pattern, const int flags);

  /*
   * Regular expression patterns for this lib can be found at:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * flags:  See enum RegexpFlag in uregex.h
   *
   */
  static std::string RegexReplaceAll(const std::string &str, const std::string pattern,
      const std::string replace, const int flags);

  static int32_t countOccurances(const std::string &strInput, const std::string &strFind,
      const int flags);

  /*! \brief Splits the given input string using the given delimiter into separate strings.

   If the given input string is empty nothing will be put into the target iterator.

   \param d_first the beginning of the destination range
   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of strings to split. 0 means infinite
   \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   */

  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const std::string &input, const char delimiter,
      size_t iMaxStrings)
  {
    return SplitTo(d_first, input, std::string(1, delimiter), iMaxStrings);
  }
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const std::string &input,
      const std::vector<std::string> &delimiters)
  {

    OutputIt dest = d_first;

    if (input.empty())
      return dest;

    if (delimiters.empty())
    {
      *d_first++ = input;
      return dest;
    }
    icu::UnicodeString uInput = toUnicodeString(input);
    icu::UnicodeString uDelimiter = toUnicodeString(delimiters[0]);

    // First, transform every occurrence of delimiters in the string with the first delimiter
    /// Then, split using the first delimiter.

    for (size_t di = 1; di < delimiters.size(); di++)
    {
      icu::UnicodeString uNextDelimiter = toUnicodeString(delimiters[di]);
      uInput.findAndReplace(uNextDelimiter, uDelimiter);
    }

    if (uInput.isEmpty())
    {
      *d_first++ = input;
      return dest;
    }

    std::vector<icu::UnicodeString> uDest = std::vector<icu::UnicodeString>();
    Unicode::SplitTo(std::back_inserter(uDest), uInput, uDelimiter);

    for (size_t i = 0; i < uDest.size(); i++)
    {
      std::string tempStr = std::string();
      tempStr = uDest[i].toUTF8String(tempStr);
      *d_first++ = tempStr;
    }
    return dest;
  }

  /*! \brief Splits the given input string using the given delimiter into separate strings.

   If the given input string is empty nothing will be put into the target iterator.

   \param d_first the beginning of the destination range
   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of splitted strings. 0 means infinite
   \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const std::string &input, const std::string &delimiter,
      unsigned int iMaxStrings = 0, int flags = to_underlying(RegexpFlag::UREGEX_LITERAL))
  {
    // This is regex based. A simpler, faster one can be derived from unistr.h using u_strFindFirst and friends

    OutputIt dest = d_first;

    if (input.empty())
      return dest;
    if (delimiter.empty())
    {
      *d_first++ = input;
      return dest;
    }

    icu::UnicodeString uInput = toUnicodeString(input);
    icu::UnicodeString uDelimiter = toUnicodeString(delimiter);
    std::vector<icu::UnicodeString> uResult = std::vector<icu::UnicodeString>();

    Unicode::SplitTo(std::back_inserter(uResult), uInput, uDelimiter, iMaxStrings, flags);

    // Convert back to utf-8
    // std::cout << "Unicode.SplitTo input: " << input << " delim: " << delimiter
    //				<< " Max split: " << iMaxStrings << std::endl;
    for (size_t i = 0; i < uResult.size(); i++)
    {
      std::string tempStr = std::string();
      tempStr = uResult[i].toUTF8String(tempStr);
      // std::cout << "splitTo idx: " << i << " value: " << tempStr << std::endl;
      *dest++ = tempStr;
    }
    return dest;
  }

  /**
   * Not required at this time since delimiters are confined to ASCII characters
   *
   static void Tokenize(const std::string &input,
   std::vector<std::string> &tokens, const std::string &delimiters);
   */

  static bool Contains(const std::string &str, const std::vector<std::string> &keywords);

  int64_t AlphaNumericCompare_new(const std::wstring &left, const std::wstring &right,
      icu::Locale locale);

private:
  static bool doneOnce;

  static bool isLatinChar(UChar32 codepoint);

  /**
   * Calculates a 'reasonable' buffer size to give some room for growth in a utf-8
   * string to accommodate some basic transformations (folding, normalization, etc.).
   * Not guaranteed to be sufficient for all needs.
   *
   * param utf8_length byte-length of UTF-8 string to be converted
   * param scale multiplier to apply to get larger buffer
   *
   * Note that the returned size has a pad of 200 bytes added to leave room
   * for growth.
   *
   */

  static size_t getBasicUTF8BufferSize(size_t utf8_length, float scale);

  /*!
   * \brief Calculates the maximum number of UChars (UTF-16) required by a wchar_t
   * string.
   *
   * \param wchar_length Char32 codepoints to be converted to UTF-16.
   * \param scale multiplier to apply to get larger buffer
   * \return A size a bit larger than wchar_length, plus 200.
   */

  static size_t getWcharToUCharBufferSize(size_t wchar_length, size_t scale);

  /**
   * Calculates the maximum number of UChars (UTF-16) required by a UTF-8
   * string.
   *
   * param utf8_length byte-length of UTF-8 string to be converted
   * param scale multiplier to apply to get larger buffer
   *
   * Note that the returned size has a pad of 200 UChars added to leave room
   * for growth.
   *
   * Note that a UTF-16 string will be at most as long as the UTF-8 string.
   */

  static size_t getUCharBufferSize(size_t utf8_length, float scale);

  /**
   * Calculates a reasonably sized UChar buffer based upon the size of existing
   * UChar string.
   *
   * param uchar_length byte-length of UTF-8 string to be converted
   * param scale multiplier to apply to get larger buffer
   *
   * Note that the returned size has a pad of 200 UChars added to leave room
   * for growth.
   *
   * Note that a UTF-16 string will be at most as long as the UTF-8 string.
   */

  static size_t getUCharWorkingSize(size_t uchar_length, size_t scale);

  /**
   * Calculates the maximum number of UTF-8 bytes required by a UTF-16 string.
   *
   * param uchar_length Number of UTF-16 code units to convert
   * param scale multiplier to apply to get a larger buffer
   *
   * Note that 200 bytes is added to the result to give some room for growth.
   *
   * Note that in addition to any scale factor, the number of UTF-8 bytes
   * returned is 3 times the number of UTF-16 code unites. This leaves enough
   * room for the 'worst case' UTF-8  expansion required for Indic, Thai and CJK.
   */

  static size_t getUTF8BufferSize(size_t uchar_length, size_t scale);

  /**
   * Calculates the maximum number of wchar_t code units required to
   * represent UTF-16 code units.
   *
   * On most systems a wchar_t represents a 32-bit Unicode codepoint. For such machines
   * a wchar_t string will require at most the same number of 32-bit codepoints as the
   * 16-bit has code-units (i.e. twice as many bytes).
   *
   * On the systems with a wchar_t representing a 16-bit UTF-16 code unit, then the
   * number of code units (and bytes) required to represent a UTF-16 UChar in wchar_t
   * will bhe the same as the original string.
   *
   * param uchar_length number of UTF-16 code units in original string
   * param scale scale factor to multiply uchar_length by to allow for growth
   * returns Number of wchar_t to allocate for the desired buffer.
   *
   * Note that an additional 200 code units is added to the result to allow for growth
   *
   */
  static size_t getWCharBufferSize(size_t uchar_length, size_t scale);

  static UChar* StringToUChar(const std::string &src, UChar *buffer, size_t bufferSize,
      int32_t &destLength, const size_t src_offset = 0,
      const size_t src_length = std::string::npos);

  static UChar* StringToUChar(const char *src, UChar *buffer, size_t bufferSize,
      int32_t &destLength, const size_t length = std::string::npos);

  static UChar* wchar_to_UChar(const wchar_t *src, UChar *buffer, size_t bufferSize,
      int32_t &destLength, const size_t length = std::string::npos);

  static std::string UCharToString(const UChar *u_str, char *buffer, size_t bufferSize,
      int32_t &destLength, const size_t u_str_length);

  static wchar_t* UChar_to_wchar(const UChar *u_str, wchar_t *buffer, size_t bufferSize,
      int32_t &destLength, const size_t length = std::string::npos);

  static icu::UnicodeString toUnicodeString(const std::wstring &wStr)
  {
#if U_SIZEOF_WCHAR_T==2
			return icu::UnicodeString(wStr.data(), wStr.length());
	#else
    return icu::UnicodeString::fromUTF32((int32_t*) wStr.data(), wStr.length());
#endif
  }

  static icu::UnicodeString toUnicodeString(const std::string &src)
  {
    return icu::UnicodeString::fromUTF8(src);
  }

  static icu::UnicodeString toUnicodeString(const icu::StringPiece &src)
  {
    return icu::UnicodeString::fromUTF8(src);
  }

  static void ToUpper(UChar *p_u_src_buffer, int32_t u_src_length, icu::Locale locale,
      UChar *p_u_toupper_buffer, const int32_t u_toupper_buffer_size, int32_t &to_upper_length,
      UErrorCode &status);

  static void toFold(const icu::StringPiece strPiece, icu::CheckedArrayByteSink &sink,
      UErrorCode &status, const int32_t options);

  static void normalize(const icu::StringPiece strPiece, icu::CheckedArrayByteSink &sink,
      UErrorCode &status, const int32_t options, const NormalizerType normalizerType);

  static icu::UnicodeString Trim(const icu::UnicodeString &str, const icu::UnicodeString &deleteChars,
      const bool trimStart, const bool trimEnd);

  static icu::UnicodeString Trim(const icu::UnicodeString &str, const bool trimStart,
      const bool trimEnd);

  static std::tuple<icu::UnicodeString, int> FindCountAndReplace(const icu::UnicodeString &kSrc,
      const icu::UnicodeString &kOldText, const icu::UnicodeString &kNewText);

  static std::tuple<icu::UnicodeString, int>
  FindCountAndReplace(const icu::UnicodeString &srcText, const int32_t start, const int32_t length,
      const icu::UnicodeString &oldText, const int32_t oldStart, const int32_t oldLength,
      const icu::UnicodeString &newText, const int32_t newStart, const int32_t newLength);

  /*
   * Regular expression patterns for this lib can be found at:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * flags:  See enum RegexpFlag in uregex.h
   *
   */
  icu::UnicodeString RegexReplaceAll(const icu::UnicodeString &kString,
      const icu::UnicodeString kPattern, const icu::UnicodeString kReplace, const int flags);

  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const icu::UnicodeString &kInput,
      const icu::UnicodeString &kDelimiter, unsigned int iMaxStrings = 0, const int flags =
          to_underlying(RegexpFlag::UREGEX_LITERAL));

  static std::vector<icu::UnicodeString> SplitMulti(const std::vector<icu::UnicodeString> &input,
      const std::vector<icu::UnicodeString> &delimiters, size_t iMaxStrings = 0, const int flags =
          to_underlying(RegexpFlag::UREGEX_LITERAL));

};

