//
//  Created by PT
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
import Combine
import AuthenticationServices

private let kWithdrawalsCapability = "crypto_withdrawals"
private let kProfileUrl = "https://wallet.uphold.com/babachainboard/settings/profile"

// MARK: IntegrationEntryPointItem

struct UpholdEntryPointItem: IntegrationEntryPointItem {
    let type: IntegrationItemType
    static let supportedCases = [.buyBabaChain, .transferBabaChain].map { UpholdEntryPointItem(type: $0) }
    
    var title: String { type.title }
    var icon: String { type.icon }
    var alwaysEnabled: Bool { type == .buyBabaChain }
    var hasAdditionalInfo: Bool { type == .buyBabaChain }

    var description: String {
        switch type {
        case .buyBabaChain:
            return NSLocalizedString("Receive directly into BabaChain Wallet", comment: "Uphold Entry Point")
        case .transferBabaChain:
            return NSLocalizedString("From Uphold to BabaChain Wallet", comment: "Uphold Entry Point")
        default:
            return ""
        }
    }
}

// MARK: - UpholdPortalModel

final class UpholdPortalModel: BaseIntegrationModel {
    private let upholdClient = UpholdClient()
    private var authenticationSession: Any?
    private var requirements: [String: [String]] = [:]
    
    override var items: [IntegrationEntryPointItem] {
        UpholdEntryPointItem.supportedCases
    }

    private(set) var babachainCard: DWUpholdCardObject?
    private var fiatCards: [DWUpholdCardObject]?
    
    override var mainAmountString: String {
        if let babachainCard = babachainCard {
            return babachainCard.formattedBabaChainAmount
        }
        
        if let lastKnownBalance = DWUpholdClient.sharedInstance().lastKnownBalance {
            return lastKnownBalance.formattedBabaChainAmount
        }
        
        return (0 as UInt64).formattedBabaChainAmount
    }

    override var supplementaryAmountString: String {
        if let babachainCard = babachainCard {
            return babachainCard.fiatBalanceFormatted(App.fiatCurrency)
        }
        
        if let lastKnownBalance = DWUpholdClient.sharedInstance().lastKnownBalance {
            if let fiatAmount = try? CurrencyExchanger.shared.convertBabaChain(amount: lastKnownBalance as Decimal, to: App.fiatCurrency) {
                return NumberFormatter.fiatFormatter.string(from: fiatAmount as NSNumber)!
            }
        }
        
        return NSLocalizedString("Syncing...", comment: "Balance")
    }
    
    override var balanceTitle: String {
        NSLocalizedString("BabaChain balance on Uphold", comment: "Uphold Entry Point")
    }
    
    override var signInTitle: String {
        NSLocalizedString("Link Uphold Account", comment: "Uphold Entry Point")
    }
    
    override var signOutTitle: String {
        NSLocalizedString("Disconnect Uphold Account", comment: "Uphold Entry Point")
    }
    
    override var authenticationUrl: URL? {
        DWUpholdClient.sharedInstance().startAuthRoutineByURL()
    }
    
    override var logoutUrl: URL? {
        URL(string: "https://wallet.uphold.com/babachainboard/more")
    }
    
    init() {
        super.init(service: .uphold)
        isLoggedIn = DWUpholdClient.sharedInstance().isAuthorized
        
        NotificationCenter.default.publisher(for: NSNotification.Name.DWUpholdClientUserDidLogout)
            .sink { [weak self] _ in
                self?.isLoggedIn = false
            }
            .store(in: &cancellableBag)
    }

    override func refresh() {
        guard isLoggedIn else { return }
        
        state = .loading

        DWUpholdClient.sharedInstance().getCards { [weak self] babachainCard, fiatCards in
            guard let self else { return }

            self.babachainCard = babachainCard
            self.fiatCards = fiatCards

            let success = babachainCard != nil
            self.state = success ? .ready : .failed
        }
        checkCapabilities()
    }

    var buyBabaChainURL: URL? {
        guard let babachainCard else {
            return nil
        }

        return DWUpholdClient.sharedInstance().buyBabaChainURL(forCard: babachainCard)
    }
    
    override func logIn(callbackUrl: URL?) {
        guard let url = callbackUrl else { return }
        
        DWUpholdClient.sharedInstance().completeAuthRoutine(with: url) { [weak self] success in
            self?.isLoggedIn = success
            self?.refresh()
        }
    }

    override func logOut() {
        DWUpholdClient.sharedInstance().logOut()
    }
    
    override func isValidCallbackUrl(url: URL) -> Bool {
        url.absoluteString.contains("uphold")
    }
    
    override func validate(operation type: IntegrationItemType) -> LocalizedError? {
        switch type {
        case .transferBabaChain:
            let reqs = requirements[kWithdrawalsCapability] ?? []
            
            if reqs.isEmpty {
                return nil
            }
            
            return UpholdError.errorCodeToError(code: reqs[0])
        default:
            return super.validate(operation: type)
        }
    }
    
    override func handle(error: Swift.Error) {
        super.handle(error: error)
        UIApplication.shared.open(URL(string: kProfileUrl)!)
    }

    func transactionURL(for transaction: DWUpholdTransactionObject) -> URL? {
        DWUpholdClient.sharedInstance().transactionURL(forTransaction: transaction)
    }

    func successMessageText(for transaction: DWUpholdTransactionObject) -> String {
        String(format: NSLocalizedString("Your transaction was sent and the amount should appear in your wallet in a few minutes.", comment: ""),
               NSLocalizedString("Transaction id", comment: ""), transaction.identifier)
    }
    
    private func checkCapabilities() {
        Task {
            do {
                guard let capability = try await upholdClient.getCapabilities(capability: kWithdrawalsCapability) else {
                    return
                }
                
                if (capability.key == kWithdrawalsCapability) {
                    requirements[capability.key] = capability.requirements
                }
            } catch {
                DSLogger.log("Error obtaining capabilities: \(error)")
            }
        }
    }
}
