// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Groestlcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "groestlcoin.h"

#include <boost/assign/list_of.hpp>

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "consensus/merkle.h"
#include "consensus/params.h"
#include "utilstrencodings.h"
#include "crypto/sha256.h"

#include "bignum.h"

#include "chainparamsseeds.h"

#ifdef _MSC_VER
#   include <intrin.h>
#endif

extern "C" {

#if !defined(UCFG_LIBEXT) && (defined(_M_IX86) || defined(_M_X64)) && defined(_MSC_VER)

    static __inline void Cpuid(int a[4], int level) {
#   ifdef _MSC_VER
        __cpuid(a, level);
#   else
        __cpuid(level, a[0], a[1], a[2], a[3]);
#   endif
    }

    char g_bHasSse2;

    static int InitBignumFuns() {
        int a[4];
        ::Cpuid(a, 1);
        g_bHasSse2 = a[3] & 0x02000000;
        return 1;
    }

    static int s_initBignumFuns = InitBignumFuns();
#endif // defined(_M_IX86) || defined(_M_X64)
} // "C"

using namespace std;

int64_t static GetBlockSubsidy(int nHeight)
{
    int64_t nSubsidy = 250 * COIN;

    // Subsidy is cut in half every 2,102,400 blocks, which will occur approximately every 2 years
    nSubsidy >>= (nHeight / 2102400); // MYNT: Halve every 2,102,400 blocks (or almost exactly 2 years with constant 30 second blocks.

    return nSubsidy;
}

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams)
{
	return GetBlockSubsidy(nHeight);
}

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
static const int64_t nTargetSpacing = 1 * 30; // Mynt every 30 seconds

unsigned int static DarkGravityWave3(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
    /* current difficulty formula, darkcoin - DarkGravity v3, written by Evan Duffield - evan@darkcoin.io */
    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    CBigNum PastDifficultyAverage;
    CBigNum PastDifficultyAveragePrev;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return UintToArith256(params.powLimit).GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        CountBlocks++;

        if(CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) { PastDifficultyAverage.SetCompact(BlockReading->nBits); }
            else { PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks)+(CBigNum().SetCompact(BlockReading->nBits))) / (CountBlocks+1); }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if(LastBlockTime > 0){
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    CBigNum bnNew(PastDifficultyAverage);

    int64_t nTargetTimespan = CountBlocks*nTargetSpacing;

    if (nActualTimespan < nTargetTimespan/3)
        nActualTimespan = nTargetTimespan/3;
    if (nActualTimespan > nTargetTimespan*3)
        nActualTimespan = nTargetTimespan*3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > CBigNum(params.powLimit)) {
        bnNew = CBigNum(params.powLimit);
    }
    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
        return DarkGravityWave3(pindexLast, pblock, params);
}

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward) {
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward) {
    const char* pszTimestamp = "Trump finally orders flags lowered to honor McCain";
    const CScript genesisOutputScript = CScript() << ParseHex("044edd9fd6f7b2b4656878e9dd8a8a430ed317bb140220f2cb66280ce12835f23c751c09d235c5e069b29dc331b190093d1eaf2d65be9840ebe59137675bf138ca") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        //consensus.BIP16Height = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x000001770565c26f7c6c511b60303df357cb441a2edc19498fc8dfb2bc2b231a"); // GENESIS HASH
        consensus.BIP66Height = 0;
        consensus.BIP65Height = INT_MAX;    //!!!?

        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1484956800; // Jan 21, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1498003200; // Jun 21, 2017

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1484956800; // Jan 21, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1498003200; // Jun 21, 2017

        // Deployment of BIP65
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP65].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP65].nStartTime = 1484956800; // Jan 21, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP65].nTimeout = 1498003200; // Jun 21, 2017

        // The best chain should have at least this much work.
        //consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000ffffffffffff");

        // By default assume that the signatures in ancestors of this block are valid.
        //consensus.defaultAssumeValid = uint256S("0x0000000000002cb0ac72cf044034b93d43244501e496e55576aa5c5111bdc362"); //2035383

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x23;
        pchMessageStart[1] = 0x91;
        pchMessageStart[2] = 0x3b;
        pchMessageStart[3] = 0x8d;

        nDefaultPort = 6522;
        nPruneAfterHeight = 10000000;

        genesis = CreateGenesisBlock(1535404413, 11633717, 0x1e0fffff, 1, 0);
        
        /*
        nNonce is: 11633717
        Hash is: 000001770565c26f7c6c511b60303df357cb441a2edc19498fc8dfb2bc2b231a
        Block is: CBlock(hash=000001770565c26f7c6c511b60303df357cb441a2edc19498fc8dfb2bc2b231a, ver=0x00000001, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=b96b4d5ce624d3e50c30943fd0ed577ea08ec9a1329c2887e460e0b646331bac, nTime=1535404413, nBits=1e0fffff, nNonce=11633717, vtx=1)
          CTransaction(hash=b96b4d5ce6, ver=1, vin.size=1, vout.size=1, nLockTime=0)
            CTxIn(COutPoint(0000000000, 4294967295), coinbase 04ffff001d0104325472756d702066696e616c6c79206f726465727320666c616773206c6f776572656420746f20686f6e6f72204d634361696e)
            CScriptWitness()
            CTxOut(nValue=0.00000000, scriptPubKey=41044edd9fd6f7b2b4656878e9dd8a)
        */

        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000001770565c26f7c6c511b60303df357cb441a2edc19498fc8dfb2bc2b231a"));
        assert(genesis.hashMerkleRoot == uint256S("0xb96b4d5ce624d3e50c30943fd0ed577ea08ec9a1329c2887e460e0b646331bac"));

        vSeeds.push_back("node1.myntcurrency.org");
        vSeeds.push_back("node2.myntcurrency.org");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,50);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        bech32_hrp = "mynt";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

//!!!?        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

#ifdef _MSC_VER //!!!
        checkpointData = CCheckpointData{
#else
        checkpointData = (CCheckpointData) {
#endif
            {
                {0, uint256S("0x000001770565c26f7c6c511b60303df357cb441a2edc19498fc8dfb2bc2b231a")},
                {10000, uint256S("0000000001861c0e67ebce9a0d508479ca7ad26b3c11d32030c1f6fcf4c34139")},

            }
        };

        chainTxData = ChainTxData{
            1535587514, // * UNIX timestamp of last checkpoint block
            0,   // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            100.0     // * estimated number of transactions per day after checkpoint
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        //consensus.BIP16Height = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x000001b05db7930dbf7050e9e378182bc11ddf93f642bff8af478b3bfa5836bb");
        consensus.BIP66Height = 0;
        consensus.BIP65Height = INT_MAX;    //!!!?

        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1484956800; // Jan 21, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1498003200; // Jun 21, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1484956800; // Jan 21, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1498003200; // Jun 21, 2017

        consensus.powLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        // The best chain should have at least this much work.
        //consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000ffff");

        // By default assume that the signatures in ancestors of this block are valid.
        //consensus.defaultAssumeValid = uint256S("0x000000d6c9c013173e2313f24edc791a9134d379522901f7e40d9fc4f0e25bb0"); //525313
        pchMessageStart[0] = 0x91;
        pchMessageStart[1] = 0x23;
        pchMessageStart[2] = 0x8d;
        pchMessageStart[3] = 0x3b;

        nDefaultPort = 6555;
        nPruneAfterHeight = 1000000;

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis = CreateGenesisBlock(1535404419, 13843878, 0x1e00ffff, 3, 0);

        /*
        nNonce is: 13843878
        Hash is: 000001b05db7930dbf7050e9e378182bc11ddf93f642bff8af478b3bfa5836bb
        Block is: CBlock(hash=000001b05db7930dbf7050e9e378182bc11ddf93f642bff8af478b3bfa5836bb, ver=0x00000003, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=b96b4d5ce624d3e50c30943fd0ed577ea08ec9a1329c2887e460e0b646331bac, nTime=1535404419, nBits=1e00ffff, nNonce=13843878, vtx=1)
          CTransaction(hash=b96b4d5ce6, ver=1, vin.size=1, vout.size=1, nLockTime=0)
            CTxIn(COutPoint(0000000000, 4294967295), coinbase 04ffff001d0104325472756d702066696e616c6c79206f726465727320666c616773206c6f776572656420746f20686f6e6f72204d634361696e)
            CScriptWitness()
            CTxOut(nValue=0.00000000, scriptPubKey=41044edd9fd6f7b2b4656878e9dd8a)
        */

        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000001b05db7930dbf7050e9e378182bc11ddf93f642bff8af478b3bfa5836bb"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back("node1.myntcurrency.org");
        vSeeds.push_back("node2.myntcurrency.org");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,110);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        bech32_hrp = "tmynt";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

//!!!?        fMiningRequiresPeers = false;         //MYNT  Testnet can have single node
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


#ifdef _MSC_VER
        checkpointData = CCheckpointData{
#else
        checkpointData = (CCheckpointData) {
#endif
            {
                {0      , uint256S("0x000001b05db7930dbf7050e9e378182bc11ddf93f642bff8af478b3bfa5836bb")},
            }
        };

        chainTxData = ChainTxData{
            1535404419,
            0,
            10
        };
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CMainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";

        //consensus.BIP16Height = 0; // always enforce P2SH BIP16 on regtest
        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 1;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18888;

        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1440000002, 6556309, 0x1e00ffff, 3, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

#ifdef _MSC_VER
        checkpointData = CCheckpointData{
#else
        checkpointData = (CCheckpointData) {
#endif
            {
                {0, uint256S("0x0")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = { 0x04, 0x35, 0x87, 0xCF };
        base58Prefixes[EXT_SECRET_KEY] = { 0x04, 0x35, 0x83, 0x94 };

        bech32_hrp = "myntrt";
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
