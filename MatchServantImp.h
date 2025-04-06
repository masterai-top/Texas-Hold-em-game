#ifndef _MatchServantImp_H_
#define _MatchServantImp_H_

//
#include "servant/Application.h"
#include "MatchServant.h"
#include "XGameComm.pb.h"
#include "CommonCode.pb.h"
#include "CommonStruct.pb.h"
#include "UserState.pb.h"
#include "match.pb.h"
#include "XGameSNG.pb.h"
#include "ConfigProto.h"
#include "TimerThread.h"
#include "TaskProto.h"
#include "UserStateProto.h"

/**
 *
 * 比赛服务接口
 */
class MatchServantImp : public match::MatchServant
{
public:
    /**
     *
     */
    MatchServantImp() {}

    /**
     *
     */
    virtual ~MatchServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

    /**
    *
    */
    void initializeTimer();

public:
    //http请求处理接口
    virtual tars::Int32 doRequest(const vector<tars::Char> &reqBuf, const map<std::string, std::string> &extraInfo, vector<tars::Char> &rspBuf, tars::TarsCurrentPtr current);
    //tcp请求处理接口
    virtual tars::Int32 onRequest(tars::Int64 lUin, const std::string &sMsgPack, const std::string &sCurServrantAddr, const JFGame::TClientParam &stClientParam, const JFGame::UserBaseInfoExt &stUserBaseInfo, tars::TarsCurrentPtr current);

public:
    //上报玩家游戏信息
    virtual tars::Int32 reportUserGameInfo(const match::ReportUserGameInfoReq &req, match::ReportUserGameInfoResp &resp, tars::TarsCurrentPtr current);
    //上报玩家淘汰信息
    virtual tars::Int32 reportUserKnockoutInfo(const match::ReportUserKnockoutInfoReq &req, match::ReportUserKnockoutInfoResp &resp, tars::TarsCurrentPtr current);
    //上报玩家状态信息
    virtual tars::Int32 reportUserState(const match::ReportUserStateReq &req, match::ReportUserStateResp &resp, tars::TarsCurrentPtr current);
    //上报比赛信息
    virtual tars::Int32 reportMatchInfo(const match::ReportMatchInfoReq &req, match::ReportMatchInfoResp &resp, tars::TarsCurrentPtr current);
    //上报滚轮赛奖励信息
    virtual tars::Int32 reportMatchReward(const match::RewardGoods &req, int matchID, int tableID, tars::TarsCurrentPtr current);
    //用户报名信息请求
    virtual tars::Int32 getUserSignUpInfo(const match::UserSignUpInfoReq &req, match::UserSignUpInfoResp &resp, tars::TarsCurrentPtr current);
    //取用户所有报名信息
    virtual tars::Int32 getUserAllSignUpInfo(const match::UserAllSignUpInfoReq &req, match::UserAllSignUpInfoResp &resp, tars::TarsCurrentPtr current);
    //增购，重购消耗请求
    virtual tars::Int32 consumeBuyRes(const match::ConsumeBuyResReq &req, match::ConsumeBuyResResp &resp, tars::TarsCurrentPtr current);
    //取比赛信息请求
    virtual tars::Int32 getMatchInfo(const match::MatchInfoReq &req, match::MatchInfoResp &resp, tars::TarsCurrentPtr current);
    //比赛报名信息请求
    virtual tars::Int32 getMatchUserInfo(const match::MatchUserInfoReq &req, match::MatchUserInfoResp &resp, tars::TarsCurrentPtr current);
    //取消比赛
    virtual tars::Int32 cancelMatch(const match::CancelMatchReq &req, match::CancelMatchResp &resp, tars::TarsCurrentPtr current);
    //清理比赛
    virtual tars::Int32 cleanMatch(const match::CleanMatchReq &req, match::CleanMatchResp &resp, tars::TarsCurrentPtr current);
    //发送奖励
    virtual tars::Int32 reward(const match::RewardReq &req, match::RewardResp &resp, tars::TarsCurrentPtr current);
    //报名请求
    virtual tars::Int32 signUp(const match::SignUpReq &req, match::SignUpResp &resp, tars::TarsCurrentPtr current);
    //取消报名请求
    virtual tars::Int32 quit(const match::QuitReq &req, match::QuitResp &resp, tars::TarsCurrentPtr current);
	//
    virtual tars::Int32 reportOnlineCount(const string& roomID, long smallBlind, long count, tars::TarsCurrentPtr current);

public:
    //比赛排名奖励
    void logMatchRank(const match::RewardReq &req, match::RewardResp &resp);
    //牌局报名记录日志
    void logMatchEnter(long uid, int matchID, const string &smatchID, int type, int enterType, long cost, long serviceFee);
    //计算奖励信息
    int calreward(const match::RewardReq &req, match::RewardResp &resp);

public:
    //报名
    int onSignUp(const XGameComm::TPackage &pkg, const matchProto::SignUpReq &signUpReq, const std::string &sCurServrantAddr);
    //退赛
    int onQuit(const XGameComm::TPackage &pkg, const matchProto::QuitReq &quitReq, const std::string &sCurServrantAddr);
    //重购
    int onRepurchase(const XGameComm::TPackage &pkg, const matchProto::RepurchaseReq &repurchaseReq, const std::string &sCurServrantAddr);
    //增购
    int onAdditional(const XGameComm::TPackage &pkg, const matchProto::AdditionalReq &additionalReq, const std::string &sCurServrantAddr);
    //奖池
    int onJackpot(const XGameComm::TPackage &pkg, const matchProto::JackpotReq &jackpotReq, const std::string &sCurServrantAddr);
    //报名人数
    int onPlayerCount(const XGameComm::TPackage &pkg, const matchProto::PlayerCountReq &playerCountReq, const std::string &sCurServrantAddr);
    //玩家信息
    int onPlayerInfo(const XGameComm::TPackage &pkg, const matchProto::PlayerInfoReq &playerInfoReq, const std::string &sCurServrantAddr);
    //游戏信息
    int onGameInfo(const XGameComm::TPackage &pkg, const matchProto::GameInfoReq &gameInfoReq, const std::string &sCurServrantAddr);
    //获取比赛奖励信息
    int onListMatchReward(const XGameComm::TPackage &pkg, const matchProto::ListRewardReq &listRewardReq, const std::string &sCurServrantAddr);
    //用户报名信息
    int onUserSignUpInfo(const XGameComm::TPackage &pkg, const matchProto::UserSignUpInfoReq &userSignUpInfoReq, const std::string &sCurServrantAddr);
    //用户是否报名比赛
    int onUserSignUpFlag(const XGameComm::TPackage &pkg, const matchProto::UserSignUpFlagReq &userSignUpFlagReq, const std::string &sCurServrantAddr);
    //SNG房间列表
    int onListSNGRoom(const XGameComm::TPackage &pkg, matchProto::SNGGetInfoListReq &sngGetInfoListReq, const std::string &sCurServrantAddr);
    //SNG房间配置
    int onListSNGConfig(const XGameComm::TPackage &pkg,  matchProto::SNGConfigReq &sngConfigReq, const std::string &sCurServrantAddr);
    //SNG排行榜
    int onListSNGRanking(const XGameComm::TPackage &pkg, int matchID, bool bUserInfo, const std::string &sCurServrantAddr);

    int onListAIRoomConfig(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr);

private:
    //发送消息到客户端
    template<typename T>
    int toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, const T &t);

public:
    //时钟周期回调
    virtual tars::Int32 doCustomMessage(bool bExpectIdle = false);

private:
    //每日重置回调
    tars::Int32 perDayReset();
    //每周重置回调
    tars::Int32 perWeekReset();
    //每月重置回调
    tars::Int32 perMonthReset();

private:
    //定时器管理
    TimerThread m_threadTimer;
};

/////////////////////////////////////////////////////
#endif
