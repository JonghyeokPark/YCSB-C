//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <atomic>
#include <cstring>
#include <string.h>
#include <iostream>
#include <vector>
#include <future>
#include "utils.h"
#include "timer.h"
#include "client.h"
#include "core_workload.h"
#include "db_factory.h"

using namespace std;


// stat
std::atomic<uint64_t> ops_cnt[ycsbc::Operation::READMODIFYWRITE + 1];
std::atomic<uint64_t> ops_time[ycsbc::Operation::READMODIFYWRITE + 1];

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, const char *argv[], utils::Properties &props);

int DelegateClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl, const int num_ops,
    bool is_loading) {
  db->Init();
  ycsbc::Client client(*db, *wl);
  int oks = 0;
  for (int i = 0; i < num_ops; ++i) {
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      oks += client.DoTransaction();
    }
  }
  db->Close();
  return oks;
}

int main(const int argc, const char *argv[]) {
  utils::Properties props;
  string file_name = ParseCommandLine(argc, argv, props);

  ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props);
  if (!db) {
    cout << "Unknown database name " << props["dbname"] << endl;
    exit(0);
  }

  ycsbc::CoreWorkload wl;
  wl.Init(props);

  const int num_threads = stoi(props.GetProperty("threadcount", "1"));
  // rocksdb
  //const bool load = utils::StrToBool(props.GetProperty("load","false"));
  //const bool run = utils::StrToBool(props.GetProperty("run","false"));
  //const bool print_stats = utils::StrToBool(props["dbstatistics"]);
  //const bool wait_for_balance = utils::StrToBool(props["dbwaitforbalance"]);

  // Loads data
  vector<future<int>> actual_ops;
  int total_ops = stoi(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);
  for (int i = 0; i < num_threads; ++i) {
    actual_ops.emplace_back(async(launch::async,
        DelegateClient, db, &wl, total_ops / num_threads, true));
  }
  assert((int)actual_ops.size() == num_threads);

  int sum = 0;
  for (auto &n : actual_ops) {
    assert(n.valid());
    sum += n.get();
  }
  cerr << "# Loading records:\t" << sum << endl;

  // Peforms transactions
  // stats
  for(int j = 0; j < ycsbc::Operation::READMODIFYWRITE + 1; j++){
	ops_cnt[j].store(0);
	ops_time[j].store(0);
  }

  actual_ops.clear();
  uint64_t run_start = get_now_micros();
  total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
  utils::Timer<double> timer;
  timer.Start();
  for (int i = 0; i < num_threads; ++i) {
    actual_ops.emplace_back(async(launch::async,
        DelegateClient, db, &wl, total_ops / num_threads, false));
  }
  assert((int)actual_ops.size() == num_threads);

  sum = 0;
  for (auto &n : actual_ops) {
    assert(n.valid());
    sum += n.get();
  }

	uint64_t run_end = get_now_micros();
	uint64_t use_time = run_end - run_start;
	uint64_t temp_cnt[ycsbc::Operation::READMODIFYWRITE + 1];
	uint64_t temp_time[ycsbc::Operation::READMODIFYWRITE + 1];

	for(int j = 0; j < ycsbc::Operation::READMODIFYWRITE + 1; j++){
      temp_cnt[j] = ops_cnt[j].load(std::memory_order_relaxed);
      temp_time[j] = ops_time[j].load(std::memory_order_relaxed);
    }

    printf("********** run result **********\n");
    printf("all opeartion records:%d  use time:%.3f s  IOPS:%.2f iops (%.2f us/op)\n\n"
		, sum, 1.0 * use_time*1e-6, 1.0 * sum * 1e6 / use_time, 1.0 * use_time / sum);
 
    if ( temp_cnt[ycsbc::INSERT] ) {
	 printf("insert ops:%7lu  use time:%7.3f s  IOPS:%7.2f iops (%.2f us/op)\n"
		, temp_cnt[ycsbc::INSERT], 1.0 * temp_time[ycsbc::INSERT]*1e-6
		, 1.0 * temp_cnt[ycsbc::INSERT] * 1e6 / temp_time[ycsbc::INSERT]
		, 1.0 * temp_time[ycsbc::INSERT] / temp_cnt[ycsbc::INSERT]);
	}
    if ( temp_cnt[ycsbc::READ] ) { 
	 printf("read ops  :%7lu  use time:%7.3f s  IOPS:%7.2f iops (%.2f us/op)\n"
		, temp_cnt[ycsbc::READ], 1.0 * temp_time[ycsbc::READ]*1e-6
		, 1.0 * temp_cnt[ycsbc::READ] * 1e6 / temp_time[ycsbc::READ]
		, 1.0 * temp_time[ycsbc::READ] / temp_cnt[ycsbc::READ]);
    }
	if ( temp_cnt[ycsbc::UPDATE] ) {
	  printf("update ops:%7lu  use time:%7.3f s  IOPS:%7.2f iops (%.2f us/op)\n"
		, temp_cnt[ycsbc::UPDATE], 1.0 * temp_time[ycsbc::UPDATE]*1e-6
		, 1.0 * temp_cnt[ycsbc::UPDATE] * 1e6 / temp_time[ycsbc::UPDATE]
		, 1.0 * temp_time[ycsbc::UPDATE] / temp_cnt[ycsbc::UPDATE]);
	}
    if ( temp_cnt[ycsbc::SCAN] ) {
	   printf("scan ops  :%7lu  use time:%7.3f s  IOPS:%7.2f iops (%.2f us/op)\n"
		, temp_cnt[ycsbc::SCAN], 1.0 * temp_time[ycsbc::SCAN]*1e-6
		, 1.0 * temp_cnt[ycsbc::SCAN] * 1e6 / temp_time[ycsbc::SCAN]
		, 1.0 * temp_time[ycsbc::SCAN] / temp_cnt[ycsbc::SCAN]);
	}
    if ( temp_cnt[ycsbc::READMODIFYWRITE] ) {
		printf("rmw ops   :%7lu  use time:%7.3f s  IOPS:%7.2f iops (%.2f us/op)\n"
		, temp_cnt[ycsbc::READMODIFYWRITE], 1.0 * temp_time[ycsbc::READMODIFYWRITE]*1e-6
		, 1.0 * temp_cnt[ycsbc::READMODIFYWRITE] * 1e6 / temp_time[ycsbc::READMODIFYWRITE]
		, 1.0 * temp_time[ycsbc::READMODIFYWRITE] / temp_cnt[ycsbc::READMODIFYWRITE]);
	}
    printf("********************************\n");

  double duration = timer.End();
  cerr << "# Transaction throughput (KTPS)" << endl;
  cerr << props["dbname"] << '\t' << file_name << '\t' << num_threads << '\t';
  cerr << total_ops / duration / 1000 << endl;
}

string ParseCommandLine(int argc, const char *argv[], utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-host") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("host", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-port") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("port", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-slaves") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("slaves", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
      ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
      argindex++;

	} else if(strcmp(argv[argindex],"-dbpath")==0){
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbpath", argv[argindex]);
      argindex++;

	} else if(strcmp(argv[argindex],"-optpath")==0){
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("optfilepath", argv[argindex]);
      argindex++;


	} else if(strcmp(argv[argindex],"-dbstatistics")==0){
	  argindex++;
      if(argindex >= argc){
        UsageMessage(argv[0]);
        exit(0);
      }	
      props.SetProperty("dbstatistics",argv[argindex]);
      argindex++;
    } else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)" << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple files can" << endl;
  cout << "                   be specified, and will be processed in the order specified" << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}

