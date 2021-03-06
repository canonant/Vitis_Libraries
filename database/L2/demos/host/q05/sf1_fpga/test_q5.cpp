/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "table_dt.hpp"
#include "utils.hpp"
#include "cfg.hpp"
#include "tpch_read_2.hpp"

#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <climits>
#include <unordered_map>
const int PU_NM = 8;
#include "gqe_api.hpp"
#include "q5.hpp"
int main(int argc, const char* argv[]) {
    std::cout << "\n------------ TPC-H GQE (1G) -------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string xclbin_path; // eg. q5kernel_VCU1525_hw.xclbin
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir) || !is_dir(in_dir)) {
        std::cout << "ERROR: input dir is not specified or not valid.\n";
        return 1;
    }
    int board = 1;
    std::string board_s;
    if (parser.getCmdOption("-b", board_s)) {
        try {
            board = std::stoi(board_s);
        } catch (...) {
            board = 1;
        }
    }

    int num_rep = 1;

    std::string num_str;
    if (parser.getCmdOption("-rep", num_str)) {
        try {
            num_rep = std::stoi(num_str);
        } catch (...) {
            num_rep = 1;
        }
    }
    if (num_rep > 20) {
        num_rep = 20;
        std::cout << "WARNING: limited repeat to " << num_rep << " times\n.";
    }
    int scale = 1;
    std::string scale_str;
    if (parser.getCmdOption("-c", scale_str)) {
        try {
            scale = std::stoi(scale_str);
        } catch (...) {
            scale = 1;
        }
    }
    std::cout << "NOTE:running in sf" << scale << " data\n.";
    int32_t lineitem_n = SF1_LINEITEM;
    int32_t supplier_n = SF1_SUPPLIER;
    int32_t nation_n = SF1_NATION;
    int32_t orders_n = SF1_ORDERS;
    int32_t customer_n = SF1_CUSTOMER;
    int32_t region_n = SF1_REGION;
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        supplier_n = SF30_SUPPLIER;
        nation_n = SF30_NATION;
        orders_n = SF30_ORDERS;
        customer_n = SF30_CUSTOMER;
        region_n = SF30_REGION;
    }

    // ********************************************************** //

    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[board];
    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    std::vector<cl::Device> devices_;
    devices_.push_back(device);
    cl::Program program(context, devices_, xclBins);
    std::cout << "Kernel has been created\n";
    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 6;
    const int NumSweep = 4;
    Table tbs[NumTable];
    tbs[0] = Table("region", region_n, 2, in_dir);
    tbs[0].addCol("r_regionkey", 4);
    tbs[0].addCol("r_name", TPCH_READ_REGION_LEN + 1, 0, 0);

    tbs[1] = Table("nation", nation_n, 3, in_dir);
    tbs[1].addCol("n_regionkey", 4);
    tbs[1].addCol("n_nationkey", 4);
    tbs[1].addCol("n_name", TPCH_READ_NATION_LEN + 1);

    tbs[2] = Table("customer", customer_n, 2, in_dir);
    tbs[2].addCol("c_nationkey", 4);
    tbs[2].addCol("c_custkey", 4);

    tbs[3] = Table("order", orders_n, 3, in_dir);
    tbs[3].addCol("o_custkey", 4);
    tbs[3].addCol("o_orderkey", 4);
    tbs[3].addCol("o_orderdate", 4);

    tbs[4] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[4].addCol("l_orderkey", 4);
    tbs[4].addCol("l_suppkey", 4);
    tbs[4].addCol("l_extendedprice", 4);
    tbs[4].addCol("l_discount", 4);

    tbs[5] = Table("supplier", supplier_n, 2, in_dir);
    tbs[5].addCol("s_suppkey", 4);
    tbs[5].addCol("s_nationkey", 4);

    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 10, 1, "");
    Table tk0("tk0", 190000 * scale, 8, "");
    Table tk1("tk1", 60000 * scale, 8, "");
    Table tk2("tk2", 7500 * scale, 2, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
        std::cout << "DEBUG0" << std::endl;
    }
    th0.allocateHost();
    tk0.allocateHost();
    tk1.allocateHost();
    tk2.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateHost();
    };
    get_cfg_dat_1(cfgcmds[0].cmd);
    get_cfg_dat_2(cfgcmds[1].cmd);
    get_cfg_dat_3(cfgcmds[2].cmd);
    get_cfg_dat_4(cfgcmds[3].cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateDevBuffer(context, 32);
    }
    tk0.allocateDevBuffer(context, 32);
    tk1.allocateDevBuffer(context, 32);
    tk2.allocateDevBuffer(context, 32);
    th0.allocateDevBuffer(context, 32);

    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateDevBuffer(context, 32);
    };

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    int kernelInd = 0;
    bufferTmp buftmp(context);
    buftmp.initBuffer(q);
    // kernel Engine
    krnlEngine krnlstep[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        krnlstep[i] = krnlEngine(program, q, "gqeJoin");
    }

    krnlstep[0].setup(th0, tbs[2], tk0, cfgcmds[0], buftmp);
    krnlstep[1].setup(tk0, tbs[3], tk1, cfgcmds[1], buftmp);
    krnlstep[2].setup(tk1, tbs[4], tk0, cfgcmds[2], buftmp);
    krnlstep[3].setup(tbs[5], tk0, tk2, cfgcmds[3], buftmp);

    // transfer Engine
    transEngine transin;
    transEngine transin1;
    transEngine transin2;
    transEngine transout;

    transin.setq(q);
    transin1.setq(q);
    transin2.setq(q);
    transout.setq(q);

    transin.add(&(tbs[2]));
    //  transin.add(&(tbs[3]));
    //  transin.add(&(tbs[4]));
    transin.add(&(tbs[5]));

    for (int i = 0; i < NumSweep; i++) {
        transin.add(&(cfgcmds[i]));
    };
    q.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[NumSweep];
    std::vector<cl::Event> eventsd2h_read[NumSweep];
    std::vector<cl::Event> events[NumSweep];
    std::vector<cl::Event> events_grp[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        events[i].resize(1);
    };
    for (int i = 0; i < NumSweep; i++) {
        eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep; i++) {
        eventsd2h_read[i].resize(1);
    };

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1;
#ifdef INI
    tk0.initBuffer(q);
    tk1.initBuffer(q);
    tk2.initBuffer(q);
#endif
    gettimeofday(&tv_r_s, 0);
    // step1 :region nation-> t1
    q5Join_r_n(tbs[0], tbs[1], th0);
    gettimeofday(&tv_r_0, 0);

    // step2 :t1 customer-> t2
    transin.add(&th0);
    transin.host2dev(0, nullptr, &(eventsh2d_write[0][0]));
    //  q5Join_t1_c(th0,tbs[2],tk0);
    krnlstep[0].run(0, &(eventsh2d_write[0]), &(events[1][0]));
    kernelInd++;

    // step3 : t2 order -> t3
    //  q5Join_t2_o(tk0,tbs[3],tk1);
    transin1.add(&tbs[3]);
    transin1.host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));
    events_grp[1].push_back(eventsh2d_write[1][0]);
    events_grp[1].push_back(events[1][0]);
    krnlstep[1].run(0, &(events_grp[1]), &(events[2][0]));
    kernelInd++;

    // step4 : t3 line -> t4
    //  q5Join_t3_l(tk1,tbs[4],tk0);
    transin2.add(&tbs[4]);
    transin2.host2dev(0, &(eventsh2d_write[1]), &(eventsh2d_write[2][0]));
    events_grp[2].push_back(eventsh2d_write[2][0]);
    events_grp[2].push_back(events[2][0]);
    krnlstep[2].run(0, &(events_grp[2]), &(events[3][0]));
    kernelInd++;

    // supplier t4-> t5
    //  q5Join_s_t4(tbs[5],tk0,tk2);
    krnlstep[3].run(0, &(events[3]), &(events[4][0]));
    kernelInd++;

    transout.add(&tk2);
    transout.dev2host(0, &(events[4]), &(eventsd2h_read[0][0]));
    q.finish();

    gettimeofday(&tv_r_1, 0);
    q5Join_t5_n(tk2, tbs[1], tk0);
    q5GroupBy(tk0, tk1);
    q5Sort(tk1, tk0);
    gettimeofday(&tv_r_e, 0);

    print_h_time(tv_r_s, tv_r_s, tv_r_0, "NationFilter..");
    int64_t offset = tvdiff(&tv_r_s, &tv_r_0) / 1000;
    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], eventsd2h_read[0][0], kstart, "all kernels", offset);
    print_d_time(eventsh2d_write[0][0], eventsh2d_write[0][0], kstart, "data trans kernel0", offset);
    print_d_time(events[1][0], events[1][0], kstart, "kernel0", offset);
    print_d_time(eventsh2d_write[1][0], eventsh2d_write[1][0], kstart, "data trans kernel1", offset);
    print_d_time(events[2][0], events[2][0], kstart, "kernel1", offset);
    print_d_time(eventsh2d_write[2][0], eventsh2d_write[2][0], kstart, "data trans kernel2", offset);
    print_d_time(events[3][0], eventsd2h_read[0][0], kstart, "kernel2/3...", offset);
    print_h_time(tv_r_s, tv_r_1, tv_r_e, "Group&Sort..");
    std::cout << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
