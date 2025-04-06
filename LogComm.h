#ifndef __LOGCOMM_H__
#define __LOGCOMM_H__

//
#include <util/tc_logger.h>
#include "servant/RemoteLogger.h"

//
using namespace tars;

//
#define ROLLLOG(level) (LOG->level() << "[" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] ")
#define ROLLLOG_DEBUG (ROLLLOG(debug))
#define ROLLLOG_INFO (ROLLLOG(info))
#define ROLLLOG_WARN (ROLLLOG(warn))
#define ROLLLOG_ERROR (ROLLLOG(error))

#define FUNC_ENTRY(in) (ROLLLOG(debug) << ">>>> Enter " << __FUNCTION__ << "() in(" << in << ")" << endl)
#define FUNC_EXIT(out, ret) (ROLLLOG(debug) << "<<<< Exit " << __FUNCTION__ << "() out[" << out << "], ret = " << ret << endl)

#define FDLOG_ERROR (FDLOG("error") << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|")
#define FDLOG_EXCEPT (FDLOG("except") << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|")

//配置信息
#define FDLOG_CONFIG_INFO (FDLOG("config_info") << "|")

//牌局报名记录日志
#define MATCH_ENTER_LOG_TOPIC "match_enter_log"
#define FDLOG_MATCH_ENTER_LOG (FDLOG("match_enter_log") << "|")

//比赛排名奖励
#define MATCH_RANK_LOG_TOPIC "match_rank_log"
#define FDLOG_MATCH_RANK_LOG (FDLOG("match_rank_log") << "|")

#define FDLOG_INIT_FORMAT(x,y,z) (TarsTimeLogger::getInstance()->initFormatWithType<LogByMinute>(x,y,z))
#define FDLOG_MATCH_ENTER_LOG_FORMAT (FDLOG_INIT_FORMAT("match_enter_log", "%Y%m%d%H%M", 5))
#define FDLOG_MATCH_RANK_LOG_FORMAT (FDLOG_INIT_FORMAT("match_rank_log", "%Y%m%d%H%M", 5))

#define COST_MS 100

#endif

