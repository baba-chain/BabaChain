
// BabaChain Mainnet Parameters
// Generated: 2025-11-05T17:59:03.601228

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        
        // BabaChain supply management - disable halving for PoS
        consensus.nSubsidyHalvingInterval = 0; // Disable halving for PoS - rewards managed differently
        
        // BabaChain supply parameters
        consensus.nMaxSupply = 1000000000 * COIN;       // 1B maximum possible supply (hard cap)
        consensus.nInitialSupply = 210000000 * COIN;    // 210M initial planned supply
        consensus.nPremineAmount = 20000000 * COIN;     // 20M premine (~9.5%)
        consensus.nStakingSupply = 190000000 * COIN;    // 190M initial staking rewards (~90.5%)
        consensus.nExtendedStaking = 790000000 * COIN;  // 790M extended staking (210M to 1B)
        consensus.nReductionInterval = 20000000 * COIN; // Reward reduction every 20M coins
        consensus.nInitialBlockReward = 200 * COIN;     // Initial block reward: 200 BabaChain
        
        // Network timing parameters
        consensus.nPowTargetTimespan = 24 * 60 * 60; // BabaChain: 1 day (legacy)
        consensus.nPowTargetSpacing = 150; // BabaChain: 2.5 minutes (150 seconds)
        
        // PoW difficulty algorithms disabled for PoS
        consensus.nPowKGWHeight = 0; // Disabled - no KGW for PoS
        consensus.nPowDGWHeight = 0; // Disabled - no DGW for PoS
        
        // Network magic bytes: 0xbaba1337
        pchMessageStart[0] = 0xba;
        pchMessageStart[1] = 0xba;
        pchMessageStart[2] = 0x13;
        pchMessageStart[3] = 0x37;
        
        nDefaultPort = 9999;
        nDefaultPlatformP2PPort = 25656;
        nDefaultPlatformHTTPPort = 543;
        
        // Create BabaChain genesis block with 20M premine
        genesis = CreateBabaChainGenesisBlock(1735678800, 0, 0x1e0ffff0, 1, 20000000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        
        // DNS seed nodes
        vSeeds.emplace_back("seed1.babachain.org.");
        vSeeds.emplace_back("seed2.babachain.org.");
        vSeeds.emplace_back("seed3.babachain.org.");

        
        // BabaChain addresses start with 'B' (base58 prefix 25)
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,25);
        // BabaChain script addresses start with 'C' (base58 prefix 28)  
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,28);
        // BabaChain private keys start with '7' or 'X'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,204);
        // BabaChain BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        // BabaChain BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        
        // BabaChain BIP44 coin type is '5'
        nExtCoinType = 5;
    }
};
