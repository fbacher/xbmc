#include "Unicode.h"

#include <chrono>
#include <bits/stdint-intn.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <tuple>
#include <unicode/brkiter.h>
#include <unicode/bytestream.h>
#include <unicode/casemap.h>
#include <unicode/coll.h>
#include <unicode/normalizer2.h>
#include <unicode/platform.h>
#include <unicode/regex.h>
#include <unicode/schriter.h>
#include <unicode/stringoptions.h>
#include <unicode/stringpiece.h>
#include <unicode/ucol.h>
#include <unicode/unistr.h>
#include <unicode/umachine.h>
#include <unicode/urename.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>
#include <algorithm>
#include <cmath>
#include <codecvt>
#include <iostream>
#include <iterator>
#include <locale>

#include "../commons/ilog.h"
#include "../LangInfo.h"
#include "log.h"
#include "Locale.h"

#define DISABLE_OPTIMIZATION 1 // Used for testing. Code should behave the same way

// convert UTF-8 string to wstring
std::wstring Unicode::UTF8ToWString(const std::string &str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string Unicode::WStringToUTF8(const std::wstring &str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.to_bytes(str);
}

const inline std::string toString(icu::UnicodeString str)
{
  std::string tempStr = std::string();
  tempStr = str.toUTF8String(tempStr);
  return tempStr;
}

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

size_t Unicode::GetBasicUTF8BufferSize(size_t utf8_length, float scale)
{

  float scale_factor = std::fmin((float) scale, 1.5);
  return (size_t) (200 + int(utf8_length * scale_factor));
}

/*!
 * \brief Calculates the maximum number of UChars (UTF-16) required by a wchar_t
 * string.
 *
 * \param wchar_length Char32 codepoints to be converted to UTF-16.
 * \param scale multiplier to apply to get larger buffer
 * \return A size a bit larger than wchar_length, plus 200.
 */

size_t Unicode::GetWcharToUCharBufferSize(size_t wchar_length, size_t scale)
{

  // Most UTF-32 strings will be BMP-only and result in a same-length
  // UTF-16 string. We overestimate the capacity just slightly,
  // just in case there are a few supplementary characters.
  // Add a bit more for some growth.
  int scale_factor = std::min((int) scale, 1);
  return (size_t) (200 + (wchar_length + (wchar_length >> 4) + 4) * scale_factor);
}

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

size_t Unicode::GetUCharBufferSize(size_t utf8_length, float scale)
{
  // utf8 string will comfortably fit in a UChar string
  // of same char length. Add a bit more for some growth.

  int scale_factor = std::fmin((int) scale, 1.0);
  return (size_t) (200 + (int) (utf8_length * scale_factor));
}

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

size_t Unicode::GetUCharWorkingSize(size_t uchar_length, size_t scale)
{
  //  Add a bit more for some growth.
  int scale_factor = std::min((int) scale, 2);
  return (size_t) (200 + uchar_length * scale_factor);
}

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

size_t Unicode::GetUTF8BufferSize(size_t uchar_length, size_t scale)
{
  // When converting from UTF-16 to UTF-8, the result will have at most 3 times
  // as many bytes as the source has UChars.
  // The "worst cases" are writing systems like Indic, Thai and CJK with
  // 3:1 bytes:UChars.
  int scale_factor = std::min((int) scale, 1);
  return (size_t) (200 + uchar_length * 3 * scale_factor);
}

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
size_t Unicode::GetWCharBufferSize(size_t uchar_length, size_t scale)
{
  size_t scale_factor = std::min((int) scale, 1);
#if U_SIZEOF_WCHAR_T==4

  // When converting from UTF-16 to UTF-32, the result will have at most 2 times
  // as many bytes as the source has UChars.

  return (size_t) (200 + uchar_length * 2 * scale_factor);
#else
	// When converting from UTF-16 to UTF-16, the size is the same
	return (size_t) (200 + uchar_length * scale_factor);
#endif
}

/**
 * Convert the given UTF-8 encoded string into a UTF-16 encoded string (the native
 * ICU encoding).
 *
 * Using a local (stack-based) char[xxx] buffer instead of a malloc allocated buffer
 * reduces the resources required to create/delete the buffer.
 *
 * src utf8 encoded string to convert to UChar
 *
 * buffer buffer to write converted UChar string into
 * bufferSize size of buffer in UChars
 * destLength number of UChars actually needed to to convert
 * src_offset Offset within src
 * src_length Convert the first length bytes of src. Use std::string::npos for entire string.
 *
 * returns UChar (UTF-16) encoding of src.
 *
 * NOTE: bad utf-8 characters (such as when not all bytes of multi-byte character are
 * included due to incorrect src_length or offset) will result in substitute Unicode
 * code-points (0xFFFD) (in UTF-16 representation) being inserted into the converted string.
 *
 * The maximum number of bytes required for the converted string will be the
 * number of bytes required by the input string (src). Actual length depends upon
 * the content.
 *
 */
UChar* Unicode::StringToUChar(const std::string &src, UChar *buffer, size_t bufferSize,
    int32_t &destLength, const size_t src_offset /* = 0 */,
    const size_t src_length /* = std::string::npos */)
{
  size_t length = std::min(src.length(), src_length);
  return Unicode::StringToUChar(src.data() + src_offset, buffer, bufferSize, destLength, length);
}

UChar* Unicode::StringToUChar(const char *src, UChar *buffer, size_t bufferSize,
    int32_t &destLength, const size_t length /* = std::string::npos */)
{
  int32_t srcLength;

  // Number of corrupt UTF8 codepoints in string. Can be caused by truncating beginning
  // or end of string in mid multi-byte character

  int32_t numberOfSubstitutions = 0;

  // Substitute codepoint to use when a corrupt UTF8 codepoint is encountered.

  const UChar32 subchar = (UChar32) 0xFFFD;

  if (length == std::string::npos)
    srcLength = -1;
  else
    srcLength = length;

  UErrorCode status = U_ZERO_ERROR;
  u_strFromUTF8WithSub(buffer, bufferSize, &destLength, src, srcLength, subchar,
      &numberOfSubstitutions, &status);

  if (numberOfSubstitutions > 0)
  {
    size_t utf8BufferSize = Unicode::GetUTF8BufferSize(bufferSize, 2);
    char utf8Buffer[utf8BufferSize];
    int32_t utf8ActualLength = 0;
    const UChar* ucharBuffer = buffer;
    const size_t u_str_length = (size_t) destLength;
    std::string newStr = Unicode::UCharToString(ucharBuffer, utf8Buffer, utf8BufferSize,
        utf8ActualLength, u_str_length);
    CLog::Log(LOGDEBUG, "{} codepoint substitutions Unicode::StringToUChar utf8: {} UChar: {}\n",
        numberOfSubstitutions, src, newStr);
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in StringToUChar: {}\n", status);
    buffer[0] = '\0';
  }
  return buffer;
}

std::string Unicode::UCharToString(const UChar *u_str, char *utf8Buffer, size_t utf8BufferSize,
    int32_t &utf8ActualLength, const size_t u_str_length)
{
  UErrorCode status = U_ZERO_ERROR;

  // Number of corrupt UTF8 codepoints in string. Can be caused by truncating beginning
  // or end of string in mid multi-byte character. May have occurred on utf8 to UChar
  // conversion.

  int32_t numberOfSubstitutions = 0;

  // Substitute codepoint to use when a corrupt UTF8 codepoint is encountered.

  const UChar32 subchar = (UChar32) 0xFFFD;
  // UChar string can expand to up to 3x UTF8 chars

  int32_t u_str_length_param;
  if (u_str_length == std::string::npos)
    u_str_length_param = -1;
  else
    u_str_length_param = u_str_length;

  status = U_ZERO_ERROR;
  u_strToUTF8WithSub(utf8Buffer, utf8BufferSize, &utf8ActualLength, u_str, u_str_length_param,
      subchar, &numberOfSubstitutions, &status);

  if (numberOfSubstitutions > 0)
  {
    CLog::Log(LOGDEBUG, "{} codepoint substitutions made in Unicode::UCharToString",
        numberOfSubstitutions);
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::UCharToString: {}\n", status);

    return nullptr;
  }
  std::string result = std::string(utf8Buffer);
  return result;
}

UChar* Unicode::WcharToUChar(const wchar_t *src, UChar *buffer, size_t bufferSize,
    int32_t &destLength, const size_t length /* = std::string::npos */)
{
  int32_t srcLength;

  // Number of corrupt UTF8 codepoints in string. Can be caused by truncating beginning
  // or end of string in mid multi-byte character

  int32_t numberOfSubstitutions = 0;

  // Substitute codepoint to use when a corrupt codepoint is encountered.

  const UChar32 subchar = (UChar32) 0xFFFD;
  if (length == std::string::npos)
    srcLength = -1;
  else
    srcLength = length;

  UErrorCode status = U_ZERO_ERROR;

#if U_SIZEOF_WCHAR_T==4
  u_strFromUTF32WithSub(buffer, bufferSize, &destLength, (int32_t*) src, srcLength, subchar,
      &numberOfSubstitutions, &status);
#else
	// Should be a way to do substitutions on bad 'codepoints' as above.
	u_strFromWCS(buffer, bufferSize, &destLength, src, srcLength, /* subchar,
			&numberOfSubstitutions, */ &status);
#endif

  if (numberOfSubstitutions > 0)
  {
    CLog::Log(LOGDEBUG, "{} codepoint substitutions made in Unicode::WcharToUChar",
        numberOfSubstitutions);
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in WcharToUChar: {}", status);
    return nullptr;
  }
  return buffer;
}

wchar_t* Unicode::UCharToWChar(const UChar *u_str, wchar_t *buffer, size_t bufferSize,
    int32_t &destLength, const size_t u_str_length)
{
  UErrorCode status = U_ZERO_ERROR;

  // Number of corrupt UTF8 codepoints in string. Can be caused by truncating beginning
  // or end of string in mid multi-byte character. May have occurred on utf8 to UChar
  // conversion.

  int32_t numberOfSubstitutions = 0;

  // Substitute codepoint to use when a corrupt UTF8 codepoint is encountered.

  // const UChar32 subchar = (UChar32) 0xFFFD;
  // UChar string can expand to up to 3x UTF8 chars

  int32_t u_str_length_param;
  if (u_str_length == std::string::npos)
    u_str_length_param = -1;
  else
    u_str_length_param = u_str_length;

  status = U_ZERO_ERROR;
  u_strToWCS(buffer, bufferSize, &destLength, u_str, u_str_length_param, /* subchar,
   &numberOfSubstitutions, */&status);

  if (numberOfSubstitutions > 0)
  {
    CLog::Log(LOGDEBUG, "{} codepoint substitutions made in Unicode::UCharToWChar",
        numberOfSubstitutions);
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::UCharToWChar: {}", status);
    return nullptr;
  }
  return buffer;
}

/*
 * Determines if UChar32 codepoint is a 'latin' character.
 *
 * Used to determine how letters are processed. (Some languages,
 * like Chinese, have words which are also letters. Code which
 * expects words to be composed of letters with spaces as separators
 * don't work so well with such languages.)
 *
 * A better solution would be to use Word BreakIterator, but
 * that would not fit well with current implementation.
 */
bool Unicode::IsLatinChar(UChar32 codepoint)
{

  // Basic Latin, 0000–007F. This block corresponds to ASCII.
  // Latin-1 Supplement, 0080–00FF
  if (codepoint < 0x00ff)
    return true;

  // 	Latin Extended-A, 0100–017F
  if (codepoint >= 0x100 and codepoint <= 0x17f)
    return true;
  // 	Latin Extended-B, 0180–024F
  if (codepoint >= 0x180 and codepoint <= 0x24f)
    return true;

  return false;
}

icu::Locale Unicode::GetICULocale(const std::locale &locale)
{

  UErrorCode status = U_ZERO_ERROR;
  icu::Locale icu_locale = Unicode::GetICULocale(locale.name().data());
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::GetICULocale: {}", status);
  }
  return icu_locale;
}

const std::string Unicode::GetICULocaleId(icu::Locale locale)
{
  std::string localeId = std::string();
  if (locale != nullptr)
  {
    if (locale.getLanguage() != nullptr)
    {
      localeId.append(locale.getLanguage());
      if (locale.getCountry() != nullptr)
      {
        localeId.append("_");
        localeId.append(locale.getCountry());
      }
    }
  }
  return localeId;
}

icu::Locale Unicode::GetDefaultICULocale()
{
  std::string language = g_langInfo.GetLocale().GetLanguageCode();
  if (language.length() < 2)
  {
    language = "en";
  }
  std::string country = g_langInfo.GetLocale().GetTerritoryCode();
  if (country.length() < 2)
  {
    country = "";
  }
  icu::Locale icu_locale = Unicode::GetICULocale(language.data(), country.data());
  return icu_locale;
}

icu::Locale Unicode::GetICULocale(const char *language, const char *country, const char *variant,
    const char *keywordsAndValues)
{

  UErrorCode status = U_ZERO_ERROR;

  icu::Locale icu_locale = icu::Locale(language, country, variant, keywordsAndValues = 0);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::GetICULocale: {}", status);
  }
  return icu_locale;
}

const std::string Unicode::ToUpper(const std::string &src, const icu::Locale &locale)
{
  // TODO: Change to icu::CaseMap::utf8ToUpper or ucasemap_utf8ToUpper

  std::string localeId = "none";
  icu::Locale currentLocale = locale;
  if (currentLocale == nullptr)
    currentLocale = Unicode::GetDefaultICULocale();

  if (currentLocale != nullptr)
  {
    localeId = Unicode::GetICULocaleId(currentLocale);
  }
  // CLog::Log(LOGINFO, "ToUpper locale: %s", localeId);

  if (src.length() == 0)
    return std::string(src);

  UErrorCode status = U_ZERO_ERROR;
  // Create buffer on stack
  int32_t u_src_size = Unicode::GetUCharBufferSize(src.length(), 1.5);
  UChar u_src[u_src_size];
  int32_t u_src_actual_length = 0;
  Unicode::StringToUChar(src.data(), u_src, u_src_size, u_src_actual_length);

  int32_t u_toupper_size = Unicode::GetUCharWorkingSize(u_src_actual_length, 2);
  UChar u_toupper_buffer[u_toupper_size];
  UChar* p_u_src = u_src;
  UChar* p_u_toupper_buffer = u_toupper_buffer;
  int32_t toupper_length = 0;

  Unicode::ToUpper(p_u_src, u_src_actual_length, currentLocale, p_u_toupper_buffer, u_toupper_size,
      toupper_length, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ToUpper: {}", status);

    return std::string();
  }

  size_t utf8_buffer_size = Unicode::GetUTF8BufferSize(toupper_length, 2);
  char utf8_buffer[utf8_buffer_size];
  int32_t utf8_actual_length = 0;
  std::string result = Unicode::UCharToString(p_u_toupper_buffer, utf8_buffer, utf8_buffer_size,
      utf8_actual_length, toupper_length);
  return result;
}

/**
 * Uppercase the characters in a string.
 * Casing is locale-dependent and context-sensitive.
 * The result may be longer or shorter than the original.
 * The source string and the destination buffer are allowed to overlap.
 *
 * @param dest      A buffer for the result string. The result will be zero-terminated if
 *                  the buffer is large enough.
 * @param destCapacity The size of the buffer (number of UChars). If it is 0, then
 *                  dest may be NULL and the function will only return the length of the result
 *                  without writing any of the result string.
 * @param src       The original string
 * @param srcLength The length of the original string. If -1, then src must be zero-terminated.
 * @param locale    The locale to consider, or "" for the root locale or NULL for the default locale.
 * @param pErrorCode Must be a valid pointer to an error code value,
 *                  which must not indicate a failure before the function call.
 * @return The length of the result string. It may be greater than destCapacity. In that case,
 *         only some of the result was written to the destination buffer.
 * @stable ICU 2.0
 *
 U_CAPI int32_t U_EXPORT2
 u_strToUpper(UChar *dest, int32_t destCapacity,
 const UChar *src, int32_t srcLength,
 const char *locale,
 UErrorCode *pErrorCode) {

 } */

void Unicode::ToUpper(UChar *p_u_src_buffer, int32_t u_src_length, const icu::Locale locale,
    UChar *p_u_toupper_buffer, const int32_t u_toupper_buffer_size, int32_t &to_upper_length,
    UErrorCode &status)
{
  if (u_src_length == 0)
  {
    to_upper_length = 0;
    return;
  }

  status = U_ZERO_ERROR;
  std::string localeId = Unicode::GetICULocaleId(locale);
  // CLog::Log(LOGINFO, "ToUpper locale: %s", localeId);
  to_upper_length = u_strToUpper(p_u_toupper_buffer, u_toupper_buffer_size, p_u_src_buffer,
      u_src_length, localeId.data(), &status);

  if (U_FAILURE(status))
  {
    // Caller supposed to examine status

    CLog::Log(LOGERROR, "Error in Unicode::ToUpper: {}", status);
  }
}

const std::string Unicode::ToLower(const std::string &src, const icu::Locale &locale)
{
  // TODO: Replace with icu::CaseMap::utf8ToLower or ucasemap_utf8ToLower

  std::string localeId = "none";
  if (locale != nullptr)
  {
    localeId = Unicode::GetICULocaleId(locale);
  }
  // CLog::Log(LOGINFO, "ToLower locale: %s", localeId);
  icu::UnicodeString uString = Unicode::ToUnicodeString(src);
  uString.toLower(locale);
  std::string result = std::string(); // Empty string
  result = uString.toUTF8String(result);
  return result;
}

const std::wstring Unicode::ToCapitalize(const std::wstring &src, const icu::Locale &locale)
{
  std::string localeId = "none";
  if (locale != nullptr)
  {
    localeId = Unicode::GetICULocaleId(locale);
  }
  icu::UnicodeString uString = Unicode::ToUnicodeString(src);
  icu::StringCharacterIterator iter = icu::StringCharacterIterator(uString);

  // TODO:  changing a character's case CAN change the number of codepoints
  // in the character. This impacts Char32 (codepoint) representation here
  // too. Fixing this looks tricky, since "full case folding" for a single
  // letter does not appear to be in exposed API.

  icu::UnicodeString titleCase = icu::UnicodeString();

  bool isFirstLetter = true;
  while (iter.hasNext())
  {
    UChar32 cp = iter.next32PostInc();

    if (u_isspace(cp) || (u_ispunct(cp) && cp != '\''))
    {
      isFirstLetter = true;
    }
    else if (isFirstLetter)
    {
      cp = u_toupper(cp); // TODO: Insufficient
      isFirstLetter = false;
    }
    titleCase += cp;
  }

  size_t wchar_buffer_size = Unicode::GetWCharBufferSize(titleCase.length(), 1);
  wchar_t wchar_buffer[wchar_buffer_size];
  int32_t destLength = 0;
  Unicode::UCharToWChar(titleCase.getBuffer(), wchar_buffer, wchar_buffer_size, destLength,
      (size_t) titleCase.length());

  std::wstring wresult = std::wstring(wchar_buffer, (size_t) destLength);
  return wresult;
}

const std::string Unicode::ToCapitalize(const std::string &src, const icu::Locale &locale)
{

  icu::UnicodeString uString = Unicode::ToUnicodeString(src);
  icu::StringCharacterIterator iter = icu::StringCharacterIterator(uString);

  // TODO:  changing a character's case CAN change the number of codepoints
  // in the character. This impacts Char32 (codepoint) representation here
  // too.
  icu::UnicodeString titleCase = icu::UnicodeString();

  bool isFirstLetter = true;
  while (iter.hasNext())
  {
    UChar32 cp = iter.next32PostInc();

    if (u_isspace(cp) || (u_ispunct(cp) && cp != '\''))
    {
      isFirstLetter = true;
    }
    else if (isFirstLetter)
    {
      cp = u_toupper(cp); // TODO: Insufficient
      isFirstLetter = false;
    }
    titleCase += cp;
  }

  std::string utf8Result = std::string();
  utf8Result = titleCase.toUTF8String(utf8Result);
  return utf8Result;
}

/*
 * Implementation using word BreakIterator. May be worthy of investigation.
 * - More expensive
 * - Behavior needs tweaking via defining rule tables
 * - Possibly more accurate for locales
 * - Not sure how accuracy and movie/song titles play well with movies with
 *   "foreign" titles. Conventions might not be normal for that language.
 *
 */

const std::wstring Unicode::ToTitle(const std::wstring &src, const icu::Locale &locale)
{
  UErrorCode status = U_ZERO_ERROR;

  icu::BreakIterator* wordIterator = icu::BreakIterator::createWordInstance(locale, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ToTitle: {}", status);
    return std::wstring(src);
  }

  icu::UnicodeString kUString = Unicode::ToUnicodeString(src);
  icu::UnicodeString result = kUString.toTitle(wordIterator, locale);

  // TODO: Yes, idiotic to go from UnicodeString to utf8 to wstring. I'll worry about it
  // later

  std::string utf8Result = std::string();
  utf8Result = result.toUTF8String(utf8Result);
  std::wstring wResult = Unicode::UTF8ToWString(utf8Result);

  delete wordIterator;
  return wResult;
}

const std::string Unicode::ToTitle(const std::string &src, const icu::Locale &locale)
{
  // TODO: change to use icu::CaseMap::utf8toTitle or ucasemap_utf8ToTitle

  UErrorCode status = U_ZERO_ERROR;

  icu::BreakIterator* wordIterator = icu::BreakIterator::createWordInstance(locale, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ToTitle: {}", status);
    return std::string(src);
  }

  icu::UnicodeString kUString = Unicode::ToUnicodeString(src);
  icu::UnicodeString result = kUString.toTitle(wordIterator);
  std::string utf8Result = std::string();
  utf8Result = result.toUTF8String(utf8Result);

  delete wordIterator;
  return utf8Result;
}

const std::wstring Unicode::ToFold(std::wstring &src, const StringOptions options)
{
  // ICU wchar_t support is spotty. Convert to native UChars (UTF-16).

  if (src.length() == 0)
    return src;

  size_t u_src_size = Unicode::GetUCharBufferSize(src.length(), 2);
  UChar u_src[u_src_size];

  // Create stack buffer for result of folding

  size_t u_dest_size = Unicode::GetUCharBufferSize(src.length(), 2);
  UChar u_dest[u_src_size];

  int32_t destLength1 = 0; // Actual number of UChars used
  Unicode::WcharToUChar(src.data(), u_src, u_src_size, destLength1, src.length());

  UErrorCode status = U_ZERO_ERROR;
  int32_t folded_length = 0;  // In UChars
  folded_length = icu::CaseMap::fold(to_underlying(options), u_src, (int) src.length(), u_dest,
      (int) u_dest_size, nullptr, status);

  int32_t destLength = 0; // Length in wchars
  int wchar_buffer_size = Unicode::GetWCharBufferSize(folded_length, 2);
  wchar_t wchar_buffer[wchar_buffer_size];
  UCharToWChar(u_dest, wchar_buffer, wchar_buffer_size, destLength, folded_length);
  src.assign(wchar_buffer, destLength);

  return src;

}

/*! \brief Folds the case of a string.

 Similar to ToLower except in addition, all accents are stripped
 and other transformations are made (such as German sharp-S is converted to ss).
 The transformation is independent of locale.

 \param str string to fold
 \param options fine tunes behavior. See StringOptions.
 \return folded string
 */
// Called by Python code: ModuleXbmc.cpp Unicode::utf8Fold(src, options);
const std::string Unicode::UTF8Fold(const std::string &src, const int32_t options)
{
  // Note: very similar to using ucasemap_utf8FoldCase

  if (src.length() == 0)
    return src;

  UErrorCode status = U_ZERO_ERROR;
  // Create buffer on stack
  int32_t bufferSize = Unicode::GetBasicUTF8BufferSize(src.length(), 1.5);
  char buffer[bufferSize];
  icu::CheckedArrayByteSink sink = icu::CheckedArrayByteSink(buffer, bufferSize);

  Unicode::ToFold(icu::StringPiece(src), sink, status, options);
  std::string result;

  if (U_FAILURE(status))
  {
    // Buffer should be big enough.
    if (sink.NumberOfBytesAppended() > bufferSize)
    {
      CLog::Log(LOGERROR, "Error in Unicode::utf8Fold: buffer not large enough need: {}",
          sink.NumberOfBytesAppended());
      return src;
    }
    else
    {
      CLog::Log(LOGERROR, "Error in Unicode::utf8Fold: {}", status);
    }
    result = src;
  }
  else
  {
    size_t size = sink.NumberOfBytesWritten();
    result = std::string(buffer, size);
  }
  return result;
}

/*! \brief Folds the case of a string.

 Similar to ToLower except in addition, insignificant accents are stripped
 and other transformations are made (such as German sharp-S is converted to ss).
 The transformation is independent of locale.

 \param str string to fold in-place
 \param options fine tunes behavior. See StringOptions.
 \return folded string
 */

const std::string Unicode::ToFold(std::string &src, const StringOptions options)
{

  if (src.length() == 0)
    return src;

  UErrorCode status = U_ZERO_ERROR;
  // Create buffer on stack
  int32_t bufferSize = Unicode::GetBasicUTF8BufferSize(src.length(), 1.5);
  char buffer[bufferSize];
  icu::CheckedArrayByteSink sink = icu::CheckedArrayByteSink(buffer, bufferSize);

  Unicode::ToFold(icu::StringPiece(src), sink, status, to_underlying(options));

  if (U_FAILURE(status))
  {
    // Buffer should be big enough
    if (sink.NumberOfBytesAppended() > bufferSize)
    {
      CLog::Log(LOGERROR, "Error in Unicode::ToFold: buffer not large enough need: {}",
          sink.NumberOfBytesAppended());
      return src;
    }
    else
    {
      CLog::Log(LOGERROR, "Error in Unicode::ToFold: {}", status);
      return src;
    }
  }
  else
  {
    size_t size = sink.NumberOfBytesWritten();
    src.assign(buffer, size);
  }
  return src;
}

/*! \brief PRIVATE Folds the case of a string.

 Case Folding is used in non-ASCII environments in a manner similar to ToLower for ASCII.
 In particular, it is heavily used to 'Normalize' strings used in maps where a caseless,
 accentless is required. ToLower can not be used in these circumstances.

 \param strPiece string to fold.
 \param sink stack-based buffer to hold result
 \param status relays status of operation
 \param options fine tunes case folding behavior. For most situations, leave at default

 Notes: length and number of bytes in strPiece may change during folding/normalization
 */

void Unicode::ToFold(const icu::StringPiece strPiece, icu::CheckedArrayByteSink &sink,
    UErrorCode &status, const int32_t options)
{
  icu::CaseMap::utf8Fold(options, strPiece, sink, nullptr, status);
  return;
}

/*! \brief Normalizes a wstring.

 There are multiple Normalizations that can be performed on Unicode. Fortunately
 normalization is not needed in many situations. An introduction can be found
 at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
 Examples and very good summary at: https://www.unicode.org/reports/tr15/tr15-51.html#Examples

 \param str string to Normalize
 \param options fine tunes behavior. See StringOptions. Frequently can leave
 at default value. NOT ALL OPTIONS recognized by all Normalizers.
 \param NormalizerType select the appropriate Normalizer for the job
 \return src
 */

const std::wstring Unicode::Normalize(const std::wstring &src, const StringOptions option,
    const NormalizerType NormalizerType)
{
  // ICU wchar_t support is spotty. Convert to native UChars (UTF-16).

  if (src.length() == 0)
    return src;

  size_t u_src_size = Unicode::GetUCharBufferSize(src.length(), 2);
  UChar u_src[u_src_size];

  // Create stack buffer for result of folding

  size_t u_dest_size = Unicode::GetUCharBufferSize(src.length(), 2);
  UChar u_dest[u_src_size];

  int32_t destLength1 = 0; // Actual number of UChars used
  Unicode::WcharToUChar(src.data(), u_src, u_src_size, destLength1, src.length());

  UErrorCode status = U_ZERO_ERROR;
  int32_t folded_length = 0;  // In UChars
  // Folds and Normalizes at same time.

  const UNormalizer2* Normalizer;

  switch (NormalizerType)
  {
    case NormalizerType::NFC:
    {
      Normalizer = unorm2_getNFCInstance(&status);
      break;
    }
    case NormalizerType::NFD:
    {
      Normalizer = unorm2_getNFDInstance(&status);
      break;
    }
    case NormalizerType::NFKC:
    {
      Normalizer = unorm2_getNFKCInstance(&status);
      break;
    }
    case NormalizerType::NFKD:
    {
      Normalizer = unorm2_getNFKDInstance(&status);
      break;
    }
    case NormalizerType::NFCKCASEFOLD:
    {
      Normalizer = unorm2_getNFKCCasefoldInstance(&status);
      break;
    }
  }

  folded_length = unorm2_normalize(Normalizer, u_src, (int) src.length(), u_dest, (int) u_dest_size,
      &status);

  std::wstring result;
  if (not U_FAILURE(status))
  {
    int32_t destLength = 0; // Length in wchars
    int wchar_buffer_size = Unicode::GetWCharBufferSize(folded_length, 2);
    wchar_t wchar_buffer[wchar_buffer_size];
    UCharToWChar(u_dest, wchar_buffer, wchar_buffer_size, destLength, folded_length);
    result = std::wstring(wchar_buffer, destLength);
  }
  else
    result = src; // Do the best we can

  return result;
}

/*! \brief Normalizes a string.

 There are multiple Normalizations that can be performed on Unicode. Fortunately
 normalization is not needed in many situations. An introduction can be found
 at: https://unicode-org.github.io/icu/userguide/transforms/normalization/

 \param str string to Normalize
 \param options fine tunes behavior. See StringOptions. Frequently can leave
 at default value. NOT ALL OPTIONS recognized by all Normalizers
 \param NormalizerType select the appropriate Normalizer for the job
 \return src
 */

const std::string Unicode::Normalize(const std::string &src, const StringOptions options,
    const NormalizerType NormalizerType)
{
  if (src.length() == 0)
    return "";

  UErrorCode status = U_ZERO_ERROR;
  // Create buffer on stack
  int32_t bufferSize = Unicode::GetBasicUTF8BufferSize(src.length(), 1.5);
  char buffer[bufferSize];
  icu::CheckedArrayByteSink sink = icu::CheckedArrayByteSink(buffer, bufferSize);

  Unicode::Normalize(icu::StringPiece(src), sink, status, to_underlying(options), NormalizerType);

  std::string result;
  if (U_FAILURE(status))
  {
    result = std::string(src);
    if (sink.NumberOfBytesAppended() > bufferSize)
    {
      CLog::Log(LOGERROR, "Error in Unicode::Normalize buffer not large enough need: {}",
          sink.NumberOfBytesAppended());
    }
    else
    {
      CLog::Log(LOGERROR, "Error in Unicode::Normalize: {}", status);
    }
  }
  else
  {
    size_t size = sink.NumberOfBytesWritten();
    result = std::string(buffer, size);
  }
  return result;
}

/*! \brief PRIVATE Normalizes a string.

 There are multiple Normalizations that can be performed on Unicode. Fortunately
 normalization is not needed in many situations. An introduction can be found
 at: https://unicode-org.github.io/icu/userguide/transforms/normalization/

 \param strPiece UTF-8 string to Normalize.
 \param sink stack-based buffer to hold result
 \param status relays status of operation
 \param options fine tunes case folding behavior. For most situations, leave at default.
 NOT ALL OPTIONS recognized by all Normalizers.
 \param NormalizerType select the appropriate Normalizer for the job

 Notes: length and number of bytes in strPiece may change during normalization

 Normalization is frequently not required, but produces more accurate results. There can be
 multiple encodings/codepoints which represent the same character. Normalization cleans
 this up. The type of normalization to use depends upon the goal.
 */

void Unicode::Normalize(const icu::StringPiece strPiece, icu::CheckedArrayByteSink &sink,
    UErrorCode &status, const int32_t options, const NormalizerType NormalizerType)
{
  const icu::Normalizer2* normalizer;
  bool composeMode;

  switch (NormalizerType)
  {
    case NormalizerType::NFC:
    {
      normalizer = icu::Normalizer2::getNFCInstance(status);
      composeMode = true;
      break;
    }
    case NormalizerType::NFD:
    {
      normalizer = icu::Normalizer2::getNFDInstance(status);
      composeMode = false;
      break;
    }
    case NormalizerType::NFKC:
    {
      normalizer = icu::Normalizer2::getNFKCInstance(status);
      composeMode = true;
      break;
    }
    case NormalizerType::NFKD:
    {
      normalizer = icu::Normalizer2::getNFKDInstance(status);
      composeMode = false;
      break;
    }
    case NormalizerType::NFCKCASEFOLD:
    {
      normalizer = icu::Normalizer2::getNFKCCasefoldInstance(status);
      composeMode = true;
      break;
    }
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGINFO, "Error in Normalize create:");
  }
  else
  {
    if (composeMode)
    {
      normalizer->normalizeUTF8(options, strPiece, sink, nullptr, status);
    }
    else
    {
      icu::UnicodeString us = Unicode::ToUnicodeString(strPiece);
      icu::UnicodeString result = normalizer->normalize(us, status);
      result.toUTF8(sink);
    }
    if (U_FAILURE(status))
    {
      CLog::Log(LOGINFO, "Error in Normalize call: ");
    }
  }
  if (U_FAILURE(status))
  {
    // Return what was input.

    sink.Reset(); // In case there is junk
    sink.Append(strPiece.data(), strPiece.length());
  }
}

int Unicode::StrCaseCmp(const std::wstring &s1, const std::wstring &s2,
    const StringOptions options, const bool Normalize /* = false */)
{
  // TODO: Add normalization

  if (s1.empty() and s2.empty())
    return 0;

  if (s1.empty())
    return -1;

  if (s2.empty())
    return 1;

  // Create buffer on stack

  size_t u_s1_size = Unicode::GetWcharToUCharBufferSize(s1.length(), 2);
  UChar u_s1[u_s1_size];
  size_t u_s2_size = Unicode::GetWcharToUCharBufferSize(s2.length(), 2);
  UChar u_s2[u_s2_size];

  // TODO: handle conversion error

  int32_t destLength1 = 0;
  int32_t destLength2 = 0;
  Unicode::WcharToUChar(s1.data(), u_s1, u_s1_size, destLength1);
  Unicode::WcharToUChar(s2.data(), u_s2, u_s2_size, destLength2);

  int result;
  if (not Normalize)
  {
    result = u_strcasecmp(u_s1, u_s2, to_underlying(options));
  }
  else
  {
    UErrorCode status = U_ZERO_ERROR;
    const UChar* p_s1 = u_s1;
    const UChar* p_s2 = u_s2;
    StringOptions opts = options | StringOptions::COMPARE_IGNORE_CASE;

    result = unorm_compare(p_s1, destLength1, p_s2, destLength2, to_underlying(opts), &status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::wstrcasecmp {}", status);
      result = 1234; // Not sure what to return
    }
  }
  return result;
}

int Unicode::StrCaseCmp(const std::string &s1, const std::string &s2,
    const StringOptions options, const bool Normalize /* = false */)
{

  if (s1.empty() and s2.empty())
    return 0;

  if (s1.empty())
    return -1;

  if (s2.empty())
    return 1;

  // Create buffer on stack

  size_t u_s1_size = Unicode::GetUCharBufferSize(s1.length(), 2);
  UChar u_s1[u_s1_size];
  size_t u_s2_size = Unicode::GetUCharBufferSize(s2.length(), 2);
  UChar u_s2[u_s2_size];

  // TODO: handle conversion error

  int32_t destLength1 = 0;
  int32_t destLength2 = 0;
  Unicode::StringToUChar(s1, u_s1, u_s1_size, destLength1);
  Unicode::StringToUChar(s2, u_s2, u_s2_size, destLength2);
  int result;
  if (not Normalize)
  {
    result = u_strcasecmp(u_s1, u_s2, to_underlying(options));
  }
  else
  {
    UErrorCode status = U_ZERO_ERROR;
    const UChar* p_s1 = u_s1;
    const UChar* p_s2 = u_s2;
    StringOptions opts = options | StringOptions::COMPARE_IGNORE_CASE;

    result = unorm_compare(p_s1, destLength1, p_s2, destLength2, to_underlying(opts), &status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::StrCaseCmp {}", status);
      result = 1234;
    }
  }
  return result;
}

int Unicode::StrCaseCmp(const std::string &s1, size_t s1_start, size_t s1_length,
    const std::string &s2, size_t s2_start, size_t s2_length, const StringOptions options,
    const bool Normalize /* = false */)
{

  // Lengths are relative to UTF8 encoding. Only send the bytes
  // specified to the Unicode lib, otherwise there would be
  // no way to apply the start and length later.
  // If a multi-byte character is truncated the resulting
  // StringToUChar conversion will contain U+FFFD codepoints.

  if (s1.empty() and s2.empty())
    return 0;

  if (s1.empty())
    return -1;

  if (s2.empty())
    return 1;

  // Create buffer on stack

  size_t u_s1_size = Unicode::GetUCharBufferSize(s1_length, 2);
  UChar u_s1[u_s1_size];
  size_t u_s2_size = Unicode::GetUCharBufferSize(s2_length, 2);
  UChar u_s2[u_s2_size];

  // TODO: handle conversion error
  int32_t destLength1 = 0;
  int32_t destLength2 = 0;

  Unicode::StringToUChar(s1, u_s1, u_s1_size, destLength1, s1_start, s1_length);
  Unicode::StringToUChar(s2, u_s2, u_s2_size, destLength2, s2_start, s2_length);
  int result;
  if (not Normalize)
  {
    result = u_strcasecmp(u_s1, u_s2, to_underlying(options));
  }
  else
  {
    UErrorCode status = U_ZERO_ERROR;
    const UChar* p_s1 = u_s1;
    const UChar* p_s2 = u_s2;
    StringOptions opts = options | StringOptions::COMPARE_IGNORE_CASE;
    result = unorm_compare(p_s1, destLength1, p_s2, destLength2, to_underlying(opts), &status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::strcasecmp {}", status);
      result = 1234;
    }
  }
  return result;
}

int Unicode::StrCaseCmp(const std::string &s1, const std::string &s2, size_t n,
    const StringOptions options, const bool Normalize /* = false */)
{

  size_t n1 = std::min(n, s1.length());
  size_t n2 = std::min(n, s2.length());

  return Unicode::StrCaseCmp(s1, 0, n1, s2, 0, n2, options, Normalize);
}

int8_t Unicode::StrCmp(const std::wstring &s1, size_t s1_start, size_t s1_length,
    const std::wstring &s2, size_t s2_start, size_t s2_length, const bool Normalize /* = false */)
{

  // TODO: Use faster icu::Collator::compareUTF8()
  // TODO: Implement Normalize

  // Lengths are relative to wchar_t encoding (usually 32-bit Unicode codepoints,
  // but can be UTF-16). Only send the bytes specified to the Unicode lib, otherwise
  // there would be no way to apply the start and length later.

  // Create buffer on stack

  size_t u_s1_size = Unicode::GetWcharToUCharBufferSize(s1.length(), 2);
  UChar u_s1[u_s1_size];
  size_t u_s2_size = Unicode::GetWcharToUCharBufferSize(s2.length(), 2);
  UChar u_s2[u_s2_size];

  int32_t destLength1 = 0;
  int32_t destLength2 = 0;
  Unicode::WcharToUChar(s1.data(), u_s1, u_s1_size, destLength1, s1.length());
  Unicode::WcharToUChar(s2.data(), u_s2, u_s2_size, destLength2, s2.length());

  // Bitwise comparison in codepoint order (no normalization, or locale considerations etc.).
  // Could use u_strcmp, but it does not take lengths

  int8_t result = u_strCompare(u_s1, destLength1, u_s2, destLength2, true);
  return result;
}

int8_t Unicode::StrCmp(const std::string &s1, size_t s1_start, size_t s1_length,
    const std::string &s2, size_t s2_start, size_t s2_length, const bool Normalize /* = false */)
{

  // TODO: Use faster icu::Collator::compareUTF8()
  // TODO: Implement Normalize

  // Lengths are relative to UTF8 encoding. Only send the bytes
  // specified to the Unicode lib, otherwise there would be
  // no way to apply the start and length later.

  // Create buffer on stack

  size_t u_s1_size = Unicode::GetUCharBufferSize(s1.length(), 2);
  UChar u_s1[u_s1_size];
  size_t u_s2_size = Unicode::GetUCharBufferSize(s2.length(), 2);
  UChar u_s2[u_s2_size];

  // destLengths are relative to UChar (UTF-16) encoding.

  int32_t destLength1 = 0;
  int32_t destLength2 = 0;

  Unicode::StringToUChar(s1, u_s1, u_s1_size, destLength1, s1_start, s1_length);
  Unicode::StringToUChar(s2, u_s2, u_s2_size, destLength2, s2_start, s2_length);

  // Bitwise comparison in codepoint order (no normalization, or locale considerations etc.).
  // Could use u_strcmp, but it does not take lengths

  int8_t result = u_strCompare(u_s1, destLength1, u_s2, destLength2, true);
  return result;
}

bool Unicode::StartsWith(const std::string &s1, const std::string &s2)
{
  icu::UnicodeString uString1 = Unicode::ToUnicodeString(s1);
  icu::UnicodeString uString2 = Unicode::ToUnicodeString(s2);
  return uString1.startsWith(uString2);
}

bool Unicode::StartsWithNoCase(const std::string &s1, const std::string &s2,
    const StringOptions options)
{
  icu::UnicodeString uString1 = Unicode::ToUnicodeString(s1);
  icu::UnicodeString uString2 = Unicode::ToUnicodeString(s2);
  uString1.foldCase(to_underlying(options));
  uString2.foldCase(to_underlying(options));

  UBool result = uString1.startsWith(uString2);
  return result;
}
bool Unicode::EndsWith(const std::string &s1, const std::string &s2)
{
  icu::UnicodeString uString1 = Unicode::ToUnicodeString(s1);
  icu::UnicodeString uString2 = Unicode::ToUnicodeString(s2);
  return uString1.endsWith(uString2);
}

bool Unicode::EndsWithNoCase(const std::string &s1, const std::string &s2,
    const StringOptions options)
{
  icu::UnicodeString uString1 = Unicode::ToUnicodeString(s1);
  icu::UnicodeString uString2 = Unicode::ToUnicodeString(s2);
  uString1.foldCase(to_underlying(options));
  uString2.foldCase(to_underlying(options));

  UBool result = uString1.endsWith(uString2);
  return result;
}

/*!
 * \brief Get the leftmost side of a UTF-8 string, limited by character count
 *
 * Unicode characters are of variable byte length. This function's
 * parameters are based on characters and NOT bytes.
 *
 * \param str to get a substring of
 * \param charIndex if keepLeft, maximum number of characters to keep from left end
 *                  else number of characters to remove from right end
 * \param keepLeft controls how charCount is interpreted
 * \return leftmost characters of string, length determined by charCount
 *
 */
std::string Unicode::Left(const std::string &str, const size_t charCount,
                         const icu::Locale icuLocale,  const bool keepLeft /* = true */)
{
  if (charCount == 0)
  {
    if (keepLeft)
      return std::string();
    else
      return std::string(str);
  }
  size_t utf8Index;
  std::string result;
  result = Unicode::Normalize(str, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  if (result.compare(str) != 0)
    CLog::Log(LOGINFO, "Unicode::GetCodeUnitIndex Normalized string different from original");

  size_t referenceCharacter = charCount;
  if (keepLeft)
    referenceCharacter--;  // We need last byte of previous character

  utf8Index = Unicode::GetCodeUnitIndex(result, referenceCharacter, true, keepLeft, icuLocale);

  size_t bytesToCopy = 0;
  if (keepLeft)
  {
    if (utf8Index == std::string::npos) // Error
    {
      bytesToCopy = result.length();
    }
    else if (utf8Index == BEFORE_START)
    {
      bytesToCopy = 0;
    }
    else if (utf8Index == AFTER_END) // Not enough chars
    {
      bytesToCopy = result.length();
    }
    else
      bytesToCopy = utf8Index + 1;
  }
  else // Remove chars from right end
  {
    if (utf8Index == std::string::npos) // Error
    {
      bytesToCopy = result.length();  // Remove none
    }
    else if (utf8Index == BEFORE_START)
    {
      bytesToCopy = 0;  // Remove none
    }
    else if (utf8Index == AFTER_END)
    {
      bytesToCopy = 0;
    }
    else
      bytesToCopy = utf8Index + 1;
  }

  return result.substr(0, bytesToCopy);
}

/*!
 *  \brief Get a substring of a UTF-8 string
 *
 * Unicode characters are of variable byte length. This function's
 * parameters are based on characters and NOT bytes.
 *
 * \param str string to extract substring from
 * \param startCharCount leftmost character of substring [0 based]
 * \param charCount maximum number of characters to include in substring
 * \return substring of str, beginning with character 'firstCharIndex',
 *         length determined by charCount
 */
std::string Unicode::Mid(const std::string &str, size_t startCharCount,
    size_t charCount /* = std::string::npos */)
{
  size_t startUTF8Index;
  size_t endUTF8Index;
  std::string result;
  result = Unicode::Normalize(str, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  if (result.compare(str) != 0)
    CLog::Log(LOGINFO, "Unicode::GetCodeUnitIndex Normalized string different from original");

  // We need the start byte to begin copy
  //   left=false keepLeft=true   Return offset of first byte of (n - 1)th character

  bool left = false;
  bool keepLeft = true;

  // Good 'old signed vs unsigned issues
  if (startCharCount > str.length())
    startCharCount = str.length() + 1;

  if (startCharCount > str.length())
    startCharCount = str.length() + 1;

  startUTF8Index = Unicode::GetCodeUnitIndex(result, (startCharCount + 1), left, keepLeft,
      Unicode::GetDefaultICULocale());
  if (startUTF8Index == std::string::npos) // Error
  {
    return std::string();
  }
  else if (startUTF8Index == BEFORE_START) // Should not occur
  {
    return std::string();
  }
  else if (startUTF8Index == AFTER_END) // Not enough chars
  {
    return std::string();
  }

  result = result.substr(startUTF8Index);

  // We specify here how many characters to copy from the first substring. But we have to translate
  // the character count into a byteindex. It does look like we want the normal Left(xxx, n) copy:
  //
  //  left=true keepLeft=true  Return offset of last byte of nth character
  //
  // So, treat this second step just like left (we could just call Left, but that would add a bit of
  // overhead

  left = true;
  keepLeft = true;
  endUTF8Index = Unicode::GetCodeUnitIndex(result, charCount, left, keepLeft,
      Unicode::GetDefaultICULocale());

  size_t bytesToCopy = 0;
  if (endUTF8Index == std::string::npos) // Error
  {
    bytesToCopy = result.length();
  }
  else if (endUTF8Index == BEFORE_START)
  {
    bytesToCopy = 0;
  }
  else if (endUTF8Index == AFTER_END) // Not enough chars
  {
    bytesToCopy = result.length();
  }
  else
    bytesToCopy = endUTF8Index;

  return result.substr(0, bytesToCopy);
}

std::string Unicode::Right(const std::string &str, size_t charCount,
    const icu::Locale &icuLocale, bool keepRight)
{
  /*
      * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
      * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
      * left=false keepLeft=true   Returns offset of first byte of (n - 1)th character (0-n). Used by Right(x, false)
      * left=false keepLeft=false  Returns offset of first byte of (n - 1)th char from right end (0-n). Used by Right(x)
      *
      * test right(3) => est == > x.last() - 3 ==>
      * test right(1, false) => est == x[1...]  substr(1, npos) Omit left n chars
      */
  if (charCount == 0)
  {
    if (keepRight)
      return std::string();
    return std::string(str);
  }
  size_t utf8Index;
  std::string result;
  result = Unicode::Normalize(str, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  if (result.compare(str) != 0)
    CLog::Log(LOGINFO, "Unicode::GetCodeUnitIndex Normalized string different from original");

  if (charCount > str.length()) // Good 'old signed/unsigned issues
    charCount = str.length() + 1;
  if (!keepRight)
    charCount++;

  utf8Index = Unicode::GetCodeUnitIndex(result, charCount, false, ! keepRight, icuLocale);
  // refstr = "est";
  // varstr = UnicodeUtils::Right(origstr, 1, false);
  // EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // --> Test

   if (utf8Index == std::string::npos) // Error
   {
     utf8Index = result.length();
   }
   else if (utf8Index == BEFORE_START)
   {
     utf8Index = 0;
   }
   else if (utf8Index == AFTER_END)
   {
     utf8Index = result.length();
   }

  return result.substr(utf8Index, std::string::npos);
}

// Expensive to create.
// TODO: Change to use a pool indexed by locale. If this consumes much storage,
// convert to a pool.

thread_local icu::BreakIterator* m_cbi = nullptr;

size_t Unicode::GetCodeUnitIndex(const std::string &str, size_t charCount,
    const bool left, const bool keepLeft, icu::Locale icuLocale)
{
  // TODO: Unicode: Note that this code is not yet able to reliably deal with
  // malformed Unicode. The most expeditious way to achieve these is to use UnicodeStrings.

#ifndef DISABLE_OPTIMIZATION
  // Disable when we want to confirm that code behaves the same without
  // this shortcut.
  //
  // If Requested # characters is 0 or > available code units (here bytes)
  // then quick calculation can give result. Humans wouldn't use this
  // codepath, but a loop or some such might.

  size_t effectiveStrLength = str.length();
  if (charCount == 0)
  {
    /*
     * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
     * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
     * left=false keepLeft=true   Returns offset of first byte of n th character (0-n). Used by Right(x, false)
     * left=false keepLeft=false  Returns offset of first byte of (n - 1) th char from right end (0-n). Used by Right(x)
     *
     */
    if (left)
    {
      if (keepLeft)
      //    left=true  keepLeft=true Return offset of last byte of nth character
        return BEFORE_START;
      else
        return AFTER_END; // Last byte of nth char from right end (doesn't validate chars....)
    }
    else // right- Return starting code-unit to begin copying
    {
      if (keepLeft) // left=false keepLeft=true Return offset of first byte of nth character (0-n).
        return 0;
      // else
      // left=false keepLeft=false  Return offset of first byte of nth char from right end (0-n).
      // Requires calculation
    }
  }
  if (charCount > effectiveStrLength) // Entire string, don't need to calculate char boundaries
  {
    if (left) // Return n code-units to copy starting from left end
    {
      if (keepLeft) //keepLeft=true Return offset of last byte of nth character (0-n)
        return str.length();
      else
        return BEFORE_START; // left=true  keepLeft=false Return offset of last byte of nth character from right end (0-n). Used by Left(x, false)
    }
    else // right- Return starting code-unit to begin copying
    {
      if (keepLeft)
        return str.length(); // left=false keepLeft=true Return offset of first byte of nth character (0-n). Used by Right(x, false)
      else
        return BEFORE_START; // left=false keepLeft=false  Returns offset of first byte of nth char from right end (0-n). Used by Right(x)
    }
  }
#endif

  UErrorCode status = U_ZERO_ERROR;
  if (m_cbi != nullptr)
  {
    //icu::Locale previousLocale = m_cbi->getLocale(ULocDataLocaleType::ULOC_ACTUAL_LOCALE, status);
    icu::Locale previousLocale = m_cbi->getLocale(ULocDataLocaleType::ULOC_VALID_LOCALE, status);

    // bool isSame = Unicode::GetICULocaleId(previousLocale) == Unicode::GetICULocaleId(icuLocale);
    // isSame = Unicode::GetICULocaleId(previousLocale2) == Unicode::GetICULocaleId(icuLocale);
    // isSame = Unicode::GetICULocaleId(previousLocale) == Unicode::GetICULocaleId(GetDefaultICULocale());
    // isSame = Unicode::GetICULocaleId(previousLocale2) == Unicode::GetICULocaleId(GetDefaultICULocale());

    // icu::Locale eng_locale = icu::Locale::getEnglish();
    // icu::Locale en_us_locale = icu::Locale::getUS();
    // icu::Locale en_us_locale2 = icu::Locale::getUS();

    // isSame = Unicode::GetICULocaleId(previousLocale) == Unicode::GetICULocaleId(en_us_locale);
    // isSame = Unicode::GetICULocaleId(previousLocale2) == Unicode::GetICULocaleId(en_us_locale);

    // isSame = Unicode::GetICULocaleId(en_us_locale) == Unicode::GetICULocaleId(en_us_locale);
    // isSame = Unicode::GetICULocaleId(GetDefaultICULocale()) == Unicode::GetICULocaleId(en_us_locale);
    // isSame = Unicode::GetICULocaleId(GetDefaultICULocale()) == Unicode::GetICULocaleId(en_us_locale);

    // isSame = Unicode::GetICULocaleId(GetDefaultICULocale()) == Unicode::GetICULocaleId(GetDefaultICULocale());

    // isSame = Unicode::GetICULocaleId(en_us_locale) == Unicode::GetICULocaleId(en_us_locale);

    /// icu::Locale::getEnglish();
    if (Unicode::GetICULocaleId(previousLocale) != Unicode::GetICULocaleId(icuLocale))
    {
      delete m_cbi;
      m_cbi = nullptr;
    }

  }
  if (m_cbi == nullptr)
  {
    m_cbi = icu::BreakIterator::createCharacterInstance(icuLocale, status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::GetCodeUnitIndex: {}", status);
      m_cbi = nullptr;
      return std::string::npos;
    }
  }

  UText* ut = nullptr;
  ut = utext_openUTF8(ut, str.data(), str.length(), &status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::GetCodeUnitIndex: {}", status);
    return std::string::npos;
  }
  m_cbi->setText(ut, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::GetCodeUnitIndex: {}", status);
    utext_close(ut);
    return std::string::npos;
  }
  if (not m_cbi->isBoundary(0))
  {
    // If was not on boundary, isBoundary advanced to next boundary (if there was one).
    // What happens if we were on last character of string?

    CLog::Log(LOGWARNING,
        "Unicode::GetCodeUnitIndex string is malformed, does not start with valid character.");
    utext_close(ut);
    return str.length() - 1;
  }

  size_t byteIndex = std::string::npos;
  if (charCount > str.length()) // Deal with unsignededness...
    charCount = INT_MAX;

  if (left)
  {
    if (keepLeft)  // left=true keepLeft=true  Return offset of last byte of nth character
    {
      byteIndex = m_cbi->first();      // At charCount 0
      //bool x = m_cbi->isBoundary(byteIndex);

      byteIndex = m_cbi->next(charCount);
      // There is a boundary between the last character and the end of string
      // Have to check both against npos and length (sigh)
      if (byteIndex != std::string::npos and byteIndex < str.length())
      {
        // At correct character, now get it's last byte
        byteIndex = m_cbi->next();
        if (byteIndex != std::string::npos)
          byteIndex--;
        else
          byteIndex = str.length() - 1; // Ran off end of string, probably can't occur
      }
      else // String not long enough
      {
        if (charCount == 0)
          byteIndex = Unicode::BEFORE_START;
        else
          byteIndex = Unicode::AFTER_END;
      }
    }
    else // left=true  keepLeft=false  Returns offset of last byte of nth character from right end
    {
      if (charCount == 0)
      {
        byteIndex = str.length() - 1;
      }
      else
      {
        byteIndex = m_cbi->last();  // BEYOND last
        size_t charsToDelete = charCount;
        if (charsToDelete < INT_MAX)
          charsToDelete++; // Backup to BEFORE last
        while (charsToDelete > 0)
        {
          byteIndex = m_cbi->next(-1);
          if (byteIndex == std::string::npos)
            break;
          charsToDelete--;
        }
        if (byteIndex == std::string::npos)
          byteIndex = Unicode::BEFORE_START; // Not enough chars
        else
        {
          // byteIndex is pointing to beginning of nth character from right end.
          // Need to advance it until it is at last byte of nth char
          byteIndex = m_cbi->next();
          byteIndex--;
        }
      }
    }
  }
  else // right
  {
    if (keepLeft) // left=false keepLeft=true   Return offset of first byte of (n - 1)th character
    {
      m_cbi->first();  // At charCount 0 (byteIndex 0)
      if (charCount == 0)
        byteIndex = Unicode::BEFORE_START;
      else
      {
        byteIndex = m_cbi->next(charCount - 1); // first byte of n-1 th char (Right(x, false) Right test line 888
        if (byteIndex == std::string::npos)
          byteIndex = Unicode::AFTER_END;
      }
    }
    else // left=false keepLeft=false  Returns offset of first byte of (n-1) th char from right end
    {
      size_t charsToDelete = charCount; // Unlike left=true keepleft=false, we want the (n-1)th char
      byteIndex = m_cbi->last();  // BEYOND last
      while (charsToDelete > 0)
      {
        byteIndex = m_cbi->next(-1);
        if (byteIndex == std::string::npos)
          break;
        charsToDelete--;
      }
      if (byteIndex == std::string::npos)
        byteIndex = BEFORE_START;
    }
  }

  utext_close(ut);
  return byteIndex;
}

std::string Unicode::Trim(const std::string &str)
{
  // TODO: change to use uniset.h spanUTF8

  icu::UnicodeString uStr = Unicode::ToUnicodeString(str);
  std::string result = std::string();
  result = uStr.trim().toUTF8String(result);
  return result;
}

std::string Unicode::TrimLeft(const std::string &str)
{
  // TODO: change to use uniset.h spanUTF8

  icu::UnicodeString kString = Unicode::ToUnicodeString(str);
  std::string result = std::string();
  Unicode::Trim(kString, true, false).toUTF8String(result);
  return result;
}

std::string Unicode::TrimLeft(const std::string &str, const std::string deleteChars)
{

  // TODO: change to use uniset.h spanUTF8

  icu::UnicodeString kString = Unicode::ToUnicodeString(str);
  icu::UnicodeString kDeleteChars = Unicode::ToUnicodeString(deleteChars);
  std::string result = std::string();
  Unicode::Trim(kString, kDeleteChars, true, false).toUTF8String(result);
  return result;
}

std::string Unicode::TrimRight(const std::string &str)
{
  // TODO: change to use uniset.h spanBackUTF8

  icu::UnicodeString kString = Unicode::ToUnicodeString(str);
  std::string result = std::string();
  Unicode::Trim(kString, false, true).toUTF8String(result);
  return result;
}

std::string Unicode::TrimRight(const std::string &str, const std::string deleteChars)
{
  // TODO: change to use uniset.h spanBackUTF8

  icu::UnicodeString kString = Unicode::ToUnicodeString(str);
  std::string result = std::string();
  icu::UnicodeString kDeleteChars = Unicode::ToUnicodeString(deleteChars);
  Unicode::Trim(kString, kDeleteChars, false, true).toUTF8String(result);

  return result;
}

// TODO: Verify that offset is in correct units: bytes, characters, codepoints, etc.
// TODO: What about multi-codepoint characters?
// TODO: What if deleteChars has a non-Character in it (i.e. an accent without a
// character that it is attached to?

icu::UnicodeString Unicode::Trim(const icu::UnicodeString &uStr,
    const icu::UnicodeString &deleteChars, const bool trimStart, const bool trimEnd)
{

  // TODO: Revisit use of size_t with int32_t
  // TODO: change to use uniset.h spanUTF8 & spanBackUTF8

  icu::UnicodeString str = icu::UnicodeString(uStr);
  if (str.length() == 0 or deleteChars.length() == 0)
    return (str);

  /**
   * remove method:
   *
   * inline UnicodeString& remove(int32_t start,
   *                           int32_t length = (int32_t)INT32_MAX);
   *
   * Remove the characters in the range
   * [`start`, `start + length`) from the UnicodeString object.
   * @param start the offset of the first character to remove
   * @param length the number of characters to remove
   * @return a reference to this
   * @stable ICU 2.0
   */

  size_t chars_to_delete = 0;
  if (trimStart)
  {
    for (size_t i = 0; i < (size_t) str.length(); i += 1)
    {
      UChar ch = str[i];
      if (deleteChars.indexOf(ch) == -1)
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }
    if (chars_to_delete > 0)
    {
      str.remove(0, chars_to_delete);
    }
  }

  chars_to_delete = 0;
  if (trimEnd)
  {
    for (size_t i = str.length() - 1; i >= 0; i -= 1)
    {
      UChar ch = str[i];
      if (deleteChars.indexOf(ch) == -1)
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }

    if (chars_to_delete > 0)
    {
      str.remove(str.length() - chars_to_delete, chars_to_delete);
    }
  }
  return str;
}

icu::UnicodeString Unicode::Trim(const icu::UnicodeString &uStr, const bool trimStart,
    const bool trimEnd)
{

  // TODO: Revisit use of size_t with int32_t
  // TODO: change to use uniset.h spanUTF8 & spanBackUTF8

  icu::UnicodeString str = icu::UnicodeString(uStr);
  if (str.length() == 0)
    return str;

  size_t chars_to_delete = 0;
  if (trimStart)
  {
    for (size_t i = 0; i < (size_t) str.length(); i += 1)
    {
      UChar ch = str[i];
      if (! u_isWhitespace(ch))
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }
    if (chars_to_delete > 0)
    {
      str.remove(0, chars_to_delete);
    }
  }

  chars_to_delete = 0;
  if (trimEnd)
  {
    for (size_t i = str.length() - 1; i >= 0; i -= 1)
    {
      UChar ch = str[i];
      if (! u_isWhitespace(ch))
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }

    if (chars_to_delete > 0)
    {
      str.remove(str.length() - chars_to_delete, chars_to_delete);
    }
  }
  return str;

}

std::string Unicode::Trim(const std::string &str, const std::string &deleteChars,
    const bool trimStart, const bool trimEnd)
{
  // TODO: change to use uniset.h spanUTF8 & spanBackUTF8

  if (str.length() == 0 or deleteChars.length() == 0)
  {
    std::string result = std::string(str);
    return result;
  }

  icu::UnicodeString kString = Unicode::ToUnicodeString(str);
  icu::UnicodeString kDelChars = Unicode::ToUnicodeString(deleteChars);

  kString = Unicode::Trim(kString, kDelChars, trimStart, trimEnd);

  if ((size_t) kString.length() == str.length())
  {
    std::string result = std::string(str);
    return result;
  }

  std::string result = std::string();
  result = kString.toUTF8String(result);
  return result;
}

/**
 * Replace all occurrences of characters in oldText with the characters
 * in newText
 * @param oldText the text containing the search text
 * @param newText the text containing the replacement text
 * @return a reference to the modified src
 * @stable ICU 2.0
 */

std::tuple<std::string, int> Unicode::FindCountAndReplace(const std::string &src,
    const std::string &oldText, const std::string &newText)
{
  // TODO: Try to change API to just return if something was changed

  icu::UnicodeString kSrc = Unicode::ToUnicodeString(src);
  icu::UnicodeString kOldText = Unicode::ToUnicodeString(oldText);
  icu::UnicodeString kNewText = Unicode::ToUnicodeString(newText);
  std::tuple<icu::UnicodeString, int> result = Unicode::FindCountAndReplace(kSrc, kOldText,
      kNewText);
  icu::UnicodeString kResultStr = std::get<0>(result);
  int changes = std::get<1>(result);
  std::string resultStr = std::string();
  kResultStr.toUTF8String(resultStr);
  return
  { resultStr, changes};
}

std::tuple<icu::UnicodeString, int> Unicode::FindCountAndReplace(const icu::UnicodeString &kSrc,
    const icu::UnicodeString &kOldText, const icu::UnicodeString &kNewText)
{
  return Unicode::FindCountAndReplace(kSrc, 0, kSrc.length(), kOldText, 0, kOldText.length(),
      kNewText, 0, kNewText.length());
}

std::tuple<icu::UnicodeString, int> Unicode::FindCountAndReplace(const icu::UnicodeString &srcText,
    const int32_t srcStart, const int32_t srcLength, const icu::UnicodeString &oldText,
    const int32_t oldStart, const int32_t oldLength, const icu::UnicodeString &newText,
    const int32_t newStart, const int32_t newLength)
{
  int changes = 0;
  icu::UnicodeString resultStr = icu::UnicodeString(srcText);
  if (srcText.isBogus() || oldText.isBogus() || newText.isBogus())
  {
    return
    { resultStr, changes};
  }
  if (oldLength == 0)
  {
    return
    { resultStr, changes};
  }

  int32_t length = srcLength;
  int32_t start = srcStart;
  while (length > 0 && length >= oldLength)
  {
    int32_t pos = resultStr.indexOf(oldText, oldStart, oldLength, start, length);
    if (pos < 0)
    {
      // no more oldText's here: done
      break;
    }
    else
    {
      // we found oldText, replace it by newText and go beyond it

      resultStr.replace(pos, oldLength, newText, newStart, newLength);
      length -= pos + oldLength - start;
      start = pos + newLength;
      changes++;
    }
  }
  return
  { resultStr, changes};
}

size_t Unicode::FindWord(const std::string &str, const std::string &word)
{
  /*
   *  TODO: Rework!!! Broken Original ONLY works with single-byte chars.
   *  Case change ONLY works with ASCII
   *
   * 	Note: is checking to see if the next series of characters in
   *  str match wordLowercase. Upper case chars in str are lowercased
   *  prior to the comparison. Note that no other restrictions are
   *  placed on the chars in wordLowerCase. It could have spaces
   *  punctuation, anything. As long as there is a match it counts as
   *  a word match.
   * /

   while (same && *c && *w) {
   unsigned char lc = *c++;
   if (lc >= 'A' && lc <= 'Z')
   lc += 'a' - 'A';

   if (lc != *w++) // different
   same = false;
   }
   if (same && *w == 0) { // only the same if word has been exhausted
   std::cout << "s - start: " <<  s - start << std::endl;
   return s - start;
   }
   // otherwise, skip current word (composed by Latin letters) or number

   // Note: Once a "word" has not matched, then:
   //    if the current char in str is a number, skip all numbers
   //    if the current char is a letter, skip all letters
   //    if the current char is anything else, skip the character
   //  Then, skip any spaces
   //  Then, start over to see if the next 'word' matches anything
   //  that is in wordLowerCase.
   *
   */
  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  uString.foldCase(U_FOLD_CASE_DEFAULT);
  icu::UnicodeString uWord = Unicode::ToUnicodeString(word);
  uWord.foldCase(U_FOLD_CASE_DEFAULT);

  /**
   * Locate in this the first occurrence in the range
   * [`start`, `start + length`) of the characters
   *  in `srcText` in the range
   * [`srcStart`, `srcStart + srcLength`),
   * using bitwise comparison.
   * @param srcText The text to search for.
   * @param srcStart the offset into `srcText` at which
   * to start matching
   * @param srcLength the number of characters in `srcText` to match
   * @param start the offset into this at which to start matching
   * @param length the number of characters in this to search
   * @return The offset into this of the start of `text`,
   * or -1 if not found.
   * @stable ICU 2.0
   */
  int32_t wordLength = uWord.length(); // Codepoint length, not character length
  int32_t offset = 0;
  int32_t srcLength = uString.length(); // Codepoint length, not character length
  size_t index = std::string::npos;

  /**
   * In normal word searching within text, any matching word must be
   * surrounded by legal word breaks (begin/end of sentence, spaces, etc.).
   * The rules here are different:
   *
   * We don't care what characters follow the found 'word' in src string.
   *
   * We also don't care what character proceeds word (if any).
   *
   * ------------- OLD Algorithm ----
   * When a match fails at the beginning of the string:
   *   * Any leading digits or alphabetic characters are trimmed from
   *     the beginning of the string.
   *   * If no digits or alphabetic characters are trimmed, then one
   *   * character is removed from the beginning of string
   *   * Any spaces are then trimmed from the beginning of the string
   *
   *  ---------- Equivalent Algorithm
   *
   * When a match fails in the middle of the string it must occur on a legal
   * "word" break. Valid word breaks are:
   * -  A space
   * -  The beginning of src
   * -  A space, or the beginning of src followed by all digits or all letters
   * -  A space, or beginning of src, followed by a single character
   *
   *
   * On each pass through the src string, the offset variable is advanced to the
   * segment of src under examination.
   *
   * At this point the process repeats.
   *
   */

  while (index == std::string::npos)
  {
    int32_t tempIndex = uString.indexOf(uWord, 0, wordLength, offset, srcLength);

    if (tempIndex == -1)
    {
      break;
    }
    if (tempIndex == offset)
    {
      index = tempIndex;
      break;
    }
    if ((tempIndex - offset) > 0)
    {
      {
        /**
         * Advance offset to a valid break and repeat.
         *
         * If the first char is a digit, advance past all of the digits.
         * If the first char is a letter, advance past all of the digits
         * Otherwise, advance one char
         * Finally, advance past all spaces
         */

        if (offset < uString.length() // TODO:  ERROR! can split characters
        and u_isdigit(uString.char32At(offset)))
        {
          offset++;
          while (offset < uString.length() and u_isdigit(uString.char32At(offset)))
          {
            offset++;
          }

          // In some languages (Chinese, Japanese...) a single character
          // is a word, not a letter. Using u_isUAlphabetic for such
          // character returns true, but misleading in this context.
          // Use u_isalpha instead. Using a word breakiterator is much
          // more accurate, but it would change this function's behavior

        }
        else if (offset < uString.length() and Unicode::IsLatinChar(uString.char32At(offset))
            and u_isalpha(uString.char32At(offset)))
        {
          offset++;
          while (offset < uString.length() and Unicode::IsLatinChar(uString.char32At(offset))
              and u_isalpha(uString.char32At(offset)))
          {
            offset++;
          }
        }
        else
        {
          if (offset < uString.length())  // Eat one character (TODO: BROKEN)
            offset++;
        }
        while (offset < uString.length())
        {
          if (u_isUWhiteSpace(uString.char32At(offset)))
          { // There are different whitespace characters
            offset++;
          }
          else
          {
            break;
          }
        }
      }
      // Try again with new src segment
    }
  }
  return index;
}

/*
 * Replaces every occurrence of oldText with newText within the string
 *
 */

std::string Unicode::FindAndReplace(const std::string &str, const std::string oldText,
    const std::string newText)
{
  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  icu::UnicodeString uOldText = Unicode::ToUnicodeString(oldText);
  icu::UnicodeString uNewText = Unicode::ToUnicodeString(newText);

  uString.findAndReplace(uOldText, uNewText);

  std::string resultStr = std::string();
  resultStr = uString.toUTF8String(resultStr);
  return resultStr;
}

/*
 * Regular expression patterns for this lib can be found at:
 * normal whitespace pattern: "\h"
 * https://unicode-org.github.io/icu/userguide/strings/regexp.html
 *
 */
size_t Unicode::RegexFind(const std::string &str, const std::string pattern, const int flags)
{
  // TODO: incomplete. Poorly defined

  //std::cout << "RegexFind str: " << str << " pat: " << pattern << std::endl;

  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  icu::UnicodeString uPattern = Unicode::ToUnicodeString(pattern);
  size_t index = str.npos;

  UErrorCode status = U_ZERO_ERROR;
  icu::RegexMatcher* matcher = new icu::RegexMatcher(uPattern, uString, flags, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexFind a {}", status);
    delete (matcher);
    return index;
  }

  while (matcher->find())
  {
    index = matcher->start(status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::RegexFind 2 {}", status);
      delete (matcher);
      return index;
    }
    int groups = matcher->groupCount();

    for (int i = 0; i <= groups; i++) // group 0 is not included in count.
    {
      icu::UnicodeString match = matcher->group(i, status);
      if (U_FAILURE(status))
      {
        CLog::Log(LOGERROR, "Error in Unicode::RegexFind c {}", status);
        delete (matcher);
        return index;
      }
      if (match.length() > 0)
      {
        index = matcher->start(i, status);
        if (U_FAILURE(status))
        {
          CLog::Log(LOGERROR, "Error in Unicode::RegexFind d {}", status);
          delete (matcher);
          return index;
        }
      }
      std::string val = std::string();
      match.toUTF8String(val);
    }
  }
  delete (matcher);
  return index;
}

/*
 * Regular expression patterns for this lib can be found at:
 * normal whitespace pattern: "\h"
 * https://unicode-org.github.io/icu/userguide/strings/regexp.html
 *
 */
std::string Unicode::RegexReplaceAll(const std::string &str, const std::string pattern,
    const std::string replace, const int flags)
{
  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  icu::UnicodeString uPattern = Unicode::ToUnicodeString(pattern);
  icu::UnicodeString uReplace = Unicode::ToUnicodeString(replace);
  UErrorCode status = U_ZERO_ERROR;
  icu::RegexMatcher* matcher = new icu::RegexMatcher(uPattern, uString, flags, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll a {}", status);
    if (matcher != nullptr)
      delete (matcher);
    std::string result = std::string(str);
    return result;
  }
  icu::UnicodeString uResult = matcher->replaceAll(uReplace, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll b {}", status);
    delete (matcher);
      std::string result = std::string(str);
      return result;
  }
  std::string result = std::string();
  result = uResult.toUTF8String(result);
  delete (matcher);
  return result;
}

icu::UnicodeString Unicode::RegexReplaceAll(const icu::UnicodeString &kString,
    const icu::UnicodeString kPattern, const icu::UnicodeString kReplace, const int flags)
{
  UErrorCode status = U_ZERO_ERROR;
  icu::RegexMatcher* matcher = new icu::RegexMatcher(kPattern, kString, flags, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll a {}", status);
    delete matcher;
    icu::UnicodeString uResult = icu::UnicodeString(kString);
    return uResult;
  }
  icu::UnicodeString result = matcher->replaceAll(kReplace, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll b {}", status);
    delete matcher;
    icu::UnicodeString uResult = icu::UnicodeString(kString);
    return uResult;
  }

  delete matcher;
  icu::UnicodeString uResult = icu::UnicodeString(kString);
  return uResult;
}

int32_t Unicode::countOccurances(const std::string &strInput, const std::string &strFind,
    const int flags)
{
  icu::UnicodeString kstr = Unicode::ToUnicodeString(strInput);
  icu::UnicodeString kstrFind = Unicode::ToUnicodeString(strFind);
  int32_t count = 0;

  UErrorCode status = U_ZERO_ERROR;

  icu::RegexMatcher* matcher = new icu::RegexMatcher(kstrFind, kstr, flags, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::countOccurances {}", status);
    delete (matcher);
    return count;
  }
  while (matcher->find())
    count++;

  delete (matcher);
  return count;
}

/**
 * UnicodeString version of SplitTo. Another function provides a utf-8 wrapper.
 */

template<typename OutputIt>
OutputIt Unicode::SplitTo(OutputIt d_first, const icu::UnicodeString &kInput,
    const icu::UnicodeString &kDelimiter, size_t iMaxStrings /* = 0 */, const bool omitEmptyStrings /*  = false */)
{

  OutputIt dest = d_first;

  // If empty string in, then nothing out (not even empty string)
  if (kInput.isEmpty())
    return dest;

  // If Nothing to split with, then no change to the input

  if (kDelimiter.isEmpty())
  {
    *d_first++ = kInput;
    return dest;
  }

  const size_t delimLen = kDelimiter.length();
  size_t nextDelim;
  size_t textPos = 0;
  do
  {
    // Keep splitting until limit reached
    if (--iMaxStrings == 0)
    {
      // Once limit reached, append remaining text to result and leave

      if (!omitEmptyStrings or (textPos < (size_t) kInput.length()))
      {
        *d_first++ = kInput.tempSubString(textPos);
      }
      break;
    }

    nextDelim = kInput.indexOf(kDelimiter, textPos);
    size_t uNextDelim = nextDelim; // std::string::npos / -1 handled differently

    // UnicodeString.tempSubstring changes any length < 0 (npos) to 0
    // However, it will change any positive number that runs off the string
    // to the maximum usable size (like std::string functions).

    if (uNextDelim == std::string::npos)
      uNextDelim = kInput.length();

    if (!omitEmptyStrings or (uNextDelim > textPos))
    {
      *d_first++ = kInput.tempSubString(textPos, uNextDelim - textPos);
    }
    textPos = uNextDelim + delimLen;
  }
  while (nextDelim != ::std::string::npos);
  return dest;
}


template<typename OutputIt>
 OutputIt Unicode::SplitTo(OutputIt d_first, icu::UnicodeString uInput,
     const std::vector<icu::UnicodeString> &uDelimiters,  size_t iMaxStrings/* = 0*/, const bool omitEmptyStrings /*= false*/)
 {

   OutputIt dest = d_first;

   if (uInput.isEmpty())
     return dest;

   if (uDelimiters.empty())
   {
     *d_first++ = uInput;
     return dest;
   }
   icu::UnicodeString uDelimiter = uDelimiters[0];

   // TODO: Review this change in behavior

   // First, transform every occurrence of delimiters in the string to the first delimiter
   // Then, split using the first delimiter.

   for (size_t di = 1; di < uDelimiters.size(); di++)
   {
     icu::UnicodeString uNextDelimiter = uDelimiters[di];
     uInput.findAndReplace(uNextDelimiter, uDelimiter);
   }

   if (uInput.isEmpty())
   {
     if (! omitEmptyStrings)
       *d_first++ = uInput;
     return dest;
   }

   Unicode::SplitTo(d_first, uInput, uDelimiter, iMaxStrings, omitEmptyStrings);
   return dest;
 }

std::vector<icu::UnicodeString> Unicode::SplitMulti(const std::vector<icu::UnicodeString> &input,
    const std::vector<icu::UnicodeString> &delimiters, size_t iMaxStrings)
{
  // For SplitMulti, returning empty strings does not make much sense since you can't correlate
  // what it relates to.

  // TODO: This implementation differs from the StringUtils version in that, like
  //       Split, it first reduces multiple delimiters to one by modifying the input strings.
  //       When iMaxStrings is exceeded we can't easily return the same partially
  //       processed input strings as before. Need to determine if this is a problem or not.
  //       A cursory examination suggests that it is not a problem for the current use cases.
  //       Need to document any changes. If old behavior is required, then revert, even
  //       if a tad slower.

  constexpr bool omitEmptyStrings = true;

  if (input.empty())
    return std::vector<icu::UnicodeString>();

  std::vector<icu::UnicodeString> results(input);

  if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
    return results;

  std::vector<icu::UnicodeString> strings1;
  std::vector<icu::UnicodeString> substrings = std::vector<icu::UnicodeString>();
  if (iMaxStrings == 0)
  {
    for (size_t di = 0; di < delimiters.size(); di++)
    {
      for (size_t i = 0; i < results.size(); i++)
      {
        substrings.clear();
        Unicode::SplitTo(std::back_inserter(substrings), results[i], delimiters[di], iMaxStrings,
            omitEmptyStrings);
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
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
    for (size_t i = 0; i < input.size(); i++)
    {
      if (iNew > 0)
      {
        substrings.clear();
        Unicode::SplitTo(std::back_inserter(substrings), results[i], delimiters[di], iNew + 1,
            omitEmptyStrings);
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
    if (iNew <= 0)
      break; // Stop trying any more delimiters
  }
  return results;
}

std::vector<std::string> Unicode::SplitMulti(const std::vector<std::string> &input,
    const std::vector<std::string> &delimiters, size_t iMaxStrings)
{
  if (input.empty())
    return std::vector<std::string>();

  std::vector<std::string> results;

  if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
  {
    results = std::vector<std::string>(input);
    return results;
  }

  std::vector<icu::UnicodeString> kInput = std::vector<icu::UnicodeString>();
  std::vector<icu::UnicodeString> kDelimiters = std::vector<icu::UnicodeString>();

  for (size_t i = 0; i < input.size(); i++)
  {
    icu::UnicodeString kI = Unicode::ToUnicodeString(input[i]);
    kInput.push_back(kI);
  }

  for (size_t i = 0; i < delimiters.size(); i++)
  {
    icu::UnicodeString kD = Unicode::ToUnicodeString(delimiters[i]);
    kDelimiters.push_back(kD);
  }

  std::vector<icu::UnicodeString> kResults = SplitMulti(kInput, kDelimiters, iMaxStrings);

  results = std::vector<std::string>();
  for (size_t i = 0; i < kResults.size(); i++)
  {
    std::string tempStr = std::string();
    tempStr = kResults[i].toUTF8String(tempStr);
    results.push_back(tempStr);
  }

  return results;
}

bool Unicode::Contains(const std::string &str, const std::vector<std::string> &keywords)
{

  // Moved code to here because it may need more than just
  // byte compare, but for the moment just do byte compare, as was
  // originally done.

  for (std::vector<std::string>::const_iterator it = keywords.begin(); it != keywords.end(); ++it)
  {
    if (str.find(*it) != str.npos)
      return true;
  }
  return false;
}

thread_local icu::Collator* myCollator = nullptr;
thread_local std::chrono::steady_clock::time_point myCollatorStart = std::chrono::steady_clock::now();

bool Unicode::InitializeCollator(std::locale locale, bool Normalize /* = false */)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  return Unicode::InitializeCollator(icuLocale, Normalize);
}

bool Unicode::InitializeCollator(icu::Locale icuLocale, bool Normalize /* = false */)
{
  UErrorCode status = U_ZERO_ERROR;
  icu::UnicodeString dispName;
  if (myCollator != nullptr)
  {
    delete myCollator;
  }
  std::string localeId = Unicode::GetICULocaleId(icuLocale);
  CLog::Log(LOGINFO, "Collate locale: {}", localeId);

  myCollator = icu::Collator::createInstance(icuLocale, status);
  if (U_FAILURE(status))
  {
    icuLocale.getDisplayName(dispName);
    CLog::Log(LOGWARNING, "Failed to create the collator for : \"{}\"\n", toString(dispName));
    return false;
  }
  myCollatorStart = std::chrono::steady_clock::now();

  // Go with default Normalization (off). Some free normalization
  // is still performed. Even with it on, you should do some
  // extra normalization up-front to handle certain
  // locales/codepoints. See the documentation for UCOL_NORMALIZATION_MODE
  // for a hint.

  status = U_ZERO_ERROR;

  if (Normalize)
    myCollator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
  else
    myCollator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
  if (U_FAILURE(status))
  {
    icuLocale.getDisplayName(dispName);
    CLog::Log(LOGWARNING, "Failed to set normalization for the collator: \"{}\"\n",
        toString(dispName));
    return false;
  }
  myCollator->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, status);
  if (U_FAILURE(status))
  {
    icuLocale.getDisplayName(dispName);
    CLog::Log(LOGWARNING, "Failed to set numeric collation: \"{}\"\n", toString(dispName));
    return false;
  }
  return true;
}

 void Unicode::SortCompleted(int sortItems)
 {
   // using namespace std::chrono;
   std::chrono::steady_clock::time_point myCollatorStop = std::chrono::steady_clock::now();
   int micros = std::chrono::duration_cast<std::chrono::microseconds>(myCollatorStop - myCollatorStart).count();
   CLog::Log(LOGINFO, "Sort of {} records took {} µs", sortItems,  micros);
 }


/**
 * Experimental Collation function aimed at being equivalent to UnicodeUtils::AlphaNumericCompare
 * but using ICU. The ICU should be superior. Need to do performance tests.
 *
 * Note that it would be faster to pass either UTF-16 (the native ICU format) or
 * UTF-8 strings. Currently wstrings are converted to UTF-16. It is fairly easy to
 * change SortUtils (and friends) to pass UTF-8 (instead of first converting to wchar),
 * however it seems that the GUI requires the wstring for BIDI. Most likely ICU has a
 * solution for that. At some point should use UTF-8 to do a performance comparison. ICU
 * has put a lot of work into performance.
 *
 * Normally Normalization does not have to be a worry, however, if say, you mix Vietnamese or Arabic
 * movie titles in an English locale, then the Collator will by default not do normalization
 * (because of the English locale). However, you might want to turn it on. I suspect
 * that you could more-or-less auto-detect this. See
 * https://unicode-org.github.io/icu/userguide/collation/faq.html
 *
 */
int32_t Unicode::Collate(const std::wstring &left, const std::wstring &right)
{

  // TODO: Create UTF8 version of Collate for coll.h compareUTF8

  UErrorCode status = U_ZERO_ERROR;

  if (myCollator == nullptr)
  {
    CLog::Log(LOGWARNING, "Collator NOT configured\n");
    return INT_MIN;
  }

  // Stack based memory. A bit tedious. There must be a way to do this in a template,
  // or not so awful Macro.

  size_t leftBufferSize = Unicode::GetWcharToUCharBufferSize(left.length(), 1);
  UChar leftBuffer[leftBufferSize];
  int32_t actualLeftBufferLength = 0;
  Unicode::WcharToUChar(left.data(), leftBuffer, leftBufferSize, actualLeftBufferLength);

  size_t rightBufferSize = Unicode::GetWcharToUCharBufferSize(right.length(), 1);
  UChar rightBuffer[rightBufferSize];
  int32_t actualRightBufferLength = 0;
  Unicode::WcharToUChar(right.data(), rightBuffer, rightBufferSize, actualRightBufferLength);
  status = U_ZERO_ERROR;
  UCollationResult result;
  result = myCollator->compare(leftBuffer, actualLeftBufferLength, rightBuffer,
      actualRightBufferLength, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGWARNING, "Failed compare");
  }
  return result;
}

