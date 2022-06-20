/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <locale>
#include "utils/StringUtils.h"

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

namespace TestStringUtils
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

TEST(TestStringUtils, Format)
{
  std::string refstr = "test 25 2.7 ff FF";

  std::string varstr = StringUtils::Format("{} {} {:.1f} {:x} {:02X}", "test", 25, 2.743f, 0x00ff,
      0x00ff);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = StringUtils::Format("", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ("", varstr.c_str());
}

TEST(TestStringUtils, FormatEnum)
{
  const char* zero = "0";
  const char* one = "1";

  std::string varstr = StringUtils::Format("{}", ECG::A);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = StringUtils::Format("{}", EG::C);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = StringUtils::Format("{}", test_enum::ECN::A);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = StringUtils::Format("{}", test_enum::EN::C);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(TestStringUtils, FormatEnumWidth)
{
  const char* one = "01";

  std::string varstr = StringUtils::Format("{:02d}", ECG::B);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = StringUtils::Format("{:02}", EG::D);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(TestStringUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(TestStringUtils, EqualsNoCase_Normalize)
{
  const std::string refstr = TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5;
  const std::string varstr = std::string(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  EXPECT_FALSE(UnicodeUtils::EqualsNoCase(refstr, varstr));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, varstr, StringOptions::FOLD_CASE_DEFAULT, true));
}

TEST(TestStringUtils, Replace)
{
  std::string refstr = "text text";

  std::string varstr = "test test";
  EXPECT_EQ(StringUtils::Replace(varstr, 's', 'x'), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(StringUtils::Replace(varstr, 's', 'x'), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = "test test";
  EXPECT_EQ(StringUtils::Replace(varstr, "s", "x"), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(StringUtils::Replace(varstr, "s", "x"), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, StartsWith)
{
  std::string refstr = "test";

  // TODO: Test with FoldCase option
  EXPECT_FALSE(StringUtils::StartsWithNoCase(refstr, "x"));

  EXPECT_TRUE(StringUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::StartsWith(refstr, "Te"));

  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "TesT"));
}

TEST(TestStringUtils, EndsWith)
{
  std::string refstr = "test";
  EXPECT_TRUE(StringUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(StringUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::EndsWith(refstr, "sT"));
}

TEST(TestStringUtils, EndsWithNoCase)
{
  // TODO: Test with FoldCase option

  std::string refstr = "test";
  EXPECT_FALSE(StringUtils::EndsWithNoCase(refstr, "x"));
  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "TesT"));
}

TEST(TestStringUtils, Join)
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
  varstr = StringUtils::Join(strarray, ",");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, FindNumber)
{
  EXPECT_EQ(3, StringUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, StringUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(TestStringUtils, Collate)
{
  int32_t ref = 0;
  int32_t var;
  EXPECT_TRUE(Unicode::InitializeCollator(getUSEnglishLocale(), false));

  const std::wstring s1 = std::wstring(L"The Farmer's Daughter");
  const std::wstring s2 = std::wstring(L"Ate Pie");
  var = StringUtils::Collate(s1, s2);
  EXPECT_GT(var, ref);

  EXPECT_TRUE(Unicode::InitializeCollator(getTurkicLocale(), true));
  const std::wstring s3 = std::wstring(
      Unicode::utf8_to_wstring(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  const std::wstring s4 = std::wstring(
      Unicode::utf8_to_wstring(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  var = StringUtils::Collate(s3, s4);
  EXPECT_EQ(var, 0);

  EXPECT_TRUE(Unicode::InitializeCollator(getTurkicLocale(), false));
  var = StringUtils::Collate(s3, s4); // No (extra) Normalization
  EXPECT_NE(var, 0);
}
TEST(TestStringUtils, AlphaNumericCompare)
{
  int64_t ref;
  int64_t var;

  ref = 0;
  var = StringUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}

TEST(TestStringUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, StringUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7 * 60, StringUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7 * 60, StringUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154 * 60, StringUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1 * 60 + 1, StringUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4 * 60 + 3, StringUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, StringUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, StringUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, StringUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1 * 3600 + 5 * 60 + 2, StringUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, StringUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, StringUtils::TimeStringToSeconds("ля-ля"));
}

TEST(TestStringUtils, RemoveCRLF)
{
  std::string refstr;
  std::string varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  StringUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, utf8_strlen)
{
  size_t ref;
  size_t var;

  ref = 9;
  var = StringUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, SecondsToTimeString)
{
  std::string refstr;
  std::string varstr;

  refstr = "21:30:55";
  varstr = StringUtils::SecondsToTimeString(77455);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, IsNaturalNumber)
{
  EXPECT_TRUE(StringUtils::IsNaturalNumber("10"));
  EXPECT_TRUE(StringUtils::IsNaturalNumber(" 10"));
  EXPECT_TRUE(StringUtils::IsNaturalNumber("0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(" 1 0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("1.0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("1.1"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("0x1"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("blah"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("120 h"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(" "));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(""));
}

TEST(TestStringUtils, IsInteger)
{
  EXPECT_TRUE(StringUtils::IsInteger("10"));
  EXPECT_TRUE(StringUtils::IsInteger(" -10"));
  EXPECT_TRUE(StringUtils::IsInteger("0"));
  EXPECT_FALSE(StringUtils::IsInteger(" 1 0"));
  EXPECT_FALSE(StringUtils::IsInteger("1.0"));
  EXPECT_FALSE(StringUtils::IsInteger("1.1"));
  EXPECT_FALSE(StringUtils::IsInteger("0x1"));
  EXPECT_FALSE(StringUtils::IsInteger("blah"));
  EXPECT_FALSE(StringUtils::IsInteger("120 h"));
  EXPECT_FALSE(StringUtils::IsInteger(" "));
  EXPECT_FALSE(StringUtils::IsInteger(""));
}

TEST(TestStringUtils, SizeToString)
{
  std::string ref;
  std::string var;

  ref = "2.00 GB";
  var = StringUtils::SizeToString(2147483647);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "0.00 B";
  var = StringUtils::SizeToString(0);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, EmptyString)
{
  EXPECT_STREQ("", StringUtils::Empty.c_str());
}

TEST(TestStringUtils, FindWord)
{
  size_t ref;
  size_t var;

  ref = 5;
  var = StringUtils::FindWord("test string", "string");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWord("12345string", "string");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWord("apple2012", "2012");
  EXPECT_EQ(ref, var);

  ref = std::string::npos;
  var = StringUtils::FindWord("12345string", "ring");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWord("12345string", "345");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWord("apple2012", "e2012");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWord("apple2012", "12");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWord("anyt]h(z_iILi234#!? 1a34#56bbc7 ", "1a34#56bbc7");
  ref = 20;
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindWord_NonAscii)
{
  size_t ref;
  size_t var;

  ref = 2;
  var = StringUtils::FindWord("我的视频", "视频");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWord("我的视频", "视");
  EXPECT_EQ(ref, var);
  ref = 6;
  var = StringUtils::FindWord("Apple ple", "ple");
  EXPECT_EQ(ref, var);
  ref = 6;
  var = StringUtils::FindWord("Äpfel.pfel", "pfel");
  EXPECT_EQ(ref, var);

  // Yea old Turkic I problem....
  ref = 11;
  var = StringUtils::FindWord("abcçdefgğh ıİi jklmnoöprsştuüvyz", "ıiİ jklmnoöprsştuüvyz");
}

TEST(TestStringUtils, FindEndBracket)
{
  int ref;
  int var;

  ref = 11;
  var = StringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, DateStringToYYYYMMDD)
{
  int ref;
  int var;

  ref = 20120706;
  var = StringUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, WordToDigits)
{
  std::string ref;
  std::string var;

  ref = "8378 787464";
  var = "test string";
  StringUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, CreateUUID)
{
  std::cout << "CreateUUID(): " << StringUtils::CreateUUID() << std::endl;
}

TEST(TestStringUtils, ValidateUUID)
{
  EXPECT_TRUE(StringUtils::ValidateUUID(StringUtils::CreateUUID()));
}

TEST(TestStringUtils, CompareFuzzy)
{
  double ref;
  double var;

  ref = 6.25;
  var = StringUtils::CompareFuzzy("test string", "string test");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindBestMatch)
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
  varint = StringUtils::FindBestMatch("test", strarray, vardouble);
  EXPECT_EQ(refint, varint);
  EXPECT_EQ(refdouble, vardouble);
}

TEST(TestStringUtils, Paramify)
{
  const char* input = "some, very \\ odd \"string\"";
  const char* ref = "\"some, very \\\\ odd \\\"string\\\"\"";

  std::string result = StringUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}
TEST(TestStringUtils, Tokenize)
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
  result = StringUtils::Tokenize(input, delimiters);
  EXPECT_EQ(1, result.size());
  EXPECT_STREQ("All good men:should not die!", result[0].c_str());

  delimiters = " :!";
  result = StringUtils::Tokenize(input, delimiters);
  EXPECT_EQ(result.size(), 6);

  EXPECT_STREQ("All", result[0].c_str());
  EXPECT_STREQ("good", result[1].data());
  EXPECT_STREQ("men", result[2].data());
  EXPECT_STREQ("should", result[3].data());
  EXPECT_STREQ("not", result[4].data());
  EXPECT_STREQ("die", result[5].data());

  input = ":! All good men:should not die! :";
  result = StringUtils::Tokenize(input, delimiters);
  EXPECT_EQ(result.size(), 6);

  EXPECT_STREQ("All", result[0].c_str());
  EXPECT_STREQ("good", result[1].data());
  EXPECT_STREQ("men", result[2].data());
  EXPECT_STREQ("should", result[3].data());
  EXPECT_STREQ("not", result[4].data());
  EXPECT_STREQ("die", result[5].data());
}

TEST(TestStringUtils, sortstringbyname)
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

TEST(TestStringUtils, FileSizeFormat)
{
  EXPECT_STREQ("0B", StringUtils::FormatFileSize(0).c_str());

  EXPECT_STREQ("999B", StringUtils::FormatFileSize(999).c_str());
  EXPECT_STREQ("0.98kB", StringUtils::FormatFileSize(1000).c_str());

  EXPECT_STREQ("1.00kB", StringUtils::FormatFileSize(1024).c_str());
  EXPECT_STREQ("9.99kB", StringUtils::FormatFileSize(10229).c_str());

  EXPECT_STREQ("10.1kB", StringUtils::FormatFileSize(10387).c_str());
  EXPECT_STREQ("99.9kB", StringUtils::FormatFileSize(102297).c_str());

  EXPECT_STREQ("100kB", StringUtils::FormatFileSize(102400).c_str());
  EXPECT_STREQ("999kB", StringUtils::FormatFileSize(1023431).c_str());

  EXPECT_STREQ("0.98MB", StringUtils::FormatFileSize(1023897).c_str());
  EXPECT_STREQ("0.98MB", StringUtils::FormatFileSize(1024000).c_str());

  //Last unit should overflow the 3 digit limit
  EXPECT_STREQ("5432PB", StringUtils::FormatFileSize(6115888293969133568).c_str());
}

TEST(TestStringUtils, ToHexadecimal)
{
  EXPECT_STREQ("", StringUtils::ToHexadecimal("").c_str());
  EXPECT_STREQ("616263", StringUtils::ToHexadecimal("abc").c_str());
  std::string a
  { "a\0b\n", 4 };
  EXPECT_STREQ("6100620a", StringUtils::ToHexadecimal(a).c_str());
  std::string nul
  { "\0", 1 };
  EXPECT_STREQ("00", StringUtils::ToHexadecimal(nul).c_str());
  std::string ff
  { "\xFF", 1 };
  EXPECT_STREQ("ff", StringUtils::ToHexadecimal(ff).c_str());
}
