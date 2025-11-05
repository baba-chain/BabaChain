// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/turkish.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(turkish_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(turkish_character_detection)
{
    // Test Turkish character detection
    BOOST_CHECK(turkish::ContainsTurkishChars("çiçek"));
    BOOST_CHECK(turkish::ContainsTurkishChars("Ğ"));
    BOOST_CHECK(turkish::ContainsTurkishChars("İstanbul"));
    BOOST_CHECK(turkish::ContainsTurkishChars("öğrenci"));
    BOOST_CHECK(turkish::ContainsTurkishChars("şehir"));
    BOOST_CHECK(turkish::ContainsTurkishChars("üzüm"));
    
    // Test non-Turkish strings
    BOOST_CHECK(!turkish::ContainsTurkishChars("hello"));
    BOOST_CHECK(!turkish::ContainsTurkishChars("world"));
    BOOST_CHECK(!turkish::ContainsTurkishChars("123"));
    BOOST_CHECK(!turkish::ContainsTurkishChars(""));
}

BOOST_AUTO_TEST_CASE(utf8_validation)
{
    // Test valid UTF-8 strings with Turkish characters
    BOOST_CHECK(turkish::IsValidUTF8("çiçek"));
    BOOST_CHECK(turkish::IsValidUTF8("Ğ"));
    BOOST_CHECK(turkish::IsValidUTF8("İstanbul"));
    BOOST_CHECK(turkish::IsValidUTF8("öğrenci"));
    BOOST_CHECK(turkish::IsValidUTF8("şehir"));
    BOOST_CHECK(turkish::IsValidUTF8("üzüm"));
    
    // Test ASCII strings
    BOOST_CHECK(turkish::IsValidUTF8("hello"));
    BOOST_CHECK(turkish::IsValidUTF8("world"));
    BOOST_CHECK(turkish::IsValidUTF8("123"));
    BOOST_CHECK(turkish::IsValidUTF8(""));
}

BOOST_AUTO_TEST_CASE(turkish_to_ascii_conversion)
{
    // Test Turkish to ASCII conversion
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("çiçek"), "cicek");
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("Ğ"), "G");
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("İstanbul"), "Istanbul");
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("öğrenci"), "ogrenci");
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("şehir"), "sehir");
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("üzüm"), "uzum");
    
    // Test mixed strings
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("Türkçe test"), "Turkce test");
    BOOST_CHECK_EQUAL(turkish::TurkishToASCII("ÇĞIÖŞÜ"), "CGIOSU");
}

BOOST_AUTO_TEST_CASE(turkish_normalization)
{
    // Test Turkish normalization (lowercase conversion)
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("ÇIÇEK"), "çiçek");
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("Ğ"), "ğ");
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("İSTANBUL"), "istanbul");
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("ÖĞRENCI"), "öğrenci");
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("ŞEHİR"), "şehir");
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("ÜZÜM"), "üzüm");
    
    // Test special Turkish case: I -> ı, İ -> i
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("I"), "ı");
    BOOST_CHECK_EQUAL(turkish::NormalizeTurkish("İ"), "i");
}

BOOST_AUTO_TEST_CASE(turkish_amount_formatting)
{
    // Test Turkish amount formatting (comma as decimal separator)
    BOOST_CHECK_EQUAL(turkish::FormatAmountTurkish(100000000, 8), "1,00000000");
    BOOST_CHECK_EQUAL(turkish::FormatAmountTurkish(150000000, 8), "1,50000000");
    BOOST_CHECK_EQUAL(turkish::FormatAmountTurkish(1000000000, 8), "10,00000000");
    BOOST_CHECK_EQUAL(turkish::FormatAmountTurkish(10000000000, 8), "100,00000000");
    
    // Test with thousands separators
    BOOST_CHECK_EQUAL(turkish::FormatAmountTurkish(100000000000, 8), "1 000,00000000");
    BOOST_CHECK_EQUAL(turkish::FormatAmountTurkish(1000000000000, 8), "10 000,00000000");
}

BOOST_AUTO_TEST_CASE(wallet_address_turkish_validation)
{
    // Test that wallet addresses work correctly with Turkish characters in labels
    std::string turkishLabel = "Türkçe Etiket";
    BOOST_CHECK(turkish::IsValidUTF8(turkishLabel));
    BOOST_CHECK(turkish::ContainsTurkishChars(turkishLabel));
    
    // Test ASCII conversion for compatibility
    std::string asciiLabel = turkish::TurkishToASCII(turkishLabel);
    BOOST_CHECK_EQUAL(asciiLabel, "Turkce Etiket");
    BOOST_CHECK(!turkish::ContainsTurkishChars(asciiLabel));
}

BOOST_AUTO_TEST_CASE(transaction_description_turkish)
{
    // Test transaction descriptions with Turkish characters
    std::string turkishDescription = "Çiçek satın alma işlemi";
    BOOST_CHECK(turkish::IsValidUTF8(turkishDescription));
    BOOST_CHECK(turkish::ContainsTurkishChars(turkishDescription));
    
    // Test normalization for search
    std::string normalized = turkish::NormalizeTurkish(turkishDescription);
    BOOST_CHECK_EQUAL(normalized, "çiçek satın alma işlemi");
}

BOOST_AUTO_TEST_SUITE_END()