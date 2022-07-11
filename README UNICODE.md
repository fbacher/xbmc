
# This document summarizes the changes made by this put request.

This PR attempts to to address problems related to switching over to Unicode which began cropping up after the infamous "Turkic I" problem was discovered (see issue 19883 https://github.com/xbmc/xbmc/issues/19883). The patch supplied had a number of bad side effects including: 

1. Not completely fixing the problem 
2. Breaking a number of python addons (changed default file name encoding to ASCII, prevented addons from discovering language)

Note: This code has ONLY be tested with Ubuntu Linux 21.10

## Goals

Identify and fix the most significant Unicode issues impacting Kodi. (In decreasing priority):

1. Make Kodi usable in all languages: No crashes, hangs, unreadable screens
2. Make all text visible in the appropriate language
3. Process text without corruption; character conversions performed correctly and correct byte/codepoint/code unit handling
4. Acceptable Collation, Formatting and other text processing. A native reader would be tolerate the results
5. Limit impact on performance
6. Good text processing so that a native reader would not groan at the many weird English artifacts

Limit impact to Kodi

1. Preserve existing APIs as much as possible
2. Use only open source libraries which are mature, well tested, available on a wide number of platforms and well maintained
3. Limit performance and memory impact

Other

1. Fix related issues that were not yet discovered
2. Provide a framework that could be built on in future releases

## What is accomplished by this PR

- Turkic I problem resolved
- Numerous other Unicode issues in StringUtils resolved
    - Left, Right used in situations where # of bytes to keep were based on lengths of non-ASCII text (i.e. ASCII suffix, but utf8 prefix)
    - Trim had similar issues to Left, Right
    - DB sort can be by either legacy code or ICU locale-dependent collation, selectable by #define. Legacy sort is English-centric.
- Preliminary build scripts to build ICU for Linux, also able to use pre-built ICU.
- Moved impacted StringUtils methods to UnicodeUtils

# Remaining Issues that should be addressed prior to Release

1. Buy-in by stakeholders
2. Integrate properly with build (a project exists that provides a CMake wrapper to simplify integrating with other projects)
3. **Port to platforms other than Linux**
4. Evaluate Python impacts
    - Investigate FoldCase/Turkic I (settings access)
    - Verify that Kodi Locale and filename encoding are correct/discoverable
    - Verify that third party Python apps/libs get correct locale, encodings (example yt-dlp).

4. Investigate what changes need to be ported to addons/kodi-dev-kit/StringUtils
5. User Testing
6. Performance Measurement
7. Performance Improvements
8. Decide:
    - Whether to use Capitalize or TitleCase
    - Whether to use ICU Collator or legacy English-centric AlphanumericCompare
9. Investigate if AlphaNumericCollation is used and if ICU should be used instead.
10. Investigate Code that maps filenames to upper or lower case (presumably for [old?] windows support. Newer windows supports non-ASCII filenames and the old filename uppercase/lowercase rules can't be applied as before. I don't know enough about the various rules on the different Windows versions, but if at all possible, the OS should worry about filename collisions. Of course, there may be a very good reason why it is done the way that it is. However, a quick look suggests that some of the code uses uppercase and some lowercase. It should be consistent.
  
# Questionable Design Decisions

Here are a few design decisions that I am not happy about and searching for a bettter way to handle. 

1. UnicodeUtils::GetCharPosition string::npos is insufficient to determine which end of a string the position ran off of. Defined:
    - ERROR = std::string::npos;
    - BEFORE_START = std::string::npos - 1;
    - AFTER_END = std::string::npos - 2;
Probably should change to return a class containing position, status.
2. Need to eliminate thread_local for icu::Collator instance in Unicode.cpp. Either move to some pool or change SortUtils to save as instance variable which is a fair amount of work given that SortUtils is all static.
3. Perhaps being overly aggressive at converting functions to Unicode that we may not have to do at this time. I did this for two reasons: one, sometimes it is difficult to know if it is safe to not convert to Unicode, two, I tend to think it is best to consistently move to Unicode processing rather than special case things.

# Future

Additional items need to be investigated/addressed but probably outside of this release timeframe:

- CompareFuzzy, FindBestMatch were not changed, but should be. They are likely not going to work very well in non-English. In the short amount of time I spent  looking, I could not find a suitable C++ version. Python has some that may be    a good starting point. Google's BERT and TensorFlow may be useful to investigate. 
  
- ICU Format/VFormat likely has better capabilities than the ones in StringUtils.
- Eliminate ASCII-only APIs
- Performance
- Analyze performance. Unicode processing can be quite expensive. There are a number of ways that performance can be improved:
     - There are multiple representations of Unicode: utf-8, utf-16, utf-32 (native Unicode). Kodi uses utf-8 and utf-32 (on the few platforms that define wchar_t as 16 bits, then utf-16 is used). ICU internally prefers utf-16, but can frequently accept utf-8 and sometimes utf-32. Conversions are expensive in cpu and memory operations.
    - If Kodi were to use UTF-16/UnicodeStrings for strings then much of the conversion expense could be avoided. This would be a large change, but could be done incrementally.
    - Kodi sorting converts the UTF-8 from the database to UTF-32 (wstring) which if the ICU collator is used, is then coverted to UTF-16 ON EACH COMPARISON. Very wasteful   
    - Sorting: ICU provides the ability to generate a sort "key" that can be used in leu of the real Unicode key. This is supposed to be much faster, but incurs the cost of building and maintaining these keys.
     
# Best Practices

Best practices stress converting text to Unicode as soon as possible after reading from media and process in Unicode from then on until you must convert on output, or due to an API. Trying to process on the fly tends to lead to madness. We can't follow best practices at this time for practical reasons, but I think we should head in that direction.

# Chosen Technology: ICU Library

The Unicode Consortium's ICU library was chosen to provide Unicode processing capabilities beyond what is available in C++

### Pros

- Mature, widely available, portable, industrial strength, well supported
- Already used by several Kodi dependencies
- Existing PR to use in Kodi's Android port

### Cons
- Not the easiest library to use
- Heavyweight
- Support for UTF8, UTF16, UTF32, etc. makes the API a bit confusing to use. 
- Depth of support in the features is uneven
- Written by expert linguists, not expert C++ library designers

The biggest challenge is not having a clue about the variability of the rules of other languages and the approaches at addressing them.

### Alternatives

We might be able to work around things like the "Turkic-I" problem by performing the appropriate operations using ASCII functions or with an English Locale. But I don't think that we know all of the places in the code where this can occur. If a release MUST be made with minimal changes, then we could probably patch just the toLower calls that I converted to FoldCase and have them use the C locale for the toLower. But we will surely battle numerous places which leak corrupted "i" conversions.

My reading discourages trying to patch yourself around multi-lingual issues. It goes against best practices and the switching back and forth of encodings leads to madness and reduced performance.

# Approach:

I took advantage of Kodi centralizing much text processing in StringUtils and set about changing it to use ICU and then making any necessary changes to the code that used StringUtils. This reduced the impact to the Kodi code as a whole. Keep in mind that there are many places in the code that does NOT call StringUtils and I did not investigate these much. It will take considerable effort to go through them all.


## Common programming mistakes

- ToUpper, ToLower (and other operations) are Locale sensitive
- The length of strings can change as a result of ToUpper, ToLower, etc.
- In UTF-8 you CAN NOT process a byte at a time. You must process a codepoint at a time (minimum) but frequently the context around the codepoint also matters, depending upon locale.
- It may take multiple 32-bit Unicode codepoints to represent what we think of as a character (Grapheme).
- Different locales sort differently. The importance of accent marks varies
- The current locale can cause even ASCII characters to be processed in an unexpected manner. This occurred with the Turkic-I.
- You can't rely on ToUpper/ToLower to "normalize" a string to make it suitable for a key to a map, or a keyword match. The current locale can mess things up. This is even worse for non-ASCII characters. (I don't believe Kodi has anything preventing non-ASCII from being used as a key).
 
## Minor performance mistakes

Noted that SortUtils::RemoveArticles calls StartsWithNoCase with multiple case versions of same articles: "the", "The". This looks like a frequent operation, perhaps for every entry sorted. It would speed things up if it first reduced the list of "articles" to a unique, case-folded list.
   
Also, while not as pretty, code which calls xxNoCase functions multiple times with the same arguments, the code could instead call:
- fold case of each argument (the minimum number of times)
- call StringUtils.compare (folded_1, folded_2) instead of compareNoCase. This would avoid cost of redundant case folds. Note that this would work for StrtsWith, EndsWith, etc.
   
# Overview of Code changes:

## Major Changes.
   
### UnicodeUtils

With all of the changes, many which, to varying degrees, alter the existing API behavior, I have moved all of the changed apis from StringUtils to a new class, UnicodeUtils. This allows more latitude in changing the api, including bringing into conformance with Kodi coding guidelines.

Another class, Unicode, discussed after UnicodeUtils, is responsible for interacting with the icu library. It hides almost all ICU functions from the rest of the code. UnicodeUtils sits on top of Unicode.
   
Many StringUtils functions now use Unicode and moved to UnicodeUtils. I left functions that appeared to work ok or that I did not understand very well alone. Several functions were converted to Unicode, but some arguments are restricted to ASCII. This was done for practical reasons because changing would require changing the API. Further, searches through code revealed no cases where non-ASCII was passed. Added WARNING if arguments were non-ASCII.
   
Generally, the reason Unicode arguments could not be used is that adding/removing/replacing a non-ASCII character in string would change its length and API would not allow it.
   
Added UnicodeUtils::Collate functions to use the ICU collator instead of StringUtils::AlphaNumericCompare. StringUtils #define USE_ICU_COLLATOR determines which gets used. It is currently set to use the ICU collator. For English there are small differences, but the ICU version should work better for other languages. It can be further tailored. 

#### FoldCase

The single most important change was to create FoldCase and switch most uses of ToLower over to it. I would much prefer to change the arguments to comply with the current coding guidelines but was concerned about others not liking the change. ToLower modifies the passed in string in-place and is a void function. Would prefer to to return modified string and not modify passed in argument. I have a branch with the changes, but would have to review it again for correctness and performance. I think that there are about 450 calls impacted.

FoldCase is appropriate for 'normalizing' the case of a string for use as a map key or similar use. It is locale independent and it is able to handle quirks such as the German sharp-s character, which is equivalent to two lower case "s" characters. It also handles the Turkic I problem.

FoldCase won't magically solve every problem. It may not handle mapping more exotic accents, etc. properly (time will tell). I know that it does not strip all accents, but this may be appropriate. Anyway, developers would be wise to test anything that needs to be case folded that can have any arbitrary unicode character, at least until we get more comfortable with it.

#### ToUpper

Modified to use ICU ToUpper. Now locale sensitive. Added signatures that accept std::Locale and icu::Locale. The default signature (no locale specified) uses the current user's locale (g_langInfo.GetLocale()) to derive the icu::Locale to use.

#### ToLower

Similar to ToLower. 

#### TitleCase/ToCapitalize

ToCapitalize modified to handle Unicode strings.
   
ICU has a TitleCase function to do the same thing as ToCapitalize. It is locale sensitive. It does not pretend to be perfect. It does not use language specific dictionaries of words to or not-to capitalize (although we could add that capability, or otherwise modify the behavior).
   
Differences between ToCapitalize and TitleCase:
- eCase lower-cases all letters not upper-cased. 
- TitleCase "n.Y.C." would be "N.y.c." 
- TitleCase "n-y-c" becomes "N-Y-C" like ToCapitalize
- TitleCase takes the context of the string into account.

The first two issues could have custom code to fix. 
- It is difficult to make ToCapitalize correctly change case of multi-Unicode characters (a unicode primary letter, followed by accents or other modifiers). 
- You might get away with not modifying the accents.   
   
Is it worth it? 
ToCapitalize is called from three places:
- Onefrom SystemGUIInfo for Linux: GetWinSystem.GetName()
- Two from GUITextLayout if FONT_STYLE_CAPITALIZE is set for the string.
   
In my opinion, we should just use the ICU TitleCase and be done with it. If there is great howling, some custom code can be added FOR SPECIFIC Locales.
  
#### Normalize

The average user is not expected to utilize this method. It is provided here for the few cases that it is needed as well as to allow testing, since Normalization is used internally.

Unicode has the concept of Normalizing a Unicode string. This is needed because there is more than one way to specify a Grapheme/character. Here is an example of five different ways to specify the same Grapheme: ộ

- U+006f (o) + U+0302 (◌̂) + U+0323 (◌̣): o◌̣◌̂
- U+006f (o) + U+0323 (◌̣) + U+0302 (◌̂)
- U+00f4 (ô) + U+0323 (◌̣)
- U+1ecd (ọ) + U+0302 (◌̂)
- U+1ed9 (ộ)

Some codepoints represent complex characters, while other times a complex character is represented by multiple codepoints: a base codepoint (such as "o") plus additional codepoints which add accents or other graphic elements.. Sometimes a character can be represented both ways (as the above character is).

The graphic elements don't care what order they are specified. The rendering engine doesn't care either. However if characters are being compared, then the order is very important. Normalization orders the codepoints into a canonical order. In addition, with characters like the one above, normalization can expand or reduce the codepoints used to represent a character. 

Unfortunately, more than one kind of normalizer is needed to serve all purposes. For more information, and a deep dive, see: https://unicode-org.github.io/icu/userguide/transforms/normalization/ 

#### Equals

Modified to work with Unicode (was assuming that different lengths meant unequal, etc.).

#### EqualsNoCase

Modified to use Unicode caseless compare

#### Compare

Similar to Equals

#### InitializeCollator

In order to support sorting with ICU library, the icu Collator needs to be initialized on each sort. SortUtils was modified to call this prior to a sort. There is a #define in StringUtils.h which controls whether the legacy or icu collator is used.

#### SortCompleted

Primarily used to allow capture of performance information about a sort. SortUtils now calls this.

#### Collate

Performs an icu::Locale sensitive Collation of two values. The Collator is configurable and is currently set to reasonably match Kodi's Legacy StringUtils::AlphaNumericCompare. The Legacy collator is English-centric, whereas the icu version is tweaked for each Locale.
   
#### CompareNoCase  

Deprecated two CompareNoCase signatures that take a byte offset. No longer referenced by Kodi. Switched all but one use of compareNoCase use to StartsWithNoCase or EndsWithNoCase.
   
It may be feasible to use the existing UTF-8 code since most comparisons are with ASCII. This would be faster, but I am afraid that it can become a nightmare determining when it can be safely used. It could consume a lot of effort to maintain it.

#### FindWord
     
Renamed Findwords to FindWord

FindWords was not Unicode safe because it assumes ASCII and single byte characters. Recoded to use Unicode and renamed to FindWord, since it actually finds a single word. A change in behavior is that FindWords returned the offset into the string where the "word" was found. FindWord simply returns true or false. The only code that checked this value simply checked for non std::string::npos.
   
FindWord is called from:
- ApplicationSettingsHandling for DARWIN for one setting.
- GUIMediaWindow where it is used to filter, and appears to be done inefficiently. It is called on each keypress to add the key to the filter, but the filter is run every time a key is added, even though nothing is displayed until the dialog is dismissed. Of course there can be other uses.
- Finally, it is also made available in the addons/kodi-dev-kit/StringUtils (which I have avoided looking at). I find no uses of it in the Kodi source, but there could be third party references.
   
The old FindWords may work as-is, depending upon whether Unicode input is present. I suspect that Unicode is used in GUIMediaWindow, but more study is needed.
   
#### Left, Right, Mid

StringUtils.Left, Right, Mid count argument works great for ASCII, but not so much for multi-byte characters, which these functions were definitely used with. Reinterpreted count to mean characters so it could work independent of encoding. 

An additional complication is that some "characters" require multiple Unicode codepoints to represent. This requires the use of a character breakiterator, which is locale dependent.

Left(str, 4) means to keep the leftmost 4 characters of str. The actual use case has the "4" representing the number of specific characters in a prefix or suffix. There are occasions where the character count was the result of some calculation based on strlen, or some such which can not be used for Unicode. To addresses this, an argument was added to Left and Right which indicate whether the character count("4") represents the characters to keep or omit. For example, Left(str, 4, true) means to: "keep the first four leftmost characters of str" while Left(str, 4, false) means to: "omit the first four leftmost characters of str". This was the simplest change that I could think of that would allow Unicode to work.

#### GetCharPosition
    
New function utilized by Left, Right, Mid and others.  Uses the icu character break iterator to convert character offsets to UTF-8 byte or UTF-16 code-unit offsets. Able to get first or last byte/code-unit of multi-byte character from a character count from either the left or right end.

Planning to refactor it a bit so that creation of iterator is separate from using it. This will help cut down overhead for Trim with multiple trim characters. See Trim for more details.

It is still a bit difficult to tell when the character iterator should be used and when simple codepoint iteration is enough. My best understanding is that when knowing the number of characters is important, then you have to use the character iterator. When you can simply process codepoints without caring if a "character"/grapheme uses one or multiple codepoints, then you don't need the character iterator. In the case of Left, Right, Mid, you are given the number of characters as an argument. It is important that you determine how many bytes/codepoints each character uses. For comparisons, copies, etc. it doesn't matter.
    
##### Alternative:

If it is assumed that the character count represents ASCII characters (which seems to be the case today for Left, Right, Mid), then we could do away with character iterator and instead use absolute byte counts from the different reference points that GetCharPosition supports. I do not recommend this.

#### Trim:

Changed to work with Unicode. One significant change is the definition of whitespace is now more in line with Unicode. Prior to Kodi 20 Trim defined whitespace as: isspace() which is [ \t\n\v\f\r] and as well as other characters, depending upon locale. Now whitespace includes: [\t\n\f\r\p{Z}] where \p{Z} means characters with the "property Z". The full list is:

- 0009..000D    ; White_Space # Cc   [5] <control-0009>..<control-000D>
- 0020          ; White_Space # Zs       SPACE
- 0085          ; White_Space # Cc       <control-0085>
- 00A0          ; White_Space # Zs       NO-BREAK SPACE
- 1680          ; White_Space # Zs       OGHAM SPACE MARK
- 2000..200A    ; White_Space # Zs  [11] EN QUAD..HAIR SPACE
- 2028          ; White_Space # Zl       LINE SEPARATOR
- 2029          ; White_Space # Zp       PARAGRAPH SEPARATOR
- 202F          ; White_Space # Zs       NARROW NO-BREAK SPACE
- 205F          ; White_Space # Zs       MEDIUM MATHEMATICAL SPACE
- 3000          ; White_Space # Zs       IDEOGRAPHIC SPACE

Trim(str, trimChars) allows for both str and trimChars to be Unicode. A character breakiterator is used to distinguish between the different characters in trimChars. To simplify things, trimChars is broken up into a vector of trimStrings. A new signature for Trim was created Trim(str, vector<string> trimStrings) which is used by Trim(str, trimChars) as well as allows for strings, not just individual characters, to be trimmed. There may be little use for this, but it is free. 

It would be more efficient if the character breakiterator exposed by GetCharPosition could be better reused between calls. I should have this complete soon. This will make this, and hopefully other operations a little more efficient.

#### TrimLeft

TrimLeft(str) and TrimLeft(str, trimChars) are similar to Trim, above. The use the same backend.
   
#### TrimRight

Similar to TrimLeft.

####  Replace

Now Deprecated and replaced by FindAndReplace (below). 

Modified to work with Unicode. This function has a quirk that nothing appears to use. The quirk is returning the number of changes made by the call, which is somewhat expensive to do with the Unicode library. It turns out there is only ONE place in the code that uses the value and ONLY uses it to tell if any change was made. 

#### FindAndReplace

Implemented to um, replace Replace (above), except it returns a bool indicating if a change was made or not. It also makes the changes with a single call to the Unicode library instead a call for each occurrence of the search string.

#### RegexReplaceAll

New function. Replaces every occurrence of a regex pattern with a string in another string.

Added since there are multiple places where regex is used in Kodi. Have not made an attempt to evaluate whether any users of regex should use this version or not. Focus has been on StringUtils unless a bug causes attention to be directed elsewhere.

#### StartsWith

Modified to be Unicode safe.

#### StartsWithNoCase

Modified to utilize Unicode caseless compare and to be Unicode safe.

#### EndsWith

Modified to be Unicode safe.

#### EndsWithNoCase

Modified to utilize Unicode caseless compare and to be Unicode safe.

#### Split

Kodi offers multiple versions of Split. Each one requires string search and substring creation. The Unicode library is required to ensure that a split does not occur in the middle of multi-codepoint character. 

#### SplitMulti

SplitMulti has same Unicode issues as Split.

#### FindNumber

Finds the number of occurrences of a string in another string.

The Unicode library is required to ensure that string comparison is done on valid character boundaries.

#### AlphaNumericCompare

Simple wrapper class that either calls the icu::Collator or StringUtils::AlphaNumericCompare depending on a #define USE_ICU_COLLATOR


#### TimeStringToSeconds

Only in UnicodeUtils because it depends upon Split. It is not Unicode sensitive. Perhaps it should remain in StringUtils since the likelihood that it would be impacted by a subtle change in Split is not large.

#### RemoveCRLF

Only in UnicodeUtils because it depends upon TrimRight. It is not Unicode sensitive. Perhaps it should remain in StringUtils since the likelihood that it would be impacted by a subtle change in Split is not large.

#### ContainsNonAscii

Utility to detect when non-ASCII characters are in string

#### DateStringToYYYYMMDD

Only in UnicodeUtils because it depends upon Split. It is not Unicode sensitive. Perhaps it should remain in StringUtils since the likelihood that it would be impacted by a subtle change in Split is not large.

#### Paramify

Only in UnicodeUtils because it depends upon FindAndReplace. It is not Unicode sensitive. Perhaps it should remain in StringUtils since the likelihood that it would be impacted by a subtle change in FindAndReplace is not large.

#### sortstringbyname

Tiny operator which calls CompareNoCase. Was in StringUtils. May as well leave here.

## Functions not changed that should be reviewed

### AlphaNumericCompare

AlphaNumericCompare is used for sorting. Currently disabled by USE_ICU_COLLATOR. See Collator, above.

### AlphaNumericCollation
   
AlphaNumericCollation is used by sqlitedataset.cpp for database sorting, however, sqlitedataset sorting does not appear to be used. My memory is that it is disabled, rather sqllite is not told about it. 

AlphaNumericCollation seems to be a UTF-8 implementation of AlphaNumericCompare. Perhaps it should be #ifdef disabled in anticipation of removal, or at least detect when it is used.

### GetCollationWeight, planmap

GetCollationWeight, planmap and friends, which define the collation weights for Unicode characters. This is only used by AlphaNumericCompare. Using a private collation table makes me a bit nervous. It is not going to be Locale sensitive like the ICU one (however, for many languages there is little difference between the maps). It can make a significant difference. I have not confirmed, but French weighs accents more heavily than most. The accent can be more important than the character, while most languages consider it a minor variation in the character.

### UTF8ToUnicode
   
UTF8ToUnicode is only used by AlphaNumericCompare. I much prefer to use ICU's version. Propose removing after demonstrating that ICU's version is equivalent.

### Format & VFormat

These are not documented and I did not take the time to figure them out. ICU provides equivalent function but I did not want to  tackle converting the code at this time, given that it seems to be adequate. Further, the ICU code most likely adds additional function that could potentially cause problems. Also would have to investigate the use cases to make sure there are not problems.

### FormatFileSize

Formats file size to human readable form. Uses FIXED ENGLISH units (MB, KB).
Should use ICU formatter, or perhaps another. 

### ISODateToLocalizedDate

Should be reviewed to see if it is sufficient

### SecondsToTimeString

Should be reviewed to see if it is sufficient

### SizeToString

Should be reviewed to see if formatting is sufficient. Probably not.
   
## Unicode.cpp

Almost all of the code which interacts directly with the ICU library is contained in utils/Unicode. It attempts to isolate ICU from the rest of the code. There are several cases in which another new class, UnicodeUtils, access ICU classes, but with only several other exceptions, UnicodeUtils is the only one that does.
   
The code in Unicode is a **work in progress and subject to change** since I'm still learning the various APIs and the best ways to go about accomplishing things. In particular there are several ways to represent Unicode data that has performance and complexity impacts:

1. UnicodeString
Roughly equivalent to std::string. It has the most comprehensive support throughout the API and is easiest to use. It does involve more overhead to create since data must be converted to native UChar (UTF-16). There is a way to cause data to be stack allocated instead of heap, which I will be leveraging soon.
2. UChar*
Roughly equivalent to char*.
Still requires conversion to UChar, like UnicodeString. Does not appear too useful
3. UText
Newest API and intended the preferred way to go. Eventually. Very little of the API uses it now.
4. UTF-8
A few APIs allow for UTF-8 to be used with the intent that it is either used without conversion to UChar, or is converted to UChar on the fly as needed.
5. Other
Other encodings and APIs are supported, but without much support. 

With all of these different ways to do things means that it may take a while to settle on the best approach for the various tasks. Up until now the priority has been to have correct code to solve a problem with a secondary priority on performance, consistency, simplicity. I tend to think that the best approach is to utilize UnicodeStrings the most and to advocate that Kodi represent all non-ASCII in UnicodeStrings, or possibly a wrapper, adapter or facade around a UnicodeString. This follows Best Practices of converting to UnicodeString (or similar) as soon as data is read. Doing otherwise leads to repeated conversions to/from UTF8 and other madness. You quickly loose whatever small gain that you have in memory footprint and other advantages by UTF8. 

But I digress (again)....

What follows is a brief rundown of the Unicode functions and some of the details which may be of interest to those working on the PR.

### Enums

Borrowed some enums from ICU to bring in my namespace. Not highly likely that others
will need more than a couple of these. The Normalizer is the least likely for the casual user to use since 1) it is not often needed and 2) not easy to figure out when and how to use

1. StringOptions
2. RegexpFlag
3. NormalizerType

These constants discussed elsewhere. Probably a bad design. Need to be able to distinguish when character break iterator runs off left or right end of string so a simple string::npos is not enough. Perhaps should create class to contain return value of byte index as well as error status.

 ERROR = std::string::npos;
  static constexpr size_t BEFORE_START = std::string::npos - 1;
  static constexpr size_t AFTER_END = std::string::npos - 2;
  
### Data Conversion
These methods provide data character conversion
- UTF8ToWString
- WStringToUTF8
- StringToUChar
- WcharToUChar
- UCharToString
- UCharToWChar
- ToUnicodeString(wstring)
- ToUnicodeString(string)
- ToUnicodeString(StringPiece

### icu::Locale

These methods get the icu::Locale. 
- The default icu::Locale is derived from g_langInfo.GetLocale(). The language and country values are used to create the icu::Locale
-- Similarly, the icu::Locale can be derived from a std::locale
-- Or from passed in values

GetICULocaleId returns a string containing the language and country in a familiar format. Example: "en_US"


### Case Folding

See the description in UnicodeUtils class. Note that the implementations here do not alter the passed in string.

### Normalization

See the description in UnicodeUtils class.

### ToUpper

See the description in UnicodeUtils class.

### ToLower

See the description in UnicodeUtils class.

### ToCapitalize

See the description in UnicodeUtils class.

### ToTitle

See the description in UnicodeUtils class.

### StrCmp

Performs the work for UnicodeUtils string comparisons (except for caseless compare). Numerous options are provided 

### StrCaseCmp

Similar to StrCmp, except caseless comparisions are performed. Note that if Normalization is selected then the case folding is performed by the normalizer as the strings are compared, so theoretically only a partial string needs to be processed if the comparison fails early. Whereas with the default case folding the entire string is folded prior to the comparison.
      
### Collator

Several methods initialize and call a Collator for a configurable sort based on locale and other things. The Collator currently tries to mimic the Kodi legacy AlphaNumericCompare which basically sorts strings as you would expect, but numeric values as numbers and not strings (ex. "10" comes after "5"). The Legacy Collator does not take locale into account.

### StartsWith

See UnicodeUtils for a description.
  
### StartsWithNoCase

 See UnicodeUtils for a description.

### EndsWith

See UnicodeUtils for a description.

### EndsWithNoCase

See UnicodeUtils for a description.

### Left, Right, Mid

 See UnicodeUtils for a description. These all use Character BreakIterator and friends.

### Character BreakIterator

The next few methods create and use a Character BreakIterator

### ConfingUText

Configures a UText object to wrap a UTF-8 string. Currently only used for use with a Character BreakIterator

### ConfigCharBreakIter

Creates and initializes a Character BreakIterator for the given UText or UnicodeString

### ReconfigCharBreakIter

Updates the Character BreakIterator to use a new or modified string value

### GetCharPosition

Gets the code-unit (byte, codepoint or UChar) index value for the given character count and reference point in the given string

### Trim, TrimLeft, Trim

See UnicodeUtils for a description.

### Split

See UnicodeUtils::Split functions for more information

### SplitMulti

See UnicodeUtils for a description.

### FindAndReplace


### FindCountAndReplace

back-end for deprecated function UnicodeUtils::Replace. Also deprecated


### FindWord

See UnicodeUtils for more information

### RegexFind

Used internally to process regular expressions. Regular expression patterns for this lib can be found at: https://unicode-org.github.io/icu/userguide/strings/regexp.html

### RegexReplaceAll

See UnicodeUtils for more information.

### CountOccurances

See UnicodeUtils for more information.

### SplitTo

The SplitTo methods are the workers for the Split methods. SplitTo uses an iterator instead of an input string. EGLUtils uses this iterator SplitTo API. See UnicodeUtils for more info.

### Contains
   
  static bool Contains(const std::string &str, const std::vector<std::string> &keywords);

### IsLatinChar

private:
  static bool doneOnce;

  static bool IsLatinChar(UChar32 codepoint);

### Buffer Size Utilities

These utilities calculate a buffer large enough to accommodate the given
string size, plus some generous room for growth. These are short lived buffers
which generally live on the stack so the oversize is not a big deal.

- GetBasicUTF8BufferSize
- GetWcharToUCharBufferSize
- GetUCharBufferSize
- GetUCharWorkingSize
- GetUTF8BufferSize
- GetWCharBufferSize

### Regex functions

Created these initially since they seemed likely to be able to be flexible enough to use in multiple situations. Not all of these are currently in use. However, there is code in Kodi that uses regex.

- RegexReplaceAll

   
## StringUtils
   
### Functions not changed and appear safe

  
### Tokenize

Very similar to Split. The delimiters must be ASCII. All know calls use ASCII. Could easily change to accept Unicode.

#### FindNumber
   
FindNumber seems very specialized and only used in TestStringUtils. Candidate for removal after further investigation.

#### DateStringToYYYYMMDD

Does not appear to be for human consumption

#### IsNaturalNumber

Just checks to see if string matches [ \t]*[0-9]+[ \t]*

#### IsInteger

Just checks to see if string matches [ \t]*[\-]*[0-9]+[ \t]*

#### Join

Builds a string by appending every string from a container, separated by a delimiter string.

This looks Unicode safe since the code simply performs appends and erase of strings. It is not important to know what is in the strings.

#### asciixdigitvalue

Tiny function. Probably duplicates c++ builtin

#### BinaryStringToString

Converts a string of escaped digits into a string of digits:

"\0\5\3\2\8\7" => "053287"

Does not seem bullet-proof, but probably ok for its use case.

#### RemoveDuplicatedSpacesAndTabs

Converts tabs to spaces and then removes duplicate space characters from str in-place. Has no Unicode impact.

#### ToHexadecimal

Converts hex string into properly formatted hex string (adds /x and 0 fills).

#### WordToDigits

Converts ASCII letters and digits to a digits, perhaps for indexing? Need to
study what it does more, but not biggest priority.

#### CreateUUID

UUID creator. No Unicode

#### ValidateUUID

No Unicode

#### ToUint64

Not quite sure what this is doing. Converts a string to a 64-bit uint. 
Anyway, doesn't appear to be impacted by Unicode.

#### CreateFromCString

Simple helper class to create std::string from char*. Added value is to
set to empty string if char* is null.

## Python

### interfaces/python/XBPython

Modified to always set python to use utf8 file paths. This was one of the problems with the patch that was made to Kodi for Turkic I problem. That patch, which set locale to C had the side effect of forcing default locale for Python file paths to ASCII. This change will keep that from happening again even if Kodi started with LANG=C.

### interfaces/legacy/ModuleXbmc

**Added several APIs**

The first is experimental. I should move this to my private branch.

**getICULanguage** provides much basic info about the current Locale. It makes up for a buggy and insufficient getLanguage API that follows it. Fixing getLanguage would require changing its API. getICULanguage gets all of its information from icu::Locale. getLanguage gets its info from various Kodi sources and is incomplete and inaccurate.

**Modified getLanguage** in several minor ways to fix some of the deficiencies of ISO_639_1 and ISO_639_2


## ICU Library

ICU is available on a number of platforms. Several Kodi dependencies already use it. On 
Ubuntu 21.10, icu4c is provided by libicu-dev. icu-doc and icu-devtools are also available.
Currently icu67.1 is provided. 

ICU source and builds for some platforms can be found at: https://github.com/unicode-org/icu/releases
The current source is available from: 
	https://github.com/unicode-org/icu/releases/download/release-71-1/icu4c-71_1-src.tgz

Pre-built binaries are available for a several platforms at: 
	https://github.com/unicode-org/icu/releases/tag/release-71-1
	
My crude build scripts now assume that ICU is installed on the system (package name: libicu-dev).
Includes are at /usr/include/unicode (just use include path /usr/include). Shared libs
are at /usr/lib/x86_64-linux-gnu/. There are some Makefiles in the mix, which may
come in useful. 

By uncommenting some lines in the build scripts icu can be built from scratch. The major piece
that is missing is integrating icu with the automatic download and build in tools/depends/target. 
This does not look difficult since ICU provides cmake files in addition to make files.


# Examination of Turkic-I problem:

For the curious, here is some detail about the Turkic-I problem and the means to work around it using ICU. See https://www.unicode.org/versions/Unicode14.0.0/ch03.pdf also https://www.unicode.org/versions/Unicode14.0.0/ch05.pdf
 
##### ToLower & FOLD CASE behavior of the 4 versions of "I" used in Turkey. ICU supports two FOLD CASE methods. For our purpose, we want to use FOLD1 (aka FOLD_CASE_DEFAULT).


	Locale                    Unicode                                      	Unicode
	                        codepoint                                    (hex 32-bit codepoint(s))
	en_US I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
	tr_TR I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
	FOLD1 I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
	FOLD2 I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
   
	en_US i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
	tr_TR i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
	FOLD1 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
	FOLD2 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069   
	
	en_US İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
	tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069 
	FOLD1 İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
	FOLD2 İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069
	 
	en_US ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
	tr_TR ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
	FOLD1 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
	FOLD2 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
     
 **Notes:**
 
The specific problem that we have with Turkic-I is that when the Locale is set to tr_TR, then ToLower of ASCII I is Turkic "Dotless small I", but ToLower of ASCII i  is ASCII i. This breaks a lot of code.
 
FOLD_CASE_DEFAULT causes FoldCase to behave like ToLower for the "en" locale, while FOLD_CASE_SPECIAL_I causes FoldCase to behave like ToLower for the "tr_TR" locale.
 
This means that there is a choice: use FOLD_CASE_DEFAULT in order to get ASCII to fold  as expected but break Turkic Dotted I folding; or use FOLD_CASE_SPECIAL_I to get Turkic Dotted I to fold to Dotless small I but break ASCII folding.
 
This is a nasty wart. Possible solutions:
1. Only allow ASCII keywords. I suspect that non-ASCII keywords are already in use
2. Make sure any users of Turkic Dotted I in keywords are aware of the unusual behavior. They are fine as long as they only use dotless small I or ASCII I/i.
3. It is rare to have case folding problems like the Turkic I. However, developers that use non-ASCII will be wise to test carefully. As long as we use FoldCase, the behavior will be the same across all Locales. That is MUCH better than the rules changing.
 
# ICU Build

ICU is a heavyweight solution that is stable and was well tested. The library is icu4c library provided by unicode-org: https://unicode-org.github.io/icu/userguide/icu4c/

This library is already in use by a Kodi dependent, libsamba and libxml2 (version 67). The latest version from ICU is 71, which contains several minor improvements. I usually use a locally built version 71 to enable debugging and have full control over build options. But I can easily use the one distributed on Linux for the other Kodi dependencies, which is ICU 67.

ICU source and builds for some platforms can be found at: https://github.com/unicode-org/icu/releases. The current source is available from: 	https://github.com/unicode-org/icu/releases/download/release-71-1/icu4c-71_1-src.tgz Pre-built binaries are available for a several platforms at: 	https://github.com/unicode-org/icu/releases/tag/release-71-1
	
The current build scripts has several environment variables which allow you to point to different lib and include paths. 

On Linux, ICU's package name is libicu-dev. Its includes are at /usr/include/unicode (just use include path /usr/include). Shared libs are at /usr/lib/x86_64-linux-gnu/.

## Building ICU

You can search for icu or ICU in build_config.sh, build_icu.sh and build.sh. From that you can see where ICU is located in tools/depends/target, the configure options, etc..

## Building ICU with CMake

The git project: https://github.com/viaduck/icu-cmake looks very promising for building ICU. 
- You can specify pre-built (unsupported) that it downloads for you.
- You can specify the installed ICU on your system.
- You can tell it to download the source and build

I started a build using this CMake, but did not spend enough time with it to figure out how to integrate it into Kodi's build environment.

There are several other git CMake projects out there that may also be of some use.
 	
# Kodi & Python's use of Locale

I looked into the way Kodi and Python interact with C/C++ Locale. Here is what I found:

When you change your language/region in Kodi, it:
- clones the classic C locale for your new locale
- Creates a temporary locale for your language_region
- Replaces facets in the cloned locale with ones from your temporary locale
  The facets are ONLY for wchar_t operations impacting: collation, ctype and time format
- However, if the resulting locale does not have the decimal point as the numeric separator, then the classic C locale is cloned again and that is used instead. This leaves Kodi using a Locale which is largely a vanilla C local. This means that Kodi relies on code such as StringUtils to compensate. Further, a number of things don't apply to Kodi, such as monetary units (at least not yet).
    
Python ONLY has access to the traditional C locale mechanism. When Kodi's locale changes, Python picks up the changes. Further, any change to the locale in Python also impacts Kodi. I haven't yet tried clobbering the classic C locale from Python, but it may be possible. You can make changes to the active Kodi locale. This is reset when the user changes the locale again.
   