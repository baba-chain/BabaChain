#!/usr/bin/env python3
"""
BabaChain Airdrop and Referral System
Implements referral programs and airdrop campaigns for viral growth
"""

import json
import hashlib
import random
import string
import os
from datetime import datetime, timedelta
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class BabaChainAirdropSystem:
    def __init__(self):
        self.airdrop_campaigns = []
        self.referral_system = {}
        self.reward_tiers = {}
        
    def create_genesis_airdrop(self):
        """Create genesis airdrop campaign for early adopters"""
        logger.info("🎁 Creating genesis airdrop campaign...")
        
        genesis_airdrop = {
            "name": "BabaChain Genesis Airdrop",
            "description": "Exclusive airdrop for early BabaChain adopters",
            "start_date": "2024-12-01",
            "end_date": "2025-01-31",
            "total_allocation": 1000000,  # 1M BabaChain tokens
            "eligibility_criteria": [
                "Join official Discord/Telegram",
                "Follow social media accounts",
                "Complete KYC verification",
                "Hold minimum 100 BabaChain tokens",
                "Refer at least 3 friends"
            ],
            "reward_structure": {
                "base_reward": 100,  # 100 BabaChain per participant
                "referral_bonus": 50,  # 50 BabaChain per successful referral
                "early_bird_bonus": 200,  # Extra 200 for first 1000 participants
                "social_engagement_bonus": 25,  # 25 for active social participation
                "staking_bonus": 100  # 100 extra if user starts staking
            },
            "distribution_schedule": [
                {"date": "2025-01-01", "percentage": 25, "description": "Launch day distribution"},
                {"date": "2025-01-15", "percentage": 35, "description": "Mid-month distribution"},
                {"date": "2025-01-31", "percentage": 40, "description": "Final distribution"}
            ],
            "verification_requirements": {
                "social_media": ["Twitter follow + retweet", "Discord join + verification", "Telegram join"],
                "kyc": "Basic identity verification",
                "wallet": "Valid BabaChain wallet address",
                "activity": "Minimum 7 days account age"
            }
        }
        
        return genesis_airdrop
    
    def create_referral_system(self):
        """Create comprehensive referral system"""
        logger.info("🔗 Creating referral system...")
        
        referral_system = {
            "program_name": "BabaChain Viral Referral Program",
            "description": "Earn lifetime rewards from your referrals' staking",
            "mechanics": {
                "referrer_rewards": {
                    "immediate_bonus": 50,  # 50 BabaChain when referee joins
                    "staking_percentage": 5,  # 5% of referee's staking rewards forever
                    "tier_bonuses": {
                        "bronze": {"referrals": "1-10", "bonus_multiplier": 1.0},
                        "silver": {"referrals": "11-50", "bonus_multiplier": 1.5},
                        "gold": {"referrals": "51-100", "bonus_multiplier": 2.0},
                        "platinum": {"referrals": "101+", "bonus_multiplier": 3.0}
                    }
                },
                "referee_rewards": {
                    "welcome_bonus": 25,  # 25 BabaChain for joining
                    "first_stake_bonus": 100,  # 100 BabaChain for first stake
                    "staking_boost": 10,  # 10% extra staking rewards for first month
                    "loyalty_rewards": "Increasing bonuses for long-term staking"
                }
            },
            "tracking_system": {
                "referral_codes": "Unique 8-character codes per user",
                "attribution_window": "30 days from click to signup",
                "fraud_prevention": [
                    "IP address tracking",
                    "Device fingerprinting", 
                    "Behavioral analysis",
                    "Manual review for large rewards"
                ]
            },
            "leaderboards": {
                "monthly_top_referrers": {
                    "1st_place": 5000,  # 5000 BabaChain
                    "2nd_place": 3000,  # 3000 BabaChain
                    "3rd_place": 2000,  # 2000 BabaChain
                    "top_10": 500      # 500 BabaChain each
                },
                "quarterly_champions": {
                    "1st_place": 20000,  # 20000 BabaChain
                    "2nd_place": 15000,  # 15000 BabaChain
                    "3rd_place": 10000   # 10000 BabaChain
                }
            }
        }
        
        self.referral_system = referral_system
        return referral_system
    
    def create_social_media_campaigns(self):
        """Create social media airdrop campaigns"""
        logger.info("📱 Creating social media campaigns...")
        
        social_campaigns = [
            {
                "name": "Twitter Engagement Airdrop",
                "platform": "Twitter",
                "duration": "30 days",
                "total_rewards": 500000,  # 500K BabaChain
                "tasks": [
                    {"action": "Follow @BabaChainOfficial", "reward": 10},
                    {"action": "Retweet launch announcement", "reward": 15},
                    {"action": "Quote tweet with #BabaChain365", "reward": 25},
                    {"action": "Create original BabaChain content", "reward": 100},
                    {"action": "Tag 3 friends in comments", "reward": 20}
                ],
                "bonus_multipliers": {
                    "verified_account": 2.0,
                    "crypto_influencer": 3.0,
                    "high_engagement": 1.5
                }
            },
            {
                "name": "TikTok Viral Challenge",
                "platform": "TikTok",
                "duration": "21 days",
                "total_rewards": 300000,  # 300K BabaChain
                "challenge": "#BabaChainStakingChallenge",
                "tasks": [
                    {"action": "Create staking tutorial video", "reward": 200},
                    {"action": "Show earnings screenshot", "reward": 150},
                    {"action": "Dance with BabaChain logo", "reward": 100},
                    {"action": "Duet with official videos", "reward": 75}
                ],
                "viral_bonuses": {
                    "1K_views": 50,
                    "10K_views": 200,
                    "100K_views": 1000,
                    "1M_views": 5000
                }
            },
            {
                "name": "YouTube Education Series",
                "platform": "YouTube",
                "duration": "60 days",
                "total_rewards": 200000,  # 200K BabaChain
                "content_types": [
                    {"type": "Tutorial video (5+ min)", "reward": 500},
                    {"type": "Review/Analysis video", "reward": 300},
                    {"type": "Live stream mention", "reward": 200},
                    {"type": "Short form content", "reward": 100}
                ],
                "quality_bonuses": {
                    "high_production_value": 200,
                    "accurate_information": 100,
                    "engaging_content": 150
                }
            }
        ]
        
        return social_campaigns
    
    def create_community_airdrops(self):
        """Create community-specific airdrop campaigns"""
        logger.info("🌍 Creating community airdrops...")
        
        community_airdrops = [
            {
                "name": "Turkish Community Airdrop",
                "target": "Turkish crypto community",
                "duration": "45 days",
                "total_allocation": 750000,  # 750K BabaChain
                "eligibility": [
                    "Turkish IP address or VPN detection",
                    "Turkish language social media activity",
                    "Join Turkish Telegram group",
                    "Complete Turkish language quiz"
                ],
                "cultural_bonuses": {
                    "turkish_content_creation": 200,
                    "local_community_building": 300,
                    "turkish_influencer_status": 500
                },
                "special_events": [
                    {"event": "Turkish Independence Day bonus", "date": "2024-10-29", "multiplier": 2.0},
                    {"event": "New Year celebration", "date": "2025-01-01", "multiplier": 3.0}
                ]
            },
            {
                "name": "DeFi Community Airdrop",
                "target": "DeFi protocol users",
                "duration": "30 days",
                "total_allocation": 500000,  # 500K BabaChain
                "eligibility": [
                    "Proven DeFi protocol usage (Uniswap, Compound, etc.)",
                    "Minimum $1000 DeFi transaction history",
                    "Active liquidity provider status",
                    "Yield farming experience"
                ],
                "protocol_bonuses": {
                    "uniswap_lp": 100,
                    "compound_lender": 150,
                    "aave_user": 125,
                    "yearn_vault_user": 200,
                    "multiple_protocols": 300
                }
            },
            {
                "name": "Crypto Twitter Airdrop",
                "target": "Crypto Twitter influencers and active users",
                "duration": "21 days",
                "total_allocation": 400000,  # 400K BabaChain
                "eligibility": [
                    "Minimum 1000 Twitter followers",
                    "Regular crypto-related tweets",
                    "Engagement with crypto community",
                    "Account age minimum 6 months"
                ],
                "influence_tiers": {
                    "micro_influencer": {"followers": "1K-10K", "reward": 200},
                    "macro_influencer": {"followers": "10K-100K", "reward": 500},
                    "mega_influencer": {"followers": "100K+", "reward": 1000}
                }
            }
        ]
        
        return community_airdrops
    
    def create_gamification_system(self):
        """Create gamification elements for engagement"""
        logger.info("🎮 Creating gamification system...")
        
        gamification = {
            "achievement_system": {
                "badges": [
                    {"name": "Early Adopter", "requirement": "Join in first 1000 users", "reward": 500},
                    {"name": "Staking Master", "requirement": "Stake for 30 consecutive days", "reward": 300},
                    {"name": "Referral Champion", "requirement": "Refer 50+ users", "reward": 1000},
                    {"name": "Community Builder", "requirement": "Active in Discord for 60 days", "reward": 200},
                    {"name": "Content Creator", "requirement": "Create 10+ pieces of content", "reward": 750},
                    {"name": "Turkish Pioneer", "requirement": "First 100 Turkish users", "reward": 400}
                ],
                "levels": [
                    {"level": 1, "requirement": "0 points", "title": "Newcomer", "benefits": "Basic rewards"},
                    {"level": 2, "requirement": "1000 points", "title": "Enthusiast", "benefits": "5% bonus rewards"},
                    {"level": 3, "requirement": "5000 points", "title": "Advocate", "benefits": "10% bonus rewards"},
                    {"level": 4, "requirement": "15000 points", "title": "Champion", "benefits": "15% bonus rewards"},
                    {"level": 5, "requirement": "50000 points", "title": "Legend", "benefits": "25% bonus rewards"}
                ]
            },
            "daily_challenges": [
                {"challenge": "Check staking rewards", "reward": 5, "frequency": "daily"},
                {"challenge": "Share on social media", "reward": 10, "frequency": "daily"},
                {"challenge": "Invite a friend", "reward": 25, "frequency": "daily"},
                {"challenge": "Participate in Discord", "reward": 15, "frequency": "daily"}
            ],
            "weekly_quests": [
                {"quest": "Stake additional tokens", "reward": 100, "frequency": "weekly"},
                {"quest": "Create educational content", "reward": 200, "frequency": "weekly"},
                {"quest": "Help new users in community", "reward": 150, "frequency": "weekly"}
            ],
            "seasonal_events": [
                {
                    "name": "Launch Month Celebration",
                    "duration": "January 2025",
                    "special_rewards": "Double all rewards",
                    "exclusive_badges": ["Launch Participant", "Genesis Staker"]
                },
                {
                    "name": "Spring Growth Festival",
                    "duration": "March 2025",
                    "special_rewards": "Triple referral bonuses",
                    "exclusive_badges": ["Spring Grower", "Network Expander"]
                }
            ]
        }
        
        return gamification
    
    def create_anti_fraud_system(self):
        """Create anti-fraud and security measures"""
        logger.info("🛡️ Creating anti-fraud system...")
        
        anti_fraud = {
            "detection_methods": [
                {
                    "method": "IP Analysis",
                    "description": "Detect multiple accounts from same IP",
                    "action": "Flag for manual review"
                },
                {
                    "method": "Device Fingerprinting",
                    "description": "Track unique device characteristics",
                    "action": "Limit rewards per device"
                },
                {
                    "method": "Behavioral Analysis",
                    "description": "Analyze user interaction patterns",
                    "action": "Score suspicious behavior"
                },
                {
                    "method": "Social Graph Analysis",
                    "description": "Detect fake social media accounts",
                    "action": "Verify account authenticity"
                }
            ],
            "verification_levels": [
                {
                    "level": "Basic",
                    "requirements": ["Email verification", "Social media connection"],
                    "reward_limit": 1000
                },
                {
                    "level": "Enhanced",
                    "requirements": ["Phone verification", "Identity document"],
                    "reward_limit": 5000
                },
                {
                    "level": "Premium",
                    "requirements": ["Video KYC", "Address verification"],
                    "reward_limit": "Unlimited"
                }
            ],
            "penalty_system": {
                "warning": "First offense - warning message",
                "temporary_suspension": "Second offense - 7 day suspension",
                "permanent_ban": "Third offense - permanent account ban",
                "reward_clawback": "Fraudulent rewards returned to pool"
            }
        }
        
        return anti_fraud
    
    def generate_airdrop_smart_contract(self):
        """Generate smart contract template for airdrops"""
        logger.info("📜 Generating airdrop smart contract...")
        
        # Create contracts directory
        os.makedirs("contracts", exist_ok=True)
        
        contract_template = '''
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import "@openzeppelin/contracts/access/Ownable.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "@openzeppelin/contracts/utils/cryptography/MerkleProof.sol";

contract BabaChainAirdrop is Ownable, ReentrancyGuard {
    IERC20 public immutable babaChainToken;
    
    // Airdrop campaigns
    struct AirdropCampaign {
        bytes32 merkleRoot;
        uint256 totalAllocation;
        uint256 claimedAmount;
        uint256 startTime;
        uint256 endTime;
        bool active;
    }
    
    mapping(uint256 => AirdropCampaign) public campaigns;
    mapping(uint256 => mapping(address => bool)) public hasClaimed;
    mapping(address => uint256) public referralRewards;
    mapping(address => address) public referrers;
    mapping(address => uint256) public referralCount;
    
    uint256 public currentCampaignId;
    uint256 public constant REFERRAL_BONUS = 50 * 10**18; // 50 BabaChain
    uint256 public constant REFEREE_BONUS = 25 * 10**18;  // 25 BabaChain
    
    event AirdropClaimed(address indexed user, uint256 campaignId, uint256 amount);
    event ReferralReward(address indexed referrer, address indexed referee, uint256 amount);
    event CampaignCreated(uint256 indexed campaignId, uint256 allocation);
    
    constructor(address _babaChainToken) {
        babaChainToken = IERC20(_babaChainToken);
    }
    
    function createCampaign(
        bytes32 _merkleRoot,
        uint256 _totalAllocation,
        uint256 _startTime,
        uint256 _endTime
    ) external onlyOwner {
        require(_startTime < _endTime, "Invalid time range");
        require(_totalAllocation > 0, "Invalid allocation");
        
        campaigns[currentCampaignId] = AirdropCampaign({
            merkleRoot: _merkleRoot,
            totalAllocation: _totalAllocation,
            claimedAmount: 0,
            startTime: _startTime,
            endTime: _endTime,
            active: true
        });
        
        emit CampaignCreated(currentCampaignId, _totalAllocation);
        currentCampaignId++;
    }
    
    function claimAirdrop(
        uint256 _campaignId,
        uint256 _amount,
        bytes32[] calldata _merkleProof,
        address _referrer
    ) external nonReentrant {
        require(_campaignId < currentCampaignId, "Invalid campaign");
        require(!hasClaimed[_campaignId][msg.sender], "Already claimed");
        
        AirdropCampaign storage campaign = campaigns[_campaignId];
        require(campaign.active, "Campaign not active");
        require(block.timestamp >= campaign.startTime, "Campaign not started");
        require(block.timestamp <= campaign.endTime, "Campaign ended");
        require(campaign.claimedAmount + _amount <= campaign.totalAllocation, "Insufficient allocation");
        
        // Verify merkle proof
        bytes32 leaf = keccak256(abi.encodePacked(msg.sender, _amount));
        require(MerkleProof.verify(_merkleProof, campaign.merkleRoot, leaf), "Invalid proof");
        
        // Mark as claimed
        hasClaimed[_campaignId][msg.sender] = true;
        campaign.claimedAmount += _amount;
        
        // Process referral if applicable
        if (_referrer != address(0) && _referrer != msg.sender && referrers[msg.sender] == address(0)) {
            referrers[msg.sender] = _referrer;
            referralCount[_referrer]++;
            
            // Referrer bonus
            referralRewards[_referrer] += REFERRAL_BONUS;
            babaChainToken.transfer(_referrer, REFERRAL_BONUS);
            
            // Referee bonus
            _amount += REFEREE_BONUS;
            
            emit ReferralReward(_referrer, msg.sender, REFERRAL_BONUS);
        }
        
        // Transfer tokens
        babaChainToken.transfer(msg.sender, _amount);
        
        emit AirdropClaimed(msg.sender, _campaignId, _amount);
    }
    
    function getReferralTier(address _user) external view returns (string memory) {
        uint256 count = referralCount[_user];
        if (count >= 101) return "platinum";
        if (count >= 51) return "gold";
        if (count >= 11) return "silver";
        if (count >= 1) return "bronze";
        return "none";
    }
    
    function emergencyWithdraw() external onlyOwner {
        uint256 balance = babaChainToken.balanceOf(address(this));
        babaChainToken.transfer(owner(), balance);
    }
}
'''
        
        with open("contracts/BabaChainAirdrop.sol", "w") as f:
            f.write(contract_template)
        
        logger.info("✅ Smart contract template generated")
        return contract_template
    
    def create_complete_airdrop_system(self):
        """Create complete airdrop and referral system"""
        logger.info("🚀 Creating complete airdrop system...")
        
        # Create all components
        genesis_airdrop = self.create_genesis_airdrop()
        referral_system = self.create_referral_system()
        social_campaigns = self.create_social_media_campaigns()
        community_airdrops = self.create_community_airdrops()
        gamification = self.create_gamification_system()
        anti_fraud = self.create_anti_fraud_system()
        smart_contract = self.generate_airdrop_smart_contract()
        
        # Compile complete system
        complete_system = {
            "overview": {
                "total_airdrop_allocation": 5000000,  # 5M BabaChain tokens
                "campaign_duration": "90 days",
                "expected_participants": 100000,
                "viral_coefficient_target": 3.5,
                "roi_for_participants": "Up to 365% APY from staking"
            },
            "genesis_airdrop": genesis_airdrop,
            "referral_system": referral_system,
            "social_campaigns": social_campaigns,
            "community_airdrops": community_airdrops,
            "gamification": gamification,
            "anti_fraud": anti_fraud,
            "technical_implementation": {
                "smart_contract": "BabaChainAirdrop.sol",
                "merkle_tree": "For efficient airdrop distribution",
                "api_endpoints": "For tracking and verification",
                "database_schema": "User tracking and rewards"
            },
            "success_metrics": {
                "user_acquisition": "100K+ new users",
                "social_engagement": "1M+ social interactions",
                "referral_rate": "Average 3+ referrals per user",
                "retention_rate": "70%+ user retention after 30 days",
                "staking_adoption": "80%+ of airdrop recipients start staking"
            }
        }
        
        # Save to files
        os.makedirs("marketing", exist_ok=True)
        os.makedirs("contracts", exist_ok=True)
        
        with open("marketing/airdrop_system.json", "w") as f:
            json.dump(complete_system, f, indent=2)
        
        print("\n" + "="*60)
        print("🎁 BABACHAIN AIRDROP SYSTEM COMPLETE!")
        print("="*60)
        print(f"💰 Total Allocation: 5,000,000 BabaChain tokens")
        print(f"🎯 Target Participants: 100,000 users")
        print(f"🔗 Referral Bonus: 5% lifetime staking rewards")
        print(f"🏆 Achievement System: 6 badges, 5 levels")
        print(f"🌍 Community Focus: Turkish + DeFi + Crypto Twitter")
        print(f"📱 Social Campaigns: Twitter, TikTok, YouTube")
        
        print("\n📊 Expected Results:")
        print("  - 100K+ new users in 90 days")
        print("  - 3.5+ viral coefficient")
        print("  - 1M+ social media interactions")
        print("  - 80%+ airdrop recipients start staking")
        print("  - 70%+ user retention after 30 days")
        
        print("\n🛡️ Security Features:")
        print("  - Multi-level fraud detection")
        print("  - KYC verification tiers")
        print("  - Smart contract security")
        print("  - Behavioral analysis")
        
        print("\n📄 Files Created:")
        print("  - marketing/airdrop_system.json")
        print("  - contracts/BabaChainAirdrop.sol")
        
        logger.info("✅ Complete airdrop system created successfully!")
        return complete_system

def main():
    """Main function"""
    airdrop_system = BabaChainAirdropSystem()
    airdrop_system.create_complete_airdrop_system()

if __name__ == "__main__":
    main()