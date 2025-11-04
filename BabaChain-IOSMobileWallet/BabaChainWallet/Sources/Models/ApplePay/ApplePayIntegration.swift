//
//  ApplePayIntegration.swift
//  BabaChainWallet
//
//  Created by BabaChain Core Group
//  Copyright © 2024 BabaChain Core Group. All rights reserved.
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
import PassKit
import UIKit

/// Manages Apple Pay integration for easy BabaChain onboarding and purchases
@objc class ApplePayIntegration: NSObject {
    
    // MARK: - Properties
    
    @objc static let shared = ApplePayIntegration()
    
    private let merchantIdentifier = "merchant.org.babachaincore.babachainwallet"
    private let supportedNetworks: [PKPaymentNetwork] = [.visa, .masterCard, .amex, .discover]
    private let merchantCapabilities: PKMerchantCapability = [.capability3DS, .capabilityEMV]
    
    // Exchange rate service
    private let exchangeRateService = ExchangeRateService.shared
    
    // MARK: - Public Methods
    
    @objc func isApplePayAvailable() -> Bool {
        return PKPaymentAuthorizationViewController.canMakePayments() &&
               PKPaymentAuthorizationViewController.canMakePayments(usingNetworks: supportedNetworks)
    }
    
    @objc func presentBuyBabaChainFlow(from viewController: UIViewController, amount: Double, currency: String = "USD") {
        guard isApplePayAvailable() else {
            showApplePayNotAvailableAlert(from: viewController)
            return
        }
        
        // Get current BabaChain price
        exchangeRateService.getBabaChainPrice(in: currency) { [weak self] result in
            DispatchQueue.main.async {
                switch result {
                case .success(let price):
                    self?.showApplePaySheet(
                        from: viewController,
                        fiatAmount: amount,
                        currency: currency,
                        babaChainPrice: price
                    )
                case .failure(let error):
                    self?.showErrorAlert(from: viewController, error: error)
                }
            }
        }
    }
    
    @objc func presentQuickBuyOptions(from viewController: UIViewController) {
        let alertController = UIAlertController(
            title: "Buy BabaChain",
            message: "Choose an amount to purchase with Apple Pay",
            preferredStyle: .actionSheet
        )
        
        // Quick buy amounts
        let amounts = [25.0, 50.0, 100.0, 250.0, 500.0]
        
        for amount in amounts {
            let action = UIAlertAction(title: "$\(Int(amount))", style: .default) { [weak self] _ in
                self?.presentBuyBabaChainFlow(from: viewController, amount: amount)
            }
            alertController.addAction(action)
        }
        
        // Custom amount option
        let customAction = UIAlertAction(title: "Custom Amount", style: .default) { [weak self] _ in
            self?.presentCustomAmountInput(from: viewController)
        }
        alertController.addAction(customAction)
        
        let cancelAction = UIAlertAction(title: "Cancel", style: .cancel)
        alertController.addAction(cancelAction)
        
        // For iPad
        if let popover = alertController.popoverPresentationController {
            popover.sourceView = viewController.view
            popover.sourceRect = CGRect(x: viewController.view.bounds.midX, y: viewController.view.bounds.midY, width: 0, height: 0)
            popover.permittedArrowDirections = []
        }
        
        viewController.present(alertController, animated: true)
    }
    
    // MARK: - Private Methods
    
    private func showApplePaySheet(from viewController: UIViewController, fiatAmount: Double, currency: String, babaChainPrice: Double) {
        let babaChainAmount = fiatAmount / babaChainPrice
        
        let request = PKPaymentRequest()
        request.merchantIdentifier = merchantIdentifier
        request.supportedNetworks = supportedNetworks
        request.merchantCapabilities = merchantCapabilities
        request.countryCode = "US"
        request.currencyCode = currency
        
        // Create payment summary items
        let babaChainItem = PKPaymentSummaryItem(
            label: String(format: "%.4f BABA", babaChainAmount),
            amount: NSDecimalNumber(value: fiatAmount)
        )
        
        let processingFee = fiatAmount * 0.029 // 2.9% processing fee
        let feeItem = PKPaymentSummaryItem(
            label: "Processing Fee",
            amount: NSDecimalNumber(value: processingFee)
        )
        
        let totalAmount = fiatAmount + processingFee
        let totalItem = PKPaymentSummaryItem(
            label: "BabaChain Wallet",
            amount: NSDecimalNumber(value: totalAmount)
        )
        
        request.paymentSummaryItems = [babaChainItem, feeItem, totalItem]
        
        // Add shipping methods for delivery options
        let instantDelivery = PKShippingMethod(
            label: "Instant Delivery",
            amount: NSDecimalNumber.zero
        )
        instantDelivery.identifier = "instant"
        instantDelivery.detail = "BabaChain will be added to your wallet immediately"
        
        request.shippingMethods = [instantDelivery]
        request.shippingType = .delivery
        
        // Present Apple Pay sheet
        let paymentController = PKPaymentAuthorizationViewController(paymentRequest: request)
        paymentController?.delegate = self
        
        if let paymentController = paymentController {
            viewController.present(paymentController, animated: true)
        } else {
            showErrorAlert(from: viewController, error: ApplePayError.failedToCreatePaymentSheet)
        }
    }
    
    private func presentCustomAmountInput(from viewController: UIViewController) {
        let alertController = UIAlertController(
            title: "Custom Amount",
            message: "Enter the amount you want to spend (USD)",
            preferredStyle: .alert
        )
        
        alertController.addTextField { textField in
            textField.placeholder = "Amount (e.g., 100.00)"
            textField.keyboardType = .decimalPad
        }
        
        let buyAction = UIAlertAction(title: "Buy", style: .default) { [weak self] _ in
            guard let textField = alertController.textFields?.first,
                  let text = textField.text,
                  let amount = Double(text),
                  amount > 0 else {
                self?.showErrorAlert(from: viewController, error: ApplePayError.invalidAmount)
                return
            }
            
            self?.presentBuyBabaChainFlow(from: viewController, amount: amount)
        }
        
        let cancelAction = UIAlertAction(title: "Cancel", style: .cancel)
        
        alertController.addAction(buyAction)
        alertController.addAction(cancelAction)
        
        viewController.present(alertController, animated: true)
    }
    
    private func showApplePayNotAvailableAlert(from viewController: UIViewController) {
        let alertController = UIAlertController(
            title: "Apple Pay Not Available",
            message: "Apple Pay is not set up on this device. Please add a payment method in the Wallet app.",
            preferredStyle: .alert
        )
        
        let settingsAction = UIAlertAction(title: "Open Wallet", style: .default) { _ in
            if let url = URL(string: "shoebox://") {
                UIApplication.shared.open(url)
            }
        }
        
        let cancelAction = UIAlertAction(title: "Cancel", style: .cancel)
        
        alertController.addAction(settingsAction)
        alertController.addAction(cancelAction)
        
        viewController.present(alertController, animated: true)
    }
    
    private func showErrorAlert(from viewController: UIViewController, error: Error) {
        let alertController = UIAlertController(
            title: "Error",
            message: error.localizedDescription,
            preferredStyle: .alert
        )
        
        let okAction = UIAlertAction(title: "OK", style: .default)
        alertController.addAction(okAction)
        
        viewController.present(alertController, animated: true)
    }
    
    private func processPurchase(payment: PKPayment, babaChainAmount: Double, completion: @escaping (Bool, Error?) -> Void) {
        // Process the payment with payment processor
        PaymentProcessor.shared.processApplePayPayment(payment) { [weak self] result in
            switch result {
            case .success(let transactionId):
                // Add BabaChain to wallet
                self?.addBabaChainToWallet(amount: babaChainAmount, transactionId: transactionId) { success in
                    completion(success, nil)
                }
                
            case .failure(let error):
                completion(false, error)
            }
        }
    }
    
    private func addBabaChainToWallet(amount: Double, transactionId: String, completion: @escaping (Bool) -> Void) {
        // Convert to satoshis
        let satoshis = UInt64(amount * Double(kOneBabaChain))
        
        // Add to wallet balance
        WalletManager.shared.addPurchasedBabaChain(satoshis, transactionId: transactionId) { success in
            if success {
                // Send success notification
                BabaChainNotificationManager.shared.sendTransactionNotification(
                    amount: satoshis,
                    isReceived: true,
                    txHash: transactionId
                )
                
                // Post notification for UI updates
                NotificationCenter.default.post(
                    name: .babaChainPurchaseCompleted,
                    object: nil,
                    userInfo: [
                        "amount": satoshis,
                        "transactionId": transactionId
                    ]
                )
            }
            
            completion(success)
        }
    }
}

// MARK: - PKPaymentAuthorizationViewControllerDelegate

extension ApplePayIntegration: PKPaymentAuthorizationViewControllerDelegate {
    
    func paymentAuthorizationViewController(_ controller: PKPaymentAuthorizationViewController, didAuthorizePayment payment: PKPayment, handler completion: @escaping (PKPaymentAuthorizationResult) -> Void) {
        
        // Extract BabaChain amount from payment summary
        guard let babaChainItem = payment.summaryItems.first,
              let babaChainAmountString = babaChainItem.label.components(separatedBy: " ").first,
              let babaChainAmount = Double(babaChainAmountString) else {
            completion(PKPaymentAuthorizationResult(status: .failure, errors: [ApplePayError.invalidPaymentData]))
            return
        }
        
        processPurchase(payment: payment, babaChainAmount: babaChainAmount) { success, error in
            DispatchQueue.main.async {
                if success {
                    completion(PKPaymentAuthorizationResult(status: .success, errors: nil))
                } else {
                    let errors = error != nil ? [error!] : [ApplePayError.processingFailed]
                    completion(PKPaymentAuthorizationResult(status: .failure, errors: errors))
                }
            }
        }
    }
    
    func paymentAuthorizationViewControllerDidFinish(_ controller: PKPaymentAuthorizationViewController) {
        controller.dismiss(animated: true)
    }
    
    func paymentAuthorizationViewController(_ controller: PKPaymentAuthorizationViewController, didSelectShippingMethod shippingMethod: PKShippingMethod, handler completion: @escaping (PKPaymentRequestShippingMethodUpdate) -> Void) {
        
        // All deliveries are instant and free
        completion(PKPaymentRequestShippingMethodUpdate(paymentSummaryItems: controller.paymentRequest.paymentSummaryItems))
    }
}

// MARK: - Supporting Services

class ExchangeRateService {
    static let shared = ExchangeRateService()
    
    func getBabaChainPrice(in currency: String, completion: @escaping (Result<Double, Error>) -> Void) {
        // Simulate API call to get BabaChain price
        DispatchQueue.global().asyncAfter(deadline: .now() + 0.5) {
            // Mock price - in production, this would fetch from a real API
            let mockPrice = 0.50 // $0.50 per BABA
            completion(.success(mockPrice))
        }
    }
}

class PaymentProcessor {
    static let shared = PaymentProcessor()
    
    func processApplePayPayment(_ payment: PKPayment, completion: @escaping (Result<String, Error>) -> Void) {
        // Process payment with payment gateway (Stripe, etc.)
        DispatchQueue.global().asyncAfter(deadline: .now() + 2.0) {
            // Mock successful processing
            let transactionId = "tx_\(UUID().uuidString)"
            completion(.success(transactionId))
        }
    }
}

// MARK: - Errors

enum ApplePayError: LocalizedError {
    case failedToCreatePaymentSheet
    case invalidAmount
    case invalidPaymentData
    case processingFailed
    
    var errorDescription: String? {
        switch self {
        case .failedToCreatePaymentSheet:
            return "Failed to create Apple Pay payment sheet"
        case .invalidAmount:
            return "Please enter a valid amount"
        case .invalidPaymentData:
            return "Invalid payment data received"
        case .processingFailed:
            return "Payment processing failed. Please try again."
        }
    }
}

// MARK: - Notifications

extension Notification.Name {
    static let babaChainPurchaseCompleted = Notification.Name("BabaChainPurchaseCompleted")
}

// MARK: - WalletManager Extension

extension WalletManager {
    func addPurchasedBabaChain(_ amount: UInt64, transactionId: String, completion: @escaping (Bool) -> Void) {
        // Add purchased BabaChain to wallet
        // This would integrate with the existing wallet system
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            print("Added \(amount) satoshis to wallet from purchase \(transactionId)")
            completion(true)
        }
    }
}