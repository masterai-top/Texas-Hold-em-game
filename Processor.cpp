#include "Processor.h"
#include "globe.h"
#include "LogComm.h"
#include "DataProxyProto.h"
#include "ServiceDefine.h"
#include "util/tc_hash_fun.h"
#include "uuid.h"
#include "MatchServer.h"
#include "util/tc_base64.h"

//
using namespace std;
using namespace dataproxy;
using namespace dbagent;

/**
 *
*/
Processor::Processor()
{

}

/**
 *
*/
Processor::~Processor()
{

}

//查询
int Processor::selectGameRank(int matchID, vector<match::UserRankInfo> &vecRankInfo)
{
    int iRet = 0;

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(matchID);
    dataReq.operateType = E_REDIS_READ;
    dataReq.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = matchID;

    //
    dataproxy::TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisRead(dataReq, dataRsp);
    if(iRet != 0 || dataRsp.iResult != 0)
    {
        return -1;
    }

    //取报名数据
    int rank = 1;
    for(vector<vector<dbagent::TField> >::iterator row_ptr = dataRsp.fields.begin();
            row_ptr != dataRsp.fields.end();
            ++row_ptr)
    {
        match::UserRankInfo userRankInfo;

        userRankInfo.uid = S2L(row_ptr->front().colValue);
        userRankInfo.chip = abs(RANKSCORES2CHIP(S2L(row_ptr->back().colValue)));
        userRankInfo.rank = rank;

        vecRankInfo.push_back(userRankInfo);

        //
        ++rank;
    }

    return 0;
}

//数量
int Processor::countGameRank(int matchID)
{
    int iRet = 0;

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(matchID);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = matchID;

    //
    dataproxy::TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisRead(dataReq, dataRsp);
    if(iRet != 0 || dataRsp.iResult != 0)
    {
        return -1;
    }

    //
    return dataRsp.fields.size();
}

//报名
int Processor::addGameRank(int matchID, const match::UserMatchInfo &userMatchInfo)
{
    //记入排名
    dataproxy::TWriteDataReq wdataReq;
    wdataReq.resetDefautlt();
    wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(matchID);
    wdataReq.operateType = E_REDIS_INSERT;
    wdataReq.clusterInfo.resetDefautlt();
    wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    wdataReq.clusterInfo.frageFactor = matchID;

    vector<dbagent::TField> fields;
    dbagent::TField tfield;
    tfield.colArithType = E_NONE;
    tfield.colValue = I2S(userMatchInfo.uid);
    fields.push_back(tfield);
    tfield.colValue = I2S(CHIP2RANKSCORES(userMatchInfo.chip) * -1);
    fields.push_back(tfield);
    wdataReq.fields = fields;

    dataproxy::TWriteDataRsp wdataRsp;
    int iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisWrite(wdataReq, wdataRsp);
    if (iRet != 0 || wdataRsp.iResult != 0)
    {
        return -1;
    }

    return 0;
}

//删除
int Processor::deleteGameRank(int matchID)
{
    int iRet = 0;

    //清理报名排名数据
    dataproxy::TWriteDataReq wdataReq;
    wdataReq.resetDefautlt();
    wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(matchID);
    wdataReq.operateType = E_REDIS_DELETE;
    wdataReq.clusterInfo.resetDefautlt();
    wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    wdataReq.clusterInfo.frageFactor = matchID;
    wdataReq.paraExt.resetDefautlt();
    wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM_RANGE_BY_RANK;
    wdataReq.paraExt.start = 0;
    wdataReq.paraExt.end = -1;

    //
    dataproxy::TWriteDataRsp wdataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisWrite(wdataReq, wdataRsp);
    if (iRet != 0 || wdataRsp.iResult != 0)
    {
        return -1;
    }

    return 0;
}

//移除
int Processor::rmGameRank(int matchID, long uid)
{
    int iRet = 0;

    //清理报名排名数据
    dataproxy::TWriteDataReq wdataReq;
    wdataReq.resetDefautlt();
    wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(GAME_MATCH_GAME_RANK) + ":" + I2S(matchID);
    wdataReq.operateType = E_REDIS_INSERT;
    wdataReq.clusterInfo.resetDefautlt();
    wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    wdataReq.clusterInfo.frageFactor = matchID;
    wdataReq.paraExt.resetDefautlt();
    wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM;

    dbagent::TField tfield;
    tfield.colArithType = E_NONE;
    vector<dbagent::TField> fields;

    tfield.colValue = I2S(uid);
    fields.push_back(tfield);

    //
    wdataReq.fields = fields;

    //
    dataproxy::TWriteDataRsp wdataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisWrite(wdataReq, wdataRsp);
    if (iRet != 0 || wdataRsp.iResult != 0)
    {
        return -1;
    }

    return 0;
}

//比赛信息
//查询
int Processor::selectGameInfo(int matchID, match::MatchInfo &matchInfo)
{
    int iRet = 0;

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(GAME_MATCH_GAME_INFO) + ":" + I2S(matchID);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = matchID;

    vector<TField> fields;
    TField tfield;
    tfield.colArithType = E_NONE;

    tfield.colName = "start_time";
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

    //保存
    dataReq.fields = fields;

    //
    dataproxy::TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisRead(dataReq, dataRsp);
    if(iRet != 0 || dataRsp.iResult != 0)
    {
        return -1;
    }

    //遍历应答字段
    for(auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
    {
        for(auto itfields = it->begin(); itfields != it->end(); ++itfields)
        {
            ROLLLOG_DEBUG << "read match game info, colName: " << itfields->colName << ", colValue: " << itfields->colValue << endl;

            //
            if(itfields->colName == "start_time")
            {
                matchInfo.startTime = S2I(itfields->colValue);
            }
            else if(itfields->colName == "remaining_count")
            {
                matchInfo.remainingCount = S2I(itfields->colValue);
            }
            else if(itfields->colName == "avg_chip")
            {
                matchInfo.avgChip = S2L(itfields->colValue);
            }
            else if(itfields->colName == "remaining_time")
            {
                matchInfo.remainingTime = S2I(itfields->colValue);
            }
            else if(itfields->colName == "blind_level")
            {
                matchInfo.blindLevel = S2I(itfields->colValue);
            }
            else if(itfields->colName == "max_blind_level")
            {
                matchInfo.maxBlindLevel = S2I(itfields->colValue);
            }
            else if(itfields->colName == "match_state")
            {
                switch(S2I(itfields->colValue))
                {
                case 1:
                    matchInfo.matchState = match::MATCH_STATE_PLAYING_GAME;
                    break;

                case 2:
                    matchInfo.matchState = match::MATCH_STATE_END;
                    break;

                default:
                    matchInfo.matchState = match::MATCH_STATE_DEFAULT;
                    break;
                }
            }
        }
    }

    return 0;
}

//增加
int Processor::UpdateGameInfo(const match::ReportMatchInfoReq &req)
{
    return 0;
}

//删除
int Processor::deleteGameInfo(int matchID)
{
    int iRet = 0;

    dataproxy::TWriteDataReq wdataReq;
    wdataReq.resetDefautlt();
    wdataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(GAME_MATCH_GAME_INFO) + ":" + L2S(matchID);
    wdataReq.operateType = E_REDIS_DELETE;
    wdataReq.clusterInfo.resetDefautlt();
    wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    wdataReq.clusterInfo.frageFactor = matchID;

    //
    vector<TField> fields;

    //请求数据
    wdataReq.fields = fields;

    //
    TWriteDataRsp wdataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisWrite(wdataReq, wdataRsp);
    ROLLLOG_DEBUG << "deleteGameInfo, iRet: " << iRet << ", matchid: " << matchID << ", wdataRsp: " << printTars(wdataRsp) << endl;
    //请求异常或应答错误
    if(iRet != 0 || wdataRsp.iResult != 0)
    {
        return -1;
    }

    return 0;
}

//查询
int Processor::selectUserInfo(long uid, match::UserSignUpInfo &info)
{
    int iRet = 0;

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_USER_INFO) + ":" + L2S(uid);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = 1;
    dataReq.clusterInfo.frageFactor = uid;

    //
    dataproxy::TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid)->redisRead(dataReq, dataRsp);
    if(iRet != 0 || dataRsp.iResult != 0)
    {
        return -1;
    }
    else
    {
        //
        ROLLLOG_DEBUG << "read match user info, uid: " << uid << ", fields size: " << dataRsp.fields.size() << endl;
        for(auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for(auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                ROLLLOG_DEBUG << "read match user info, colName: " << itfields->colName << ", colValue: " << itfields->colValue
                              << ", colValue length: " << itfields->colValue.length() << endl;

                __TRY__

                string decode = TC_Base64::decode(itfields->colValue);
                toObj(decode, info);

                __CATCH__

                ROLLLOG_DEBUG << "user sign up info : " << printTars(info) << endl;
            }
        }
    }

    return 0;
}

//更新
int Processor::updateUserInfo(long uid, match::UserSignUpInfo &info)
{
    return 0;
}

//GAME_MATCH_JACKPOT   = 104,   //比赛奖池，key:matchid
//查询
int Processor::selectJackpot(int matchID)
{
    int iRet = 0;

    TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_STRING) + ":" + I2S(GAME_MATCH_JACKPOT) + ":" + I2S(matchID);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = matchID;

    //
    dataproxy::TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(matchID)->redisRead(dataReq, dataRsp);
    if(iRet != 0 || dataRsp.iResult != 0)
    {
        return -1;
    }

    //
    for(auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
    {
        for(auto itfields = it->begin(); itfields != it->end(); ++itfields)
        {
            //奖池
            long jackpot = S2L(itfields->colValue);
            return jackpot;
        }
    }

    return -2;
}

//更新
int Processor::updateJackpot(int matchID)
{
    return 0;
}

//清理
int Processor::cleanJackpot(int matchID)
{
    return 0;
}

