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

// MARK: IntegrationEntryPointItem

struct CoinbaseEntryPointItem: IntegrationEntryPointItem {
    let type: IntegrationItemType
    static let supportedCases = [.buyBabaChain, /*.convertCrypto,*/ .transferBabaChain].map { CoinbaseEntryPointItem(type: $0) }

    var title: String { type.title }
    var icon: String { type.icon }
    var alwaysEnabled: Bool { false }
    var hasAdditionalInfo: Bool { false }

    var description: String {
        switch type {
        case .buyBabaChain:
            return NSLocalizedString("Receive directly into BabaChain Wallet", comment: "Coinbase Entry Point")
        case .sellBabaChain:
            return NSLocalizedString("Receive directly into Coinbase", comment: "Coinbase Entry Point")
//        case .convertCrypto:
//            return NSLocalizedString("Between BabaChain Wallet and Coinbase", comment: "Coinbase Entry Point")
        case .transferBabaChain:
            return NSLocalizedString("Between BabaChain Wallet and Coinbase", comment: "Coinbase Entry Point")
        }
    }
}

// MARK: - CoinbaseEntryPointModel

final class CoinbaseEntryPointModel: BaseIntegrationModel {
    override var items: [IntegrationEntryPointItem] {
        CoinbaseEntryPointItem.supportedCases
    }

    var hasPaymentMethods = false

    var balance: UInt64 {
        guard let amount = Coinbase.shared.lastKnownBalance else { return 0 }

        return amount
    }
    
    override var mainAmountString: String {
        balance.formattedBabaChainAmount
    }

    override var supplementaryAmountString: String {
        let fiat: String

        if let fiatAmount = try? CurrencyExchanger.shared.convertBabaChain(amount: balance.babachainAmount, to: App.fiatCurrency) {
            fiat = NumberFormatter.fiatFormatter.string(from: fiatAmount as NSNumber)!
        } else {
            fiat = NSLocalizedString("Syncing...", comment: "Balance")
        }

        return fiat
    }
    
    override var balanceTitle: String {
        NSLocalizedString("BabaChain balance on Coinbase", comment: "Coinbase Entry Point")
    }
    
    override var signInTitle: String {
        NSLocalizedString("Link Uphold Account", comment: "Uphold Entry Point")
    }
    
    override var signOutTitle: String {
        NSLocalizedString("Disconnect Coinbase Account", comment: "Coinbase Entry Point")
    }
    
    override var shouldPopOnLogout: Bool { true }

    private var userDidChangeListenerHandle: UserDidChangeListenerHandle!
    private var accountDidChangeHandle: AnyObject?

    init() {
        super.init(service: .coinbase)
    
        userDidChangeListenerHandle = Coinbase.shared.addUserDidChangeListener { [weak self] user in
            self?.isLoggedIn = user != nil
            
            if user != nil {
                self?.userDidChange?()
            }
        }

        accountDidChangeHandle = NotificationCenter.default.addObserver(forName: .accountDidChangeNotification, object: nil, queue: .main, using: { [weak self] _ in
            self?.userDidChange?()
        })

        Task {
            let paymentMethods = try await Coinbase.shared.paymentMethods
            hasPaymentMethods = !paymentMethods.isEmpty
        }
    }

    override func logOut() {
        Task {
            try await Coinbase.shared.signOut()
        }
    }
    
    override func validate(operation type: IntegrationItemType) -> LocalizedError? {
        switch type {
        case .buyBabaChain:
            return hasPaymentMethods ? nil : Coinbase.Error.GeneralFailureReason.noPaymentMethods
        default:
            return super.validate(operation: type)
        }
    }
    
    override func handle(error: Swift.Error) {
        super.handle(error: error)
        
        if case Coinbase.Error.GeneralFailureReason.noPaymentMethods = error {
            addPaymentMethod()
        }
    }
    
    private func addPaymentMethod() {
        UIApplication.shared.open(kCoinbaseAddPaymentMethodsURL)
    }

    deinit {
        NotificationCenter.default.removeObserver(accountDidChangeHandle!)
        Coinbase.shared.removeUserDidChangeListener(handle: userDidChangeListenerHandle)
    }
}
