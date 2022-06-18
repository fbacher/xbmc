/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StringValidation.h"

#include "utils/StringUtils.h"
#include "utils/UnicodeUtils.h"

bool StringValidation::IsInteger(const std::string &input, void *data)
{
  return StringUtils::IsInteger(input);
}

bool StringValidation::IsPositiveInteger(const std::string &input, void *data)
{
  return StringUtils::IsNaturalNumber(input);
}

bool StringValidation::IsTime(const std::string &input, void *data)
{
  std::string strTime = UnicodeUtils::Trim(input);

  if (StringUtils::EndsWithNoCase(strTime, " min"))
  {
    strTime = UnicodeUtils::TrimRight(UnicodeUtils::Left(strTime, 4, false));

    return IsPositiveInteger(strTime, NULL);
  }
  else
  {
    // support [[HH:]MM:]SS
    std::vector<std::string> bits = UnicodeUtils::Split(input, ":");
    if (bits.size() > 3)
      return false;

    for (std::vector<std::string>::const_iterator i = bits.begin(); i != bits.end(); ++i)
      if (!IsPositiveInteger(*i, NULL))
        return false;

    return true;
  }
  return false;
}
