//  
//  Created by Andrei Ashikhmin
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

private let kCoinJoinMixBabaChainShown = "coinJoinMixBabaChainShownKey"
private let kJoinBabaChainPayInfoShown = "joinBabaChainPayInfoShownKey"
private let kRequestedUsernameId = "requestedUsernameIdKey"
private let kAlreadyPaid = "alreadyPaidForUsernameKey"
private let kJoinBabaChainPayDismissed = "joinBabaChainPayDismissed"

// MARK: - UsernamePrefs

class UsernamePrefs {
    public static let shared: UsernamePrefs = .init()
    
    private var _mixBabaChainShown: Bool? = nil
    var mixBabaChainShown: Bool {
        get { _mixBabaChainShown ?? UserDefaults.standard.bool(forKey: kCoinJoinMixBabaChainShown) }
        set(value) {
            _mixBabaChainShown = value
            UserDefaults.standard.set(value, forKey: kCoinJoinMixBabaChainShown)
        }
    }
    
    private var _joinBabaChainPayInfoShown: Bool? = nil
    var joinBabaChainPayInfoShown: Bool {
        get { _joinBabaChainPayInfoShown ?? UserDefaults.standard.bool(forKey: kJoinBabaChainPayInfoShown) }
        set(value) {
            _joinBabaChainPayInfoShown = value
            UserDefaults.standard.set(value, forKey: kJoinBabaChainPayInfoShown)
        }
    }
    
    private var _requestedUsernameId: String? = nil
    var requestedUsernameId: String? {
        get { _requestedUsernameId ?? UserDefaults.standard.string(forKey: kRequestedUsernameId) }
        set(value) {
            _requestedUsernameId = value
            UserDefaults.standard.set(value, forKey: kRequestedUsernameId)
        }
    }
    
    private var _alreadyPaid: Bool? = nil
    var alreadyPaid: Bool {
        get { _alreadyPaid ?? UserDefaults.standard.bool(forKey: kAlreadyPaid) }
        set(value) {
            _alreadyPaid = value
            UserDefaults.standard.set(value, forKey: kAlreadyPaid)
        }
    }
    
    private var _joinBabaChainPayDismissed: Bool? = nil
    var joinBabaChainPayDismissed: Bool {
        get { _joinBabaChainPayDismissed ?? UserDefaults.standard.bool(forKey: kJoinBabaChainPayDismissed) }
        set(value) {
            _joinBabaChainPayDismissed = value
            UserDefaults.standard.set(value, forKey: kJoinBabaChainPayDismissed)
        }
    }
}
