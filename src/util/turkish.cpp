// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/turkish.h>
#include <util/strencodings.h>

#include <algorithm>
#include <locale>
#include <codecvt>
#include <iomanip>
#include <sstream>

namespace turkish {

bool ContainsTurkishChars(const std::string& str)
{
    for (const auto& turkishChar : TURKISH_CHARS) {
        if (str.find(turkishChar) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool IsValidUTF8(const std::string& str)
{
    try {
        // Try to convert to UTF-8 and back
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wide = converter.from_bytes(str);
        std::string back = converter.to_bytes(wide);
        return back == str;
    } catch (const std::exception&) {
        return false;
    }
}

std::string TurkishToASCII(const std::string& str)
{
    std::string result = str;
    
    // Replace Turkish characters with ASCII equivalents
    const std::vector<std::pair<std::string, std::string>> replacements = {
        {"ç", "c"}, {"Ç", "C"},
        {"ğ", "g"}, {"Ğ", "G"},
        {"ı", "i"}, {"İ", "I"},
        {"ö", "o"}, {"Ö", "O"},
        {"ş", "s"}, {"Ş", "S"},
        {"ü", "u"}, {"Ü", "U"}
    };
    
    for (const auto& replacement : replacements) {
        size_t pos = 0;
        while ((pos = result.find(replacement.first, pos)) != std::string::npos) {
            result.replace(pos, replacement.first.length(), replacement.second);
            pos += replacement.second.length();
        }
    }
    
    return result;
}

std::string NormalizeTurkish(const std::string& str)
{
    std::string result = str;
    
    // Convert to lowercase using Turkish rules
    const std::vector<std::pair<std::string, std::string>> turkishLowercase = {
        {"Ç", "ç"}, {"Ğ", "ğ"}, {"İ", "i"}, {"I", "ı"},
        {"Ö", "ö"}, {"Ş", "ş"}, {"Ü", "ü"}
    };
    
    for (const auto& conversion : turkishLowercase) {
        size_t pos = 0;
        while ((pos = result.find(conversion.first, pos)) != std::string::npos) {
            result.replace(pos, conversion.first.length(), conversion.second);
            pos += conversion.second.length();
        }
    }
    
    // Convert ASCII characters to lowercase
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    
    return result;
}

bool IsTurkishLocaleAvailable()
{
    try {
        std::locale turkishLocale("tr_TR.UTF-8");
        return true;
    } catch (const std::exception&) {
        try {
            std::locale turkishLocale("Turkish");
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
}

std::string FormatAmountTurkish(int64_t amount, int decimals)
{
    std::ostringstream oss;
    
    // Set Turkish locale if available
    if (IsTurkishLocaleAvailable()) {
        try {
            oss.imbue(std::locale("tr_TR.UTF-8"));
        } catch (const std::exception&) {
            // Fallback to manual formatting
        }
    }
    
    // Convert to decimal representation
    double decimalAmount = static_cast<double>(amount) / std::pow(10, decimals);
    
    // Format with Turkish decimal separator (comma)
    oss << std::fixed << std::setprecision(decimals) << decimalAmount;
    std::string result = oss.str();
    
    // Replace decimal point with comma for Turkish formatting
    size_t dotPos = result.find('.');
    if (dotPos != std::string::npos) {
        result[dotPos] = ',';
    }
    
    // Add thousands separators (thin space)
    size_t commaPos = result.find(',');
    if (commaPos == std::string::npos) {
        commaPos = result.length();
    }
    
    // Add thin space every 3 digits from right to left
    for (int i = commaPos - 3; i > 0; i -= 3) {
        result.insert(i, " ");
    }
    
    return result;
}

} // namespace turkish