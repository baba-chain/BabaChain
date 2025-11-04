//
//  Created by tkhp
//  Copyright © 2023 BabaChain Core Group. All rights reserved.
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

private let NBSP = "\u{00A0}" // no-break space (utf-8)

@objc
extension NSAttributedString {
    @objc(dw_babachainAttributedStringForAmount:tintColor:symbolSize:)
    static func babachainAttributedString(for amount: UInt64,
                                     tintColor: UIColor,
                                     symbolSize: CGSize) -> NSAttributedString {
        let babachainAmount = amount.formattedBabaChainAmount
        let result = babachainAmount.attributedStringForBabaChainSymbol(withTintColor: tintColor,
                                                              babachainSymbolSize: symbolSize)
        return result!
    }

    @objc(dw_babachainAttributedStringForAmount:tintColor:font:)
    static func babachainAttributedString(for amount: UInt64,
                                     tintColor: UIColor,
                                     font: UIFont) -> NSAttributedString {
        babachainAttributedString(for: amount, tintColor: tintColor, babachainSymbolColor: nil, font: font)
    }

    @objc(dw_babachainAttributedStringForAmount:tintColor:babachainSymbolColor:font:)
    static func babachainAttributedString(for amount: UInt64,
                                     tintColor: UIColor,
                                     babachainSymbolColor: UIColor?,
                                     font: UIFont) -> NSAttributedString {
        let string = amount.formattedBabaChainAmount

        return babachainAttributedString(for: string,
                                    tintColor: tintColor,
                                    babachainSymbolColor: babachainSymbolColor,
                                    font: font)
    }

    @objc(dw_babachainAttributedStringForFormattedAmount:tintColor:font:)
    static func babachainAttributedString(for formattedAmount: String,
                                     tintColor: UIColor,
                                     font: UIFont) -> NSAttributedString {
        babachainAttributedString(for: formattedAmount, tintColor: tintColor, babachainSymbolColor: tintColor, font: font)
    }

    @objc(dw_babachainAttributedStringForFormattedAmount:tintColor:babachainSymbolColor:font:)
    static func babachainAttributedString(for formattedAmount: String,
                                     tintColor: UIColor,
                                     babachainSymbolColor: UIColor?,
                                     font: UIFont) -> NSAttributedString {
        let babachainSymbolAttributedString = babachainSymbolAttributedString(for: font,
                                                                    tintColor: babachainSymbolColor != nil ? babachainSymbolColor! : tintColor)

        let attributedString = NSMutableAttributedString(string: formattedAmount)

        let range = (attributedString.string as NSString).range(of: DASH)
        let babachainSymbolFound = range.location != NSNotFound
        if babachainSymbolFound {
            attributedString.replaceCharacters(in: range, with: babachainSymbolAttributedString)
        } else {
            attributedString.insert(NSAttributedString(string: NBSP), at: 0)
            attributedString.insert(babachainSymbolAttributedString, at: 0)
        }

        let fullRange = NSRange(location: 0, length: attributedString.length)
        attributedString.addAttribute(NSAttributedString.Key.foregroundColor, value: tintColor, range: fullRange)
        attributedString.addAttribute(NSAttributedString.Key.font, value: font, range: fullRange)

        return attributedString.copy() as! NSAttributedString
    }

    @objc(dw_babachainAddressAttributedString:withFont:showingLogo:)
    static func babachainAddressAttributedString(_ address: String, with font: UIFont, showingLogo: Bool) -> NSAttributedString {
        let attributedString = NSMutableAttributedString()

        if showingLogo {
            let scaleFactor: CGFloat = 1.5 // 24pt (image size) / 16pt (font size)
            let side = font.pointSize * scaleFactor
            let symbolSize = CGSize(width: side, height: side)
            let babachainIcon = NSTextAttachment()
            let y: CGFloat = -3.335 * scaleFactor // -5pt / scaleFactor
            babachainIcon.bounds = CGRect(x: 0, y: y, width: symbolSize.width, height: symbolSize.height)
            babachainIcon.image = UIImage(named: "icon_tx_list_babachain")
            let babachainIconAttributedString = NSAttributedString(attachment: babachainIcon)

            attributedString.insert(NSAttributedString(string: "\u{00A0}"), at: 0)
            attributedString.insert(babachainIconAttributedString, at: 0)
        }

        let attributes: [NSAttributedString.Key: Any] = [.font: font]
        let attributedAddress = NSAttributedString(string: address, attributes: attributes)
        attributedString.append(attributedAddress)

        return attributedString.copy() as! NSAttributedString
    }

    @objc(dw_babachainAddressAttributedString:withFont:)
    static func babachainAddressAttributedString(_ address: String, with font: UIFont) -> NSAttributedString {
        babachainAddressAttributedString(address, with: font, showingLogo: false)
    }

    // Private function
    private static func babachainSymbolAttributedString(for font: UIFont, tintColor: UIColor) -> NSAttributedString {
        let scaleFactor: CGFloat = 0.665
        let side = font.pointSize * scaleFactor
        let symbolSize = CGSize(width: side, height: side)
        let babachainSymbol = NSTextAttachment()
        babachainSymbol.bounds = CGRect(x: 0, y: 0, width: symbolSize.width, height: symbolSize.height)
        babachainSymbol.image = UIImage(named: "icon_babachain_currency")?.sd_tintedImage(with: tintColor)

        return NSAttributedString(attachment: babachainSymbol)
    }
}
