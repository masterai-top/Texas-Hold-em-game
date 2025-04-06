#ifndef _Processor_H_
#define _Processor_H_

//
#include <util/tc_singleton.h>
#include "MatchServant.h"

#define STARTCHIP2RANKSCORES(x) (x * 100000 + 100000 - time(NULL) % 100000)
#define CHIP2RANKSCORES(x) (x * 100000 + time(NULL) % 100000)
#define RANKSCORES2CHIP(x) (x / 100000)

//
using namespace tars;

/**
 *请求处理类
 *
 */
class Processor
{
public:
    /**
     *
    */
    Processor();

    /**
     *
    */
    ~Processor();

public:
    //GAME_MATCH_GAME_RANK = 101,   //比赛报名集合，key:matchid
    //查询
    int selectGameRank(int matchID, vector<match::UserRankInfo> &vecRankInfo);
    //数量
    int countGameRank(int matchID);
    //新增
    int addGameRank(int matchID, const match::UserMatchInfo &userMatchInfo);
    //删除
    int deleteGameRank(int matchID);
    //移除
    int rmGameRank(int matchID, long uid);

    //GAME_MATCH_GAME_INFO = 102,   //比赛信息, key:matchid
    //查询
    int selectGameInfo(int matchID, match::MatchInfo &matchInfo);
    //增加
    int UpdateGameInfo(const match::ReportMatchInfoReq &req);
    //删除
    int deleteGameInfo(int matchID);

    //GAME_MATCH_USER_INFO = 103,   //玩家的比赛信息, key:uid
    //查询
    int selectUserInfo(long uid, match::UserSignUpInfo &info);
    //更新
    int updateUserInfo(long uid, match::UserSignUpInfo &info);

    //GAME_MATCH_JACKPOT   = 104,   //比赛奖池，key:matchid
    //查询
    int selectJackpot(int matchID);
    //更新
    int updateJackpot(int matchID);
    //清理
    int cleanJackpot(int matchID);
};

//singleton
typedef TC_Singleton<Processor, CreateStatic, DefaultLifetime> ProcessorSingleton;

#endif

