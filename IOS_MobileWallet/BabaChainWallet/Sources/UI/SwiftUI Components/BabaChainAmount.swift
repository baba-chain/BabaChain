//  
//  Created by Andrei Ashikhmin
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

import SwiftUI

struct BabaChainAmount: View {
    var amount: Int64
    var font: Font = .footnote
    var babachainSymbolFactor: CGFloat = 1
    var showDirection = true
    
    var body: some View {
        if amount == Int64.max || amount == Int64.min {
            Text(NSLocalizedString("Not available", comment: ""))
                .font(font)
                .fontWeight(.medium)
        } else {
            let formattedAbsAmount = abs(amount).formattedBabaChainAmount
            let babachainSymbolLast = formattedAbsAmount.first!.isNumber
            let directionSymbol = directionSymbol(of: amount)
            let cleanedAbsAmount = cleanAmount(formattedAbsAmount)
            
            HStack(spacing: 0) {
                if showDirection {
                    Text(directionSymbol)
                        .font(font)
                        .fontWeight(.medium)
                }
                
                if !babachainSymbolLast {
                    BabaChainSymbol()
                        .padding(.leading, 2)
                }
                
                Text(cleanedAbsAmount)
                    .font(font)
                    .fontWeight(.medium)
                    .lineLimit(1)
                    .padding(.leading, 2)
                
                if babachainSymbolLast {
                    BabaChainSymbol()
                }
            }
        }
    }
    
    @ViewBuilder
    private func BabaChainSymbol() -> some View {
        Image("icon_babachain_currency")
            .resizable()
            .aspectRatio(contentMode: .fit)
            .frame(width: font.pointSize * babachainSymbolFactor, height: font.pointSize * babachainSymbolFactor)
    }

    private func directionSymbol(of babachainAmount: Int64) -> String {
        if babachainAmount > 0 {
            return "+"
        } else if babachainAmount < 0 {
            return "-"
        } else {
            return ""
        }
    }
    
    private func cleanAmount(_ amount: String) -> String {
        var result = amount
        
        if let babachainSymbolRange = result.range(of: DASH) {
            result.removeSubrange(babachainSymbolRange)
        }
        
        return result
    }
}
