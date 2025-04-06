#include "MatchServantImp.h"
#include "servant/Application.h"
#include "LogComm.h"
#include "globe.h"
#include "JFGameHttpProto.h"
#include "CommonStruct.h"
#include "CommonCode.h"
#include "XGameComm.pb.h"
#include "CommonCode.pb.h"
#include "CommonStruct.pb.h"
#include "match.pb.h"
#include "DataProxyProto.h"
#include "ServiceDefine.h"
#include "util/tc_hash_fun.h"
#include "MatchServer.h"
#include "Push.h"
#include "util/tc_base64.h"
#include "LogDefine.h"
#include "Processor.h"

//
using namespace std;
using namespace JFGame;
using namespace JFGameHttpProto;
using namespace match;
using namespace DaqiGame;
using namespace dataproxy;
using namespace userinfo;

static wbl::ReadWriteLocker m_lockOnline;
static map<string, map<long, long>> m_UOline;

//////////////////////////////////////////////////////
void MatchServantImp::initialize()
{
    //initialize servant here:
    //...
    initializeTimer();
    m_UOline.clear();
}

//////////////////////////////////////////////////////
void MatchServantImp::destroy()
{
    //destroy servant here:
    //...
}

void MatchServantImp::initializeTimer()
{
    m_threadTimer.initialize(this);
    m_threadTimer.start();
}

//http请求处理接口
tars::Int32 MatchServantImp::doRequest(const vector<tars::Char> &reqBuf, const map<std::string, std::string> &extraInfo,
                                       vector<tars::Char> &rspBuf, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;

    __TRY__

    ROLLLOG_DEBUG << "rec doRequest, reqBuf size : " << reqBuf.size() << endl;


    if (reqBuf.empty())
    {
        iRet = -1;
        return iRet;
    }

    THttpPackage thttpPack;
    THttpPackage thttpPackRsp;

    __TRY__
    if (!reqBuf.empty())
    {
        toObj(reqBuf, thttpPack);
    }
    __CATCH__

    if (thttpPack.vecData.empty())
    {
        iRet = -2;
        return iRet;
    }

    CommonReqHead reqHead;
    CommonRespHead rspHead;

    __TRY__
    if (!thttpPack.vecData.empty())
    {
        toObj(thttpPack.vecData, reqHead);
    }
    __CATCH__

    ROLLLOG_DEBUG << "request head, actionName id: " << reqHead.actionName << ", actionName: " << etos((ActionName)reqHead.actionName) << endl;

    if (reqHead.reqBodyBytes.empty())
    {
        iRet = -3;
        return iRet;
    }

    switch(reqHead.actionName)
    {
    default:
    {
        ROLLLOG_ERROR << "invalid msgid." << endl;
        iRet = -101;
        return iRet;
    }
    }

    thttpPackRsp.stUid.lUid = thttpPack.stUid.lUid;
    thttpPackRsp.stUid.sToken = thttpPack.stUid.sToken;
    thttpPackRsp.iVer = thttpPack.iVer;
    thttpPackRsp.iSeq = thttpPack.iSeq;
    tobuffer(rspHead, thttpPackRsp.vecData);
    tobuffer(thttpPackRsp, rspBuf);
    ROLLLOG_DEBUG << "response buff size: " << rspBuf.size() << endl;

    __CATCH__

    FUNC_EXIT("", iRet);
    return iRet;
}

//tcp请求处理接口
tars::Int32 MatchServantImp::onRequest(tars::Int64 lUin, const std::string &sMsgPack, const std::string &sCurServrantAddr,
                                       const JFGame::TClientParam &stClientParam, const JFGame::UserBaseInfoExt &stUserBaseInfo, tars::TarsCurrentPtr current)
{
    int iRet = 0;

    __TRY__

    ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", addr : " << stClientParam.sAddr << endl;

    MatchServant::async_response_onRequest(current, 0);

    XGameComm::TPackage pkg;
    pbToObj(sMsgPack, pkg);
    if (pkg.vecmsghead_size() == 0)
    {
        ROLLLOG_DEBUG << "package empty, uid: " << lUin << endl;
        return -1;
    }

    ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", msg : " << logPb(pkg) << endl;
    for (int i = 0; i < pkg.vecmsghead_size(); ++i)
    {
        int64_t ms1 = TNOWMS;

        auto &head = pkg.vecmsghead(i);
        switch(head.nmsgid())
        {
        case XGameProto::ActionName::MATCH_SIGN_UP:
        {
            matchProto::SignUpReq signUpReq;
            pbToObj(pkg.vecmsgdata(i), signUpReq);
            iRet = onSignUp(pkg, signUpReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_QUIT:
        {
            matchProto::QuitReq quitReq;
            pbToObj(pkg.vecmsgdata(i), quitReq);
            iRet = onQuit(pkg, quitReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_REPURCHASE:
        {
            matchProto::RepurchaseReq repurchaseReq;
            pbToObj(pkg.vecmsgdata(i), repurchaseReq);
            iRet = onRepurchase(pkg, repurchaseReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_ADDITIONAL:
        {
            matchProto::AdditionalReq additionalReq;
            pbToObj(pkg.vecmsgdata(i), additionalReq);
            iRet = onAdditional(pkg, additionalReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_JACKPOT:
        {
            matchProto::JackpotReq jackpotReq;
            pbToObj(pkg.vecmsgdata(i), jackpotReq);
            iRet = onJackpot(pkg, jackpotReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_PLAYER_COUNT:
        {
            matchProto::PlayerCountReq playerCountReq;
            pbToObj(pkg.vecmsgdata(i), playerCountReq);
            iRet = onPlayerCount(pkg, playerCountReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_PLAYER_INFO:
        {
            matchProto::PlayerInfoReq playerInfoReq;
            pbToObj(pkg.vecmsgdata(i), playerInfoReq);
            iRet = onPlayerInfo(pkg, playerInfoReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_GAME_INFO:
        {
            matchProto::GameInfoReq gameInfoReq;
            pbToObj(pkg.vecmsgdata(i), gameInfoReq);
            iRet = onGameInfo(pkg, gameInfoReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_USER_SIGN_UP_INFO:
        {
            matchProto::UserSignUpInfoReq userSignUpInfoReq;
            pbToObj(pkg.vecmsgdata(i), userSignUpInfoReq);
            iRet = onUserSignUpInfo(pkg, userSignUpInfoReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_USER_SIGN_UP_FLAG:
        {
            matchProto::UserSignUpFlagReq userSignUpFlagReq;
            pbToObj(pkg.vecmsgdata(i), userSignUpFlagReq);
            iRet = onUserSignUpFlag(pkg, userSignUpFlagReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::MATCH_LIST_MATCH_REWARD:
        {
            matchProto::ListRewardReq listRewardReq;
            pbToObj(pkg.vecmsgdata(i), listRewardReq);
            iRet = onListMatchReward(pkg, listRewardReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::E_SNG_INFO_LIST_REQ:
        {
            matchProto::SNGGetInfoListReq sngGetInfoListReq;
            pbToObj(pkg.vecmsgdata(i), sngGetInfoListReq);
            iRet = onListSNGRoom(pkg, sngGetInfoListReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::E_SNG_CONFIG_REQ:
        {
            matchProto::SNGConfigReq sngConfigReq;
            pbToObj(pkg.vecmsgdata(i), sngConfigReq);
            iRet = onListSNGConfig(pkg, sngConfigReq, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::E_SNG_PROCESS_RANKING_LIST_REQ:
        {
            matchProto::SNGProcessRankingListReq sngProcessRankingListReq;
            pbToObj(pkg.vecmsgdata(i), sngProcessRankingListReq);
            iRet = onListSNGRanking(pkg, S2I(sngProcessRankingListReq.matchid()), false, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::E_SNG_PROCESS_REFRESH_RANKING_LIST_REQ:
        {
            matchProto::SNGProcessRakningRefreshReq sngProcessRakningRefreshReq;
            pbToObj(pkg.vecmsgdata(i), sngProcessRakningRefreshReq);
            iRet = onListSNGRanking(pkg, S2I(sngProcessRakningRefreshReq.matchid()), true, sCurServrantAddr);
            break;
        }
        case XGameProto::ActionName::E_AI_PROCESS_ROOM_LIST_REQ:
        {
            iRet = onListAIRoomConfig(pkg, sCurServrantAddr);
            break;
        }
        default:
        {
            ROLLLOG_ERROR << "invalid msg id, uid: " << lUin << ", msg id: " << head.nmsgid() << endl;
            break;
        }
        }

        if (iRet != 0)
        {
            ROLLLOG_ERROR << "Process msg fail, uid: " << lUin << ", msg id: " << head.nmsgid()  << ", iRet: " << iRet << endl;
        }
        else
        {
            ROLLLOG_DEBUG << "Process msg succ, uid: " << lUin << ", msg id: " << head.nmsgid() << endl;
        }

        int64_t ms2 = TNOWMS;
        if ((ms2 - ms1) > COST_MS)
        {
            ROLLLOG_WARN << "@Performance, msgid:" << head.nmsgid() << ", costTime:" << (ms2 - ms1) << endl;
        }
    }

    __CATCH__
    return iRet;
}

//上报玩家游戏信息
tars::Int32 MatchServantImp::reportUserGameInfo(const match::ReportUserGameInfoReq &req, match::ReportUserGameInfoResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    if (req.matchID < 0)
    {
        ROLLLOG_ERROR << "parameter err, matchID: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    if (req.userInfo.size() == 0)
    {
        ROLLLOG_ERROR << "request data empty, user info size: " << req.userInfo.size() << endl;
        resp.resultCode = -2;
        return -2;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    ROLLLOG_DEBUG << "receive user game info report, matchID: " << req.matchID << ", userInfo size: " << req.userInfo.size() << endl;

    //加入排名
    for (auto it = req.userInfo.begin(); it != req.userInfo.end(); ++it)
    {
        //记入排名
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;

        vector<dbagent::TField> fields;
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        tfield.colValue = I2S(it->second.uid);
        fields.push_back(tfield);
        tfield.colValue = L2S(it->second.chip * -1);
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
            resp.resultCode = -3;
            return -3;
        }

        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
    }

    resp.resultCode = 0;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//上报玩家淘汰信息
tars::Int32 MatchServantImp::reportUserKnockoutInfo(const match::ReportUserKnockoutInfoReq &req, match::ReportUserKnockoutInfoResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;

    __TRY__

    if (req.matchID < 0)
    {
        ROLLLOG_DEBUG << "parameter err, matchID: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    if (req.vecUin.size() == 0)
    {
        ROLLLOG_DEBUG << "request data empty, user size: " << req.vecUin.size() << endl;
        resp.resultCode = -2;
        return -2;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    ROLLLOG_DEBUG << "report user ko info, matchID: " << req.matchID << ", user size: " << req.vecUin.size() << endl;

    for (auto it = req.vecUin.begin(); it != req.vecUin.end(); ++it)
    {
        //用户的报名信息
        UserSignUpInfo userSignUpInfo;
        {
            //是否已经报名
            TReadDataReq dataReq;
            dataReq.resetDefautlt();
            dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + I2S(*it);
            dataReq.operateType = E_REDIS_READ;
            dataReq.clusterInfo.resetDefautlt();
            dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            dataReq.clusterInfo.frageFactorType = 1;
            dataReq.clusterInfo.frageFactor = *it;

            dataproxy::TReadDataRsp dataRsp;
            iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
            if (iRet != 0 || dataRsp.iResult != 0)
            {
                ROLLLOG_DEBUG << "read match user info err, uid: " << *it << ", matchid: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
                resp.resultCode = -3;
                return -3;
            }

            ROLLLOG_DEBUG << "read match user info, uid: " << *it
                          << ", matchid: " << req.matchID
                          << ", fields size: " << dataRsp.fields.size() << endl;

            for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
            {
                for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
                {
                    ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                                  << ", colValue: " << itfields->colValue
                                  << ", colValue length: " << itfields->colValue.length() << endl;

                    __TRY__

                    string decode = TC_Base64::decode(itfields->colValue);
                    toObj(decode, userSignUpInfo);

                    __CATCH__
                }
            }
        }

        {
            //淘汰
            dataproxy::TWriteDataReq wdataReq;
            wdataReq.resetDefautlt();
            wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + I2S(*it);
            wdataReq.operateType = E_REDIS_INSERT;
            wdataReq.clusterInfo.resetDefautlt();
            wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            wdataReq.clusterInfo.frageFactor = *it;

            //查找对应比赛
            auto itsignup = userSignUpInfo.data.find(req.matchID);
            if (itsignup == userSignUpInfo.data.end())
                continue;

            itsignup->second.userState = USER_STATE_KNOCKOUT;

            dbagent::TField tfield;
            tfield.colArithType = E_NONE;
            vector<dbagent::TField> fields;
            string encode = TC_Base64::encode(tostring(userSignUpInfo));
            tfield.colValue = encode;
            if (tfield.colValue.length() == 0)
            {
                tfield.colValue = " ";
            }
            tfield.colArithType = dbagent::E_NONE;
            fields.push_back(tfield);
            wdataReq.fields = fields;

            dataproxy::TWriteDataRsp wdataRsp;
            iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
            ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
            if (iRet != 0 || wdataRsp.iResult != 0)
            {
                ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
                resp.resultCode = -4;
                return -4;
            }
        }
    }

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//上报玩家状态信息
tars::Int32 MatchServantImp::reportUserState(const match::ReportUserStateReq &req, match::ReportUserStateResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    if (req.matchID < 0)
    {
        ROLLLOG_DEBUG << "parameter err, matchID: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    if (req.mapState.size() == 0)
    {
        ROLLLOG_DEBUG << "request data empty, user size: " << req.mapState.size() << endl;
        resp.resultCode = -2;
        return -2;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    ROLLLOG_DEBUG << "report user state info, matchID: " << req.matchID << ", user state size: " << req.mapState.size() << endl;

    for (auto it = req.mapState.begin(); it != req.mapState.end(); ++it)
    {
        //用户的报名信息
        UserSignUpInfo userSignUpInfo;
        {
            //是否已经报名
            TReadDataReq dataReq;
            dataReq.resetDefautlt();
            dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + I2S(it->first);
            dataReq.operateType = E_REDIS_READ;
            dataReq.clusterInfo.resetDefautlt();
            dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            dataReq.clusterInfo.frageFactorType = 1;
            dataReq.clusterInfo.frageFactor = it->first;

            dataproxy::TReadDataRsp dataRsp;
            iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
            if (iRet != 0 || dataRsp.iResult != 0)
            {
                ROLLLOG_ERROR << "read match user info err, uid: " << it->first << ", matchid: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
                resp.resultCode = -3;
                return -3;
            }

            ROLLLOG_DEBUG << "read match user info, uid: " << it->first << ", matchid: " << req.matchID << ", fields size: " << dataRsp.fields.size() << endl;

            for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
            {
                for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
                {
                    ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                                  << ", colValue: " << itfields->colValue
                                  << ", colValue length: " << itfields->colValue.length() << endl;

                    __TRY__

                    string decode = TC_Base64::decode(itfields->colValue);
                    toObj(decode, userSignUpInfo);

                    __CATCH__
                }
            }
        }

        {
            //状态
            dataproxy::TWriteDataReq wdataReq;
            wdataReq.resetDefautlt();
            wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + I2S(it->first);
            wdataReq.operateType = E_REDIS_INSERT;
            wdataReq.clusterInfo.resetDefautlt();
            wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            wdataReq.clusterInfo.frageFactor = it->first;

            auto itsignup = userSignUpInfo.data.find(req.matchID);
            if (itsignup == userSignUpInfo.data.end())
                continue;

            itsignup->second.userState = it->second;

            dbagent::TField tfield;
            tfield.colArithType = E_NONE;
            vector<dbagent::TField> fields;

            string encode = TC_Base64::encode(tostring(userSignUpInfo));
            tfield.colValue = encode;
            if (tfield.colValue.length() == 0)
            {
                tfield.colValue = " ";
            }
            tfield.colArithType = dbagent::E_NONE;
            fields.push_back(tfield);
            wdataReq.fields = fields;

            dataproxy::TWriteDataRsp wdataRsp;
            iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
            ROLLLOG_DEBUG << "update game user state info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
            if (iRet != 0 || wdataRsp.iResult != 0)
            {
                ROLLLOG_ERROR << "update game user state info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
                resp.resultCode = -4;
                return -4;
            }
        }
    }

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//上报比赛信息
tars::Int32 MatchServantImp::reportMatchInfo(const match::ReportMatchInfoReq &req, match::ReportMatchInfoResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    if (req.data.size() <= 0)
    {
        ROLLLOG_ERROR << "parameter err, match info size: " << req.data.size() << endl;
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.data.size());
    if (!pDBAgentServant)
    {
        return -9110;
    }

    ROLLLOG_DEBUG << "report match info, req: " << printTars(req) << endl;

    for (auto it = req.data.begin(); it != req.data.end(); ++it)
    {
        ROLLLOG_DEBUG << "report match info, matchid: " << it->first << endl;

        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(GAME_MATCH_GAME_INFO) + ":" + L2S(it->first);
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = it->first;

        vector<TField> fields;
        TField tfield;
        tfield.colArithType = E_NONE;
        if (it->second.startTime > 0)
        {
            tfield.colName = "start_time";
            tfield.colType = INT;
            tfield.colValue = I2S(it->second.startTime);
            fields.push_back(tfield);
        }

        tfield.colName = "start_time_type";
        tfield.colType = INT;
        tfield.colValue = I2S(it->second.startTimeType);
        fields.push_back(tfield);
        tfield.colName = "remaining_count";
        tfield.colType = INT;
        tfield.colValue = I2S(it->second.remainingCount);
        fields.push_back(tfield);
        tfield.colName = "avg_chip";
        tfield.colType = BIGINT;
        tfield.colValue = L2S(it->second.avgChip);
        fields.push_back(tfield);
        tfield.colName = "remaining_time";
        tfield.colType = INT;
        tfield.colValue = I2S(it->second.remainingTime);
        fields.push_back(tfield);
        tfield.colName = "blind_level";
        tfield.colType = INT;
        tfield.colValue = I2S(it->second.blindLevel);
        fields.push_back(tfield);
        tfield.colName = "max_blind_level";
        tfield.colType = INT;
        tfield.colValue = I2S(it->second.maxBlindLevel);
        fields.push_back(tfield);
        tfield.colName = "match_state";
        tfield.colType = INT;
        tfield.colValue = I2S(it->second.matchState);
        fields.push_back(tfield);
        wdataReq.fields = fields;

        TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "set game info data, iRet: " << iRet << ", wdataRsp: " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "save game info data err, iRet: " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            return -2;
        }
    }

    resp.resultCode = 0;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

tars::Int32 MatchServantImp::reportMatchReward(const match::RewardGoods &req, int matchID, int tableID, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(S2I(L2S(matchID) + L2S(tableID)));
    if (!pDBAgentServant)
    {
        return -9110;
    }

    dataproxy::TWriteDataReq wdataReq;
    wdataReq.resetDefautlt();
    wdataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(GAME_MATCH_REWARD) + ":" + L2S(matchID) +  L2S(tableID);
    wdataReq.operateType = E_REDIS_WRITE;
    wdataReq.clusterInfo.resetDefautlt();
    wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    wdataReq.clusterInfo.frageFactor = S2I(L2S(matchID) + L2S(tableID));
    wdataReq.paraExt.resetDefautlt();
    wdataReq.paraExt.queryType = E_REPLACE;

    vector<TField> fields;
    TField tfield;
    tfield.colArithType = E_NONE;
    tfield.colName = "propID";
    tfield.colType = INT;
    tfield.colValue = I2S(req.propsID);
    fields.push_back(tfield);
    tfield.colName = "number";
    tfield.colType = INT;
    tfield.colValue = I2S(req.number);
    fields.push_back(tfield);
    wdataReq.fields = fields;

    TWriteDataRsp wdataRsp;
    iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
    ROLLLOG_DEBUG << "set game reward data, iRet: " << iRet << ", wdataRsp: " << printTars(wdataRsp) << endl;
    if (iRet != 0 || wdataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "save game reward data err, iRet: " << iRet << ", iResult: " << wdataRsp.iResult << endl;
        return -2;
    }

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//用户报名信息请求
tars::Int32 MatchServantImp::getUserSignUpInfo(const match::UserSignUpInfoReq &req, match::UserSignUpInfoResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;

    __TRY__

    if (req.uid < 0 || req.matchID < 0)
    {
        ROLLLOG_ERROR << "paramter err, uid: " << req.uid << ", matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.uid);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    //用户的报名信息
    UserSignUpInfo userSignUpInfo;

    {
        //是否已经报名
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = 1;
        dataReq.clusterInfo.frageFactor = req.uid;

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, uid: " << req.uid << ", iResult: " << dataRsp.iResult << endl;
            resp.resultCode = -2;
            return -2;
        }

        ROLLLOG_DEBUG << "read match user info, uid: " << req.uid << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                              << ", colValue: " << itfields->colValue
                              << ", colValue length: " << itfields->colValue.length() << endl;

                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, userSignUpInfo);

                __CATCH__

                auto itfind = userSignUpInfo.data.find(req.matchID);
                if (itfind != userSignUpInfo.data.end())
                {
                    resp.resultCode = 0;
                    resp.info = itfind->second;
                    itfind->second.showtype = req.showtype;
                    ROLLLOG_DEBUG << "matchID: " << req.matchID << ", showtype: " <<  req.showtype << endl;
                }
            }
        }
    }

    {

        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.uid;

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;

        string encode = TC_Base64::encode(tostring(userSignUpInfo));
        tfield.colValue = encode;
        if (tfield.colValue.length() == 0)
        {
            tfield.colValue = " ";
        }
        tfield.colArithType = dbagent::E_NONE;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -12;
            return -12;
        }
    }

    resp.resultCode = -3;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//取用户所有报名信息
tars::Int32 MatchServantImp::getUserAllSignUpInfo(const match::UserAllSignUpInfoReq &req, match::UserAllSignUpInfoResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;

    __TRY__

    if (req.uid < 0)
    {
        ROLLLOG_ERROR << "paramter err, uid: " << req.uid << endl;
        resp.resultCode = -1;
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.uid);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    //用户的报名信息
    UserSignUpInfo userSignUpInfo;

    {
        //是否已经报名
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = 1;
        dataReq.clusterInfo.frageFactor = req.uid;

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, uid: " << req.uid << ", iResult: " << dataRsp.iResult << endl;
            resp.resultCode = -2;
            return -2;
        }

        ROLLLOG_DEBUG << "read match user info, uid: " << req.uid << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                              << ", colValue: " << itfields->colValue
                              << ", colValue length: " << itfields->colValue.length() << endl;

                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, userSignUpInfo);

                __CATCH__

                resp.resultCode = 0;
                resp.info = userSignUpInfo;
                return 0;
            }
        }
    }

    resp.resultCode = -3;
    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//增购，重购消耗请求
tars::Int32 MatchServantImp::consumeBuyRes(const match::ConsumeBuyResReq &req, match::ConsumeBuyResResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid);
    if (!pHallServant)
    {
        return -9111;
    }

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(req.matchID);
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "can not find match room config, matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    MatchRoom matchRoom = it->second;
    resp.add = req.add;

    //查询用户所有的报名信息
    match::UserAllSignUpInfoReq userAllSignUpInfoReq;
    match::UserAllSignUpInfoResp userAllSignUpInfoResp;
    userAllSignUpInfoReq.uid = req.uid;
    iRet = getUserAllSignUpInfo(userAllSignUpInfoReq, userAllSignUpInfoResp, current);
    if (iRet != 0)
    {
        ROLLLOG_DEBUG << "get user signup info, uid: " << req.uid << endl;
        resp.resultCode = -2;
        return -2;
    }

    //消费类型
    switch(req.consumeType)
    {
    case CONSUME_TYPE_REPURCHASE:  //重购
    {
        auto it = userAllSignUpInfoResp.info.data.find(req.matchID);
        //找不到报名的比赛
        if (it == userAllSignUpInfoResp.info.data.end())
        {
            resp.resultCode = -3;
            return -3;
        }

        //重购机会
        if (it->second.repurchase > 0)
        {
            it->second.repurchase--;
            resp.left = it->second.repurchase;
        }
        else
        {
            resp.resultCode = -4;
            return -4;
        }
    }
    break;

    case CONSUME_TYPE_ADDITIONAL: //增购
    {
        auto it = userAllSignUpInfoResp.info.data.find(req.matchID);
        if (it == userAllSignUpInfoResp.info.data.end())
        {
            resp.resultCode = -5;
            return -5;
        }

        //增购机会
        if (it->second.additional > 0)
        {
            it->second.additional--;
            resp.left = it->second.additional;
        }
        else
        {
            resp.resultCode = -6;
            return -6;
        }
    }
    break;
    default:
    {
        resp.resultCode = -7;
        return -7;
    }
    }

    //扣除费用
    if (req.add)
    {
        auto itFee = allMatchConfig.mapEntryFeeConfig.find(matchRoom.entryFeeID);
        if (itFee == allMatchConfig.mapEntryFeeConfig.end())
        {
            ROLLLOG_ERROR << "get entry fee config err" << endl;
            resp.resultCode = -8;
            return -8;
        }

        //玩家货币信息
        GetUserAccountReq getUserAccountReq;
        GetUserAccountResp getUserAccountResp;
        getUserAccountReq.uid = req.uid;
        iRet = pHallServant->getUserAccount(getUserAccountReq, getUserAccountResp);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "get entry fee config err" << endl;
            resp.resultCode = -9;
            return -9;
        }

        EntryFee entryFee;
        for (auto itentryFee = itFee->second.begin(); itentryFee != itFee->second.end(); ++itentryFee)
        {
            switch(req.consumeType)
            {
            case CONSUME_TYPE_REPURCHASE:  //重购
                if (itentryFee->style == E_BUY_STYLE_REPURCHASE)
                {
                    entryFee = *itentryFee;
                    break;
                }
            case CONSUME_TYPE_ADDITIONAL: //增购
                if (itentryFee->style == E_BUY_STYLE_INCREASE)
                {
                    entryFee = *itentryFee;
                    break;
                }
            }
        }

        //道具
        auto itProps = allMatchConfig.mapPropsConfig.find(entryFee.propsID);
        if (itProps != allMatchConfig.mapPropsConfig.end())
        {
            ModifyUserAccountReq modifyUserAccountReq;
            modifyUserAccountReq.uid = req.uid;

            if (req.consumeType == CONSUME_TYPE_REPURCHASE)
            {
                modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_REPURCHASE;  //比赛重购费
            }
            else
            {
                modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_ADD_PURCHASE;  //比赛增购费
            }

            switch (itProps->second.type)
            {
            case E_GOODS_TYPE_JETTON: //金币
            {
                if (getUserAccountResp.gold >= entryFee.number)
                {
                    modifyUserAccountReq.goldChange = -entryFee.number;
                }
                else
                {
                    resp.resultCode = -10;
                    return -10;
                }
            }
            break;
            case E_GOODS_TYPE_POINT:
            {
                if (getUserAccountResp.score >= entryFee.number)
                {
                    modifyUserAccountReq.scoreChange = -entryFee.number;
                }
                else
                {
                    resp.resultCode = -10;
                    return -10;
                }
            }
            default:
            {
                break;
            }
            }

            iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
            if (iRet != 0)
            {
                ROLLLOG_ERROR << "modifyUserAccount err iRet:" << iRet << endl;
            }
        }
    }

    //保存用户报名信息
    {
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.uid;

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;

        string encode = TC_Base64::encode(tostring(userAllSignUpInfoResp.info));
        tfield.colValue = encode;
        if (tfield.colValue.length() == 0)
        {
            tfield.colValue = " ";
        }
        tfield.colArithType = dbagent::E_NONE;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -12;
            return -12;
        }
    }

    //增加奖池
    {
        /*dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = L2S(jackpot);
        tfield.colArithType = Eum_Col_Arith_Type::E_ADD;
        fields.push_back(tfield);

        wdataReq.fields = fields;
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game match jackpot, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_DEBUG << "update game match jackpot err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -13;
            return -13;
        }*/
    }

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//取比赛信息请求
tars::Int32 MatchServantImp::getMatchInfo(const match::MatchInfoReq &req, match::MatchInfoResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    if (req.matchID < 0)
    {
        ROLLLOG_ERROR << "paramter err, matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    //查询比赛信息
    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(GAME_MATCH_GAME_INFO) + ":" + I2S(req.matchID);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = req.matchID;

    vector<TField> fields;
    TField tfield;
    tfield.colArithType = E_NONE;
    tfield.colName = "start_time";
    tfield.colType = INT;
    fields.push_back(tfield);
    tfield.colName = "start_time_type";
    tfield.colType = INT;
    fields.push_back(tfield);
    tfield.colName = "remaining_count";
    tfield.colType = INT;
    fields.push_back(tfield);
    tfield.colName = "avg_chip";
    tfield.colType = BIGINT;
    fields.push_back(tfield);
    tfield.colName = "remaining_time";
    tfield.colType = INT;
    fields.push_back(tfield);
    tfield.colName = "blind_level";
    tfield.colType = INT;
    fields.push_back(tfield);
    tfield.colName = "max_blind_level";
    tfield.colType = INT;
    fields.push_back(tfield);
    tfield.colName = "match_state";
    tfield.colType = INT;
    fields.push_back(tfield);
    dataReq.fields = fields;

    dataproxy::TReadDataRsp dataRsp;
    iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "read match game info, match id: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
        resp.resultCode = -2;
        return -2;
    }

    ROLLLOG_DEBUG << "read match game info, match id: " << req.matchID << ", fields size: " << dataRsp.fields.size() << endl;

    // for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
    // {
    //     for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
    //     {
    //         ROLLLOG_DEBUG << "read match game info, colName: " << itfields->colName << ", colValue: " << itfields->colValue << endl;
    //     }
    // }

    resp.resultCode = 0;
    //resp.matchID = req.matchID;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//比赛报名信息请求
tars::Int32 MatchServantImp::getMatchUserInfo(const match::MatchUserInfoReq &req, match::MatchUserInfoResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    if (req.matchID < 0)
    {
        ROLLLOG_ERROR << "paramter err, matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = req.matchID;
    //dataReq.paraExt.start = 0;
    //dataReq.paraExt.end = g_app.getOuterFactoryPtr()->showPlayerCount();

    dataproxy::TReadDataRsp dataRsp;
    iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "read match user info err, match id: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
        resp.resultCode = -2;
        return -2;
    }

    ROLLLOG_DEBUG << "read match user info, match id: " << req.matchID << ", fields size: " << dataRsp.fields.size() << endl;

    for (auto row_ptr = dataRsp.fields.begin(); row_ptr != dataRsp.fields.end(); ++row_ptr)
    {
        tars::Int64 member = S2L(row_ptr->front().colValue);
        //tars::Int64 score = S2L(row_ptr->back().colValue);
        resp.vecUid.push_back(member);
    }

    resp.resultCode = 0;
    ROLLLOG_DEBUG << "get match user info, req: " << printTars(req) << ", resp: " << printTars(resp) << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//取消比赛
tars::Int32 MatchServantImp::cancelMatch(const match::CancelMatchReq &req, match::CancelMatchResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    if (req.matchID < 0)
    {
        ROLLLOG_ERROR << "paramter err, matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    //报名用户
    vector<long> vecUid;
    {
        //查询报名用户
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = req.matchID;

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, match id: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
            resp.resultCode = -2;
            return -2;
        }

        ROLLLOG_DEBUG << "read match user info, match id: " << req.matchID << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto row_ptr = dataRsp.fields.begin(); row_ptr != dataRsp.fields.end(); ++row_ptr)
        {
            tars::Int64 member = S2L(row_ptr->front().colValue);
            //tars::Int64 score = S2L(row_ptr->back().colValue);
            vecUid.push_back(member);
        }
    }

    //清理用户的报名
    for (auto uid : vecUid)
    {
        auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(uid);
        if (!pHallServant)
        {
            LOG_DEBUG << "pHallServant is null, uid: "  << uid << endl;
            continue;
        }

        //查询用户所有的报名信息
        match::UserAllSignUpInfoReq userAllSignUpInfoReq;
        match::UserAllSignUpInfoResp userAllSignUpInfoResp;
        userAllSignUpInfoReq.uid = uid;
        iRet = getUserAllSignUpInfo(userAllSignUpInfoReq, userAllSignUpInfoResp, current);
        if (iRet != 0)
            continue;

        //删除报名信息
        auto itsignup = userAllSignUpInfoResp.info.data.find(req.matchID);
        if (itsignup != userAllSignUpInfoResp.info.data.end())
        {
            //道具配置
            auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
            auto itProps = allMatchConfig.mapPropsConfig.find(itsignup->second.userEntryFee.propsID);
            if (itProps == allMatchConfig.mapPropsConfig.end())
            {
                ROLLLOG_ERROR << "find props config err, iRet : " << ", uid: " << uid << iRet << ", matchid: " << req.matchID << endl;
                resp.resultCode = -6;
                return -6;
            }

            ModifyUserAccountReq modifyUserAccountReq;
            modifyUserAccountReq.uid = uid;
            modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SNG_SIGN;
            switch (itProps->second.type)
            {
            case E_GOODS_TYPE_JETTON: //金币
                modifyUserAccountReq.goldChange = itsignup->second.userEntryFee.number;
                break;
            case E_GOODS_TYPE_POINT://积分
                modifyUserAccountReq.scoreChange = itsignup->second.userEntryFee.number;
                break;
            default:
                break;
            }

            iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
            if (iRet != 0)
            {
                ROLLLOG_ERROR << "restore entry fee err, iRet : " << iRet << ", uid: " << uid << ", matchid: " << req.matchID << endl;
                resp.resultCode = -7;
                return -7;
            }

            userAllSignUpInfoResp.info.data.erase(itsignup);
        }

        //保存用户报名信息
        {
            dataproxy::TWriteDataReq wdataReq;
            wdataReq.resetDefautlt();
            wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(uid);
            wdataReq.operateType = E_REDIS_INSERT;
            wdataReq.clusterInfo.resetDefautlt();
            wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            wdataReq.clusterInfo.frageFactor = uid;

            //data
            dbagent::TField tfield;
            tfield.colArithType = E_NONE;
            vector<dbagent::TField> fields;

            string encode = TC_Base64::encode(tostring(userAllSignUpInfoResp.info));
            tfield.colValue = encode;
            if (tfield.colValue.length() == 0)
            {
                tfield.colValue = " ";
            }
            tfield.colArithType = dbagent::E_NONE;
            fields.push_back(tfield);
            wdataReq.fields = fields;

            dataproxy::TWriteDataRsp wdataRsp;
            iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
            ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
            if (iRet != 0 || wdataRsp.iResult != 0)
            {
                ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
                continue;
            }
        }
    }

    {
        //清理报名排名数据
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_DELETE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;
        wdataReq.paraExt.resetDefautlt();
        wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM_RANGE_BY_RANK;
        wdataReq.paraExt.start = 0;
        wdataReq.paraExt.end = -1;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
        }
    }

    {
        //清理奖池
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = "0";
        tfield.colArithType = Eum_Col_Arith_Type::E_NONE;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game match jackpot, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game match jackpot err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -5;
            return -5;
        }
    }

    //清理比赛信息
    iRet = ProcessorSingleton::getInstance()->deleteGameInfo(req.matchID);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "delete game info err, iRet : " << iRet << endl;
        return -6;
    }

    resp.resultCode = 0;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//清理比赛
tars::Int32 MatchServantImp::cleanMatch(const match::CleanMatchReq &req, match::CleanMatchResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    if (req.matchID < 0)
    {
        ROLLLOG_ERROR << "paramter err, matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    //报名用户
    vector<long> vecUid;
    if (req.bSng && req.Uid > 0)
    {
        vecUid.push_back(req.Uid);
    }
    else
    {
        //查询报名用户
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = req.matchID;

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, match id: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
            resp.resultCode = -2;
            return -2;
        }

        ROLLLOG_DEBUG << "read match user info, match id: " << req.matchID << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto row_ptr = dataRsp.fields.begin(); row_ptr != dataRsp.fields.end();  ++row_ptr)
        {
            tars::Int64 member = S2L(row_ptr->front().colValue);
            vecUid.push_back(member);
        }
    }

    //清理用户的报名
    for (auto uid : vecUid)
    {
        //查询用户所有的报名信息
        match::UserAllSignUpInfoReq userAllSignUpInfoReq;
        match::UserAllSignUpInfoResp userAllSignUpInfoResp;
        userAllSignUpInfoReq.uid = uid;
        iRet = getUserAllSignUpInfo(userAllSignUpInfoReq, userAllSignUpInfoResp, current);
        if (iRet != 0)
            continue;

        //删除报名信息
        auto itsignup = userAllSignUpInfoResp.info.data.find(req.matchID);
        if (itsignup != userAllSignUpInfoResp.info.data.end())
        {
            userAllSignUpInfoResp.info.data.erase(itsignup);
        }

        //保存用户报名信息
        {
            dataproxy::TWriteDataReq wdataReq;
            wdataReq.resetDefautlt();
            wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(uid);
            wdataReq.operateType = E_REDIS_INSERT;
            wdataReq.clusterInfo.resetDefautlt();
            wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            wdataReq.clusterInfo.frageFactor = uid;

            //data
            dbagent::TField tfield;
            tfield.colArithType = E_NONE;
            vector<dbagent::TField> fields;

            string encode = TC_Base64::encode(tostring(userAllSignUpInfoResp.info));
            tfield.colValue = encode;
            if (tfield.colValue.length() == 0)
            {
                tfield.colValue = " ";
            }
            fields.push_back(tfield);
            wdataReq.fields = fields;

            dataproxy::TWriteDataRsp wdataRsp;
            iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
            ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", uid : " << uid << ", value:" << tfield.colValue << ", matchID:" << req.matchID << endl;
            if (iRet != 0 || wdataRsp.iResult != 0)
            {
                ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
                continue;
            }
        }
    }

    if (!req.bSng)
    {
        //清理报名排名数据
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_DELETE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;
        wdataReq.paraExt.resetDefautlt();
        wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM_RANGE_BY_RANK;
        wdataReq.paraExt.start = 0;
        wdataReq.paraExt.end = -1;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
        }
    }
    else
    {
        //清理报名排名数据
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;
        wdataReq.paraExt.resetDefautlt();
        wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM;

        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = I2S(req.Uid);
        fields.push_back(tfield);
        wdataReq.fields = fields;

        //
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
        }
    }

    {
        //清理奖池
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = "0";
        tfield.colArithType = Eum_Col_Arith_Type::E_NONE;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game match jackpot, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game match jackpot err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -5;
            return -5;
        }
    }

    //清理比赛信息
    if (!req.bSng)
    {
        iRet = ProcessorSingleton::getInstance()->deleteGameInfo(req.matchID);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "delete game info err, iRet : " << iRet << endl;
            resp.resultCode = -6;
            return -6;
        }
    }

    resp.resultCode = 0;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//奖池分配比列
#define JACKPOT_RATIO_DIVIDEND   10000

//计算奖励
int MatchServantImp::calreward(const match::RewardReq &req, match::RewardResp &resp)
{
    int iRet = 0;

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(req.matchID);
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "can not find match room config, matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    //
    string roomName = it->second.name;
    string roomID = it->second.roomID;

    //比赛奖励
    MatchReward matchReward;
    std::vector<long> vecAllUid;
    ROLLLOG_ERROR << "isSng: " << req.isSng << ", items size:" << req.items.size() << ", uid:" << req.vecUid[0] << ", matchID:" << req.matchID << endl;

    if (req.isSng && req.items.size() > 0)
    {
        long uid = req.vecUid[0];

        //查找排名奖励
        auto itdata = resp.data.find(1);
        if (itdata != resp.data.end())
        {
            match::RewardGoods rewardGoods;
            rewardGoods.propsID = req.items[0].propsID;
            rewardGoods.number = req.items[0].number;
            rewardGoods.type = 0;
            itdata->second.goods.push_back(rewardGoods);
        }
        else
        {
            match::RewardGoods rewardGoods;
            rewardGoods.propsID = req.items[0].propsID;
            rewardGoods.number = req.items[0].number;
            rewardGoods.type = 0;

            UserRewardGoods userRewardGoods;
            userRewardGoods.uid = uid;
            userRewardGoods.total = 0;
            userRewardGoods.ratio = 1;
            userRewardGoods.goods.push_back(rewardGoods);
            resp.data.insert(make_pair(1, userRewardGoods));
        }
    }
    else
    {
        //比赛奖励配置
        auto itMatchReward = allMatchConfig.mapMatchRewardConfigExt.find(it->second.id);
        if (itMatchReward == allMatchConfig.mapMatchRewardConfigExt.end())
        {
            ROLLLOG_ERROR << "can not find match reward config, matchid: " << req.matchID << endl;
            resp.resultCode = -2;
            return -2;
        }

        //
        int playerNum = 0;
        if (req.isSng && roomID.size() > 3)
        {
            playerNum = roomID[2] - '0';
        }
        else
        {
            TReadDataReq dataReq;
            dataReq.resetDefautlt();
            dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
            dataReq.operateType = E_REDIS_READ;
            dataReq.clusterInfo.resetDefautlt();
            dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            dataReq.clusterInfo.frageFactor = req.matchID;

            dataproxy::TReadDataRsp dataRsp;
            iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
            if (iRet != 0 || dataRsp.iResult != 0)
            {
                ROLLLOG_DEBUG << "read player info err, match id: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
                resp.resultCode = -3;
                return -3;
            }

            playerNum = dataRsp.fields.size();
        }

        ROLLLOG_DEBUG << "read player info, match id: " << req.matchID << ", playerNum: " << playerNum << endl;

        //比赛奖励配置
        bool bfind = false;
        for (auto itreward = itMatchReward->second.begin(); itreward != itMatchReward->second.end(); ++itreward)
        {
            if (playerNum  >= itreward->minPlay && playerNum <= itreward->maxPlay)
            {
                matchReward = *itreward;
                bfind = true;
            }
        }

        //未找到对应奖励配置
        if (!bfind)
        {
            ROLLLOG_DEBUG << "find match reward err, match id: " << req.matchID << endl;
            resp.resultCode = -4;
            return -4;
        }

        auto itreward = allMatchConfig.mapRewardConfigExt.find(matchReward.rewardID);
        if (itreward == allMatchConfig.mapRewardConfigExt.end())
        {
            ROLLLOG_ERROR << "can not find reward config, matchid: " << req.matchID << endl;
            resp.resultCode = -6;
            return -6;
        }

        //奖池
        long jackpot = 0;

        //取奖池
        {
            TReadDataReq dataReq;
            dataReq.resetDefautlt();
            dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(req.matchID);
            dataReq.operateType = E_REDIS_READ;
            dataReq.clusterInfo.resetDefautlt();
            dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            dataReq.clusterInfo.frageFactor = req.matchID;

            dataproxy::TReadDataRsp dataRsp;
            iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
            if (iRet != 0 || dataRsp.iResult != 0)
            {
                ROLLLOG_DEBUG << "read match jackpot err, match id: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
                resp.resultCode = -7;
                return -7;
            }

            //奖池数据
            ROLLLOG_DEBUG << "read match jackpot, match id: " << req.matchID << ", fields size: " << dataRsp.fields.size() << endl;

            for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
            {
                for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
                {
                    ROLLLOG_DEBUG << "read match jackpot, colName: " << itfields->colName << ", colValue: " << itfields->colValue << endl;
                    //奖池
                    jackpot = S2L(itfields->colValue);
                    //
                    ROLLLOG_DEBUG << "reward jackpot: " << jackpot << endl;
                }
            }

            //初始奖池
            if (!req.isSng)
            {
                auto it = allMatchConfig.mapMatchRoomConfig.find(req.matchID);
                if (it != allMatchConfig.mapMatchRoomConfig.end())
                {
                    jackpot += it->second.initialPool;
                }
            }
        }

        //派奖
        int start = 0;
        int end = 0;
        if (vecAllUid.size() == 1 && req.isSng)
        {
            start = req.iRank;
            end = req.iRank;
        }
        else
        {
            start = 1;
            end = (unsigned int)matchReward.number > vecAllUid.size() ? (int)vecAllUid.size() : matchReward.number;
        }

        if (start > end)
        {
            ROLLLOG_ERROR << "start: " << start << ", end:" << end << endl;
            return -7;
        }

        for (int i = start; i <= end; ++i)
        {
            //累计奖励
            long sumreward = 0;
            ROLLLOG_DEBUG << "reward rank: " << i << ", uid:" << vecAllUid[i - 1] << endl;
            for (auto it = itreward->second.begin(); it != itreward->second.end(); ++it)
            {
                //按照排名发送奖励
                if (it->rank == i)
                {
                    long rankJackpot = 0;

                    //道具奖励
                    auto &mapRewardProps = allMatchConfig.mapRewardPropsConfigExt;
                    auto itrewardProps = mapRewardProps.find(it->id);
                    if (itrewardProps != mapRewardProps.end())
                    {
                        auto itrewardRank = itrewardProps->second.find(it->rank);
                        if (itrewardRank != itrewardProps->second.end())
                        {
                            //排名奖励
                            rankJackpot = jackpot * itrewardRank->second.ratio / JACKPOT_RATIO_DIVIDEND;

                            ROLLLOG_DEBUG << "rank props size: " << itrewardRank->second.item.size() << endl;
                            for (auto itrankProps = itrewardRank->second.item.begin(); itrankProps != itrewardRank->second.item.end(); ++itrankProps)
                            {
                                //道具配置
                                auto itProps = allMatchConfig.mapPropsConfig.find(itrankProps->propsID);
                                if (itProps != allMatchConfig.mapPropsConfig.end())
                                {
                                    sumreward += itrankProps->number * itProps->second.value;
                                    long uid = vecAllUid[i - 1];
                                    if (sumreward > rankJackpot)
                                    {
                                        sumreward -= itrankProps->number * itProps->second.value;
                                        // break;
                                        continue;
                                    }

                                    //查找排名奖励
                                    auto itdata = resp.data.find(i);
                                    if (itdata != resp.data.end())
                                    {
                                        match::RewardGoods rewardGoods;
                                        rewardGoods.propsID = itProps->second.id;
                                        rewardGoods.number = itrankProps->number;
                                        rewardGoods.type = 0;
                                        itdata->second.goods.push_back(rewardGoods);
                                    }
                                    else
                                    {
                                        match::RewardGoods rewardGoods;
                                        rewardGoods.propsID = itProps->second.id;
                                        rewardGoods.number = itrankProps->number;
                                        rewardGoods.type = 0;

                                        UserRewardGoods userRewardGoods;
                                        userRewardGoods.uid = uid;
                                        userRewardGoods.total = 0;
                                        userRewardGoods.ratio = it->ratio;
                                        userRewardGoods.goods.push_back(rewardGoods);
                                        resp.data.insert(make_pair(i, userRewardGoods));
                                    }
                                }
                            }
                        }
                    }

                    //剩余金币奖励
                    long remain = rankJackpot - sumreward;
                    if (remain < 0)
                    {
                        break;
                    }

                    //查找排名奖励
                    auto itdata = resp.data.find(i);
                    if (itdata != resp.data.end())
                    {
                        match::RewardGoods rewardGoods;
                        rewardGoods.propsID = 10000;
                        rewardGoods.number = remain;
                        rewardGoods.type = 1;
                        itdata->second.goods.push_back(rewardGoods);
                    }
                    else
                    {
                        match::RewardGoods rewardGoods;
                        rewardGoods.propsID = 10000;
                        rewardGoods.number = remain;
                        rewardGoods.type = 1;

                        UserRewardGoods userRewardGoods;
                        userRewardGoods.uid = 0;
                        userRewardGoods.total = rankJackpot;
                        userRewardGoods.ratio = it->ratio;
                        userRewardGoods.goods.push_back(rewardGoods);
                        resp.data.insert(make_pair(i, userRewardGoods));
                    }
                }
            }
        }
    }

    return iRet;
}

//发送奖励
tars::Int32 MatchServantImp::reward(const match::RewardReq &req, match::RewardResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    //比赛配置
    const ListAllMatchConfigResp &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    map<int, MatchRoom>::const_iterator it1 = allMatchConfig.mapMatchRoomConfig.find(req.matchID);
    if (it1 == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_DEBUG << "can not find match room config, matchid: " << req.matchID << endl;
        resp.resultCode = -1;

        return -1;
    }

    iRet = calreward(req, resp);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "cal reward err, iRet: " << iRet << endl;
        resp.resultCode = iRet;

        return iRet;
    }

    //发放奖品
    auto itreward = resp.data.begin();
    if (itreward != resp.data.end())
    {
        for (auto itprops = itreward->second.goods.begin(); itprops != itreward->second.goods.end(); ++itprops)
        {
            auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(itreward->second.uid);
            if (!pHallServant)
            {
                LOG_ERROR << "pHallServant is null, uid: " << itreward->second.uid << endl;
                continue;
            }

            //物品奖励
            GiveGoodsReq giveGoodsReq;
            GiveGoodsRsp giveGoodsRsp;
            giveGoodsReq.uid = itreward->second.uid;
            giveGoodsReq.goods.goodsID = itprops->propsID;
            giveGoodsReq.goods.count = itprops->number;
            giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SNG_RANK_REWARD;
            iRet = pHallServant->giveGoods(giveGoodsReq, giveGoodsRsp);
            if (iRet != 0)
            {
                ROLLLOG_DEBUG << "give goods, req: " << printTars(giveGoodsReq) << ", rsp: " << printTars(giveGoodsRsp) << endl;
            }
        }

        //pHallServant->asyncMailNotifyReward(itreward->second.uid, itreward.first,  it1->second.name, itreward->second);
    }

    logMatchRank(req, resp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//比赛排名奖励
void MatchServantImp::logMatchRank(const match::RewardReq &req, match::RewardResp &resp)
{
    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(req.matchID);
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "find match room err, matchid: " << req.matchID << endl;
        return;
    }

    //奖励日志
    for (auto it = resp.data.begin(); it != resp.data.end(); ++it)
    {
        auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(it->second.uid);
        if (!pHallServant)
        {
            ROLLLOG_ERROR << "pHallServant is null, uid: " << it->second.uid << endl;
            continue;
        }

        string propsReward = "{";
        long propsValue = 0;
        long jackpotCoin = 0;
        for (auto itgoods = it->second.goods.begin(); itgoods != it->second.goods.end();)
        {
            if (itgoods->type == 0)
            {
                propsReward = propsReward + "\"" + I2S(itgoods->propsID) + "\"" + ":" + I2S(itgoods->number);

                ++itgoods;

                if (itgoods != it->second.goods.end())
                {
                    propsReward = propsReward + ",";
                }
            }
            else
            {
                jackpotCoin += itgoods->number;
                ++itgoods;
            }
        }

        propsReward = propsReward + "}";
        propsValue = it->second.total - jackpotCoin;

        userinfo::GetUserReq getUserReq;
        userinfo::GetUserResp getUserResp;
        getUserReq.uid = it->second.uid;
        pHallServant->getUser(getUserReq, getUserResp);

        FDLOG_MATCH_RANK_LOG << g_app.getOuterFactoryPtr()->GetTimeFormat()
                             << "|" << LOG_VERSION
                             << "|" << APP_ID
                             << "|" << MATCH_RANK_LOG_TOPIC
                             << "|" << TNOWMS
                             << "|" << getUserResp.platform
                             << "|" << ZONE_ID
                             << "|" << GAME_VERSION
                             << "|" << "1001"
                             << "|" << getUserResp.channnelID
                             << "|" << L2S(it->second.uid)
                             << "|" << ""
                             << "|" << getUserResp.areaID
                             << "|" << "1001001"
                             << "|" << ""
                             << "|" << it->first
                             << "|" << it->second.ratio
                             << "|" << propsReward
                             << "|" << propsValue
                             << "|" << jackpotCoin << endl;
    }
}

//时区
#define ONE_DAY_TIME (24*60*60)
#define ZONE_TIME_OFFSET (8*60*60)

//报名请求
tars::Int32 MatchServantImp::signUp(const match::SignUpReq &req, match::SignUpResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid);
    if (!pHallServant)
    {
        return -9111;
    }

    auto pConfigServant = g_app.getOuterFactoryPtr()->getConfigServantPrx();
    if (!pConfigServant)
    {
        return -9112;
    }

    auto pPushServant = g_app.getOuterFactoryPtr()->getPushServantPrx(req.uid);
    if (!pPushServant)
    {
        return -9113;
    }

    resp.delay = req.delay;

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(req.matchID);
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "can not find match room config, uid: " << req.uid << ", matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    MatchRoom matchRoom = it->second;
    ROLLLOG_DEBUG << "isSNG:" << req.isSng << ", isRobot:" << req.isRobot << endl;

    resp.tableID = req.tableID;
    //检查赛季
    if (req.isSng)
    {
        task::SeasonInfoGetReq seasonInfoGetReq;
        seasonInfoGetReq.type = (task::E_TASK_SEASON_TYPE)1;
        task::SeasonInfoGetResp seasonInfoGetResp;
        int iRet = pHallServant->QuerySeasonInfo(seasonInfoGetReq, seasonInfoGetResp);
        if (iRet != 0 || seasonInfoGetResp.cfgId <= 0 || seasonInfoGetResp.cfgId > 4)
        {
            ROLLLOG_ERROR << "season id err iRet:" << iRet << ", cfgId:" << seasonInfoGetResp.cfgId << endl;
            resp.resultCode = -30;
            return -30;
        }

        config::SeasonRewardsGroupCfgListResp seasonRewardsGroupCfgListResp;
        iRet = pConfigServant->ListSeasonRewardsGroupList(seasonRewardsGroupCfgListResp);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "season group err iRet:" << iRet << endl;
            resp.resultCode = -31;
            return -31;
        }

        auto it = seasonRewardsGroupCfgListResp.data.find(seasonInfoGetResp.cfgId);
        if (it == seasonRewardsGroupCfgListResp.data.end())
        {
            ROLLLOG_ERROR << "season group err cfgId:" << seasonInfoGetResp.cfgId << endl;
            resp.resultCode = -32;
            return -32;
        }

        if (TNOW < it->second.beginTime || TNOW > it->second.banApplyTime)
        {
            ROLLLOG_ERROR << "season time err cfgId:" << seasonInfoGetResp.cfgId << ",  now:" << TNOW << ", begin time:" << it->second.beginTime << ", banApplyTime:" << it->second.banApplyTime << endl;
            resp.resultCode = -33;
            return -33;
        }
    }

    //用户的报名信息
    UserSignUpInfo userSignUpInfo;
    {
        //获取玩家在线状态
        userstate::GameStateReq gameStateReq;
        gameStateReq.uid = req.uid;
        userstate::GameStateResp gameStateResp;
        iRet = pPushServant->gameState(gameStateReq, gameStateResp);
        if (iRet == 0 && S2I(gameStateResp.data.matchID) > 0)//只有sng 存储matchid
        {
            ROLLLOG_ERROR << "user exist other room matchID: " << gameStateResp.data.matchID << ", roomID: " << gameStateResp.data.sRoomID << endl;
            resp.resultCode = -40;
            resp.roomID = gameStateResp.data.sRoomID;
            resp.matchID = gameStateResp.data.matchID;
            resp.tableID = gameStateResp.data.tableID;
            return -40;
        }

        //是否已经报名
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = 1;
        dataReq.clusterInfo.frageFactor = req.uid;

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, uid: " << req.uid << ", matchid: " << req.matchID << ", iResult: " << dataRsp.iResult << endl;
            resp.resultCode = -2;
            return -2;
        }

        //是否已经报名
        ROLLLOG_DEBUG << "read match user info, uid: " << req.uid
                      << ", matchid: " << req.matchID
                      << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                              << ", colValue: " << itfields->colValue
                              << ", colValue length: " << itfields->colValue.length() << endl;

                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, userSignUpInfo);

                __CATCH__

                ///已报名
                for (auto it = userSignUpInfo.data.begin(); it != userSignUpInfo.data.end(); ++it)
                {
                    if (it->first == req.matchID)
                    {
                        ROLLLOG_ERROR << "sign up already, uid: " << req.uid << ", matchid: " << req.matchID << endl;
                        resp.resultCode = -3;
                        return -3;
                    }
                }
            }
        }
    }

    {
        //超过报名人数
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = req.matchID;

        //
        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_DEBUG << "read player info err, uid: " << req.uid
                          << ", match id: " << req.matchID
                          << ", iResult: " << dataRsp.iResult << endl;
            resp.resultCode = -4;
            return -4;
        }


        ROLLLOG_DEBUG << "read player info, uid: " << req.uid
                      << ", match id: " << req.matchID
                      << ", fields size: " << dataRsp.fields.size() << endl;

        //超过最多人数
        if ((size_t)matchRoom.maxNumber <= dataRsp.fields.size())
        {
            ROLLLOG_DEBUG << "exceed the max number, uid: " << req.uid
                          << ", match id: " << req.matchID
                          << ", max number: " << matchRoom.maxNumber
                          << ", fields size: " << dataRsp.fields.size() << endl;
            resp.resultCode = -5;
            return -5;
        }
    }

    //奖池变化
    long jackpot = 0;
    //报名费
    match::UserEntryFee userEntryFee;

    //报名消耗
    if (!req.isRobot)
    {
        auto itFee = allMatchConfig.mapEntryFeeConfig.find(matchRoom.entryFeeID);
        if (itFee == allMatchConfig.mapEntryFeeConfig.end())
        {
            ROLLLOG_ERROR << "get entry fee config err." << endl;
            resp.resultCode = -6;
            return -6;
        }

        //玩家货币信息
        GetUserAccountReq getUserAccountReq;
        GetUserAccountResp getUserAccountResp;
        getUserAccountReq.uid = req.uid;
        iRet = pHallServant->getUserAccount(getUserAccountReq, getUserAccountResp);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "get entry fee config err." << endl;
            resp.resultCode = -7;
            return -7;
        }

        //报名费
        vector<EntryFee> entryFee;

        //延迟报名
        if (req.delay)
        {
            for (auto itentryFee = itFee->second.begin(); itentryFee != itFee->second.end(); ++itentryFee)
            {
                if (itentryFee->style == E_BUY_STYLE_DELAY)
                {
                    entryFee.push_back(*itentryFee);
                }
            }
        }
        //正常流程
        else
        {
            for (auto itentryFee = itFee->second.begin(); itentryFee != itFee->second.end(); ++itentryFee)
            {
                if (itentryFee->style == E_BUY_STYLE_DEFAULT)
                {
                    entryFee.push_back(*itentryFee);
                }
            }
        }


        ROLLLOG_DEBUG << "entry fee, uid: " << req.uid << ", match id: " << req.matchID << ", fee size: " << entryFee.size() << ", delay: " << req.delay << ", resp: " << printTars(getUserAccountResp) << endl;

        //无报名费配置
        if (entryFee.size() == 0)
        {
            ROLLLOG_ERROR << "entry fee empty." << endl;
            resp.resultCode = -8;
            return -8;
        }

        //服务费
        bool bSuccess = false;
        for (auto itentryFee = entryFee.begin(); itentryFee != entryFee.end(); ++itentryFee)
        {
            //扣除报名费
            auto itProps = allMatchConfig.mapPropsConfig.find(itentryFee->propsID);
            if (itProps != allMatchConfig.mapPropsConfig.end())
            {
                ModifyUserAccountReq modifyUserAccountReq;
                modifyUserAccountReq.uid = req.uid;
                if (req.delay)
                {
                    modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_DELAY_SIGN;  //比赛延迟报名费
                }
                else
                {
                    modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SNG_SIGN; //比赛报名费用
                }
                ROLLLOG_DEBUG << "type:" << itProps->second.type << ", number:" << itentryFee->number << endl;
                switch (itProps->second.type)
                {
                case E_GOODS_TYPE_JETTON: //筹码
                {
                    if (getUserAccountResp.gold - itentryFee->serviceFee >= itentryFee->number)
                    {
                        modifyUserAccountReq.goldChange = -itentryFee->number;
                    }
                    else
                    {
                        ROLLLOG_ERROR << "service fee not enough." << endl;
                        resp.resultCode = -9;
                        return -9;
                    }
                }
                break;
                case E_GOODS_TYPE_POINT:
                {
                    if (getUserAccountResp.score - itentryFee->serviceFee >= itentryFee->number)
                    {
                        modifyUserAccountReq.scoreChange = -itentryFee->number;
                    }
                    else
                    {
                        ROLLLOG_ERROR << "service fee not enough." << endl;
                        resp.resultCode = -9;
                        return -9;
                    }
                }
                default:
                    ROLLLOG_ERROR << "invalid prop type." << endl;
                    break;
                }

                iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
                if (iRet == 0)
                {
                    bSuccess = true;
                    //奖池
                    jackpot = itentryFee->number * itProps->second.value;
                    //服务费
                    userEntryFee.propsID = itProps->second.id;
                    userEntryFee.number = itentryFee->number;
                    userEntryFee.serviceFee = itentryFee->serviceFee;
                    break;
                }
            }

            //扣费成功
            if (bSuccess)
            {
                //服务费
                if (itentryFee->serviceFee > 0)
                {
                    ModifyUserAccountReq modifyUserAccountReq;
                    modifyUserAccountReq.uid = req.uid;
                    modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SERVICE_CHARGE; //比赛服务费
                    modifyUserAccountReq.goldChange = -itentryFee->serviceFee;
                    iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
                    if (iRet != 0)
                    {
                        ROLLLOG_ERROR << "service fee not enough." << endl;
                        resp.resultCode = -10;
                        return -10;
                    }
                }

                //  //推送货币变化
                //  iRet = pHallServant->pushUserAccountInfo(modifyUserAccountReq.uid);
                //  //
                //  userEntryFee.serviceFee = itentryFee->serviceFee;
                // }
                break;
            }
        }

        //扣费失败
        if (!bSuccess)
        {
            ROLLLOG_DEBUG << "entry fee err" << endl;
            resp.resultCode = -11;
            return -11;
        }
    }

    {
        //报名
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;

        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = I2S(req.uid);
        fields.push_back(tfield);
        tfield.colValue = I2S(STARTCHIP2RANKSCORES(it->second.initialCard) * -1);
        fields.push_back(tfield);
        wdataReq.fields = fields;

        //
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -12;
            return -12;
        }
    }

    {
        //用户报名
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.uid;

        //data
        SignUpInfo signUpInfo;
        signUpInfo.matchID = req.matchID;
        signUpInfo.chip = matchRoom.initialCard;
        signUpInfo.ranking = 1;
        signUpInfo.additional = matchRoom.additionalTimes;
        signUpInfo.repurchase = matchRoom.repurchaseTimes;
        signUpInfo.userEntryFee = userEntryFee;

        //
        userSignUpInfo.data[req.matchID] = signUpInfo;

        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;

        string encode = TC_Base64::encode(tostring(userSignUpInfo));
        tfield.colValue = encode;
        if (tfield.colValue.length() == 0)
        {
            tfield.colValue = " ";
        }
        tfield.colArithType = dbagent::E_NONE;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        //
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << ", wdataReq:" << printTars(wdataReq) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -13;
            return -13;
        }
    }

    //增加奖池
    {
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = L2S(jackpot);
        tfield.colArithType = Eum_Col_Arith_Type::E_ADD;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        //
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game match jackpot, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game match jackpot err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -14;
            return -14;
        }
    }

    resp.resultCode = 0;
    resp.entryFee = userEntryFee;
    //牌局报名记录日志
    int type = 1;
    switch (userEntryFee.propsID)
    {
    case 10000:
        type = 1;
        break;
    case 20000:
        type = 2;
        break;
    default:
        break;
    }

    logMatchEnter(req.uid, req.matchID, "", type, (req.delay ? 0 : 1), jackpot, userEntryFee.serviceFee);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//牌局报名记录日志
void MatchServantImp::logMatchEnter(long uid, int matchID, const string &smatchID, int type, int enterType, long cost, long serviceFee)
{
    auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(uid);
    if (!pHallServant)
    {
        ROLLLOG_ERROR << "pHallServant is null" << endl;
        return;
    }

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(matchID);
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "find match room err, matchid: " << matchID << endl;
        return;
    }

    userinfo::GetUserReq getUserReq;
    userinfo::GetUserResp getUserResp;
    getUserReq.uid = uid;
    pHallServant->getUser(getUserReq, getUserResp);

    FDLOG_MATCH_ENTER_LOG << g_app.getOuterFactoryPtr()->GetTimeFormat()
                          << "|" << LOG_VERSION
                          << "|" << APP_ID
                          << "|" << MATCH_ENTER_LOG_TOPIC
                          << "|" << TNOWMS
                          << "|" << getUserResp.platform
                          << "|" << ZONE_ID
                          << "|" << GAME_VERSION
                          << "|" << "1001"
                          << "|" << getUserResp.channnelID
                          << "|" << L2S(uid)
                          << "|" << ""
                          << "|" << getUserResp.areaID
                          << "|" << "1001001"
                          << "|" << smatchID
                          << "|" << type
                          << "|" << enterType
                          << "|" << cost
                          << "|" << serviceFee << endl;
}

tars::Int32 MatchServantImp::reportOnlineCount(const string &roomID, long smallBlind, long count, tars::TarsCurrentPtr current)
{
    wbl::WriteLocker lock(m_lockOnline);

    auto it = m_UOline.find(roomID);
    if(smallBlind == 0 && count == 0 && it != m_UOline.end())
    {
        m_UOline.erase(it);
    }
    else
    {
        if(it == m_UOline.end())
        {
            map<long, long> subItem;
            subItem.insert(std::make_pair(smallBlind, count));
            m_UOline.insert(std::make_pair(roomID, subItem));
        }
        else
        {
            auto itt = it->second.find(smallBlind);
            if(itt == it->second.end())
            {
                it->second.insert(std::make_pair(smallBlind, count));
            }
            else
            {
                itt->second = count;
            }
        }
    }
    LOG_DEBUG << "m_UOline size: " << m_UOline.size() << ", roomID: " << roomID << endl;
    return 0;
}

//取消报名请求
tars::Int32 MatchServantImp::quit(const match::QuitReq &req, match::QuitResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.matchID);
    if (!pDBAgentServant)
    {
        return -9110;
    }

    auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid);
    if (!pHallServant)
    {
        return -9111;
    }

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(req.matchID);
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "can not find match room config, matchid: " << req.matchID << endl;
        resp.resultCode = -1;
        return -1;
    }

    //游戏开局，不能退赛
    if (!req.isSng)
    {
        match::MatchInfo matchInfo;
        iRet = ProcessorSingleton::getInstance()->selectGameInfo(req.matchID, matchInfo);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "select game info err, matchid: " << req.matchID << endl;
            resp.resultCode = -2;
            return -2;
        }

        //游戏已开局
        if (matchInfo.matchState == MATCH_STATE_PLAYING_GAME)
        {
            ROLLLOG_ERROR << "match state err, matchid: " << req.matchID << endl;
            resp.resultCode = -3;
            return -3;
        }
    }

    //奖池变化
    long jackpot = 0;
    //报名费
    match::UserEntryFee userEntryFee;
    //用户的报名信息
    UserSignUpInfo userSignUpInfo;
    {
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = 1;
        dataReq.clusterInfo.frageFactor = req.uid;

        //
        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, uid: " << req.uid << ", iResult: " << dataRsp.iResult << endl;
            resp.resultCode = -4;
            return -4;
        }

        ROLLLOG_DEBUG << "read match user info, uid: " << req.uid << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                              << ", colValue: " << itfields->colValue
                              << ", colValue length: " << itfields->colValue.length() << endl;

                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, userSignUpInfo);

                __CATCH__

                ROLLLOG_DEBUG << "user sign up info : " << printTars(userSignUpInfo) << endl;

                auto it = userSignUpInfo.data.find(req.matchID);
                if (it != userSignUpInfo.data.end())
                {
                    //报名费
                    userEntryFee = it->second.userEntryFee;
                    //
                    userSignUpInfo.data.erase(it);
                }
                ROLLLOG_DEBUG << "user sign up info : " << printTars(userSignUpInfo) << ", matchid: " << req.matchID << endl;
            }
        }

        {
            //用户报名
            dataproxy::TWriteDataReq wdataReq;
            wdataReq.resetDefautlt();
            wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(req.uid);
            wdataReq.operateType = E_REDIS_INSERT;
            wdataReq.clusterInfo.resetDefautlt();
            wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            wdataReq.clusterInfo.frageFactor = req.uid;

            dbagent::TField tfield;
            tfield.colArithType = E_NONE;
            vector<dbagent::TField> fields;
            string encode = TC_Base64::encode(tostring(userSignUpInfo));
            tfield.colValue = encode;
            if (tfield.colValue.length() == 0)
            {
                tfield.colValue = " ";
            }
            tfield.colArithType = dbagent::E_NONE;
            fields.push_back(tfield);
            wdataReq.fields = fields;

            //
            dataproxy::TWriteDataRsp wdataRsp;
            iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
            ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
            if (iRet != 0 || wdataRsp.iResult != 0)
            {
                ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
                resp.resultCode = -6;
                return -6;
            }
        }
    }

    {
        //清理报名排名数据
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;
        wdataReq.paraExt.resetDefautlt();
        wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM;

        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = I2S(req.uid);
        fields.push_back(tfield);
        wdataReq.fields = fields;

        //
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -7;
            return -7;
        }
    }

    //退还报名费
    if (!req.isSng || (req.isSng && req.drawback))
    {
        auto itProps = allMatchConfig.mapPropsConfig.find(userEntryFee.propsID);
        if (itProps == allMatchConfig.mapPropsConfig.end())
        {
            ROLLLOG_ERROR << "find props config err, iRet : " << iRet << ", uid: " << req.uid << ", matchid: " << req.matchID << endl;
            resp.resultCode = -8;
            return -8;
        }

        jackpot = userEntryFee.number * itProps->second.value;
        ROLLLOG_DEBUG << "jackpot: " << jackpot << endl;

        ModifyUserAccountReq modifyUserAccountReq;
        modifyUserAccountReq.uid = req.uid;
        modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SNG_SIGN;  //退赛返还报名费
        switch (itProps->second.type)
        {
        case E_GOODS_TYPE_JETTON: //金币
            modifyUserAccountReq.goldChange = userEntryFee.number;
            break;
        case E_GOODS_TYPE_POINT: //积分
            modifyUserAccountReq.scoreChange = userEntryFee.number;
            break;
        default:
            break;
        }

        iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "restore entry fee err, iRet : " << iRet << ", uid: " << req.uid << ", matchid: " << req.matchID << endl;
            resp.resultCode = -9;
            return -9;
        }
    }

    //退还服务费
    if (!req.isSng || (req.isSng && req.drawback))
    {
        if (userEntryFee.serviceFee > 0)
        {
            ModifyUserAccountReq modifyUserAccountReq;
            modifyUserAccountReq.uid = req.uid;
            modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SERVICE_CHARGE;  //退赛返还服务费
            modifyUserAccountReq.goldChange = userEntryFee.serviceFee;
            iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
            if (iRet != 0)
            {
                ROLLLOG_ERROR << "restore service fee err, iRet : " << iRet << ", uid: " << req.uid << ", matchid: " << req.matchID << endl;
                resp.resultCode = -11;
                return -11;
            }
        }
    }

    //减少奖池
    {
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(req.matchID);
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = req.matchID;

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = L2S(-jackpot);
        tfield.colArithType = Eum_Col_Arith_Type::E_SUB;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        //
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game match jackpot, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game match jackpot err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            resp.resultCode = -12;
            return -12;
        }
    }

    resp.resultCode = 0;
    resp.entryFee = userEntryFee;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//报名
int MatchServantImp::onSignUp(const XGameComm::TPackage &pkg, const matchProto::SignUpReq &signUpReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(signUpReq.matchid());
    if (!pDBAgentServant)
    {
        return -9110;
    }

    auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(pkg.stuid().luid());
    if (!pHallServant)
    {
        return -9111;
    }

    matchProto::SignUpResp signUpResp;
    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(signUpReq.matchid());
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "can not find match room config, matchid: " << signUpReq.matchid() << endl;
        signUpResp.set_resultcode(XGameRetCode::MATCH_NO_CONFIG);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
        return -1;
    }

    MatchRoom matchRoom = it->second;

    //开赛前10秒不能报名
    int tnow = 0;
    int tstart = 0;
    if (matchRoom.startTimeType == 0)
    {
        tnow = (TNOW + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
        tstart = (matchRoom.startTime + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
    }
    else if (matchRoom.startTimeType == 1)
    {
        tnow = TNOW + ZONE_TIME_OFFSET;
        tstart = matchRoom.startTime + ZONE_TIME_OFFSET;
    }

    ROLLLOG_DEBUG << "check start time, tnow: " << tnow << ", tstart: " << tstart << endl;

    if ((tnow != 0) && tstart <= tnow)
    {
        ROLLLOG_ERROR << "match is expired, matchid: " << signUpReq.matchid() << endl;
        signUpResp.set_resultcode(XGameRetCode::MATCH_EXPIRED);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
        return -2;
    }

    //10's前不能报名
    if ((tnow != 0) && (tnow > (tstart - g_app.getOuterFactoryPtr()->getSignUpOrQuitTime())))
    {
        ROLLLOG_ERROR << "can not signup before game, matchid: " << signUpReq.matchid() << endl;
        signUpResp.set_resultcode(XGameRetCode::MATCH_CAN_NOT_SIGNUP_BEFORE_GAME);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
        return -2;
    }

    //用户的报名信息
    UserSignUpInfo userSignUpInfo;
    {
        //是否已经报名
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(pkg.stuid().luid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = 1;
        dataReq.clusterInfo.frageFactor = pkg.stuid().luid();

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, uid: " << pkg.stuid().luid() << ", iResult: " << dataRsp.iResult << endl;
            signUpResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -3;
        }

        ROLLLOG_DEBUG << "read match user info, uid: " << pkg.stuid().luid() << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                              << ", colValue: " << itfields->colValue
                              << ", colValue length: " << itfields->colValue.length() << endl;

                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, userSignUpInfo);

                __CATCH__

                //已报名
                for (auto it = userSignUpInfo.data.begin(); it != userSignUpInfo.data.end(); ++it)
                {
                    if (it->first == signUpReq.matchid())
                    {
                        ROLLLOG_DEBUG << "sign up already, uid: " << pkg.stuid().luid() << endl;
                        signUpResp.set_resultcode(XGameRetCode::MATCH_SIGNUP_ALREADY);
                        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
                        return -4;
                    }
                }
            }
        }
    }

    {
        //超过报名人数
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(signUpReq.matchid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = signUpReq.matchid();

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read player info err, uid: " << pkg.stuid().luid() << ", match id: " << signUpReq.matchid() << ", iResult: " << dataRsp.iResult << endl;
            signUpResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -5;
        }


        ROLLLOG_DEBUG << "read player info, uid: " << pkg.stuid().luid()
                      << ", match id: " << signUpReq.matchid()
                      << ", fields size: " << dataRsp.fields.size() << endl;

        //超过最多人数
        if ((size_t)matchRoom.maxNumber <= dataRsp.fields.size())
        {
            signUpResp.set_resultcode(XGameRetCode::MATCH_MAX_SIGNUP_NUMBER);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -6;
        }
    }

    //奖池变化
    long jackpot = 0;
    match::UserEntryFee userEntryFee;

    //报名消耗
    {
        auto itFee = allMatchConfig.mapEntryFeeConfig.find(matchRoom.entryFeeID);
        if (itFee == allMatchConfig.mapEntryFeeConfig.end())
        {
            ROLLLOG_ERROR << "get entry fee config err." << endl;
            signUpResp.set_resultcode(XGameRetCode::MATCH_NO_ENTRY_FEE_CONFIG);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -7;
        }

        //玩家货币信息
        GetUserAccountReq getUserAccountReq;
        GetUserAccountResp getUserAccountResp;
        getUserAccountReq.uid = pkg.stuid().luid();
        iRet = pHallServant->getUserAccount(getUserAccountReq, getUserAccountResp);
        ROLLLOG_DEBUG << "get user account resp: " << printTars(getUserAccountResp) << endl;
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "get entry fee config err." << endl;
            signUpResp.set_resultcode(XGameRetCode::MATCH_NO_ENTRY_FEE_CONFIG);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -8;
        }

        vector<EntryFee> entryFee;

        //延迟报名
        /*
        if (req.delay)
        {
            for (auto itentryFee = itFee->second.begin(); itentryFee != itFee->second.end(); ++itentryFee)
            {
                if (itentryFee->style == E_BUY_STYLE_DELAY)
                {
                    entryFee.push_back(*itentryFee);
                }
            }
        }
        else
        {
        */
        for (auto itentryFee = itFee->second.begin(); itentryFee != itFee->second.end(); ++itentryFee)
        {
            if (itentryFee->style == E_BUY_STYLE_DEFAULT)
            {
                entryFee.push_back(*itentryFee);
            }
        }
        //}

        ROLLLOG_DEBUG << "entry fee, uid: " << pkg.stuid().luid() << ", match id: " << signUpReq.matchid() << ", fee size: " << entryFee.size() << endl;

        //无报名费配置
        if (entryFee.size() == 0)
        {
            ROLLLOG_DEBUG << "entry fee empty." << endl;
            signUpResp.set_resultcode(XGameRetCode::MATCH_NO_ENTRY_FEE_CONFIG);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -9;
        }

        //报名费
        bool bSuccess = false;
        for (auto itentryFee = entryFee.begin(); itentryFee != entryFee.end(); ++itentryFee)
        {
            //报名费
            auto itProps = allMatchConfig.mapPropsConfig.find(itentryFee->propsID);
            if (itProps != allMatchConfig.mapPropsConfig.end())
            {
                ModifyUserAccountReq modifyUserAccountReq;
                modifyUserAccountReq.uid = pkg.stuid().luid();
                modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SNG_SIGN;  //比赛报名费用
                switch (itProps->second.type)
                {
                case E_GOODS_TYPE_JETTON: //
                {
                    if (getUserAccountResp.gold - itentryFee->serviceFee >= itentryFee->number)
                    {
                        modifyUserAccountReq.goldChange = -itentryFee->number;
                    }
                    else
                    {
                        ROLLLOG_DEBUG << "props not enough." << endl;
                        signUpResp.set_resultcode(XGameRetCode::MATCH_GOLD_NOT_ENOUGH);
                        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
                        return -10;
                    }
                }
                break;
                case E_GOODS_TYPE_POINT:
                {
                    if (getUserAccountResp.score - itentryFee->serviceFee >= itentryFee->number)
                    {
                        modifyUserAccountReq.scoreChange = -itentryFee->number;
                    }
                    else
                    {
                        ROLLLOG_DEBUG << "props not enough." << endl;
                        signUpResp.set_resultcode(XGameRetCode::HALL_EXCHANGE_POINTS_NOTENOUGH);
                        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
                        return -10;
                    }
                }
                default:
                {
                    ROLLLOG_DEBUG << "invalid prop type:" << itProps->second.type << endl;
                    break;
                }
                }

                iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
                if (iRet == 0)
                {
                    bSuccess = true;
                    //奖池
                    jackpot = itentryFee->number * itProps->second.value;
                    //
                    userEntryFee.propsID = itProps->second.id;
                    userEntryFee.number = itentryFee->number;
                    userEntryFee.serviceFee = itentryFee->serviceFee;
                    break;
                }

                ROLLLOG_ERROR << "modifiy user account err, uid: " << pkg.stuid().luid() << ", match id: " << signUpReq.matchid() << endl;
            }
            else
            {
                ROLLLOG_DEBUG << "can not find props, props id: " << itentryFee->propsID << endl;
            }

            //扣费成功
            if (bSuccess)
            {
                //服务费
                if (itentryFee->serviceFee > 0)
                {
                    ModifyUserAccountReq modifyUserAccountReq;
                    modifyUserAccountReq.uid = pkg.stuid().luid();
                    modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SERVICE_CHARGE;  //比赛服务费
                    modifyUserAccountReq.scoreChange = -itentryFee->serviceFee;
                    iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
                    if (iRet != 0)
                    {
                        ROLLLOG_ERROR << "change user account fail." << endl;
                        signUpResp.set_resultcode(XGameRetCode::SYS_BUSY);
                        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
                        return -10;
                    }
                }

                //  //推送货币变化
                //  iRet = pHallServant->pushUserAccountInfo(modifyUserAccountReq.uid);
                //  //服务费
                //  userEntryFee.serviceFee = itentryFee->serviceFee;
                // }
                break;
            }
        }

        //报名扣费失败
        if (!bSuccess)
        {
            ROLLLOG_ERROR << "entry fee err, uid: " << pkg.stuid().luid() << ", match id: " << signUpReq.matchid() << endl;
            signUpResp.set_resultcode(XGameRetCode::MATCH_GOLD_NOT_ENOUGH);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -11;
        }
    }

    {
        //报名
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(signUpReq.matchid());
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = signUpReq.matchid();

        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = I2S(pkg.stuid().luid());
        fields.push_back(tfield);
        tfield.colValue = I2S(STARTCHIP2RANKSCORES(it->second.initialCard) * -1);
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            signUpResp.set_resultcode(XGameRetCode::MATCH_SIGNUP_FAIL);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -12;
        }
    }

    {
        //用户报名
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(pkg.stuid().luid());
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = pkg.stuid().luid();

        //data
        SignUpInfo signUpInfo;
        signUpInfo.matchID = signUpReq.matchid();
        signUpInfo.chip = matchRoom.initialCard;
        signUpInfo.ranking = 1;
        signUpInfo.additional = matchRoom.additionalTimes;
        signUpInfo.repurchase = matchRoom.repurchaseTimes;
        signUpInfo.userEntryFee = userEntryFee;

        userSignUpInfo.data[signUpReq.matchid()] = signUpInfo;

        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        string encode = TC_Base64::encode(tostring(userSignUpInfo));
        tfield.colValue = encode;
        if (tfield.colValue.length() == 0)
        {
            tfield.colValue = " ";
        }
        tfield.colArithType = dbagent::E_NONE;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            signUpResp.set_resultcode(XGameRetCode::MATCH_SIGNUP_FAIL);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -13;
        }
    }

    //增加奖池
    {
        /*dataproxy::TWriteDataReq wdataReq;
        wadataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(signUpReq.matchid());
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = signUpReq.matchid();

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = L2S(jackpot);
        tfield.colArithType = Eum_Col_Arith_Type::E_ADD;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        //
        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game match jackpot, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_DEBUG << "update game match jackpot err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            signUpResp.set_resultcode(XGameRetCode::MATCH_SIGNUP_FAIL);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);
            return -14;
        }*/
    }

    //报名账单
    {
        int type = 1;
        switch (userEntryFee.propsID)
        {
        case 10000:
            type = 1;
            break;
        case 20000:
            type = 2;
            break;
        case 30000:
            type = 2;
            break;
        default:
            break;
        }

        logMatchEnter(pkg.stuid().luid(), signUpReq.matchid(), "", type, 1, jackpot, userEntryFee.serviceFee);
    }

    signUpResp.set_resultcode(0);
    signUpResp.mutable_entryfee()->set_propsid(userEntryFee.propsID);
    signUpResp.mutable_entryfee()->set_number(userEntryFee.number);
    signUpResp.mutable_entryfee()->set_servicefee(userEntryFee.serviceFee);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_SIGN_UP, signUpResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//退赛
int MatchServantImp::onQuit(const XGameComm::TPackage &pkg, const matchProto::QuitReq &quitReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::QuitResp quitResp;

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(quitReq.matchid());
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_DEBUG << "can not find match room config, matchid: " << quitReq.matchid() << endl;
        quitResp.set_resultcode(XGameRetCode::MATCH_NO_CONFIG);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
        return -1;
    }

    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(quitReq.matchid());
    if (!pDBAgentServant)
    {
        return -9110;
    }

    auto pHallServant = g_app.getOuterFactoryPtr()->getHallServantPrx(pkg.stuid().luid());
    if (!pHallServant)
    {
        return -9111;
    }

    MatchRoom matchRoom = it->second;

    //开赛前10秒不能退赛
    int tnow = 0;
    int tstart = 0;
    if (matchRoom.startTimeType == 0)
    {
        tnow = (TNOW + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
        tstart = (matchRoom.startTime + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
    }
    else if (matchRoom.startTimeType == 1)
    {
        tnow = TNOW + ZONE_TIME_OFFSET;
        tstart = matchRoom.startTime + ZONE_TIME_OFFSET;
    }

    ROLLLOG_DEBUG << "check start time, tnow: " << tnow << ", tstart: " << tstart << endl;

    //10's前不能退赛
    if ((tnow != 0) && (tnow > (tstart - g_app.getOuterFactoryPtr()->getSignUpOrQuitTime())))
    {
        ROLLLOG_DEBUG << "can not quit before game, matchid: " << quitReq.matchid() << endl;
        quitResp.set_resultcode(XGameRetCode::MATCH_ACN_NOT_QUIT_BEFORE_GAME);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
        return -2;
    }

    //游戏已开局，不能退赛
    match::MatchInfo matchInfo;
    iRet = ProcessorSingleton::getInstance()->selectGameInfo(quitReq.matchid(), matchInfo);
    if (iRet != 0)
    {
        ROLLLOG_DEBUG << "select game info err, matchid: " << quitReq.matchid() << endl;
        quitResp.set_resultcode(XGameRetCode::SYS_BUSY);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
        return -3;
    }

    //游戏已开局
    if (matchInfo.matchState == MATCH_STATE_PLAYING_GAME)
    {
        ROLLLOG_DEBUG << "match state err, matchid: " << quitReq.matchid() << endl;
        quitResp.set_resultcode(XGameRetCode::MATCH_START_NOT_ALLOW_QUIT);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
        return -4;
    }

    //奖池变化
    long jackpot = 0;
    match::UserEntryFee userEntryFee;

    //用户的报名信息
    UserSignUpInfo userSignUpInfo;
    {
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(pkg.stuid().luid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = 1;
        dataReq.clusterInfo.frageFactor = pkg.stuid().luid();

        dataproxy::TReadDataRsp dataRsp;
        iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, uid: " << pkg.stuid().luid() << ", iResult: " << dataRsp.iResult << endl;
            quitResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
            return -5;
        }

        ROLLLOG_DEBUG << "read match user info, uid: " << pkg.stuid().luid() << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName
                              << ", colValue: " << itfields->colValue
                              << ", colValue length: " << itfields->colValue.length() << endl;

                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, userSignUpInfo);

                __CATCH__

                ROLLLOG_DEBUG << "user sign up info : " << printTars(userSignUpInfo) << endl;

                auto it = userSignUpInfo.data.find(quitReq.matchid());
                if (it == userSignUpInfo.data.end())
                {
                    quitResp.set_resultcode(XGameRetCode::MATCH_NO_SIGNUP);
                    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
                    return -6;
                }

                //报名费
                userEntryFee = it->second.userEntryFee;
                userSignUpInfo.data.erase(it);
                ROLLLOG_DEBUG << "user sign up info : " << printTars(userSignUpInfo) << ", matchid: " << quitReq.matchid() << endl;
            }
        }

        {
            //用户报名
            dataproxy::TWriteDataReq wdataReq;
            wdataReq.resetDefautlt();
            wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(pkg.stuid().luid());
            wdataReq.operateType = E_REDIS_INSERT;
            wdataReq.clusterInfo.resetDefautlt();
            wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            wdataReq.clusterInfo.frageFactor = pkg.stuid().luid();

            dbagent::TField tfield;
            tfield.colArithType = E_NONE;
            vector<dbagent::TField> fields;

            string encode = TC_Base64::encode(tostring(userSignUpInfo));
            tfield.colValue = encode;
            if (tfield.colValue.length() == 0)
            {
                tfield.colValue = " ";
            }
            tfield.colArithType = dbagent::E_NONE;
            fields.push_back(tfield);
            wdataReq.fields = fields;

            dataproxy::TWriteDataRsp wdataRsp;
            iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
            ROLLLOG_DEBUG << "update game user info, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
            if (iRet != 0 || wdataRsp.iResult != 0)
            {
                ROLLLOG_ERROR << "update game user info err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
                quitResp.set_resultcode(XGameRetCode::SYS_BUSY);
                toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
                return -7;
            }
        }
    }

    {
        //清理报名排名数据
        dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(quitReq.matchid());
        wdataReq.operateType = E_REDIS_INSERT;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = quitReq.matchid();
        wdataReq.paraExt.resetDefautlt();
        wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM;

        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = I2S(pkg.stuid().luid());
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game rank, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "update game rank err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            quitResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
            return -8;
        }
    }

    //退还报名费
    {
        //奖池
        auto itProps = allMatchConfig.mapPropsConfig.find(userEntryFee.propsID);
        if (itProps == allMatchConfig.mapPropsConfig.end())
        {
            ROLLLOG_ERROR << "find props config err, iRet : " << iRet
                          << ", uid: " << pkg.stuid().luid()
                          << ", matchid: " << quitReq.matchid()
                          << ", propsID: " << userEntryFee.propsID
                          << ", match config: " << printTars(allMatchConfig) << endl;

            quitResp.set_resultcode(XGameRetCode::MATCH_NO_ENTRY_FEE_CONFIG);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
            return -9;
        }

        //奖池
        jackpot = userEntryFee.number * itProps->second.value;
        ROLLLOG_DEBUG << "jackpot: " << jackpot << ", type:" << itProps->second.type
                      << ", number:" << userEntryFee.number << ", props_id:" << userEntryFee.propsID << endl;

        ModifyUserAccountReq modifyUserAccountReq;
        modifyUserAccountReq.uid = pkg.stuid().luid();
        modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SNG_SIGN;  //退赛返还报名费
        switch (itProps->second.type)
        {
        case E_GOODS_TYPE_JETTON: //金币
            modifyUserAccountReq.goldChange = userEntryFee.number;
            break;
        case E_GOODS_TYPE_POINT:
            modifyUserAccountReq.scoreChange = userEntryFee.number;
        default:
            break;
        }

        iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "restore entry fee err, iRet : " << iRet << ", uid: " << pkg.stuid().luid() << ", matchid: " << quitReq.matchid() << endl;
            quitResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
            return -10;
        }
    }

    //退还服务费
    if (userEntryFee.serviceFee > 0)
    {
        ModifyUserAccountReq modifyUserAccountReq;
        modifyUserAccountReq.uid = pkg.stuid().luid();
        modifyUserAccountReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SERVICE_CHARGE;  //退赛返还服务费
        modifyUserAccountReq.goldChange = userEntryFee.serviceFee;
        iRet = pHallServant->modifyUserAccount(modifyUserAccountReq);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "restore service fee err, iRet : " << iRet << ", uid: " << pkg.stuid().luid() << ", matchid: " << quitReq.matchid() << endl;
            quitResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
            return -12;
        }
    }

    //减少奖池
    {
        /*dataproxy::TWriteDataReq wdataReq;
        wdataReq.resetDefautlt();
        wdataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(quitReq.matchid());
        wdataReq.operateType = E_REDIS_WRITE;
        wdataReq.clusterInfo.resetDefautlt();
        wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        wdataReq.clusterInfo.frageFactor = quitReq.matchid();

        //data
        dbagent::TField tfield;
        tfield.colArithType = E_NONE;
        vector<dbagent::TField> fields;
        tfield.colValue = L2S(-jackpot);
        tfield.colArithType = Eum_Col_Arith_Type::E_SUB;
        fields.push_back(tfield);
        wdataReq.fields = fields;

        dataproxy::TWriteDataRsp wdataRsp;
        iRet = pDBAgentServant->redisWrite(wdataReq, wdataRsp);
        ROLLLOG_DEBUG << "update game match jackpot, iRet : " << iRet << ", data : " << printTars(wdataRsp) << endl;
        if (iRet != 0 || wdataRsp.iResult != 0)
        {
            ROLLLOG_DEBUG << "update game match jackpot err, iRet : " << iRet << ", iResult: " << wdataRsp.iResult << endl;
            quitResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);
            return -13;
        }*/
    }

    quitResp.set_resultcode(0);
    quitResp.mutable_entryfee()->set_propsid(userEntryFee.propsID);
    quitResp.mutable_entryfee()->set_number(userEntryFee.number);
    quitResp.mutable_entryfee()->set_servicefee(userEntryFee.serviceFee);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_QUIT, quitResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//重购
int MatchServantImp::onRepurchase(const XGameComm::TPackage &pkg, const matchProto::RepurchaseReq &repurchaseReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::RepurchaseResp repurchaseResp;
    repurchaseResp.set_resultcode(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_REPURCHASE, repurchaseResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//增购
int MatchServantImp::onAdditional(const XGameComm::TPackage &pkg, const matchProto::AdditionalReq &additionalReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::AdditionalResp additionalResp;
    additionalResp.set_resultcode(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_ADDITIONAL, additionalResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//奖池
int MatchServantImp::onJackpot(const XGameComm::TPackage &pkg, const matchProto::JackpotReq &jackpotReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__


    matchProto::JackpotResp jackpotResp;

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(jackpotReq.matchid());
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = jackpotReq.matchid();

    dataproxy::TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(jackpotReq.matchid())->redisRead(dataReq, dataRsp);
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "read match jackpot err, uid: " << pkg.stuid().luid()
                      << ", match id: " << jackpotReq.matchid()
                      << ", iResult: " << dataRsp.iResult << endl;

        jackpotResp.set_resultcode(XGameRetCode::SYS_BUSY);
        jackpotResp.set_matchid(jackpotReq.matchid());
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_JACKPOT, jackpotResp);
        return -1;
    }

    ROLLLOG_DEBUG << "read match jackpot, uid: " << pkg.stuid().luid()
                  << ", match id: " << jackpotReq.matchid()
                  << ", fields size: " << dataRsp.fields.size() << endl;

    //jackpot
    for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
    {
        for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
        {
            ROLLLOG_DEBUG << "read match jackpot, colName: " << itfields->colName
                          << ", colValue: " << itfields->colValue << endl;

            jackpotResp.set_value(S2I(itfields->colValue));
        }
    }

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(jackpotReq.matchid());
    if (it != allMatchConfig.mapMatchRoomConfig.end())
    {
        jackpotResp.set_value(jackpotResp.value() + it->second.initialPool);
    }

    jackpotResp.set_resultcode(0);
    jackpotResp.set_matchid(jackpotReq.matchid());
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_JACKPOT, jackpotResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}


//报名人数
int MatchServantImp::onPlayerCount(const XGameComm::TPackage &pkg, const matchProto::PlayerCountReq &playerCountReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::PlayerCountResp playerCountResp;

    {
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(playerCountReq.matchid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = playerCountReq.matchid();
        dataReq.paraExt.resetDefautlt();
        dataReq.paraExt.subOperateType = E_REDIS_SORTSET_ZCARD;

        dataproxy::TReadDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(playerCountReq.matchid())->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read player count err, uid: " << pkg.stuid().luid()
                          << ", match id: " << playerCountReq.matchid()
                          << ", iResult: " << dataRsp.iResult << endl;

            playerCountResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_PLAYER_COUNT, playerCountResp);
            return -1;
        }

        ROLLLOG_DEBUG << "read player count, uid: " << pkg.stuid().luid()
                      << ", match id: " << playerCountReq.matchid()
                      << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read player count, colName: " << itfields->colName
                              << ", colValue: " << itfields->colValue << endl;
                playerCountResp.set_number(S2I(itfields->colValue));
            }
        }
    }

    playerCountResp.set_resultcode(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_PLAYER_COUNT, playerCountResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//玩家信息
int MatchServantImp::onPlayerInfo(const XGameComm::TPackage &pkg, const matchProto::PlayerInfoReq &playerInfoReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::PlayerInfoResp playerInfoResp;

    {
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(playerInfoReq.matchid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = playerInfoReq.matchid();
        dataReq.paraExt.resetDefautlt();
        dataReq.paraExt.start = 0;
        dataReq.paraExt.end = g_app.getOuterFactoryPtr()->showPlayerCount();

        dataproxy::TReadDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(playerInfoReq.matchid())->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_DEBUG << "read player info err, uid: " << pkg.stuid().luid()
                          << ", match id: " << playerInfoReq.matchid()
                          << ", iResult: " << dataRsp.iResult << endl;

            playerInfoResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_PLAYER_INFO, playerInfoResp);
            return -1;
        }

        ROLLLOG_DEBUG << "read player info, uid: " << pkg.stuid().luid()
                      << ", match id: " << playerInfoReq.matchid()
                      << ", fields size: " << dataRsp.fields.size() << endl;

        //int max_len = g_app.getOuterFactoryPtr()->showPlayerCount();
        for (auto row_ptr = dataRsp.fields.begin(); row_ptr != dataRsp.fields.end(); ++row_ptr)
        {
            tars::Int64 member = S2L(row_ptr->front().colValue);
            tars::Int64 score = S2L(row_ptr->back().colValue);
            if (RANKSCORES2CHIP(score * -1) == 0)
                continue;

            matchProto::PlayerInfo *playerInfo = playerInfoResp.add_data();
            playerInfo->set_uid(member);
            playerInfo->set_chip(RANKSCORES2CHIP(score * -1));
            //if (playerInfoResp.data_size() == (size_t)max_len)
            //{
            //    break;
            //}
        }
    }

    playerInfoResp.set_resultcode(0);
    ROLLLOG_DEBUG << "send response to client, rsp: " << logPb(playerInfoResp) << endl;
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_PLAYER_INFO, playerInfoResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

int MatchServantImp::onListSNGRanking(const XGameComm::TPackage &pkg, int matchID, bool bUserInfo, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(matchID);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = matchID;

    dataproxy::TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisRead(dataReq, dataRsp);
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "read player info err, uid: " << pkg.stuid().luid() << ", match id: " << matchID
                      << ", iResult: " << dataRsp.iResult << endl;
        return -1;
    }

    ROLLLOG_DEBUG << "read player info, uid: " << pkg.stuid().luid() << ", match id: " << matchID
                  << ", fields size: " << dataRsp.fields.size() << endl;

    if (bUserInfo)
    {
        int iRank = 0;
        matchProto::SNGProcessRankingListResp resp;
        for (auto row_ptr = dataRsp.fields.begin(); row_ptr != dataRsp.fields.end(); ++row_ptr)
        {
            tars::Int64 member = S2L(row_ptr->front().colValue);
            tars::Int64 score = S2L(row_ptr->back().colValue);
            if (RANKSCORES2CHIP(score * -1) == 0)
                continue;

            auto sngRankingBaseInfo = resp.add_rankinglist();
            sngRankingBaseInfo->mutable_base()->set_ranking(iRank++);
            sngRankingBaseInfo->mutable_base()->set_uid(member);
            sngRankingBaseInfo->mutable_base()->set_chip(RANKSCORES2CHIP(score * -1));
        }

        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::E_SNG_PROCESS_RANKING_LIST_RESP, resp);
    }
    else
    {
        int iRank = 0;
        matchProto::SNGProcessRakningRefreshResp resp;
        for (auto row_ptr = dataRsp.fields.begin(); row_ptr != dataRsp.fields.end(); ++row_ptr)
        {
            tars::Int64 member = S2L(row_ptr->front().colValue);
            tars::Int64 score = S2L(row_ptr->back().colValue);
            if (RANKSCORES2CHIP(score * -1) == 0)
                continue;

            auto sngRankingBaseInfo = resp.add_rankinglist();
            sngRankingBaseInfo->set_ranking(iRank++);
            sngRankingBaseInfo->set_uid(member);
            sngRankingBaseInfo->set_chip(RANKSCORES2CHIP(score * -1));
        }

        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::E_SNG_PROCESS_REFRESH_RANKING_LIST_RESP, resp);
    }

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//游戏信息
int MatchServantImp::onGameInfo(const XGameComm::TPackage &pkg, const matchProto::GameInfoReq &gameInfoReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::GameInfoResp gameInfoResp;
    auto pGameInfo = gameInfoResp.mutable_gameinfo();
    pGameInfo->set_systime(TNOW);  //系统时间

    {
        //查询比赛信息
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(GAME_MATCH_GAME_INFO) + ":" + I2S(gameInfoReq.matchid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = gameInfoReq.matchid();

        vector<TField> fields;
        TField tfield;
        tfield.colArithType = E_NONE;
        tfield.colName = "start_time";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "start_time_type";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "remaining_count";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "avg_chip";
        tfield.colType = BIGINT;
        fields.push_back(tfield);
        tfield.colName = "remaining_time";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "blind_level";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "max_blind_level";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "match_state";
        tfield.colType = INT;
        fields.push_back(tfield);
        dataReq.fields = fields;

        dataproxy::TReadDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(gameInfoReq.matchid())->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_DEBUG << "read match game info, uid: " << pkg.stuid().luid()
                          << ", match id: " << gameInfoReq.matchid()
                          << ", iResult: " << dataRsp.iResult << endl;

            gameInfoResp.set_resultcode(XGameRetCode::SYS_BUSY);
            gameInfoResp.set_matchid(gameInfoReq.matchid());
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_GAME_INFO, gameInfoResp);
            return -1;
        }

        ROLLLOG_DEBUG << "read match game info, uid: " << pkg.stuid().luid()
                      << ", match id: " << gameInfoReq.matchid()
                      << ", fields size: " << dataRsp.fields.size() << endl;

        if (dataRsp.fields.size() > 0)
        {
            for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
            {
                for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
                {
                    ROLLLOG_DEBUG << "read match game info, colName: " << itfields->colName << ", colValue: " << itfields->colValue << endl;

                    //开赛时间
                    if (itfields->colName == "start_time")
                    {
                        pGameInfo->set_starttime(S2I(itfields->colValue));
                    }
                    //开赛时间类型
                    if (itfields->colName == "start_time_type")
                    {
                        pGameInfo->set_starttimetype(S2I(itfields->colValue));
                    }
                    //剩余人数
                    else if (itfields->colName == "remaining_count")
                    {
                        pGameInfo->set_remainingcount(S2I(itfields->colValue));
                    }
                    //平均记分牌
                    else if (itfields->colName == "avg_chip")
                    {
                        pGameInfo->set_avgchip(S2L(itfields->colValue));
                    }
                    //升盲剩余时间, 秒
                    else if (itfields->colName == "remaining_time")
                    {
                        pGameInfo->set_remainingtime(S2I(itfields->colValue));
                    }
                    //当前盲注级别
                    else if (itfields->colName == "blind_level")
                    {
                        pGameInfo->set_blindlevel(S2I(itfields->colValue));
                    }
                    //最大盲注级别
                    else if (itfields->colName == "max_blind_level")
                    {
                        pGameInfo->set_maxblindlevel(S2I(itfields->colValue));
                    }
                    //比赛状态
                    else if (itfields->colName == "match_state")
                    {
                        switch(S2I(itfields->colValue))
                        {
                        case 1: //比赛中
                            pGameInfo->set_matchstate(matchProto::MATCH_STATE_PLAYING_GAME);
                            break;
                        case 2: //比赛结束
                            pGameInfo->set_matchstate(matchProto::MATCH_STATE_END);
                            break;
                        default:
                            pGameInfo->set_matchstate(matchProto::MATCH_STATE_DEFAULT);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
            auto it = allMatchConfig.mapMatchRoomConfig.find(gameInfoReq.matchid());
            if (it == allMatchConfig.mapMatchRoomConfig.end())
            {
                ROLLLOG_ERROR << "can not find match room config, matchid: " << gameInfoReq.matchid() << endl;
                return -2;
            }

            //时间过了
            // int tnow = (TNOW + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
            // int tstart = (it->second.startTime + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
            // if ((tnow != 0) && (tnow > tstart))
            // {
            //  pGameInfo->set_matchstate(matchProto::MATCH_STATE_END);
            // }

            int tnow = 0;
            int tstart = 0;
            if (it->second.startTimeType == 0)
            {
                tnow = (TNOW + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
                tstart = (it->second.startTime + ZONE_TIME_OFFSET) % ONE_DAY_TIME;
            }
            else if (it->second.startTimeType == 1)
            {
                tnow = TNOW + ZONE_TIME_OFFSET;
                tstart = it->second.startTime + ZONE_TIME_OFFSET;
            }

            if ((tnow != 0) && (tnow > tstart))
            {
                pGameInfo->set_matchstate(matchProto::MATCH_STATE_END);
            }
        }
    }

    //报名人数
    {
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(gameInfoReq.matchid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = gameInfoReq.matchid();
        dataReq.paraExt.resetDefautlt();
        dataReq.paraExt.subOperateType = E_REDIS_SORTSET_ZCARD;

        dataproxy::TReadDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(gameInfoReq.matchid())->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read player count err, uid: " << pkg.stuid().luid()
                          << ", match id: " << gameInfoReq.matchid()
                          << ", iResult: " << dataRsp.iResult << endl;
            gameInfoResp.set_resultcode(XGameRetCode::SYS_BUSY);
            gameInfoResp.set_matchid(gameInfoReq.matchid());
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_GAME_INFO, gameInfoResp);
            return -3;
        }

        ROLLLOG_DEBUG << "read player count, uid: " << pkg.stuid().luid()
                      << ", match id: " << gameInfoReq.matchid()
                      << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                //
                ROLLLOG_DEBUG << "read player count, colName: " << itfields->colName << ", colValue: " << itfields->colValue << endl;
                //参赛人数
                pGameInfo->set_count(S2I(itfields->colValue));
            }
        }

        auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
        auto it = allMatchConfig.mapMatchRoomConfig.find(gameInfoReq.matchid());
        if (it == allMatchConfig.mapMatchRoomConfig.end())
        {
            ROLLLOG_ERROR << "can not find match room config, matchid: " << gameInfoReq.matchid() << endl;
            return -2;
        }

        MatchRoom matchRoom = it->second;
        ROLLLOG_DEBUG << "onGameInfo, matchID:" << gameInfoReq.matchid()
                      << ", maxNumber: " << (size_t)matchRoom.maxNumber
                      << ", num: " << pGameInfo->count() << endl;

        pGameInfo->set_isfull((size_t)matchRoom.maxNumber <= (size_t)pGameInfo->count());
    }

    gameInfoResp.set_resultcode(0);
    gameInfoResp.set_matchid(gameInfoReq.matchid());
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_GAME_INFO, gameInfoResp);

    //
    ROLLLOG_DEBUG << "onGameInfo, req: " << logPb(gameInfoReq) << ", resp: " << logPb(gameInfoResp) << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

// 获取比赛奖励信息
int MatchServantImp::onListMatchReward(const XGameComm::TPackage &pkg, const matchProto::ListRewardReq &listRewardReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::ListRewardResp listRewardResp;

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto it = allMatchConfig.mapMatchRoomConfig.find(listRewardReq.matchid());
    if (it == allMatchConfig.mapMatchRoomConfig.end())
    {
        ROLLLOG_ERROR << "can not find match room config, matchid: " << listRewardReq.matchid() << endl;
        listRewardResp.set_resultcode(XGameRetCode::MATCH_NO_CONFIG);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_LIST_MATCH_REWARD, listRewardResp);
        return -1;
    }

    //报名人数
    int numbers = 0;
    {
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(listRewardReq.matchid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = listRewardReq.matchid();
        dataReq.paraExt.resetDefautlt();
        dataReq.paraExt.subOperateType = E_REDIS_SORTSET_ZCARD;

        dataproxy::TReadDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(listRewardReq.matchid())->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_DEBUG << "read player count err, uid: " << pkg.stuid().luid()
                          << ", match id: " << listRewardReq.matchid()
                          << ", iResult: " << dataRsp.iResult << endl;

            listRewardResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_LIST_MATCH_REWARD, listRewardResp);
            return -2;
        }


        ROLLLOG_DEBUG << "read player count, uid: " << pkg.stuid().luid()
                      << ", match id: " << listRewardReq.matchid()
                      << ", fields size: " << dataRsp.fields.size() << endl;

        //遍历应答字段
        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                //
                ROLLLOG_DEBUG << "read player count, colName: " << itfields->colName << ", colValue: " << itfields->colValue << endl;

                //总人数
                numbers = S2I(itfields->colValue);
            }
        }
    }

    auto itMatchReward = allMatchConfig.mapMatchRewardConfigExt.find(it->second.id);
    if (itMatchReward == allMatchConfig.mapMatchRewardConfigExt.end())
    {
        //无奖励配置
        ROLLLOG_ERROR << "sign player count, numbers: " << numbers << ", awards id: " << it->second.awardsID << endl;
        //
        listRewardResp.set_resultcode(XGameRetCode::MATCH_NO_REWARD_CONFIG);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_LIST_MATCH_REWARD, listRewardResp);
        return -3;
    }

    //
    if (itMatchReward != allMatchConfig.mapMatchRewardConfigExt.end())
    {
        for (auto itmatchreward = itMatchReward->second.begin(); itmatchreward != itMatchReward->second.end();  ++itmatchreward)
        {
            //不在区间
            if (numbers < itmatchreward->minPlay || numbers > itmatchreward->maxPlay)
                continue;

            //比赛奖励
            auto pmatchReward = listRewardResp.mutable_matchreward();
            pmatchReward->set_id(itmatchreward->id);
            pmatchReward->set_minplay(itmatchreward->minPlay);
            pmatchReward->set_maxplay(itmatchreward->maxPlay);
            pmatchReward->set_rewardid(itmatchreward->rewardID);
            pmatchReward->set_number(itmatchreward->number);

            int actnumber = 0;  //实际获奖人数
            auto &mapReward = allMatchConfig.mapRewardConfigExt;
            auto itfind = mapReward.find(itmatchreward->rewardID);
            if (itfind != mapReward.end())
            {
                for (auto itreward = itfind->second.begin(); itreward != itfind->second.end(); ++itreward)
                {
                    //无奖励人数
                    if (itmatchreward->number <= 0)
                        break;

                    //排名奖励
                    auto preward = listRewardResp.add_data();
                    preward->set_id(itreward->id);
                    preward->set_rank(itreward->rank);
                    preward->set_ratio(itreward->ratio);
                    preward->set_rankstart(itreward->rankStart);
                    preward->set_rankend(itreward->rankEnd);

                    //道具
                    auto &mapRewardProps = allMatchConfig.mapRewardPropsConfigExt;
                    auto itrewardProps = mapRewardProps.find(itmatchreward->rewardID);
                    if (itrewardProps != mapRewardProps.end())
                    {
                        //排名
                        auto itrewardRank = itrewardProps->second.find(itreward->rank);
                        if (itrewardRank != itrewardProps->second.end())
                        {
                            for (auto itProps = itrewardRank->second.item.begin(); itProps != itrewardRank->second.item.end(); ++itProps)
                            {
                                auto pProps = preward->add_props();
                                pProps->set_id(itProps->propsID);
                                pProps->set_number(itProps->number);
                            }
                        }
                    }

                    //
                    actnumber++;
                    if (actnumber >= itmatchreward->number)
                    {
                        break;
                    }
                }
            }
        }
    }

    ROLLLOG_DEBUG << "send response to client, rsp: " << logPb(listRewardResp) << endl;
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_LIST_MATCH_REWARD, listRewardResp);

    __CATCH__

    FUNC_EXIT("", iRet);
    return iRet;
}

//用户报名信息
int MatchServantImp::onUserSignUpInfo(const XGameComm::TPackage &pkg, const matchProto::UserSignUpInfoReq &userSignUpInfoReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::UserSignUpInfoResp userSignUpInfoResp;
    auto psignUpInfo = userSignUpInfoResp.mutable_info();
    if (pkg.stuid().luid() < 0 || userSignUpInfoReq.matchid() < 0)
    {
        ROLLLOG_ERROR << "paramter err, uid: " << pkg.stuid().luid() << ", matchid: " << userSignUpInfoReq.matchid() << endl;
        userSignUpInfoResp.set_resultcode(XGameRetCode::ARG_INVALIDATE_ERROR);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_INFO, userSignUpInfoResp);
        return -1;
    }

    //查询排名
    {
        vector<match::UserRankInfo> vecRankInfo;
        iRet = ProcessorSingleton::getInstance()->selectGameRank(userSignUpInfoReq.matchid(), vecRankInfo);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "select game rank err, uid: " << pkg.stuid().luid() << ", matchid: " << userSignUpInfoReq.matchid() << endl;
            userSignUpInfoResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_INFO, userSignUpInfoResp);
            return -2;
        }

        //查找排名
        for (auto it = vecRankInfo.begin(); it != vecRankInfo.end(); ++it)
        {
            if (it->uid == pkg.stuid().luid())
            {
                psignUpInfo->set_chip(it->chip);   //记分牌
                psignUpInfo->set_ranking(it->rank);  //排名
            }
        }
    }

    //玩家信息
    {
        match::UserSignUpInfo info;
        iRet = ProcessorSingleton::getInstance()->selectUserInfo(pkg.stuid().luid(), info);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "select user info err, uid: " << pkg.stuid().luid() << ", matchid: " << userSignUpInfoReq.matchid() << endl;
            userSignUpInfoResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_INFO, userSignUpInfoResp);
            return -3;
        }

        for (auto item : info.data)
        {
            userSignUpInfoResp.mutable_info()->set_matchid(item.second.matchID);
            // userSignUpInfoResp.mutable_info()->set_chip(item.second.chip);
            // userSignUpInfoResp.mutable_info()->set_ranking(item.second.ranking);
            userSignUpInfoResp.mutable_info()->set_additional(item.second.additional);
            userSignUpInfoResp.mutable_info()->set_repurchase(item.second.repurchase);
            userSignUpInfoResp.mutable_info()->set_userstate((matchProto::UserState)int(item.second.userState));
            userSignUpInfoResp.mutable_info()->set_showtype(item.second.showtype);
            break;
        }

        //增购，重购
        auto it = info.data.find(userSignUpInfoReq.matchid());
        if (it != info.data.end())
        {
            psignUpInfo->set_additional(it->second.additional);
            psignUpInfo->set_repurchase(it->second.repurchase);
            switch (it->second.userState)
            {
            case match::USER_STATE_PLAYING_GAME:
                psignUpInfo->set_userstate(matchProto::USER_STATE_PLAYING_GAME);
                break;
            case match::USER_STATE_KNOCKOUT:
                psignUpInfo->set_userstate(matchProto::USER_STATE_KNOCKOUT);
                break;
            default:
                psignUpInfo->set_userstate(matchProto::USER_STATE_DEFAULT);
                break;
            }
        }
    }

    userSignUpInfoResp.set_resultcode(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_INFO, userSignUpInfoResp);
    ROLLLOG_DEBUG << "onUserSignUpInfo, uid: " << pkg.stuid().luid() << ", matchid: " << userSignUpInfoReq.matchid() << ", req: " << logPb(userSignUpInfoReq) << ", resp: " << logPb(userSignUpInfoResp) << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//用户是否报名比赛
int MatchServantImp::onUserSignUpFlag(const XGameComm::TPackage &pkg, const matchProto::UserSignUpFlagReq &userSignUpFlagReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    matchProto::UserSignUpFlagResp userSignUpFlagResp;
    if (pkg.stuid().luid() < 0 || userSignUpFlagReq.matchid() < 0)
    {
        ROLLLOG_ERROR << "paramter err, uid: " << pkg.stuid().luid() << ", matchid: " << userSignUpFlagReq.matchid() << endl;
        userSignUpFlagResp.set_resultcode(XGameRetCode::ARG_INVALIDATE_ERROR);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_FLAG, userSignUpFlagResp);
        return -1;
    }

    //用户的报名信息
    UserSignUpInfo userSignUpInfo;

    {
        //是否已经报名
        TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(pkg.stuid().luid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = 1;
        dataReq.clusterInfo.frageFactor = pkg.stuid().luid();

        dataproxy::TReadDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(pkg.stuid().luid())->redisRead(dataReq, dataRsp);
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read match user info err, uid: " << pkg.stuid().luid() << ", iResult: " << dataRsp.iResult << endl;
            userSignUpFlagResp.set_resultcode(XGameRetCode::SYS_BUSY);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_FLAG, userSignUpFlagResp);
            return -2;
        }

        ROLLLOG_DEBUG << "read match user info, uid: " << pkg.stuid().luid() << ", fields size: " << dataRsp.fields.size() << endl;

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, userSignUpInfo);

                __CATCH__

                auto itfind = userSignUpInfo.data.find(userSignUpFlagReq.matchid());
                if (itfind != userSignUpInfo.data.end())
                {
                    userSignUpFlagResp.set_resultcode(0);
                    switch (itfind->second.userState)
                    {
                    case match::USER_STATE_DEFAULT:
                        userSignUpFlagResp.set_flag(matchProto::USER_STATE_PLAYING_GAME);
                        break;
                    case match::USER_STATE_PLAYING_GAME:
                        userSignUpFlagResp.set_flag(matchProto::USER_STATE_PLAYING_GAME);
                        break;
                    case match::USER_STATE_KNOCKOUT:
                        userSignUpFlagResp.set_flag(matchProto::USER_STATE_KNOCKOUT);
                        break;
                    default:
                        userSignUpFlagResp.set_flag(matchProto::USER_STATE_DEFAULT);
                        break;
                    }

                    userSignUpFlagResp.set_matchid(userSignUpFlagReq.matchid());
                    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_FLAG, userSignUpFlagResp);
                    return 0;
                }
            }
        }
    }

    userSignUpFlagResp.set_resultcode(0);
    userSignUpFlagResp.set_flag(0);
    userSignUpFlagResp.set_matchid(userSignUpFlagReq.matchid());
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::MATCH_USER_SIGN_UP_FLAG, userSignUpFlagResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

int MatchServantImp::onListSNGRoom(const XGameComm::TPackage &pkg, matchProto::SNGGetInfoListReq &sngGetInfoListReq, const std::string &sCurServrantAddr)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();

    matchProto::SNGGetInfoListResp resp;
    config::ListMatchRoomResp listMatchRoomResp;
    iRet = g_app.getOuterFactoryPtr()->getConfigServantPrx()->ListMatchRoom(listMatchRoomResp);
    if (iRet != 0 )
    {
        ROLLLOG_ERROR << "list sng room list error: " << ", iRet: " << iRet << endl;
        return iRet;
    }

    ROLLLOG_DEBUG << "room size:" << listMatchRoomResp.data.size() << endl;

    for (auto item : listMatchRoomResp.data)
    {
        if (item.second.type == 1)
            continue;

        auto pSngRoom = resp.add_infos();
        pSngRoom->set_name(item.second.name);
        pSngRoom->set_startgamepeople(item.second.roomID.size() > 3 ? item.second.roomID[2] - '0' : 0);
        pSngRoom->set_gametype(item.second.minNumber);
        pSngRoom->set_initialchip(item.second.initialCard);
        pSngRoom->set_matchid(I2S(item.second.id));
        pSngRoom->set_roomid(item.second.roomID);

        auto itFee = allMatchConfig.mapEntryFeeConfig.find(item.second.entryFeeID);
        if (itFee != allMatchConfig.mapEntryFeeConfig.end())
        {
            for (auto itentryFee = itFee->second.begin(); itentryFee != itFee->second.end(); ++itentryFee)
            {
                if (itentryFee->style == E_BUY_STYLE_DEFAULT)
                {
                    pSngRoom->set_joincondition(itentryFee->number);
                }
            }
        }

        {
            TReadDataReq dataReq;
            dataReq.resetDefautlt();
            dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(item.second.id);
            dataReq.operateType = E_REDIS_READ;
            dataReq.clusterInfo.resetDefautlt();
            dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
            dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
            dataReq.clusterInfo.frageFactor = item.second.id;
            dataReq.paraExt.resetDefautlt();
            dataReq.paraExt.subOperateType = E_REDIS_SORTSET_ZCARD;

            dataproxy::TReadDataRsp dataRsp;
            iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(item.second.id)->redisRead(dataReq, dataRsp);
            if (iRet != 0 || dataRsp.iResult != 0)
            {
                ROLLLOG_DEBUG << "read player count err, uid: " << pkg.stuid().luid()
                              << ", match id: " << item.second.id
                              << ", iResult: " << dataRsp.iResult << endl;
            }
            else
            {
                ROLLLOG_DEBUG << "read player count, uid: " << pkg.stuid().luid()
                              << ", match id: " << item.second.id
                              << ", fields size: " << dataRsp.fields.size() << endl;

                for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
                {
                    for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
                    {
                        ROLLLOG_DEBUG << "read player count, colName: " << itfields->colName
                                      << ", colValue: " << itfields->colValue << endl;

                        //参赛人数
                        pSngRoom->set_joingamepeople(S2I(itfields->colValue));
                    }
                }
            }
        }
    }

    resp.set_resultcode(iRet);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::E_SNG_INFO_LIST_RESP, resp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

int MatchServantImp::onListSNGConfig(const XGameComm::TPackage &pkg, matchProto::SNGConfigReq &sngConfigReq, const std::string &sCurServrantAddr)
{
    int iRet = 0;
    __TRY__

    matchProto::SNGConfigResp sngConfigResp;

    auto &allMatchConfig = g_app.getOuterFactoryPtr()->getAllMatchConfig();
    auto matchConfig = allMatchConfig.mapMatchRoomConfig.find(S2I(sngConfigReq.matchid()));
    auto itBlind = allMatchConfig.mapMatchBlindConfig.find(matchConfig->second.blindID);
    if (itBlind != allMatchConfig.mapMatchBlindConfig.end())
    {
        for (auto item : itBlind->second)
        {
            auto blindConfigs = sngConfigResp.add_blindconfigs();
            blindConfigs->set_level(item.round);
            blindConfigs->set_minblind(item.smallBlind);
            blindConfigs->set_maxblind(item.bigBlind);
            blindConfigs->set_frontblind(item.ante);
            blindConfigs->set_raisetime(matchConfig->second.blindTime);
        }
    }

    string roomID = matchConfig->second.roomID;

    //道具奖励
    if (roomID.size() > 3 && roomID[2] != '3')
    {
        auto itMatchReward = allMatchConfig.mapMatchRewardConfigExt.find(S2I(sngConfigReq.matchid()));
        if (itMatchReward == allMatchConfig.mapMatchRewardConfigExt.end())
        {
            ROLLLOG_ERROR << "awards is empty , match id: " << sngConfigReq.matchid() << endl;
            return -1;
        }

        if (itMatchReward != allMatchConfig.mapMatchRewardConfigExt.end())
        {
            int rewardID = 0;
            int rewardNum = 0;
            //奖励配置
            for (auto itreward = itMatchReward->second.begin(); itreward != itMatchReward->second.end(); ++itreward)
            {
                if (roomID[2] - '0' <= itreward->maxPlay && roomID[2] - '0' > itreward->minPlay)
                {
                    rewardID = itreward->rewardID;
                    rewardNum = itreward->number;
                }
            }

            const auto &mapRewardProps = allMatchConfig.mapRewardPropsConfigExt;
            auto itrewardProps = mapRewardProps.find(rewardID);
            if (itrewardProps != mapRewardProps.end())
            {
                for (int rank = 1; rank <= rewardNum; ++rank)
                {
                    auto preward = sngConfigResp.add_rewardconfigs();
                    preward->set_ranking(rank);
                    auto itrewardRank = itrewardProps->second.find(rank);
                    if (itrewardRank != itrewardProps->second.end())
                    {
                        for (auto itProps = itrewardRank->second.item.begin(); itProps != itrewardRank->second.item.end(); ++itProps)
                        {
                            auto pProps = preward->add_rewards();
                            pProps->set_rewardid(itProps->propsID);
                            pProps->set_rewardnum(itProps->number);
                        }
                    }
                }
            }
        }
    }
    else
    {
        auto preward = sngConfigResp.add_rewardconfigs();
        auto pProps = preward->add_rewards();
        preward->set_ranking(1);

        dataproxy::TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(GAME_MATCH_REWARD) + ":" + sngConfigReq.matchid() +  L2S(sngConfigReq.tableid() == 0 ? 1 : sngConfigReq.tableid());
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
        dataReq.clusterInfo.frageFactor = S2I(sngConfigReq.matchid() +  L2S(sngConfigReq.tableid() == 0 ? 1 : sngConfigReq.tableid()));

        vector<TField> fields;
        TField tfield;
        tfield.colArithType = E_NONE;
        tfield.colName = "propID";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "number";
        tfield.colType = INT;
        fields.push_back(tfield);
        dataReq.fields = fields;

        TReadDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(sngConfigReq.matchid())->redisRead(dataReq, dataRsp);
        ROLLLOG_DEBUG << "read game reward data, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read game reward data err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
            return -2;
        }

        if (dataRsp.fields.size() > 0)
        {
            //遍历应答字段
            for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
            {
                for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
                {
                    if (itfields->colName == "propID")
                    {
                        pProps->set_rewardid(S2I(itfields->colValue));
                    }
                    else if (itfields->colName == "number")
                    {
                        pProps->set_rewardnum(S2I(itfields->colValue));
                    }
                }
            }
        }
    }
    sngConfigResp.set_matchid(sngConfigReq.matchid());
    sngConfigResp.set_resultcode(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::E_SNG_CONFIG_RESP, sngConfigResp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

int MatchServantImp::onListAIRoomConfig(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr)
{
    int iRet = 0;
    __TRY__

    matchProto::AIProcessRoomListResp resp;

    config::ListMatchRoomResp listMatchRoomResp;
    iRet = g_app.getOuterFactoryPtr()->getConfigServantPrx()->ListMatchRoom(listMatchRoomResp);
    if (iRet != 0 )
    {
        ROLLLOG_ERROR << "list room list error: " << ", iRet: " << iRet << endl;
        return iRet;
    }

    map<string, long> m_TUOnline;
    for(auto itemOnline : m_UOline)
    {
        long total_online = 0;
        for(auto sub : itemOnline.second)
        {
            total_online += sub.second;
        }
        m_TUOnline.insert(std::make_pair(itemOnline.first, total_online));
    }

    for(auto itemRoom : listMatchRoomResp.data)
    {
        auto roomID = itemRoom.second.roomID;
        auto roomType = roomID[0] - 48;
        if(roomType != 8 && roomID[2] - 48 != 2)
        {
            continue;
        }

        map<int, config::PrivateRoomBlind> cfg;
        g_app.getOuterFactoryPtr()->getConfigServantPrx()->getPrivateRoomBlind(roomID[0] - 48 + roomID[2] - 49, cfg);

        auto it = m_TUOnline.find(roomID);
        auto curOlineCount = it == m_TUOnline.end() ? 0 : it->second;
        LOG_DEBUG << "m_TUOnline size: " << m_TUOnline.size() << ", roomID: " << roomID << ", curOlineCount: " << curOlineCount << endl;

        if(curOlineCount >= itemRoom.second.maxNumber - 3)
        {
            continue;
        }
        for(auto itemCfg : cfg)
        {
            int levelOnlineCount = 0;
            auto it = m_UOline.find(roomID);
            if(it != m_UOline.end())
            {
                auto itt = it->second.find(itemCfg.second.smallBlind);
                if(itt != it->second.end())
                {
                    levelOnlineCount = itt->second;
                }
            }

            LOG_DEBUG << "blindLevel: " << itemCfg.second.level << ", levelOnlineCount: " << levelOnlineCount << ", takenin: " << itemCfg.second.fastGold << endl;
            auto info = resp.add_infos();
            info->set_roomid(roomID);
            info->set_blindlevel(itemCfg.second.level);
            info->set_smallblind(itemCfg.second.smallBlind);
            info->set_bigblind(itemCfg.second.bigBlind);
            info->set_curolinecount(levelOnlineCount);
            info->set_takenin(itemCfg.second.fastGold);
        }
    }

    resp.set_resultcode(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::E_AI_PROCESS_ROOM_LIST_RESP, resp);

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}


//发送消息到客户端
template<typename T>
int MatchServantImp::toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, const T &t)
{
    XGameComm::TPackage rsp;
    rsp.set_iversion(tPackage.iversion());

    auto ptuid = rsp.mutable_stuid();
    ptuid->set_luid(tPackage.stuid().luid());
    rsp.set_igameid(tPackage.igameid());
    rsp.set_sroomid(tPackage.sroomid());
    rsp.set_iroomserverid(tPackage.iroomserverid());
    rsp.set_isequence(tPackage.isequence());
    rsp.set_iflag(tPackage.iflag());

    auto mh = rsp.add_vecmsghead();
    mh->set_nmsgid(actionName);
    mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_RESPONSE);
    mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_MATCH);
    rsp.add_vecmsgdata(pbToString(t));

    auto pPushPrx = Application::getCommunicator()->stringToProxy<JFGame::PushPrx>(sCurServrantAddr);
    if (pPushPrx)
    {
        ROLLLOG_DEBUG << "toClientPb, uid: " << tPackage.stuid().luid() << ", actionName: " << actionName << ", toclient pb: " << logPb(rsp) << ", t: " << logPb(t) << endl;
        pPushPrx->tars_hash(tPackage.stuid().luid())->async_doPushBuf(NULL, tPackage.stuid().luid(), pbToString(rsp));
    }
    else
    {
        ROLLLOG_ERROR << "pPushPrx is null: " << tPackage.stuid().luid() << ", actionName: " << actionName << ", toclient pb: " << logPb(rsp) << ", t: " << logPb(t) << endl;
    }

    return 0;
}

tars::Int32 MatchServantImp::doCustomMessage(bool bExpectIdle)
{
    if (bExpectIdle)
    {
        // LOG_DEBUG << "-----------------MatchServantImp::doCustomMessage()-------------------------" << endl;
    }

    return 0;
}

//每日重置回调
tars::Int32 MatchServantImp::perDayReset()
{
    return 0;
}

//每周重置回调
tars::Int32 MatchServantImp::perWeekReset()
{
    return 0;
}

//每月重置回调
tars::Int32 MatchServantImp::perMonthReset()
{
    return 0;
}

