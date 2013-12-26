/***************************************************************************
 *
 * Copyright (c) 2013 lker-zy. All Rights Reserved
 * $Id$
 *
 **************************************************************************/
/**
 * @file fsystem.cpp
 * @author lker-zy(sdu2007gj@gmail.com)
 * @date 2013/05/23 :19:24
 * @version $Revision$
 * @brief
 *
 **/

#ifndef __FSYSTEM_H_
#define __FSYSTEM_H_

#include <regex.h>
#include <boost/function.hpp>


class FileQueue;

class RegxFilter {
public:
	RegxFilter (const char* ex_file);

	~RegxFilter () {
	
	}

	bool operator() (const char* path);

private:
	const char* ex_file_;
	int exclude_num_;
	regex_t ** regexp_; 
	const char ** exclude_list_;
};

/**
 * @brief Fsystem class
 *
 **/
class Fsystem{
public:
	typedef boost::function<bool(const char*)> FilterFuncType;
	typedef boost::function<bool(const char*, struct stat*)> HandlerFuncType;
	typedef boost::function<bool(void)> bvFuncType;
	struct TraversalCallback {
		HandlerFuncType dir_handler, file_handler, symlink_handler;
		bvFuncType done_notify;
	};
public:
	Fsystem(const char* basedir);
	~Fsystem() {}

	void walk(); /**<遍历文件系统 */ 
	void register_travel_callback(TraversalCallback cb) {
		travel_cb_ = cb;
	}


	void set_filter() {
	//	filter_ = FilterFuncType(RegxFilter("./exclude.list"));
	}

private:
	void walkdir(const char * pathname);

	const char * basedir_;

	FilterFuncType filter_;
	TraversalCallback travel_cb_;
};

class FileHandler {
public:
	FileHandler() 
	{
	
	}

	~FileHandler() {
	
	}

	bool operator() (const char*);
	bool operator() () {
		check_queue_->end();
		return true;
	}

	bool file_handler(const char* path, struct stat*);

	bool done_notify() {
		return true;
	};

private:
	const char * basedir_;
};
#endif //__FSYSTEM_H_
