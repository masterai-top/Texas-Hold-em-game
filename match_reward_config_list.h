#ifndef __MATCH_REWARD_CONFIG_LIST_H__
#define __MATCH_REWARD_CONFIG_LIST_H__
class MatchRewardConfigListRequest
{
public:
	MatchRewardConfigListRequest(const string& json) {
		this->Deserialize(json);
	}
	template <typename Writer>
	void Serialize(Writer& writer) const {
		writer.StartObject();
		SERIALIZE_MEMBER(writer,id);
		SERIALIZE_MEMBER(writer,startDate);
		SERIALIZE_MEMBER(writer,endDate);

		writer.EndObject();
	}

	void toString(std::string& json) {
		StringBuffer sb;
		Writer<StringBuffer> writer(sb);
		Serialize(writer);
		json = sb.GetString();
	}

	void Deserialize(const string& json)
	{
		Document d;
		if (d.Parse(json.c_str()).HasParseError()){
			throw logic_error("parse json error. raw data : " + json);
		}
		SET_DOC_MEMBER(d,id);
		SET_DOC_MEMBER(d,startDate);
		SET_DOC_MEMBER(d,endDate);

	}

	static tars::Int32 handler(const vector<tars::Char>& reqBuf, const map<std::string, std::string>& extraInfo, vector<tars::Char>& rspBuf)
	{
	    return 0;
	}
private:
	CString        	_id             ;  //主键id
	CString        	_startDate      ;  //开始时间
	CString        	_endDate        ;  //结束时间

};
class MatchRewardConfigListResponse
{
public:
	MatchRewardConfigListResponse(const string& json) {
		this->Deserialize(json);
	}
	template <typename Writer>
	void Serialize(Writer& writer) const {
		writer.StartObject();
		SERIALIZE_MEMBER(writer,id);
		SERIALIZE_MEMBER(writer,rewardQuota);
		SERIALIZE_MEMBER(writer,coinReward);
		SERIALIZE_MEMBER(writer,jackpotReward);
		SERIALIZE_MEMBER(writer,propsReward);
		SERIALIZE_MEMBER(writer,value);
		SERIALIZE_MEMBER(writer,optUser);
		SERIALIZE_MEMBER(writer,createDate);

		writer.EndObject();
	}

	void toString(std::string& json) {
		StringBuffer sb;
		Writer<StringBuffer> writer(sb);
		Serialize(writer);
		json = sb.GetString();
	}

	void Deserialize(const string& json)
	{
		Document d;
		if (d.Parse(json.c_str()).HasParseError()){
			throw logic_error("parse json error. raw data : " + json);
		}
		SET_DOC_MEMBER(d,id);
		SET_DOC_MEMBER(d,rewardQuota);
		SET_DOC_MEMBER(d,coinReward);
		SET_DOC_MEMBER(d,jackpotReward);
		SET_DOC_MEMBER(d,propsReward);
		SET_DOC_MEMBER(d,value);
		SET_DOC_MEMBER(d,optUser);
		SET_DOC_MEMBER(d,createDate);

	}

	static tars::Int32 handler(const vector<tars::Char>& reqBuf, const map<std::string, std::string>& extraInfo, vector<tars::Char>& rspBuf)
	{
	    return 0;
	}
private:
	CInteger       	_id             ;  //主键
	CString        	_rewardQuota    ;  //奖励名额,格式：（参赛最小人数 - 参赛最大人数：奖励数，多个用'&#124;'隔开）
	CInteger       	_coinReward     ;  //金币奖励
	CInteger       	_jackpotReward  ;  //百分比奖池奖励 ,例：10%
	CString        	_propsReward    ;  //道具奖励,格式：（道具1：数量，多个用'&#124;'隔开）
	CString        	_value          ;  //道具价值换算（1钻石=10金币）
	CString        	_optUser        ;  //操作者
	CString        	_createDate     ;  //创建时间

};
#endif