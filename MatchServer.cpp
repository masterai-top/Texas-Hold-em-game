#include "MatchServer.h"
#include "MatchServantImp.h"
#include "LogComm.h"

//
using namespace std;

//
MatchServer g_app;

/////////////////////////////////////////////////////////////////
void
MatchServer::initialize()
{
    //initialize application here:
    //...

    ///
    addServant<MatchServantImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".MatchServantObj");

    //初始化外部接口对象
    initOuterFactory();

    // 注册动态加载命令
    TARS_ADD_ADMIN_CMD_NORMAL("reload", MatchServer::reloadSvrConfig);

    // 加载比赛配置
    TARS_ADD_ADMIN_CMD_NORMAL("matchconfig", MatchServer::reloadMatchConfig);

    // 清理比赛场
    TARS_ADD_ADMIN_CMD_NORMAL("cleanmatchgame", MatchServer::cleanMatchGame);
}

/////////////////////////////////////////////////////////////////
void
MatchServer::destroyApp()
{
    //destroy application here:
    //...
}

/*
* 配置变更，重新加载配置
*/
bool MatchServer::reloadSvrConfig(const string &command, const string &params, string &result)
{
    try
    {
        //加载配置
        getOuterFactoryPtr()->load();

        result = "reload server config success.";

        LOG_DEBUG << "reloadSvrConfig: " << result << endl;

        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception.";
    }

    result += "\n fail, please check it.";

    LOG_DEBUG << "reloadSvrConfig: " << result << endl;

    return true;
}

/**
 * 加载比赛场配置
*/
bool MatchServer::reloadMatchConfig(const string &command, const string &params, string &result)
{
    try
    {
        getOuterFactoryPtr()->loadMatchConfig();

        result = "reload match config success.";

        LOG_DEBUG << "reloadMatchConfig: " << result << endl;

        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception.";
    }

    result += "\n fail, please check it.";

    LOG_DEBUG << "reloadMatchConfig: " << result << endl;

    return true;
}

/**
 * 清理某比赛场
*/
bool MatchServer::cleanMatchGame(const string &command, const string &params, string &result)
{
    try
    {
        result = "clean match game success.";

        LOG_DEBUG << "cleanMatchGame: " << result << endl;

        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception.";
    }

    result += "\n fail, please check it.";

    LOG_DEBUG << "cleanMatchGame: " << result << endl;

    return true;
}

/**
* 初始化外部接口对象
**/
int MatchServer::initOuterFactory()
{
    _pOuter = new OuterFactoryImp();
    return 0;
}

/////////////////////////////////////////////////////////////////
int
main(int argc, char *argv[])
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch (std::exception &e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }

    return -1;
}
/////////////////////////////////////////////////////////////////
