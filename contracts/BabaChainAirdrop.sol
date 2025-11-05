
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
