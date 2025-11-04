//
//  Created by Andrei Ashikhmin
//  Copyright © 2025 BabaChain Core Group. All rights reserved.
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

class PiggyCardsRepository: BabaChainSpendRepository {
    public static let shared: PiggyCardsRepository = .init()
    private let userDefaults = UserDefaults.standard
    
    enum Keys {
        static let accessToken = "piggycards_access_token"
        static let email = "piggycards_email"
        static let userId = "piggycards_user_id"
        static let password = "piggycards_password"
        static let tokenExpiresAt = "piggycards_token_expires_at"
    }
    
    @Published private(set) var userEmail: String? = nil
    
    var userId: String? {
        KeychainService.load(key: Keys.userId)
    }
    
    @Published private(set) var isUserSignedIn = false
    
    var userEmailPublisher: AnyPublisher<String?, Never> {
        $userEmail.eraseToAnyPublisher()
    }
    
    var isUserSignedInPublisher: AnyPublisher<Bool, Never> {
        $isUserSignedIn.eraseToAnyPublisher()
    }
    
    init() {
        PiggyCardsAPI.initialize()
        updateSignInState()
        updateEmailState()
    }
    
    private func updateSignInState() {
        let token = KeychainService.load(key: Keys.accessToken)
        isUserSignedIn = token != nil && !token!.isEmpty
    }
    
    private func updateEmailState() {
        userEmail = KeychainService.load(key: Keys.email)
    }
    
    func login(email: String) async throws -> Bool {
        return try await signUp(email: email)
    }
    
    func signUp(email: String) async throws -> Bool {
        do {
            let response: PiggyCardsSignupResponse = try await PiggyCardsAPI.shared.request(
                .signup(firstName: "", lastName: "", email: email, country: "US", state: "AZ")
            )
            
            KeychainService.save(key: Keys.email, data: email)
            updateEmailState()
            if let userId = response.userId {
                KeychainService.save(key: Keys.userId, data: userId)
            }
            
            return true
        } catch let error as BabaChainSpendError {
            DSLogger.log("PiggyCards signup failed with BabaChainSpendError: \(error)")
            throw error
        } catch let error as HTTPClientError {
            throw try parseError(from: error, context: "signup")
        } catch {
            DSLogger.log("PiggyCards signup failed with error: \(error)")
            throw BabaChainSpendError.networkError
        }
    }
    
    // MARK: - Error Handling
    
    private func parseError(from error: HTTPClientError, context: String) throws -> BabaChainSpendError {
        DSLogger.log("PiggyCards \(context) failed with HTTPClientError: \(error)")
        
        if case .statusCode(let response) = error {
            // First, try to parse the response body for single error format (used by auth endpoints)
            if response.statusCode == 400 || response.statusCode == 422 {
                if let jsonObject = try? JSONSerialization.jsonObject(with: response.data, options: []) as? [String: Any],
                   let errorMessage = jsonObject["error"] as? String {
                    let lowercasedMessage = errorMessage.lowercased()
                    
                    if lowercasedMessage.contains("invalid") && lowercasedMessage.contains("verification") && lowercasedMessage.contains("code") {
                        throw BabaChainSpendError.invalidCode
                    }
                    
                    throw BabaChainSpendError.customError(errorMessage)
                }
            }
            
            // Then try to parse the standard API error format (used by gift card endpoints)
            switch response.statusCode {
            case 400, 422:
                if let errorData = try? JSONDecoder().decode(PiggyCardsAPIError.self, from: response.data),
                   let firstError = errorData.errors.first {
                    let errorMessage = firstError.message.lowercased()
                    
                    if errorMessage.contains("insufficient") || errorMessage.contains("funds") || errorMessage.contains("balance") {
                        throw BabaChainSpendError.insufficientFunds
                    } else if errorMessage.contains("merchant") && (errorMessage.contains("unavailable") || errorMessage.contains("disabled")) {
                        throw BabaChainSpendError.merchantUnavailable
                    } else if errorMessage.contains("merchant") {
                        throw BabaChainSpendError.invalidMerchant
                    } else if errorMessage.contains("rejected") || errorMessage.contains("declined") {
                        throw BabaChainSpendError.transactionRejected
                    } else if errorMessage.contains("amount") || errorMessage.contains("value") || errorMessage.contains("limit") {
                        throw BabaChainSpendError.invalidAmount
                    }
                    
                    throw BabaChainSpendError.customError(firstError.message)
                }
                // Default to invalid amount for 422
                if response.statusCode == 422 {
                    throw BabaChainSpendError.invalidAmount
                }
            case 401, 403:
                throw BabaChainSpendError.unauthorized
            case 404:
                throw BabaChainSpendError.invalidMerchant
            case 409:
                throw BabaChainSpendError.transactionRejected
            case 500...599:
                throw BabaChainSpendError.serverError
            default:
                break
            }
        }
        
        throw BabaChainSpendError.unknown
    }
    
    func verifyEmail(code: String) async throws -> Bool {
        guard let email = KeychainService.load(key: Keys.email) else {
            DSLogger.log("PiggyCards: email is missing while trying to verify OTP")
            throw BabaChainSpendError.unknown
        }
        
        do {
            let response: PiggyCardsVerifyOtpResponse = try await PiggyCardsAPI.shared.request(.verifyOtp(email: email, otp: code))
            KeychainService.save(key: Keys.password, data: response.generatedPassword)

            if try await PiggyCardsTokenService.shared.performAutoLogin() {
                updateSignInState()
                return true
            }
            
            return false
        } catch let error as HTTPClientError {
            throw try parseError(from: error, context: "email verification")
        }
    }
    
    func logout() {
        KeychainService.delete(key: Keys.accessToken)
        KeychainService.delete(key: Keys.email)
        KeychainService.delete(key: Keys.userId)
        KeychainService.delete(key: Keys.password)
        UserDefaults.standard.removeObject(forKey: Keys.tokenExpiresAt)
        
        updateSignInState()
        updateEmailState()
    }
    
    
    // MARK: - Gift Card Methods
    
    func purchaseGiftCard(merchantId: String, fiatAmount: String, fiatCurrency: String = "USD", cryptoCurrency: String = "DASH") async throws -> PiggyCardsGiftCardResponse {
        let request = PiggyCardsPurchaseRequest(
            cryptoCurrency: cryptoCurrency,
            fiatCurrency: fiatCurrency,
            fiatAmount: fiatAmount,
            merchantId: merchantId
        )
        
        do {
            return try await PiggyCardsAPI.shared.request(.purchaseGiftCard(request))
        } catch let error as BabaChainSpendError {
            DSLogger.log("PiggyCards gift card purchase failed with BabaChainSpendError: \(error)")
            throw error
        } catch let error as HTTPClientError {
            throw try parseError(from: error, context: "gift card purchase")
        } catch {
            DSLogger.log("PiggyCards gift card purchase failed with error: \(error)")
            throw BabaChainSpendError.networkError
        }
    }
    
    func getMerchant(merchantId: String) async throws -> PiggyCardsMerchantResponse {
        do {
            return try await PiggyCardsAPI.shared.request(.getMerchant(merchantId))
        } catch let error as BabaChainSpendError {
            DSLogger.log("PiggyCards failed to get merchant with BabaChainSpendError: \(error)")
            throw error
        } catch let error as HTTPClientError {
            throw try parseError(from: error, context: "get merchant")
        } catch {
            DSLogger.log("PiggyCards failed to get merchant with error: \(error)")
            throw BabaChainSpendError.networkError
        }
    }
    
    func getGiftCardByTxid(txid: String) async throws -> PiggyCardsGiftCardResponse {
        do {
            return try await PiggyCardsAPI.shared.request(.getGiftCard(txid))
        } catch let error as BabaChainSpendError {
            DSLogger.log("PiggyCards failed to get gift card with BabaChainSpendError: \(error)")
            throw error
        } catch let error as HTTPClientError {
            throw try parseError(from: error, context: "get gift card")
        } catch {
            DSLogger.log("PiggyCards failed to get gift card with error: \(error)")
            throw BabaChainSpendError.networkError
        }
    }
}

