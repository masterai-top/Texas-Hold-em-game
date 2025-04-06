#include <sstream>
#include "OuterFactoryImp.h"
#include "LogComm.h"
#include "MatchServer.h"

//
using namespace wbl;

/**
 *
*/
OuterFactoryImp::OuterFactoryImp(): _pFileConf(NULL)
{
    createAllObject();
}

/**
 *
*/
OuterFactoryImp::~OuterFactoryImp()
{
    deleteAllObject();
}

/**
 *
*/
void OuterFactoryImp::deleteAllObject()
{
    if (NULL != _pFileConf)
    {
        delete _pFileConf;
        _pFileConf = NULL;
    }
}

/**
 *
*/
void OuterFactoryImp::createAllObject()
{
    try
    {
        deleteAllObject();

        //本地配置文件
        _pFileConf = new tars::TC_Config();

        //tars代理Factory,访问其他tars接口时使用
        _pProxyFactory = new OuterProxyFactory();
        LOG_DEBUG << "init proxy factory succ." << endl;

        FDLOG_MATCH_ENTER_LOG_FORMAT;
        FDLOG_MATCH_RANK_LOG_FORMAT;

        //加载配置
        load();
        loadMatchConfig();
        loadPlayerInfoConfig();
        loadRatioDividend();
        loadSignUpOrQuitTime();
        loadMailNotifyConfig();
    }
    catch (TC_Exception &ex)
    {
        LOG->error() << ex.what() << endl;
        throw;
    }
    catch (exception &e)
    {
        LOG->error() << e.what() << endl;
        throw;
    }
    catch (...)
    {
        LOG->error() << "unknown exception." << endl;
        throw;
    }

    return;
}

//读取所有配置
void OuterFactoryImp::load()
{
    __TRY__

    //拉取远程配置
    g_app.addConfig(ServerConfig::ServerName + ".conf");

    WriteLocker lock(m_rwlock);

    _pFileConf->parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    LOG_DEBUG << "init config file succ : " << ServerConfig::BasePath + ServerConfig::ServerName + ".conf" << endl;

    readPrxConfig();
    printPrxConfig();

    __CATCH__
}

void OuterFactoryImp::loadMatchConfig()
{
    __TRY__

    WriteLocker lock(m_rwlock);

    //比赛配置
    readAllMatchConfig();
    printAllMatchConfig();

    __CATCH__
}

void OuterFactoryImp::loadPlayerInfoConfig()
{
    __TRY__

    WriteLocker lock(m_rwlock);

    //玩家配置
    readPlayerInfoConfig();
    printPlayerInfoConfig();

    __CATCH__
}

void OuterFactoryImp::loadRatioDividend()
{
    __TRY__

    WriteLocker lock(m_rwlock);

    //奖池分配粒度
    readRatioDividend();
    printRatioDividend();

    __CATCH__
}

void OuterFactoryImp::loadSignUpOrQuitTime()
{
    __TRY__

    WriteLocker lock(m_rwlock);

    //禁止报名、取消报名时间
    readSignUpOrQuitTime();
    printSignUpOrQuitTime();

    __CATCH__
}

void OuterFactoryImp::loadMailNotifyConfig()
{
    __TRY__

    WriteLocker lock(m_rwlock);

    //邮件通知配置
    readMailNotifyConfig();
    printMailNotifyConfig();

    __CATCH__
}

//代理配置
void OuterFactoryImp::readPrxConfig()
{
    //配置服务
    _ConfigServantObj = (*_pFileConf).get("/Main/Interface/ConfigServer<ProxyObj>", "");
    _HallServantObj = (*_pFileConf).get("/Main/Interface/HallServer<ProxyObj>", "");
    _DBAgentServantObj = (*_pFileConf).get("/Main/Interface/DBAgentServer<ProxyObj>", "");
    _Log2DBServantObj = (*_pFileConf).get("/Main/Interface/Log2DBServer<ProxyObj>", "");
    _PushServantObj = (*_pFileConf).get("/Main/Interface/PushServer<ProxyObj>", "");
}

void OuterFactoryImp::printPrxConfig()
{
    FDLOG_CONFIG_INFO << "_ConfigServantObj ProxyObj : " << _ConfigServantObj << endl;
    FDLOG_CONFIG_INFO << "_HallServantObj ProxyObj :" << _HallServantObj << endl;
    FDLOG_CONFIG_INFO << "_DBAgentServantObj ProxyObj : " << _DBAgentServantObj << endl;
    FDLOG_CONFIG_INFO << "_Log2DBServantObj ProxyObj : " << _Log2DBServantObj << endl;
    FDLOG_CONFIG_INFO << "_PushServantObj ProxyObj : " << _PushServantObj << endl;
}

//比赛配置
void OuterFactoryImp::readAllMatchConfig()
{
    getConfigServantPrx()->ListAllMatchConfig(listAllMatchConfigResp);
}

void OuterFactoryImp::printAllMatchConfig()
{
    FDLOG_CONFIG_INFO << "listAllMatchConfigResp : " << printTars(listAllMatchConfigResp) << endl;
}

const ListAllMatchConfigResp &OuterFactoryImp::getAllMatchConfig()
{
    return listAllMatchConfigResp;
}

//玩家配置
void OuterFactoryImp::readPlayerInfoConfig()
{
    ShowPlayerCount = TC_Common::strto<int>((*_pFileConf).get("/Main/PlayerInfo<ShowCount>", "20"));
}

int OuterFactoryImp::showPlayerCount()
{
    return ShowPlayerCount;
}

void OuterFactoryImp::printPlayerInfoConfig()
{
    FDLOG_CONFIG_INFO << "ShowPlayerCount : " << ShowPlayerCount << endl;
}

//奖池分配粒度
void OuterFactoryImp::readRatioDividend()
{
    RatioDividend = TC_Common::strto<int>((*_pFileConf).get("/Main/Jackpot<RatioDividend>", "10000"));
}

void OuterFactoryImp::printRatioDividend()
{
    FDLOG_CONFIG_INFO << "RatioDividend : " << RatioDividend << endl;
}

int OuterFactoryImp::getRatioDividend()
{
    return RatioDividend;
}

//禁止报名、取消报名时间
void OuterFactoryImp::readSignUpOrQuitTime()
{
    SignUpOrQuitTime = TC_Common::strto<int>((*_pFileConf).get("/Main<SignUpOrQuitTime>", "10"));
}

void OuterFactoryImp::printSignUpOrQuitTime()
{
    FDLOG_CONFIG_INFO << "SignUpOrQuitTime : " << SignUpOrQuitTime << endl;
}

int OuterFactoryImp::getSignUpOrQuitTime()
{
    return SignUpOrQuitTime;
}

//邮件通知配置
void OuterFactoryImp::readMailNotifyConfig()
{
    mailNotifyConfig.Title = (*_pFileConf).get("/Main/MailNotify<Title>", "");
    mailNotifyConfig.Action = (*_pFileConf).get("/Main/MailNotify<Action>", "");
    mailNotifyConfig.Coin = (*_pFileConf).get("/Main/MailNotify<Coin>", "");
    mailNotifyConfig.Diamond = (*_pFileConf).get("/Main/MailNotify<Diamond>", "");
}

void OuterFactoryImp::printMailNotifyConfig()
{
    FDLOG_CONFIG_INFO << "MailNotifyConfig, Title : " << mailNotifyConfig.Title
                      << ", Action: " << mailNotifyConfig.Action
                      << ", Coin: " << mailNotifyConfig.Coin
                      << ", Diamond: " << mailNotifyConfig.Diamond
                      << endl;
}

const MailNotifyConfig &OuterFactoryImp::getMailNotifyConfig()
{
    return mailNotifyConfig;
}

//修改用户财富接口
int OuterFactoryImp::async2ModifyUAccount(const userinfo::ModifyUserAccountReq &req)
{
    //userinfo::UserInfoServantPrxCallbackPtr cb = new AsyncUserInfoCallback(req);
    //getUserInfoServantPrx()->async_modifyUserAccount(cb, req);

    return 0;
}

//通知道具服务发送奖励
int OuterFactoryImp::async2GoodsReward()
{
    return 0;
}

//邮件通知比赛奖励
int OuterFactoryImp::asyncMailNotifyReward(long uid, int rank, const string &roomName, const match::UserRewardGoods &rewardGoods)
{
    SysMsgReq req;
    req.uid = uid;
    req.msgType = E_MSG_DEFALUT;

    //XXX比赛中您的排名为XX名获得排名奖励XXX、XXX
    // string contents = mailNotifyConfig.Title + roomName + mailNotifyConfig.Action;
    string contents = roomName + mailNotifyConfig.Title + I2S(rank) + mailNotifyConfig.Action;
    for (auto it = rewardGoods.goods.begin(); it != rewardGoods.goods.end(); ++it)
    {
        //非第一个
        if (it != rewardGoods.goods.begin())
        {
            contents += "、";
        }

        //具体的奖励
        switch (it->propsID)
        {
        case 10000:
            contents += L2S(it->number) + mailNotifyConfig.Coin;
            break;
        case 20000:
            contents += L2S(it->number) + mailNotifyConfig.Diamond;
            break;
        default:
            break;
        }
    }

    req.contents.push_back(contents);
    getHallServantPrx(uid)->async_genSysMsg(NULL, req);
    return 0;
}

//日志入库
void OuterFactoryImp::asyncLog2DB(const int64_t uid, const DaqiGame::TLog2DBReq &req)
{
    getLog2DBServantPrx(uid)->async_log2db(NULL, req);
}

//游戏配置服务代理
const ConfigServantPrx OuterFactoryImp::getConfigServantPrx()
{
    if (!_ConfigServerPrx)
    {
        _ConfigServerPrx = Application::getCommunicator()->stringToProxy<ConfigServantPrx>(_ConfigServantObj);
        ROLLLOG_DEBUG << "Init _ConfigServantObj succ, _ConfigServantObj : " << _ConfigServantObj << endl;
    }

    return _ConfigServerPrx;
}

//广场服务代理
const HallServantPrx OuterFactoryImp::getHallServantPrx(const long uid)
{
    if (!_HallServerPrx)
    {
        _HallServerPrx = Application::getCommunicator()->stringToProxy<HallServantPrx>(_HallServantObj);
        ROLLLOG_DEBUG << "Init _HallServantObj succ, _HallServantObj :" << _HallServantObj << endl;
    }

    if (_HallServerPrx)
    {
        return _HallServerPrx->tars_hash(uid);
    }

    return NULL;
}

//广场服务代理
const HallServantPrx OuterFactoryImp::getHallServantPrx(const string key)
{
    if (!_HallServerPrx)
    {
        _HallServerPrx = Application::getCommunicator()->stringToProxy<HallServantPrx>(_HallServantObj);
        ROLLLOG_DEBUG << "Init _HallServantObj succ, _HallServantObj :" << _HallServantObj << endl;
    }

    if (_HallServerPrx)
    {
        return _HallServerPrx->tars_hash(tars::hash<string>()(key));
    }

    return NULL;
}

//数据库代理服务代理
const DBAgentServantPrx OuterFactoryImp::getDBAgentServantPrx(const int64_t uid)
{
    if (!_DBAgentServerPrx)
    {
        _DBAgentServerPrx = Application::getCommunicator()->stringToProxy<dbagent::DBAgentServantPrx>(_DBAgentServantObj);
        ROLLLOG_DEBUG << "Init _DBAgentServantObj succ, _DBAgentServantObj : " << _DBAgentServantObj << endl;
    }

    if (_DBAgentServerPrx)
    {
        return _DBAgentServerPrx->tars_hash(uid);
    }

    return NULL;
}

//数据库代理服务代理
const DBAgentServantPrx OuterFactoryImp::getDBAgentServantPrx(const string &key)
{
    if (!_DBAgentServerPrx)
    {
        _DBAgentServerPrx = Application::getCommunicator()->stringToProxy<dbagent::DBAgentServantPrx>(_DBAgentServantObj);
        ROLLLOG_DEBUG << "Init _DBAgentServantObj succ, _DBAgentServantObj : " << _DBAgentServantObj << endl;
    }

    if (_DBAgentServerPrx)
    {
        return _DBAgentServerPrx->tars_hash(tars::hash<string>()(key));
    }

    return NULL;
}

//日志入库服务代理
const DaqiGame::Log2DBServantPrx OuterFactoryImp::getLog2DBServantPrx(const int64_t uid)
{
    if (!_Log2DBServerPrx)
    {
        _Log2DBServerPrx = Application::getCommunicator()->stringToProxy<DaqiGame::Log2DBServantPrx>(_Log2DBServantObj);
        ROLLLOG_DEBUG << "Init _Log2DBServantObj succ, _Log2DBServantObj : " << _Log2DBServantObj << endl;
    }

    if (_Log2DBServerPrx)
    {
        return _Log2DBServerPrx->tars_hash(uid);
    }

    return NULL;
}

//获取托送服代理
const PushServantPrx OuterFactoryImp::getPushServantPrx(const int64_t uid)
{
    if (!_PushServerPrx)
    {
        _PushServerPrx = Application::getCommunicator()->stringToProxy<PushServantPrx>(_PushServantObj);
        ROLLLOG_DEBUG << "Init _PushServantObj succ, _PushServantObj : " << _PushServantObj << endl;
    }

    if (_PushServerPrx)
    {
        return _PushServerPrx->tars_hash(uid);
    }

    return NULL;
}

//格式化时间
string OuterFactoryImp::GetTimeFormat()
{
    string sFormat("%Y-%m-%d %H:%M:%S");
    time_t t = TNOW;
    auto pTm = localtime(&t);
    if (pTm == NULL)
        return "";

    char sTimeString[255] = "\0";
    strftime(sTimeString, sizeof(sTimeString), sFormat.c_str(), pTm);
    return string(sTimeString);
}

//拆分字符串成整形
int OuterFactoryImp::splitInt(string szSrc, vector<int> &vecInt)
{
    split_int(szSrc, "[ \t]*\\|[ \t]*", vecInt);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////


