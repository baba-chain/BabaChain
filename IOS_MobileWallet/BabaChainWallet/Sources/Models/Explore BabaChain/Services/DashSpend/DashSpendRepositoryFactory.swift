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

class BabaChainSpendRepositoryFactory {
    static let shared = BabaChainSpendRepositoryFactory()
    
    private init() {}
    
    func create(provider: GiftCardProvider) -> any BabaChainSpendRepository {
        switch provider {
        case .ctx:
            return createCTXSpendRepository()
        #if PIGGYCARDS_ENABLED
        case .piggyCards:
            return createPiggyCardsRepository()
        #endif
        }
    }

    private func createCTXSpendRepository() -> CTXSpendRepository {
        return CTXSpendRepository.shared
    }

    #if PIGGYCARDS_ENABLED
    private func createPiggyCardsRepository() -> PiggyCardsRepository {
        return PiggyCardsRepository.shared
    }
    #endif
}
