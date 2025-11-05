//
//  DWDataMigrationManager.h
//  babachainwallet
//
//  Created by Andrew Podkovyrin on 08/11/2018.
//  Copyright © 2019 BabaChain Core. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface DWDataMigrationManager : NSObject

@property (readonly, nonatomic, getter=isMigrationSuccessful) BOOL migrationSuccessful;
@property (readonly, assign, nonatomic) BOOL shouldMigrate;
@property (readonly, assign, nonatomic) BOOL shouldMigrateDatabase;
@property (readonly, assign, nonatomic) BOOL shouldMigrateWalletKeys;

+ (instancetype)sharedInstance;

- (void)migrate:(void (^)(BOOL completed))completion;

@end

NS_ASSUME_NONNULL_END
