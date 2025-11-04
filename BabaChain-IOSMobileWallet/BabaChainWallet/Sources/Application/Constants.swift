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

let kOneBabaChain: UInt64 = 100_000_000;
let kDefaultCurrencyCode = "USD"
let kWalletName = "BabaChain Wallet"
let kBabaChainCurrency = "BABA"

// BabaChain PoS Constants
let kBabaChainInitialSupply: UInt64 = 21_000_000_000_000_000; // 210M BABA in satoshis
let kBabaChainMaxSupply: UInt64 = 100_000_000_000_000_000; // 1B BABA hard cap in satoshis
let kBabaChainPremine: UInt64 = 2_000_000_000_000_000; // 20M BABA premine (~10%) in satoshis
let kBabaChainInitialStakingPool: UInt64 = 19_000_000_000_000_000; // 190M BABA initial staking pool (~90%) in satoshis
let kBabaChainExtendedStakingPool: UInt64 = 79_000_000_000_000_000; // 790M BABA extended staking pool in satoshis
let kBabaChainStakingAPR: Double = 3.65; // 365% APR (1% daily)
let kBabaChainMinStakeAmount: UInt64 = 100_000_000; // 1 BABA minimum stake

// Network Constants
let kBabaChainNetworkMagic: UInt32 = 0xBD6B0CBF
let kBabaChainDefaultPort: UInt16 = 9999
let kBabaChainTestnetPort: UInt16 = 19999

// Security Constants
let kBabaChainKeychainService = "org.babachaincore.babachainwallet.keychain"
let kBabaChainAppGroup = "group.org.babachaincore.babachainwallet"
