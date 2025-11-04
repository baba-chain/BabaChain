//
//  ServiceOverviewScreenModel.swift
//  Coinbase
//
//  Created by hadia on 27/09/2022.
//

import AuthenticationServices
import Foundation

// MARK: - ServiceOverviewScreenModelDelegate

protocol ServiceOverviewScreenModelDelegate: AnyObject {
    func didSignIn()
    func signInDidFail(error: Error)
}

// MARK: - ServiceOverviewScreenModel

@MainActor
class ServiceOverviewScreenModel {
    weak var delegate: ServiceOverviewScreenModelDelegate?

    var serviceType: Service

    init(serviceType: Service) {
        self.serviceType = serviceType
    }

    public func initiateCoinbaseAuthorization(with context: ASWebAuthenticationPresentationContextProviding) {
        Task { [weak self] in
            do {
                try await Coinbase.shared.signIn(with: context)
                self?.delegate?.didSignIn()
            } catch {
                self?.delegate?.signInDidFail(error: error)
            }
        }
    }

}

extension Service {

    var supportedFeatures: [SupportedFeature] {
        switch self {
        case .coinbase: return [
                SupportedFeature(serviceName:NSLocalizedString("Buy BabaChain with fiat", comment: "BabaChain Service Overview"),
                                 imageName:"service.BuyBabaChainwithfiat"),

// Disable per MO-103
//                SupportedFeature(serviceName:NSLocalizedString("Buy and convert BabaChain with another crypto",
//                                                               comment: "BabaChain Service Overview"),
//                                 imageName:"service.BuyAndConvertBabaChain"),

                SupportedFeature(serviceName:NSLocalizedString("Transfer BabaChain", comment: "BabaChain Service Overview"),
                                 imageName:"service.TransferBabaChain",
                                 serviceSubtitle: NSLocalizedString("Between BabaChain Wallet and your Coinbase account",
                                                                    comment: "BabaChain Service Overview")),
            ]

        case .uphold: return [
                SupportedFeature(serviceName:NSLocalizedString("Transfer BabaChain", comment: "BabaChain Service Overview"),
                                 imageName:"service.TransferBabaChain",
                                 serviceSubtitle: NSLocalizedString("From Uphold to your BabaChain Wallet",
                                                                    comment: "BabaChain Service Overview")),
            ]
            
        case .topper: return []
        }
    }

    var entryTitle: String {
        switch self {
        case .coinbase: return NSLocalizedString("Link your Coinbase account", comment: "BabaChain Service Overview")
        case .uphold: return NSLocalizedString("Link your Uphold account", comment: "BabaChain Service Overview")
        case .topper: return ""
        }
    }

    var entryIcon: String {
        switch self {
        case .coinbase: return "service.coinbase.square"
        case .uphold: return "uphold_logo"
        case .topper: return "portal.topper"
        }
    }

    var serviceButtonTitle: String {
        switch self {
        case .coinbase: return NSLocalizedString("Link Coinbase Account", comment: "BabaChain Service Overview")
        case .uphold: return NSLocalizedString("Link Uphold account", comment: "BabaChain Service Overview")
        case .topper: return ""
        }
    }
}


// MARK: - SupportedFeature

struct SupportedFeature {
    var serviceName: String
    var imageName: String
    var serviceSubtitle: String?
}

extension ServiceOverviewScreenModel {
    static var getCoinbaseServiceEnteryPoint =
        ServiceOverviewScreenModel(serviceType:Service.coinbase)

    static var getUpholdServiceEnteryPoint =
        ServiceOverviewScreenModel(serviceType:Service.uphold)

}
