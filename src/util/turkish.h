// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BABACHAIN_UTIL_TURKISH_H
#define BABACHAIN_UTIL_TURKISH_H

#include <string>
#include <vector>

namespace turkish {

/**
 * Turkish character constants
 */
static const std::vector<std::string> TURKISH_CHARS = {
    "ç", "Ç", "ğ", "Ğ", "ı", "İ", "ö", "Ö", "ş", "Ş", "ü", "Ü"
};

/**
 * Check if a string contains Turkish characters
 * @param str The string to check
 * @return true if the string contains Turkish characters
 */
bool ContainsTurkishChars(const std::string& str);

/**
 * Validate that a string is properly UTF-8 encoded and can handle Turkish characters
 * @param str The string to validate
 * @return true if the string is valid UTF-8
 */
bool IsValidUTF8(const std::string& str);

/**
 * Convert Turkish characters to their ASCII equivalents for compatibility
 * @param str The string to convert
 * @return The converted string with ASCII equivalents
 */
std::string TurkishToASCII(const std::string& str);

/**
 * Normalize Turkish string for case-insensitive comparison
 * @param str The string to normalize
 * @return The normalized string
 */
std::string NormalizeTurkish(const std::string& str);

/**
 * Check if Turkish locale is available on the system
 * @return true if Turkish locale is available
 */
bool IsTurkishLocaleAvailable();

/**
 * Format amount with Turkish locale formatting (comma as decimal separator)
 * @param amount The amount to format
 * @param decimals Number of decimal places
 * @return Formatted string with Turkish formatting
 */
std::string FormatAmountTurkish(int64_t amount, int decimals = 8);

} // namespace turkish

#endif // BABACHAIN_UTIL_TURKISH_H