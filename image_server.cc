#include <signal.h>
#include <jsoncpp/json/json.h>
#include "image_server.hpp"
#include "httplib.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <openssl/md5.h>
#include <sys/time.h>

const std::string base_path = "./image_pic/";

MYSQL* mysql = NULL;

namespace File
{
	bool Read(string path, string& body, int t)
	{
		FILE* fp;

		if ((fp = fopen(path.c_str(), "rb")) == NULL)
		{
			cout << "Open image failed!" << endl;
			exit(0);
		}

		//根据图像数据长度分配内存buffer
		body.resize(t);

		//将图像数据读入buffer
		fread((char*)body.c_str(), t, 1, fp);

		fclose(fp);
	}
}

std::string StringMD5(const std::string& str)
{
	const int MD5LENTH = 16;
	unsigned char MD5result[MD5LENTH];

	// 调用 openssl 的函数计算 md5
	MD5((const unsigned char*)str.c_str(), str.size(), MD5result);

	// 转换成字符串的形式方便存储和观察
	char output[1024] = { 0 };
	int offset = 0;

	for (int i = 0; i < MD5LENTH; ++i)
	{
		offset += sprintf(output + offset, "%x", MD5result[i]);
	}

	return std::string(output);
}

void GetNowTime(string& Time)
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
}

int main()
{
	using namespace httplib;

	Server server;

	// 1. 数据库客户端初始化和释放
	mysql = MySQLInit();

	signal(SIGINT, [](int)
	{
		MySQLRelease(mysql);
		exit(0);
	});

	image_table it(mysql);

	// 2. 新增图片.
	server.Post("/image", [&it](const Request& req, Response& resp)
	{
		Json::Value resp_json;
		Json::FastWriter fw;

		cout << "上传图片" << endl;

		auto size = req.files.size();
		auto ret = req.has_file("filename");

		if (!ret)
		{
			resp_json["ok"] = false;
			resp_json["reason"] = "上传文件错误!";
			resp.status = 400;
			resp.set_content(fw.write(resp_json), "application/json");
			return;
		}

		const auto& file = req.get_file_value("filename");
        auto body = req.body.substr(file.offset, file.length);
		// file.filename;
		// file.content_type;

		Json::Value image;

		string Time;
		GetNowTime(Time);

		image["image_name"] = file.filename;
		image["image_size"] = (int)file.length;
		image["upload_time"] = Time;
		image["md5"] = StringMD5(body);
		image["image_type"] = file.content_type;
		image["image_path"] = base_path + file.filename;

		//插入数据库
        if(file.filename=="")
        {
            cout<<"上传文件错误,未添加图片"<<endl;
            resp_json["ok"] = false;
			resp_json["reason"] = "上传文件错误,请添加图片";
			resp.status = 400;
			resp.set_content(fw.write(resp_json), "application/json");
            return;
        }

		ret = it.Insert(image);

		if(!ret)
		{
		     resp_json["ok"] = false;
		     resp_json["reason"] = "插入数据库失败!";
		     resp.status = 500;
		     resp.set_content(fw.write(resp_json), "application/json");
		     return;
		}

		//保存文件到磁盘
		ofstream outfile;
		outfile.open(image["image_path"].asCString());

		outfile << body;

		cout << "保存成功" << endl;

		//构造响应
		resp_json["ok"] = true;
		resp.set_content(fw.write(resp_json), "text/html");

		cout << "响应完毕" << endl;
        cout << endl;
	});

	// 3. 查看所有图片的元信息
	server.Get("/image", [&it](const Request& req, Response& resp)
	{
		cout << "查看所有图片信息" << endl;

		// 没有任何实际的效果
		(void)req; 

		Json::Value resp_json;
		Json::FastWriter fw;

		//1.调用数据库接口来获取数据
		Json::Value images;

		bool ret = it.SelectAll(&images);

		if (!ret)
		{
			resp_json["ok"] = false;
			resp_json["reason"] = "查找失败!\n";
			resp.status = 500;
			resp.set_content(fw.write(resp_json), "application/json");
			return;
		}

		//2.构造响应结果返回客户端
		resp.set_content(fw.write(images), "application/json");

		cout << "响应完毕" << endl;
        cout << endl;
		return;
	});

	// 4. 查看图片信息
	server.Get(R"(/image/(\d+))", [&it](const Request& req, Response& resp)
	{
		cout << "查看指定图片信息" << endl;

		Json::Value resp_json;
		Json::FastWriter fw;

		//1.先获取到图片ID
		int image_id = std::stoi(req.matches[1]);

		//2.根据ID查询数据库
		Json::Value image;

		bool ret = it.SelectOne(image_id, &image);

		if (!ret)
		{
			resp_json["ok"] = false;
			resp_json["reason"] = "SelectOne failed!\n";
			resp.status = 500;
			resp.set_content(fw.write(resp_json), "application/json");
			return;
		}

		//3.查询结果返回给客户端
		resp_json["ok"] = true;
		resp.set_content(fw.write(image), "application/json");

		cout << "响应完毕" << endl;
        cout << endl;
		return;
	});

	// 5. 查看图片内容
	server.Get(R"(/image/show/(\d+))", [&it](const Request& req, Response& resp)
	{
		cout << "查看图片内容" << endl;

		Json::Value resp_json;
		Json::FastWriter fw;

		//1.先获取到图片ID
		int image_id = std::stoi(req.matches[1]);

		//2.找到数据库对应的目录
		Json::Value image;

		bool ret = it.SelectOne(image_id, &image);

		if (!ret)
		{
			resp_json["ok"] = false;
			resp_json["reason"] = "SelectOne failed!\n";
			resp.status = 404;
			resp.set_content(fw.write(resp_json), "application/json");
			return;
		}

		//3.根据路径找到文件内容,读取
		int t = stoi(image["image_size"].asString());

		string image_body;

		ret = File::Read(image["image_path"].asString(), image_body, t);

		if (!ret)
		{
			resp_json["ok"] = false;
			resp_json["reason"] = "读取图片内容失败!\n";
			resp.status = 500;
			resp.set_content(fw.write(resp_json), "application/json");
			return;
		}

		cout << "读取完毕" << endl;

		resp.set_content(image_body, image["image_type"].asCString());
		cout << "响应完毕" << endl;

        cout << endl;
		return;
	});

      server.Delete(R"(/image/(\d+))", [&it](const Request& req, Response& resp) 
      {
      Json::FastWriter writer;
      Json::Value resp_json;

      // 1. 根据图片 id 去数据库中查到对应的目录
      int image_id = std::stoi(req.matches[1]);

      cout<<"删除ID为:"<<image_id<<"的图片"<<endl;

      // 2. 查找到对应文件的路径
      Json::Value image;

      bool ret = it.SelectOne(image_id, &image);

      if (!ret) 
      {
        cout<<"删除图片失败!"<<endl;

        resp_json["ok"] = false;
        resp_json["reason"] = "删除图片失败";
        resp.status = 404;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
      }

      // 3. 数据库进行删除操作
      ret = it.Delete(image_id);

      if (!ret) 
      {
        cout<<"删除图片失败!"<<endl;
        resp_json["ok"] = false;
        resp_json["reason"] = "删除图片失败";
        resp.status = 404;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
      }

      // 4. 删除磁盘上的文件
      unlink(image["image_path"].asCString());
      
      // 5. 构造响应
      resp_json["ok"] = true;
      resp.status = 200;
      resp.set_content(writer.write(resp_json), "application/json");
    });

	server.set_base_dir("./wwwroot");

	server.listen("0.0.0.0", 9094);

	return 0;
}
