/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <locale>
#include "utils/StringUtils.h"
#include "utils/UnicodeUtils.h"

#include <algorithm>

#include <gtest/gtest.h>
enum class ECG
{
  A, B
};

enum EG
{
  C, D
};

namespace test_enum
{
enum class ECN
{
  A = 1, B
};
enum EN
{
  C = 1, D
};
}

namespace TestUnicodeUtils
{
// MULTICODEPOINT_CHARS

// There are multiple ways to compose some graphemes. These are the same grapheme:

// Each of these five series of unicode codepoints represent the
// SAME grapheme (character)! To compare them correctly they should be
// normalized first. Normalization should reduce each sequence to the
// single codepoint (although some graphemes require more than one
// codepoint after normalization).
//
// TODO: It may be a good idea to normalize everything on
// input and renormalize when something requires it.
// A: U+006f (o) + U+0302 (◌̂) + U+0323 (◌̣): o◌̣◌̂
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_1[] =
{ 0x006f, 0x0302, 0x0323 };
// B: U+006f (o) + U+0323 (◌̣) + U+0302 (◌̂)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_2[] =
{ 0x006f, 0x0323, 0x302 };
// C: U+00f4 (ô) + U+0323 (◌̣)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_3[] =
{ 0x00f4, 0x0323 };
// D: U+1ecd (ọ) + U+0302 (◌̂)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_4[] =
{ 0x1ecd, 0x0302 };
// E: U+1ed9 (ộ)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_5[] =
{ 0x1ed9 };

// UTF8 versions of the above.

const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1[] =
{ "\x6f\xcc\x82\xcc\xa3\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2[] =
{ "\x6f\xcc\xa3\xcc\x82\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_3[] =
{ "\xc3\xb4\xcc\xa3\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_4[] =
{ "\xe1\xbb\x8d\xcc\x82\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5[] =
{ "\xe1\xbb\x99\x00" };

// 			u"óóßChloë" // German "Sharp-S" ß is (mostly) equivalent to ss (lower case).
//                     Lower case of two SS characters can either be ss or ß,
//                     depending upon context.
const char UTF8_GERMAN_SAMPLE[] =
{ "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
// u"ÓÓSSCHLOË";
const char* UTF8_GERMAN_UPPER =
{ "\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b" };
// u"óósschloë"
const char UTF8_GERMAN_LOWER[] =
{ "\xc3\xb3\xc3\xb3\xc3\x9f\x63\x68\x6c\x6f\xc3\xab" };
// óóßchloë
const char* UTF8_GERMAN_LOWER_SS =
{ "\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab" };
// u"óósschloë";
}
static icu::Locale getCLocale()
{
  icu::Locale c_locale = Unicode::getICULocale(std::locale::classic().name().c_str());
  return c_locale;
}
static icu::Locale getTurkicLocale()
{
  icu::Locale turkic_locale = Unicode::getICULocale("tr", "TR");
  return turkic_locale;
}
static icu::Locale getUSEnglishLocale()
{
  icu::Locale us_english_locale = Unicode::getICULocale("en", "US");
  return us_english_locale;
}

static icu::Locale getUkranianLocale()
{
  icu::Locale ukranian_locale = Unicode::getICULocale("uk", "UA");
  return ukranian_locale;
}

TEST(TestUnicodeUtils, ToUpper)
{

  std::string refstr = "TEST";

  std::string varstr = "TeSt";
  varstr = UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  varstr = UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë
}

TEST(TestUnicodeUtils, ToUpper_w)
{
  std::wstring refstr = L"TEST";
  std::wstring varstr = L"TeSt";
  varstr = UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_GERMAN_UPPER)); // ÓÓSSCHLOË
  varstr = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE)); // óóßChloë
  varstr = UnicodeUtils::ToUpper(varstr);
  int32_t result = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(result, 0);
}

TEST(TestUnicodeUtils, ToUpper_Locale)
{
  std::string refstr = "TWITCH";
  std::string varstr = "Twitch";
  varstr = UnicodeUtils::ToUpper(varstr, getCLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  varstr = UnicodeUtils::ToUpper(varstr, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  varstr = UnicodeUtils::ToUpper(varstr, getTurkicLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  varstr = UnicodeUtils::ToUpper(varstr, getUkranianLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  varstr = UnicodeUtils::ToUpper(varstr, getCLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë
}

TEST(TestUnicodeUtils, ToLower)
{
  std::string refstr = "test";

  std::string varstr = "TeSt";
  varstr = UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  refstr = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  varstr = UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë

  // ToLower of string with (with sharp-s) should not change it.

  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  refstr = TestUnicodeUtils::UTF8_GERMAN_LOWER; // óóßChloë
  varstr = UnicodeUtils::ToLower(varstr);
  int32_t result = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(result, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, ToLower_w)
{
  std::wstring refstr = L"test";

  std::wstring varstr = L"TeSt";
  varstr = UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str()); // Binary compare should work

  varstr = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_GERMAN_UPPER)); // ÓÓSSCHLOË
  refstr = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_GERMAN_LOWER_SS)); // óóßChloë
  varstr = UnicodeUtils::ToLower(varstr);
  int32_t result = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(result, 0);

  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë

  // ToLower of string with (with sharp-s) should not change it.

  varstr = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE)); // óóßChloë
  refstr = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_GERMAN_LOWER)); // óóßchloë
  varstr = UnicodeUtils::ToLower(varstr);
  result = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(result, 0);

  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, Turkic_I)
{

  // Specifically test behavior of Turkic I
  // US English
  // First, ToLower

  std::string originalInput = "I i İ ı";

  /*
   * For US English locale behavior of the 4 versions of "I" used in Turkey
   * ToLower yields:
   *
   * I (Dotless I \u0049)        -> i (Dotted small I)                 \u0069
   * i (Dotted small I \u0069)   -> i (Dotted small I)                 \u0069
   * İ (Dotted I) \u0130         -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * ı (Dotless small I) \u0131  -> ı (Dotless small I)                \u0131
   */
  std::string varstr = std::string(originalInput); // hex: 0049 0069 0130 0131
  std::string refstr = "iii̇ı";  // hex: 0069 0069 0069 0307 0131
  // Convert to native Unicode, UChar32
  std::string orig = std::string(varstr);
  std::wstring w_varstr_in = Unicode::utf8_to_wstring(varstr);
  varstr = UnicodeUtils::ToLower(varstr, getUSEnglishLocale());
  std::wstring w_varstr_out = Unicode::utf8_to_wstring(varstr);
  std::stringstream ss;
  std::string prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  std::string hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  std::string hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  std::cout << "US English ToLower input: " << orig << " output: " << varstr << " input hex: "
      << hexInput << " output hex: " << hexOutput << std::endl;

  /*
   * For US English locale behavior of the 4 versions of "I" used in Turkey
   * ToUpper yields:
   *
   * I (Dotless I)       \u0049 -> I (Dotless I) \u0049
   * i (Dotted small I)  \u0069 -> I (Dotless I) \u0049
   * İ (Dotted I)        \u0130 -> İ (Dotted I)  \u0130
   * ı (Dotless small I) \u0131 -> I (Dotless I) \u0049
   */

  varstr = std::string(originalInput);
  refstr = "IIİI";
  orig = std::string(varstr);
  w_varstr_in = Unicode::utf8_to_wstring(varstr);
  varstr = UnicodeUtils::ToUpper(varstr, getUSEnglishLocale());
  w_varstr_out = Unicode::utf8_to_wstring(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  std::cout << "US English ToUpper input: " << orig << " output: " << varstr << " input hex: "
      << hexInput << " output hex: " << hexOutput << std::endl;

  /*
   * For Turkish locale behavior of the 4 versions of "I" used in Turkey
   * ToLower yields:
   *
   * I (Dotless I)       \u0049 -> ı (Dotless small I) \u0131
   * i (Dotted small I)  \u0069 -> i (Dotted small I)  \u0069
   * İ (Dotted I)        \u0130 -> i (Dotted small I)  \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I) \u0131
   */

  varstr = std::string(originalInput);
  refstr = "ıiiı";
  // Convert to native Unicode, UChar32
  orig = std::string(varstr);
  w_varstr_in = Unicode::utf8_to_wstring(varstr);
  varstr = UnicodeUtils::ToLower(varstr, getTurkicLocale());
  w_varstr_out = Unicode::utf8_to_wstring(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  std::cout << "Turkic ToLower input: " << orig << " output: " << varstr << " input hex: "
      << hexInput << " output hex: " << hexOutput << std::endl;
  /*
   * For Turkish locale behavior of the 4 versions of "I" used in Turkey
   * ToUpper yields:
   *
   * I (Dotless I)       \u0049 -> I (Dotless I) \u0049
   * i (Dotted small I)  \u0069 -> İ (Dotted I)  \u0130
   * İ (Dotted I)        \u0130 -> İ (Dotted I)  \u0130
   * ı (Dotless small I) \u0131 -> I (Dotless I) \u0049
   */

  varstr = std::string(originalInput);
  refstr = "IİİI";
  orig = std::string(varstr);
  w_varstr_in = Unicode::utf8_to_wstring(varstr);
  varstr = UnicodeUtils::ToUpper(varstr, getTurkicLocale());
  w_varstr_out = Unicode::utf8_to_wstring(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  std::cout << "Turkic ToUpper input: " << orig << " output: " << varstr << " input hex: "
      << hexInput << " output hex: " << hexOutput << std::endl;
}

TEST(TestUnicodeUtils, ToCapitalize)
{
  std::string refstr = "Test";
  std::string varstr = "test";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "  JuST Another\t\tTEst:\nWoRKs ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N.Y.P.D";
  varstr = "n.y.p.d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, TitleCase)
{
  // Different from ToCapitalize (single word not title cased)

  std::string refstr = "Test";
  std::string varstr = "test";
  UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "  Just Another\t\tTest:\nWorks ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N.y.p.d";
  varstr = "n.y.p.d";
  UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(TestUnicodeUtils, EqualsNoCase_Normalize)
{
  const std::string refstr = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5;
  const std::string varstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  EXPECT_FALSE(UnicodeUtils::EqualsNoCase(refstr, varstr));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, varstr, StringOptions::FOLD_CASE_DEFAULT, true));
}
TEST(TestUnicodeUtils, Left_Basic)
{
  std::string refstr;
   std::string varstr;
   std::string origstr = "Test";

   // First, request n chars to copy from left end

   refstr = "";
   varstr = UnicodeUtils::Left(origstr, 0);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "Te";
   varstr = UnicodeUtils::Left(origstr, 2);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "Test";
   varstr = UnicodeUtils::Left(origstr, 4);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "Test";
   varstr = UnicodeUtils::Left(origstr, 10);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "Test";
   varstr = UnicodeUtils::Left(origstr, std::string::npos);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   // # of characters to omit from right end

   refstr = "Tes";
   varstr = UnicodeUtils::Left(origstr, 1, false);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "Test";
   varstr = UnicodeUtils::Left(origstr, 0, false);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "T";
   varstr = UnicodeUtils::Left(origstr, 3, false);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "";
   varstr = UnicodeUtils::Left(origstr, 4, false);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "";
   varstr = UnicodeUtils::Left(origstr, 5, false);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

   refstr = "";
   varstr = UnicodeUtils::Left(origstr, std::string::npos, false);
   EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // TODO: Add test to ensure count works properly for multi-codepoint characters
}

TEST(TestUnicodeUtils, Left_Advanced)
{
  std::string origstr;
  std::string refstr;
  std::string varstr;

  // Multi-byte characters character count != byte count

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1); // 3 codepoints, 1 char
  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 0, true, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Interesting case. All five VARIENTs can be normalized
  // to a single codepoint. We are NOT normalizing here.

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5); // 2 codepoints, 1 char
  varstr = UnicodeUtils::Left(origstr, 2, true, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE); // u"óóßChloë"
  refstr = "óó";
  varstr = UnicodeUtils::Left(origstr, 2, true, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Get leftmost substring removing n characters from end of string

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string("");
  varstr = UnicodeUtils::Left(origstr, 1, false, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Interesting case. All five VARIENTs can be normalized
  // to a single codepoint. We are NOT normalizing here.

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string("");
  varstr = UnicodeUtils::Left(origstr, 2, false, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 5, false, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE); // u"óóßChloë"
  refstr = "óóßChl";
  varstr = UnicodeUtils::Left(origstr, 2, false, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // TODO: Add test to ensure count works properly for multi-codepoint characters
}

TEST(TestUnicodeUtils, Mid)
{
  std::string refstr;
  std::string varstr;
  std::string origstr = "test";

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 0, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "te";
  varstr = UnicodeUtils::Mid(origstr, 0, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = UnicodeUtils::Mid(origstr, 0, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Mid(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Mid(origstr, 2, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "es";
  varstr = UnicodeUtils::Mid(origstr, 1, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "est";
  varstr = UnicodeUtils::Mid(origstr, 1, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "est";
  varstr = UnicodeUtils::Mid(origstr, 1, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 1, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 4, 1);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, Right)
{
  std::string refstr;
  std::string varstr;
  std::string origstr = "Test";

  // First, request n chars to copy from right end

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Right(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // # of characters to omit from left end

  refstr = "est";
  varstr = UnicodeUtils::Right(origstr, 1, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 0, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "t";
  varstr = UnicodeUtils::Right(origstr, 3, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 4, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 5, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, std::string::npos, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, Trim)
{
  std::string refstr = "test test";

  std::string varstr = " test test   ";
  UnicodeUtils::Trim(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  UnicodeUtils::Trim(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "\nx\r\t   x";
  varstr = "$ \nx\r\t   x?\t";
  UnicodeUtils::Trim(varstr, "$? \t");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, TrimLeft)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  UnicodeUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  UnicodeUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "\nx\r\t   x?\t";
  varstr = "$ \nx\r\t   x?\t";
  UnicodeUtils::TrimLeft(varstr, "$? \t");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, TrimRight)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  UnicodeUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  UnicodeUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "$ \nx\r\t   x";
  varstr = "$ \nx\r\t   x?\t";
  UnicodeUtils::TrimRight(varstr, "$? \t");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, Replace)
{
  std::string refstr = "text text";

  std::string varstr = "test test";
  EXPECT_EQ(UnicodeUtils::Replace(varstr, 's', 'x'), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(UnicodeUtils::Replace(varstr, 's', 'x'), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = "test test";
  EXPECT_EQ(UnicodeUtils::Replace(varstr, "s", "x"), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(UnicodeUtils::Replace(varstr, "s", "x"), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, StartsWith)
{
  std::string refstr = "test";

  // TODO: Test with FoldCase option
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, "x"));

  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "Te"));

  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, "TesT"));
}

TEST(TestUnicodeUtils, EndsWith)
{
  std::string refstr = "test";
  EXPECT_TRUE(UnicodeUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(UnicodeUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(UnicodeUtils::EndsWith(refstr, "sT"));
}

TEST(TestUnicodeUtils, EndsWithNoCase)
{
  // TODO: Test with FoldCase option

  std::string refstr = "test";
  EXPECT_FALSE(UnicodeUtils::EndsWithNoCase(refstr, "x"));
  EXPECT_TRUE(UnicodeUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(UnicodeUtils::EndsWithNoCase(refstr, "TesT"));
}

TEST(TestUnicodeUtils, FoldCase)
{
  /*
   *  FOLD_CASE_DEFAULT
   * I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
   * İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   *
   * FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (Dotless I)       \u0049 -> ı (Dotless small I) \u0131
   * İ (Dotted I)        \u0130 -> i (Dotted small I)  \u0069
   * i (Dotted small I)  \u0069 -> i (Dotted small I)  \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I) \u0131
   */
  std::string s1 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  std::string s2 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  std::string r1;
  std::string r2;

  s1 = UnicodeUtils::FoldCase(s1);
  s2 = UnicodeUtils::FoldCase(s2);
  int32_t result = UnicodeUtils::Compare(s1, s2);
  EXPECT_NE(result, 0);

  s1 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  s2 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  r1 = UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // td::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_NE(result, 0);

  s1 = "I İ İ i ı";
  s2 = "i i̇ i̇ i ı";

  r1 = UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + i_COMBINING_DOUBLE_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;

  r1 = UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_EQ(result, 0);
  /*
   FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (\u0049) -> ı (\u0131)
   * i (\u0069) -> i (\u0069)
   * İ (\u0130) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   *
   */
  s1 = I + I_DOT + i + i_DOTLESS;
  s2 = i_DOTLESS + i + i + i_DOTLESS;

  r1 = UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  r2 = UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_EQ(result, 0);

  /*
   *  FOLD_CASE_DEFAULT
   * I (\u0049) -> i (\u0069)
   * İ (\u0130) -> i̇ (\u0069 \u0307)
   * i (\u0069) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   */
  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  r1 = UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;

  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_EQ(result, 0);

}

TEST(TestUnicodeUtils, FoldCase_W)
{
  std::wstring w_s1 = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  std::wstring w_s2 = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));

  std::wstring r1;
  std::wstring r2;

  r1 = UnicodeUtils::FoldCase(w_s1);
  r2 = UnicodeUtils::FoldCase(w_s2);
  int32_t result = UnicodeUtils::Compare(r1, r2);
  EXPECT_NE(result, 0);

  w_s1 = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  w_s2 = Unicode::utf8_to_wstring(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  r1 = UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // td::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_NE(result, 0);

  std::string s1 = "I İ İ i ı";
  std::string s2 = "i i̇ i̇ i ı";
  w_s1 = Unicode::utf8_to_wstring(s1);
  w_s2 = Unicode::utf8_to_wstring(s2);
  r1 = UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + i_COMBINING_DOUBLE_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;

  w_s1 = Unicode::utf8_to_wstring(s1);
  w_s2 = Unicode::utf8_to_wstring(s2);
  r1 = UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);

  EXPECT_EQ(result, 0);
  /*
   FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (\u0049) -> ı (\u0131)
   * i (\u0069) -> i (\u0069)
   * İ (\u0130) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   *
   */
  s1 = I + I_DOT + i + i_DOTLESS;
  s2 = i_DOTLESS + i + i + i_DOTLESS;

  w_s1 = Unicode::utf8_to_wstring(s1);
  w_s2 = Unicode::utf8_to_wstring(s2);
  r1 = UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  r2 = UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_EQ(result, 0);

  /*
   *  FOLD_CASE_DEFAULT
   * I (\u0049) -> i (\u0069)
   * İ (\u0130) -> i̇ (\u0069 \u0307)
   * i (\u0069) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   */
  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = Unicode::utf8_to_wstring(s1);
  w_s2 = Unicode::utf8_to_wstring(s2);
  r1 = UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  r2 = UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(r1, r2);
  EXPECT_EQ(result, 0);
}

TEST(TestUnicodeUtils, Join)
{
  std::string refstr;
  std::string varstr;
  std::vector<std::string> strarray;

  strarray.emplace_back("a");
  strarray.emplace_back("b");
  strarray.emplace_back("c");
  strarray.emplace_back("de");
  strarray.emplace_back(",");
  strarray.emplace_back("fg");
  strarray.emplace_back(",");
  refstr = "a,b,c,de,,,fg,,";
  varstr = UnicodeUtils::Join(strarray, ",");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, Normalize)
{
  // TODO: These tests are all on essentially the same caseless string.
  // The FOLD_CASE_DEFAULT option (which only impacts NFKD and maybe NFKC)
  // is not being tested, neither are the other alternatives to it.

  std::string s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  std::string result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFD);
  int cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFKC);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFD);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFKD);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);
}

TEST(TestUnicodeUtils, Split)
{
  std::vector<std::string> varresults;

  // test overload with string as delimiter
  varresults = UnicodeUtils::Split("g,h,ij,k,lm,,n", ",");
  EXPECT_STREQ("g", varresults.at(0).c_str());
  EXPECT_STREQ("h", varresults.at(1).c_str());
  EXPECT_STREQ("ij", varresults.at(2).c_str());
  EXPECT_STREQ("k", varresults.at(3).c_str());
  EXPECT_STREQ("lm", varresults.at(4).c_str());
  EXPECT_STREQ("", varresults.at(5).c_str());
  EXPECT_STREQ("n", varresults.at(6).c_str());

  EXPECT_TRUE(UnicodeUtils::Split("", "|").empty());

  EXPECT_EQ(4U, UnicodeUtils::Split("a bc  d ef ghi ", " ", 4).size());
  EXPECT_STREQ("d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", " ", 4).at(3).c_str())
      << "Last part must include rest of the input string";
  EXPECT_EQ(7U, UnicodeUtils::Split("a bc  d ef ghi ", " ").size())
      << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(1).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(2).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(6).c_str());

  EXPECT_EQ(2U, UnicodeUtils::Split("a bc  d ef ghi ", "  ").size());
  EXPECT_EQ(2U, UnicodeUtils::Split("a bc  d ef ghi ", "  ", 10).size());
  EXPECT_STREQ("a bc", UnicodeUtils::Split("a bc  d ef ghi ", "  ", 10).at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", " z").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", " z").at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", "").at(0).c_str());

  // test overload with char as delimiter
  EXPECT_EQ(4U, UnicodeUtils::Split("a bc  d ef ghi ", ' ', 4).size());
  EXPECT_STREQ("d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", ' ', 4).at(3).c_str());
  EXPECT_EQ(7U, UnicodeUtils::Split("a bc  d ef ghi ", ' ').size())
      << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(1).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(2).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(6).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", 'z').size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());
}

TEST(TestUnicodeUtils, FindNumber)
{
  EXPECT_EQ(3, UnicodeUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, UnicodeUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(TestUnicodeUtils, Collate)
{
  int32_t ref = 0;
  int32_t var;
  EXPECT_TRUE(Unicode::InitializeCollator(getUSEnglishLocale(), false));

  const std::wstring s1 = std::wstring(L"The Farmer's Daughter");
  const std::wstring s2 = std::wstring(L"Ate Pie");
  var = UnicodeUtils::Collate(s1, s2);
  EXPECT_GT(var, ref);

  EXPECT_TRUE(Unicode::InitializeCollator(getTurkicLocale(), true));
  const std::wstring s3 = std::wstring(
      Unicode::utf8_to_wstring(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  const std::wstring s4 = std::wstring(
      Unicode::utf8_to_wstring(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  var = UnicodeUtils::Collate(s3, s4);
  EXPECT_EQ(var, 0);

  EXPECT_TRUE(Unicode::InitializeCollator(getTurkicLocale(), false));
  var = UnicodeUtils::Collate(s3, s4); // No (extra) Normalization
  EXPECT_NE(var, 0);
}
TEST(TestUnicodeUtils, AlphaNumericCompare)
{
  int64_t ref;
  int64_t var;

  ref = 0;
  var = UnicodeUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}

TEST(TestUnicodeUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, UnicodeUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7 * 60, UnicodeUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7 * 60, UnicodeUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154 * 60, UnicodeUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1 * 60 + 1, UnicodeUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1 * 3600 + 5 * 60 + 2, UnicodeUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, UnicodeUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, UnicodeUtils::TimeStringToSeconds("ля-ля"));
}

TEST(TestUnicodeUtils, RemoveCRLF)
{
  std::string refstr;
  std::string varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  UnicodeUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, utf8_strlen)
{
  size_t ref;
  size_t var;

  ref = 9;
  var = UnicodeUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

#ifdef UNICODE_STRING_DISABLE
TEST(TestUnicodeUtils, SecondsToTimeString)
{
  std::string refstr;
  std::string varstr;

  refstr = "21:30:55";
  varstr = UnicodeUtils::SecondsToTimeString(77455);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}
#endif

TEST(TestUnicodeUtils, IsNaturalNumber)
{
  EXPECT_TRUE(UnicodeUtils::IsNaturalNumber("10"));
  EXPECT_TRUE(UnicodeUtils::IsNaturalNumber(" 10"));
  EXPECT_TRUE(UnicodeUtils::IsNaturalNumber("0"));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber(" 1 0"));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber("1.0"));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber("1.1"));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber("0x1"));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber("blah"));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber("120 h"));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber(" "));
  EXPECT_FALSE(UnicodeUtils::IsNaturalNumber(""));
}

TEST(TestUnicodeUtils, IsInteger)
{
  EXPECT_TRUE(UnicodeUtils::IsInteger("10"));
  EXPECT_TRUE(UnicodeUtils::IsInteger(" -10"));
  EXPECT_TRUE(UnicodeUtils::IsInteger("0"));
  EXPECT_FALSE(UnicodeUtils::IsInteger(" 1 0"));
  EXPECT_FALSE(UnicodeUtils::IsInteger("1.0"));
  EXPECT_FALSE(UnicodeUtils::IsInteger("1.1"));
  EXPECT_FALSE(UnicodeUtils::IsInteger("0x1"));
  EXPECT_FALSE(UnicodeUtils::IsInteger("blah"));
  EXPECT_FALSE(UnicodeUtils::IsInteger("120 h"));
  EXPECT_FALSE(UnicodeUtils::IsInteger(" "));
  EXPECT_FALSE(UnicodeUtils::IsInteger(""));
}

#ifdef UNICODE_STRING_DISABLE
TEST(TestUnicodeUtils, SizeToString)
{
  std::string ref;
  std::string var;

  ref = "2.00 GB";
  var = UnicodeUtils::SizeToString(2147483647);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "0.00 B";
  var = UnicodeUtils::SizeToString(0);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}
#endif

#ifdef UNICODE_STRING_DISABLE
TEST(TestUnicodeUtils, EmptyString)
{
  EXPECT_STREQ("", UnicodeUtils::Empty.c_str());
}
#endif

TEST(TestUnicodeUtils, FindWord)
{
  size_t ref;
  size_t var;

  ref = 5;
  var = UnicodeUtils::FindWord("test string", "string");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("12345string", "string");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "2012");
  EXPECT_EQ(ref, var);

  ref = std::string::npos;
  var = UnicodeUtils::FindWord("12345string", "ring");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("12345string", "345");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "e2012");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "12");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("anyt]h(z_iILi234#!? 1a34#56bbc7 ", "1a34#56bbc7");
  ref = 20;
  EXPECT_EQ(ref, var);
}

TEST(TestUnicodeUtils, FindWord_NonAscii)
{
  size_t ref;
  size_t var;

  ref = 2;
  var = UnicodeUtils::FindWord("我的视频", "视频");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("我的视频", "视");
  EXPECT_EQ(ref, var);
  ref = 6;
  var = UnicodeUtils::FindWord("Apple ple", "ple");
  EXPECT_EQ(ref, var);
  ref = 6;
  var = UnicodeUtils::FindWord("Äpfel.pfel", "pfel");
  EXPECT_EQ(ref, var);

  // Yea old Turkic I problem....
  ref = 11;
  var = UnicodeUtils::FindWord("abcçdefgğh ıİi jklmnoöprsştuüvyz", "ıiİ jklmnoöprsştuüvyz");
}

TEST(TestUnicodeUtils, FindEndBracket)
{
  int ref;
  int var;

  ref = 11;
  var = UnicodeUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(TestUnicodeUtils, DateStringToYYYYMMDD)
{
  int ref;
  int var;

  ref = 20120706;
  var = UnicodeUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(TestUnicodeUtils, WordToDigits)
{
  std::string ref;
  std::string var;

  ref = "8378 787464";
  var = "test string";
  UnicodeUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestUnicodeUtils, CreateUUID)
{
  std::cout << "CreateUUID(): " << UnicodeUtils::CreateUUID() << std::endl;
}

TEST(TestUnicodeUtils, ValidateUUID)
{
  EXPECT_TRUE(UnicodeUtils::ValidateUUID(UnicodeUtils::CreateUUID()));
}

TEST(TestUnicodeUtils, CompareFuzzy)
{
  double ref;
  double var;

  ref = 6.25;
  var = UnicodeUtils::CompareFuzzy("test string", "string test");
  EXPECT_EQ(ref, var);
}

TEST(TestUnicodeUtils, FindBestMatch)
{
  double refdouble;
  double vardouble;
  int refint;
  int varint;
  std::vector<std::string> strarray;

  refint = 3;
  refdouble = 0.5625;
  strarray.emplace_back("");
  strarray.emplace_back("a");
  strarray.emplace_back("e");
  strarray.emplace_back("es");
  strarray.emplace_back("t");
  varint = UnicodeUtils::FindBestMatch("test", strarray, vardouble);
  EXPECT_EQ(refint, varint);
  EXPECT_EQ(refdouble, vardouble);
}

TEST(TestUnicodeUtils, Paramify)
{
  const char* input = "some, very \\ odd \"string\"";
  const char* ref = "\"some, very \\\\ odd \\\"string\\\"\"";

  std::string result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}
TEST(TestUnicodeUtils, Tokenize)
{
  // \brief Split a string by the specified delimiters.

  //Splits a string using one or more delimiting characters, ignoring empty tokens.
  //Differs from Split() in two ways:
  //  1. The delimiters are treated as individual characters, rather than a single delimiting string.
  //  2. Empty tokens are ignored.
  // \return a vector of tokens

  std::vector<std::string> result;

  std::string input = "All good men:should not die!";
  std::string delimiters = "";
  result = UnicodeUtils::Tokenize(input, delimiters);
  EXPECT_EQ(1, result.size());
  EXPECT_STREQ("All good men:should not die!", result[0].c_str());

  delimiters = " :!";
  result = UnicodeUtils::Tokenize(input, delimiters);
  EXPECT_EQ(result.size(), 6);

  EXPECT_STREQ("All", result[0].c_str());
  EXPECT_STREQ("good", result[1].data());
  EXPECT_STREQ("men", result[2].data());
  EXPECT_STREQ("should", result[3].data());
  EXPECT_STREQ("not", result[4].data());
  EXPECT_STREQ("die", result[5].data());

  input = ":! All good men:should not die! :";
  result = UnicodeUtils::Tokenize(input, delimiters);
  EXPECT_EQ(result.size(), 6);

  EXPECT_STREQ("All", result[0].c_str());
  EXPECT_STREQ("good", result[1].data());
  EXPECT_STREQ("men", result[2].data());
  EXPECT_STREQ("should", result[3].data());
  EXPECT_STREQ("not", result[4].data());
  EXPECT_STREQ("die", result[5].data());
}

TEST(TestUnicodeUtils, sortstringbyname)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("B");
  strarray.emplace_back("c");
  strarray.emplace_back("a");
  std::sort(strarray.begin(), strarray.end(), sortstringbyname());

  EXPECT_STREQ("a", strarray[0].c_str());
  EXPECT_STREQ("B", strarray[1].c_str());
  EXPECT_STREQ("c", strarray[2].c_str());
}

TEST(TestUnicodeUtils, ToHexadecimal)
{
  EXPECT_STREQ("", UnicodeUtils::ToHexadecimal("").c_str());
  EXPECT_STREQ("616263", UnicodeUtils::ToHexadecimal("abc").c_str());
  std::string a
  { "a\0b\n", 4 };
  EXPECT_STREQ("6100620a", UnicodeUtils::ToHexadecimal(a).c_str());
  std::string nul
  { "\0", 1 };
  EXPECT_STREQ("00", UnicodeUtils::ToHexadecimal(nul).c_str());
  std::string ff
  { "\xFF", 1 };
  EXPECT_STREQ("ff", UnicodeUtils::ToHexadecimal(ff).c_str());
}
