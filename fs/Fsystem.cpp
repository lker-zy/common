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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <regex.h>

#include <fstream>
#include <string>
#include <vector>

#include "Fsystem.h"


RegxFilter::RegxFilter (const char* ex_file)
	: ex_file_ (ex_file)
{
	std::ifstream read_in;
	std::string pattern;
	std::vector<std::string> pattern_vec;

	read_in.open(ex_file_);
	while(read_in >> pattern) {
		pattern_vec.push_back(pattern);
	}
	int pattern_num = pattern_vec.size();
	std::vector<std::string>::iterator pattern_iter = pattern_vec.begin();
	exclude_list_ = (const char**)malloc(sizeof(char*) * (pattern_num + 1));
	for (int i = 0;pattern_iter != pattern_vec.end(); ++pattern_iter, i++) {
		exclude_list_[i] = pattern_iter->c_str();
	}
	exclude_list_[pattern_num] = NULL;

	regex_t** regexp = (regex_t**)malloc(sizeof(regex_t*) * 10);
	int ret = 0;

	int i = 0;
	for (i = 0; exclude_list_[i] != NULL; i++) {
		regexp[i] = (regex_t*) malloc(sizeof(regex_t));
		ret = regcomp(regexp[i], exclude_list_[i], REG_EXTENDED|REG_NOSUB);
		if (ret != 0) {
			size_t length = regerror (ret, regexp[i], NULL, 0);
			char * buffer = (char*)malloc(length);
			(void) regerror (ret, regexp[i], buffer, length);
		}
	}

	exclude_num_ = i;
	regexp_ = regexp;
	
}

bool RegxFilter::operator() (const char* path) {
	for (int i = 0; i < exclude_num_; i++) {
		regex_t * reg = *(regexp_ + i);
		int match = regexec(reg, path, 0, NULL, 0);
		if (match == REG_NOMATCH) {
			continue;
		} else {
			return true;
		}
	} 
	return false;
}

void Fsystem::walk() {
	sleep(3);
	this->walkdir(basedir_);
	travel_cb_.done_notify();
	sleep(10);
}

void Fsystem::walkdir(const char * pathname) {
	int32_t max_path_size = 1024;
	char absolute_path[1024] = {'\0'};
	DIR* dirp = opendir(pathname);
	struct dirent* p_dirent;
	struct dirent dirent;
	if (NULL == dirp) {
		char* err_msg = NULL;
		if (errno == EACCES) {
			err_msg = "no permission";
		} else if (errno == ENFILE) {
			err_msg = "too many files open";
		} else if (errno == EMFILE) {
			err_msg = "too many fd open";
		} else if (errno == ENOMEM) {
			err_msg = "too many dirs open";
		} else {
			err_msg = "Unknown";
		}
		exit(-1);
	}

	int ret = chdir(pathname);
	if (ret != 0) {
		exit(-1);
	}

	while (0 == readdir_r(dirp, &dirent, &p_dirent)) {
		if (NULL == p_dirent) {
			break;
		}
		char * d_name = dirent.d_name;
		if (!strcmp(d_name, "..") || !strcmp(d_name, ".")) {
			continue;
		}

		struct stat * st = (struct stat *)malloc(sizeof(struct stat));;
		// todo: check malloc
		int status = lstat(d_name, st);
		if (status == -1){
			exit(-1);
		}else{
			int path_len = snprintf(absolute_path, max_path_size, "%s/%s", pathname, d_name);
			absolute_path[path_len] = '\0';

			// implantation by function objection
			if (filter_ && filter_(absolute_path))
				continue;

			if(S_ISDIR(st->st_mode)){
				this->walkdir(absolute_path);
			}else
			if (S_ISREG(st->st_mode)) {
				travel_cb_.file_handler(absolute_path, st);
			}else
			if (S_ISLNK(st->st_mode)) {
			//	check_queue_->insert(absolute_path, st);
				travel_cb_.symlink_handler(absolute_path, st);
			}else{
			}
		}
		absolute_path[0] = '\0';
	}
	closedir(dirp);
	ret	= chdir("../");
	if(ret != 0){
		exit(-1);
	}
}

bool FileHandler::operator() (const char* path) {
	struct stat* st = NULL;
	check_queue_->insert(const_cast<char*>(path), st);
	return true;
}

bool FileHandler::file_handler (const char* path, struct stat* st) {
	check_queue_->insert(const_cast<char*>(path), st);
	return true;
}

int main(int argc, char **argv) {
		filesystem_ = new Fsystem(basedir);
		FileHandler* handlerobj = new FileHandler();
		Fsystem::TraversalCallback travel_cb;
		Fsystem::HandlerFuncType handler (boost::bind(&FileHandler::file_handler, handlerobj, _1, _2));
		travel_cb.dir_handler = handler;
		travel_cb.symlink_handler = handler;
		travel_cb.file_handler = handler;
		boost::function<bool(void)> done_notify(boost::bind(&FileHandler::done_notify, handlerobj));
		travel_cb.done_notify = done_notify;

		filesystem_->set_filter();
		filesystem_->register_travel_callback(travel_cb);
}
