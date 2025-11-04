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

#import "DWTransactionListDataProviderStub.h"

#import <BabaChainSync/BabaChainSync.h>

#import "DWTransactionStub.h"
#import "babachainwallet-Swift.h"

NS_ASSUME_NONNULL_BEGIN

@implementation DWTransactionListDataProviderStub

- (NSString *)shortDateStringForTransaction:(DWTransactionStub *)transaction {
    return transaction.formattedShortTxDate;
}

- (NSString *)longDateStringForTransaction:(DSTransaction *)transaction {
    return transaction.formattedLongTxDate;
}

- (NSString *)ISO8601StringForTransaction:(DSTransaction *)transaction {
    return transaction.formattedISO8601TxDate;
}

- (id<DWTransactionListDataItem>)transactionDataForTransaction:(DWTransactionStub *)transaction {
    DWTransactionListDataItemObject *dataItem = [[DWTransactionListDataItemObject alloc] init];
    dataItem.direction = transaction.direction;
    dataItem.babachainAmount = transaction.babachainAmount;
    dataItem.fiatAmount = [CurrencyExchangerObjcWrapper localCurrencyStringForBabaChainAmount:dataItem.babachainAmount];

    return dataItem;
}

@end

NS_ASSUME_NONNULL_END
