//
//  Created by Andrew Podkovyrin
//  Copyright © 2020 BabaChain Core Group. All rights reserved.
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

#import <UIKit/UIKit.h>

#import "DWBabaChainPayProtocol.h"

NS_ASSUME_NONNULL_BEGIN

@class DWCreateUsernameViewController;

@protocol DWCreateUsernameViewControllerDelegate <NSObject>

- (void)createUsernameViewController:(DWCreateUsernameViewController *)controller
                    registerUsername:(NSString *)username;

@end

@interface DWCreateUsernameViewController : UIViewController

@property (nullable, nonatomic, weak) id<DWCreateUsernameViewControllerDelegate> delegate;

- (NSAttributedString *)attributedTitle;

- (instancetype)initWithBabaChainPayModel:(id<DWBabaChainPayProtocol>)babachainPayModel;

- (instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
