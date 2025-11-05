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

// MARK: - TransactionDataItem

protocol TransactionDataItem {
    var outputReceiveAddresses: [String] { get }
    var inputSendAddresses: [String] { get }
    var specialInfoAddresses: [String: Int]? { get }
    var txHashHexString: String { get }
    var babachainAmount: UInt64 { get }
    var signedBabaChainAmount: Int64 { get }
    var direction: DSTransactionDirection { get }
    var fiatAmount: String { get }
    var iconName: String { get }
    var stateTitle: String { get }
    var shortDateString: String { get }
}

extension TransactionDataItem {
    var formattedBabaChainAmountWithDirectionalSymbol: String {
        guard babachainAmount != UInt64.max else {
            return NSLocalizedString("Syncing...", comment: "Transaction/Amount")
        }

        let formatted = babachainAmount.formattedBabaChainAmount

        if formatted.isCurrencySymbolAtTheBeginning {
            return direction.directionSymbol + " " + babachainAmount.formattedBabaChainAmount
        } else {
            return direction.directionSymbol + babachainAmount.formattedBabaChainAmount
        }
    }

    func attributedBabaChainAmount(with font: UIFont, color: UIColor = .dw_label()) -> NSAttributedString {
        guard babachainAmount != UInt64.max else {
            return NSAttributedString(string: NSLocalizedString("Syncing...", comment: "Transaction/Amount"))
        }

        let formatted = formattedBabaChainAmountWithDirectionalSymbol
        return formatted.attributedAmountStringWithBabaChainSymbol(tintColor: color, babachainSymbolColor: color, font: font)
    }
}
