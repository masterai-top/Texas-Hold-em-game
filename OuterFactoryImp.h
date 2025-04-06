#ifndef _OUTER_FACTORY_IMP_H_
#define _OUTER_FACTORY_IMP_H_

//
#include <string>
#include <map>

//
#include "servant/Application.h"
#include "globe.h"
#include "OuterFactory.h"
#include "util/tc_hash_fun.h"

//wbl
#include <wbl/regex_util.h>

//服务代理
#include "ConfigServant.h"
#include "HallServant.h"
#include "DBAgentServant.h"
#include "Log2DBServant.h"
#include "MatchProto.h"
#include "PushServant.h"

//
using namespace config;
using namespace hall;
using namespace push;
using namespace GoodsManager;
using namespace mail;
using namespace dataproxy;
using namespace dbagent;

//时区
//#define ONE_DAY_TIME (24*60*60)
//#define ZONE_TIME_OFFSET (8*60*60)

//邮件通知
typedef struct _MailNotifyConfig
{
    string Title;
    string Action;
    string Coin;
    string Diamond;
} MailNotifyConfig;

//
class OuterFactoryImp;
typedef TC_AutoPtr<OuterFactoryImp> OuterFactoryImpPtr;

/**
 * 外部工具接口对象工厂
 */
class OuterFactoryImp : public OuterFactory
{
private:
    /**
     *
    */
    OuterFactoryImp();

    /**
     *
    */
    ~OuterFactoryImp();

    //
    friend class MatchServantImp;
    friend class MatchServer;

public:
    //框架中用到的outer接口(不能修改):
    const OuterProxyFactoryPtr &getProxyFactory() const
    {
        return _pProxyFactory;
    }

    tars::TC_Config &getConfig() const
    {
        return *_pFileConf;
    }

public:
    //加载配置
    void load();
    //
    void loadMatchConfig();
    //
    void loadPlayerInfoConfig();
    //
    void loadRatioDividend();
    //
    void loadSignUpOrQuitTime();
    //
    void loadMailNotifyConfig();
    //代理配置
    void readPrxConfig();
    //
    void printPrxConfig();
    //比赛配置
    void readAllMatchConfig();
    //
    void printAllMatchConfig();
    //
    const ListAllMatchConfigResp &getAllMatchConfig();
    //玩家配置
    void readPlayerInfoConfig();
    //
    void printPlayerInfoConfig();
    //
    int showPlayerCount();
    //奖池分配粒度
    void readRatioDividend();
    //
    void printRatioDividend();
    //
    int getRatioDividend();
    //禁止报名、取消报名时间
    void readSignUpOrQuitTime();
    //
    void printSignUpOrQuitTime();
    //
    int getSignUpOrQuitTime();
    //邮件通知配置
    void readMailNotifyConfig();
    //
    void printMailNotifyConfig();
    //
    const MailNotifyConfig &getMailNotifyConfig();

private:
    //
    void createAllObject();
    //
    void deleteAllObject();

public:
    //修改用户财富接口
    int async2ModifyUAccount(const userinfo::ModifyUserAccountReq &req);
    //通知道具服务发送奖励
    int async2GoodsReward();
    //邮件通知比赛奖励
    int asyncMailNotifyReward(long uid, int rank, const string &roomName, const match::UserRewardGoods &rewardGoods);
    //日志入库
    void asyncLog2DB(const int64_t uid, const DaqiGame::TLog2DBReq &req);

public:
    //配置服务代理
    const ConfigServantPrx getConfigServantPrx();
    //广场服务代理
    const HallServantPrx getHallServantPrx(const long uid);
    //广场服务代理
    const HallServantPrx getHallServantPrx(const string key);
    //数据库代理服务代理
    const DBAgentServantPrx getDBAgentServantPrx(const int64_t uid);
    //数据库代理服务代理
    const DBAgentServantPrx getDBAgentServantPrx(const string &key);
    //日志入库服务代理
    const DaqiGame::Log2DBServantPrx getLog2DBServantPrx(const int64_t uid);
    //推送服
    const PushServantPrx getPushServantPrx(const int64_t uid);

public:
    //格式化时间
    string GetTimeFormat();

private:
    //拆分字符串成整形
    int splitInt(string szSrc, vector<int> &vecInt);

private:
    //读写锁，防止脏读
    wbl::ReadWriteLocker m_rwlock;
    //框架用到的共享对象(不能修改):
    tars::TC_Config *_pFileConf;
    //代理管理对象
    OuterProxyFactoryPtr _pProxyFactory;

private:
    //业务自有的共享对象:
    //配置服务
    std::string _ConfigServantObj;
    ConfigServantPrx _ConfigServerPrx;

    //广场服务
    std::string _HallServantObj;
    HallServantPrx _HallServerPrx;

    //数据库代理服务
    std::string _DBAgentServantObj;
    DBAgentServantPrx _DBAgentServerPrx;

    //日志入库服务
    std::string _Log2DBServantObj;
    DaqiGame::Log2DBServantPrx _Log2DBServerPrx;

    //消息推送服务
    std::string _PushServantObj;
    PushServantPrx _PushServerPrx;

private:
    //比赛配置信息
    ListAllMatchConfigResp listAllMatchConfigResp;
    //展示的玩家数量
    int ShowPlayerCount;
    //奖池分配粒度
    int RatioDividend;
    //禁止报名、取消报名时间
    int SignUpOrQuitTime;
    //邮件通知配置
    MailNotifyConfig mailNotifyConfig;
};

////////////////////////////////////////////////////////////////////////////////
#endif


