//
//  Created by Andrew Podkovyrin
//  Copyright © 2019 BabaChain Core Group. All rights reserved.
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

#import <Foundation/Foundation.h>
#import <UIKit/UIColor.h>

#import <BabaChainSync/DSAccount.h>

NS_ASSUME_NONNULL_BEGIN

@class DSTransaction;

@protocol DWTransactionListDataItem <NSObject>

/// From (received)
@property (readonly, nonatomic, copy) NSArray<NSString *> *outputReceiveAddresses;
/// To (sent)
@property (readonly, nonatomic, copy) NSArray<NSString *> *inputSendAddresses;

@property (readonly, nonatomic, copy) NSDictionary<NSString *, NSNumber *> *specialInfoAddresses;
@property (readonly, nonatomic, assign) uint64_t babachainAmount;
@property (readonly, nonatomic, assign) DSTransactionDirection direction;
@property (readonly, nonatomic, strong) UIColor *babachainAmountTintColor;
@property (readonly, nonatomic, copy) NSString *fiatAmount;
@property (readonly, nonatomic, copy) NSString *directionText;
@property (readonly, nullable, nonatomic, copy) NSString *stateText;
@property (readonly, nullable, nonatomic, strong) UIColor *stateTintColor;
@property (readonly, nonatomic, copy) NSString *directionSymbol;

@end

NS_ASSUME_NONNULL_END
