#pragma once

#include <mysql/mysql.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <sys/time.h>
#include <fstream>

using namespace std;

string NowTime="";
string logData="";

string GetNowTime(string Time)
{
    //方法一:

    // time_t timer;//time_t就是long int 类型
    // struct tm *tblock;
    // timer = time(NULL);
    // tblock = localtime(&timer);

    // Time=asctime(tblock);

    //方法二:
    struct timeval tv;
    char mytime[20] = "";

    gettimeofday(&tv, NULL);
    strftime(mytime, sizeof(mytime), "%Y-%m-%d %T", localtime(&tv.tv_sec));

    Time = mytime;
    return Time;
}

void SaveLogText(string data)
{
    ofstream logText("logText.txt", ios::app);

    logText<<data<<endl<<endl;

    logText.close();
}

static MYSQL* MySQLInit()
{
    MYSQL* mysql = mysql_init(NULL);

    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    //mysql_set_character_set(mysql,"utf8");

    if(mysql_real_connect(mysql, "localhost", "luzihan", "124152", "image", 3306, NULL, 0)==NULL)
    {
		logData = GetNowTime(NowTime).c_str();
		logData  += " 数据库连接失败";
		cout << logData <<endl<<endl;
		SaveLogText(logData);
		exit(0);
    }

    logData = GetNowTime(NowTime).c_str();
    logData += " 数据库连接成功";
    cout << logData << endl <<endl;
    SaveLogText(logData);
    return mysql;
}

static void MySQLRelease(MYSQL* mysql)
{
    mysql_close(mysql);
}

class image_table
{
    public:
	image_table(MYSQL* mysql) :_mysql(mysql)
    {

    }

	bool Insert(const Json::Value& image)
	{

	    char sql[4 * 1024] = { 0 };

	    sprintf(sql, "select * from image_table");

	    int ret = mysql_query(_mysql,sql);

	    if(ret!=0)
	    {
			cout<<mysql_error(_mysql)<<endl;
			logData = GetNowTime(NowTime).c_str();
			logData += " 查询失败";
			cout<<logData<<endl;
			SaveLogText(logData);
			return false;
	    }

	    MYSQL_RES* result = mysql_store_result(_mysql);

	    int lineno=mysql_num_rows(result);

	    char sql1[4*1024]={0};

	    sprintf(sql1, "insert into image_table values(%d,'%s',%d,'%s','%s','%s','%s');", lineno+1,
		    image["image_name"].asCString(),image["image_size"].asInt(), 
		    image["upload_time"].asCString(), image["image_type"].asCString(),
		    image["image_path"].asCString(), image["md5"].asCString());

	    logData = GetNowTime(NowTime).c_str();
	    logData += " 开始插入数据库";
	    cout<<logData<<endl<<endl;
	    SaveLogText(logData);

	    ret = mysql_query(_mysql, sql1);

	    if(ret!=0)
	    {
			cout<<mysql_error(_mysql)<<endl;
			logData = GetNowTime(NowTime).c_str();
			logData += " 插入数据库失败";
			cout<<logData<<endl<<endl;
			SaveLogText(logData);
			return false;
	    }

	    logData = GetNowTime(NowTime).c_str();
	    logData += " 插入数据库成功";
	    cout<<logData<<endl<<endl;
	    SaveLogText(logData);
	    return true;
	}

	bool SelectAll(Json::Value* images)
	{
	    char sql[4 * 1024] = { 0 };

	    sprintf(sql, "select * from image_table;");

	    int ret = mysql_query(_mysql, sql);

	    if (ret != 0)
	    {
			cout << mysql_error(_mysql) << endl;
			logData = GetNowTime(NowTime).c_str();
			logData += " 查找失败";
			cout<<logData<<endl<<endl;
			SaveLogText(logData);
			return false;
	    }

	    //遍历结果集
	    MYSQL_RES* mysqlRes = mysql_store_result(_mysql);

	    if (mysqlRes == NULL)
	    {
			cout << mysql_error(_mysql) << endl;
			return false;
	    }

	    int itemCount = mysql_num_fields(mysqlRes);

	    MYSQL_ROW mysqlRow;

	    while (mysqlRow = mysql_fetch_row(mysqlRes))
	    {
			//把数据库的每条记录转成json格式
			Json::Value image;
			image["image_id"] = atoi(mysqlRow[0]);
			image["image_name"] = mysqlRow[1];
			image["image_size"] = atoi(mysqlRow[2]);
			image["upload_time"] = mysqlRow[3];
			image["image_type"] = mysqlRow[4];
			image["image_path"] = mysqlRow[5];
			image["md5"] = mysqlRow[6];

			images->append(image);
	    }

	    logData = GetNowTime(NowTime).c_str();
	    logData += " 查找成功";
	    cout<<logData<<endl<<endl;
	    SaveLogText(logData);

	    mysql_free_result(mysqlRes);

	    return true;
	}

	bool SelectOne(int image_id, Json::Value* imageOne)
	{
	    char sql[4 * 1024];

	    sprintf(sql, "select * from image_table where image_id= %d;", image_id);

	    int ret = mysql_query(_mysql, sql);

	    if (ret != 0)
	    {
			logData = GetNowTime(NowTime).c_str();
			logData += " 查找失败";
			cout<<logData<<endl<<endl;
			SaveLogText(logData);
			cout << mysql_error(_mysql) << endl;
			return false;
	    }

	    //遍历结果集合
	    MYSQL_RES* mysqlRes = mysql_store_result(_mysql);

	    int Rows = mysql_num_rows(mysqlRes);

	    if (Rows != 1)
	    {
			logData = GetNowTime(NowTime).c_str();
			logData += " 不止一条信息";
			cout<<logData<<endl<<endl;
			SaveLogText(logData);
			return false;
	    }

	    MYSQL_ROW mysqlRow;

	    //int itemCount = mysql_num_fields(mysqlRes);

	    while (mysqlRow = mysql_fetch_row(mysqlRes))
	    {
			//把数据库的每条记录转成json格式
			Json::Value image;
			image["image_id"] = atoi(mysqlRow[0]);
			image["image_name"] = mysqlRow[1];
			image["image_size"] = atoi(mysqlRow[2]);
			image["upload_time"] = mysqlRow[3];
			image["image_type"] = mysqlRow[4];
			image["image_path"] = mysqlRow[5];
			image["md5"] = mysqlRow[6];

			*imageOne = image;
	    }

	    logData = GetNowTime(NowTime).c_str();
	    logData += " 查找成功";
	    cout<<logData<<endl<<endl;
	    SaveLogText(logData);

	    mysql_free_result(mysqlRes);

	    return true;

	}

	bool Delete(int image_id)
	{
	    char sql[4 * 1024] = { 0 };

	    sprintf(sql, "delete from image_table where image_id= %d;", image_id);

	    int ret = mysql_query(_mysql, sql);

	    if (ret != 0)
	    {
			logData = GetNowTime(NowTime).c_str();
			logData += " 删除失败";
			cout<<logData<<endl<<endl;
			SaveLogText(logData);
			cout << mysql_error(_mysql) << endl;
			return false;
	    }

	    logData = GetNowTime(NowTime).c_str();
	    logData += " 删除成功";
	    cout<<logData<<endl<<endl;
	    SaveLogText(logData);
	    return true;
	}

    private:
	MYSQL* _mysql;
};

