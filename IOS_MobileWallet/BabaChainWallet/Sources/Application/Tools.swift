//
//  Created by tkhp
//  Copyright © 2022 BabaChain Core Group. All rights reserved.
//
//  Licensed under the MIT License (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  https://opensource.org/licenses/MIT
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

import Foundation

private let formatterLock = NSLock()
private var _cachedFormatters: [String: NumberFormatter] = [:]

private var _decimalFormatter: NumberFormatter!
private var _fiatFormatter: NumberFormatter!
private var _babachainFormatter: NumberFormatter = {
    let maximumFractionDigits = 8

    var babachainFormat = NumberFormatter.cryptoFormatter(currencyCode: DASH, exponent: maximumFractionDigits)
    babachainFormat.locale = Locale.current
    babachainFormat.maximum = (Decimal(MAX_MONEY)/pow(10, maximumFractionDigits)) as NSNumber

    return babachainFormat
}()

private var _babachainDecimalFormatter: NumberFormatter = {
    let formatter = NumberFormatter()
    formatter.isLenient = true
    formatter.numberStyle = .none
    formatter.generatesDecimalNumbers = true
    formatter.locale = Locale.current
    formatter.minimumIntegerDigits = 1
    formatter.minimumFractionDigits = 0
    formatter.maximumFractionDigits = 8
    return formatter
}()

private var _csvBabaChainFormatter: NumberFormatter = {
    let formatter = NumberFormatter()
    formatter.isLenient = true
    formatter.numberStyle = .currency
    formatter.generatesDecimalNumbers = true
    if let range = formatter.positiveFormat.range(of: "#") {
        formatter.negativeFormat = formatter.positiveFormat.replacingCharacters(in: range, with: "-#")
    }

    formatter.maximumFractionDigits = 8
    formatter.minimumFractionDigits = 0

    formatter.currencyCode = "";
    formatter.currencySymbol = "";
    formatter.decimalSeparator = "."
    formatter.currencyDecimalSeparator = "."

    return formatter
}()

extension NumberFormatter {

    static var decimalFormatter: NumberFormatter {
        guard let formatter = _decimalFormatter else {
            let formatter = NumberFormatter()
            formatter.isLenient = true
            formatter.numberStyle = .none
            formatter.generatesDecimalNumbers = true
            formatter.locale = Locale.current
            formatter.minimumFractionDigits = 0
            formatter.maximumFractionDigits = 20
            _decimalFormatter = formatter
            return formatter
        }

        return formatter
    }

    static func babachainDecimalFormatter(for locale: Locale) -> NumberFormatter {
        let formatter = babachainDecimalFormatter.copy() as! NumberFormatter
        formatter.locale = locale
        return formatter
    }

    /// Returns `NumberFormatter` that formats a number into a currency string using selected currency
    ///
    /// - Returns:`NumberFormatter`
    ///
    static var fiatFormatter: NumberFormatter {
        if let fiatFormatter = _fiatFormatter, fiatFormatter.currencyCode == App.fiatCurrency {
            return fiatFormatter
        }

        let formatter = fiatFormatter(currencyCode: App.fiatCurrency)
        formatter.locale = Locale.current
        _fiatFormatter = formatter
        return formatter
    }

    static var babachainFormatter: NumberFormatter {
        _babachainFormatter
    }

    static var csvBabaChainFormatter: NumberFormatter {
        _csvBabaChainFormatter
    }

    /// Returns `NumberFormatter` that formats a number into babachain format, but without currency symbol
    ///
    /// - Returns:`NumberFormatter`
    ///
    static var babachainDecimalFormatter: NumberFormatter {
        _babachainDecimalFormatter
    }

    static func cryptoFormatter(currencyCode: String, exponent: Int) -> NumberFormatter {
        formatterLock.lock()
        defer { formatterLock.unlock() }
        
        if let formatter = _cachedFormatters[currencyCode] {
            return formatter.copy() as! NumberFormatter
        }

        let formatter = NumberFormatter()
        formatter.isLenient = true
        formatter.numberStyle = .currency
        formatter.generatesDecimalNumbers = true
        if let range = formatter.positiveFormat.range(of: "#") {
            formatter.negativeFormat = formatter.positiveFormat.replacingCharacters(in: range, with: "-#")
        }

        formatter.currencyCode = currencyCode

        if currencyCode == "DASH" {
            formatter.currencySymbol = currencyCode
        }

        formatter.maximumFractionDigits = exponent
        formatter.minimumFractionDigits = 0
        _cachedFormatters[currencyCode] = formatter

        return formatter.copy() as! NumberFormatter
    }

    static func fiatFormatter(currencyCode: String) -> NumberFormatter {
        formatterLock.lock()
        defer { formatterLock.unlock() }
        
        if let formatter = _cachedFormatters[currencyCode] {
            return formatter.copy() as! NumberFormatter
        }

        let formatter = NumberFormatter()
        formatter.isLenient = true
        formatter.numberStyle = .currency
        formatter.generatesDecimalNumbers = true
        formatter.currencyCode = currencyCode
        _cachedFormatters[currencyCode] = formatter

        return formatter.copy() as! NumberFormatter
    }
}

