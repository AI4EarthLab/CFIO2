#include <control_center.h>
#include <unordered_set>
using namespace std;
std::unordered_set<std::string> set_filename;

int IO_process_per_node;
int p_size;

void Control::gather_process_names(int multi_nodes, MPI_Comm all_comm)
{
    int slave_num;
    MPI_Comm_size(all_comm, &slave_num);
    slave_num--;
    ProcessNameCollector::gatherNames(slave_num, multi_nodes, all_comm);
}
void Control::startScheduling(MPI_Comm all_comm)
{
    int slave_num;
    MPI_Comm_size(all_comm, &slave_num);
    slave_num--;
    std::unordered_map<string, std::vector<struct_request>> waitting_request;

    // ??需要增加一个功能，IO进程应当反馈一个IO时间，告诉主进程，主进程根据IO速度来调整快慢

    int finalize_count = 0; // 控制让control_center结束用
    int write_out_count = 0;

    while (true)
    {
        struct_request reqmsg = receiveMessages(all_comm);

        int request_rank = reqmsg.rank;
        int subcommand = 0;
        // 如果所有的slave都调用了finalize，那么control退出
        if (reqmsg.state == state_finalize)
        {
            finalize_count++;
            if (finalize_count == slave_num)
                break;
        }

        if (reqmsg.state == state_write_out)
        {
            MPI_Comm group_comm;
            write_out_count++;

            CommandMessage msg_request;
            struct_command send_data_request;

            // 缓存同一个变量的获得了同一个subcommand，也就是mpi_split时候的color
            msg_request.setStructValues(send_data_request, request_rank, command_write_out /*交给别人*/, request_rank, ScheduleHolder::getKeysForProcess(request_rank) /*color*/);
            msg_request.sendStruct(send_data_request, request_rank, 1, all_comm);

            // control也要参与他们的split，但是如果把slave单独划分为一个通信域，那么应该就不需要下面这段了
            if (write_out_count == slave_num)
            {
                // 让所有的slave写出
                int group_id = -1;
                MPI_Comm_split(all_comm, group_id, 0 /*myrank*/, &group_comm);
                write_out_count = 0;

                // begin clear
                NodeChecker::clear();
                LoaderChecker::clear();
                IOMission::clear();
                IOstate::clear();
                ScheduleHolder::clear();
                write_out_count = 0;
            }
        }

        if (reqmsg.state == state_needio) // 请求IO
        {

            string request_name = ProcessNameCollector::get_rank_name(request_rank);

            if ((LoaderChecker::LoadRank(request_rank) && ScheduleHolder::getVectorSize(reqmsg.filename) == 0) || // 当前进程已经有任务，并且当前变量还没人负责
                (!LoaderChecker::LoadRank(request_rank) && ScheduleHolder::getVectorSize(reqmsg.filename) < p_size && NodeChecker::LoadNode(request_name) == IO_process_per_node))
            {

                waitting_request[charArrayToString(reqmsg.filename)].push_back(reqmsg);
                continue;
            }
            //  如果该进程自己没有数据，并且该变量的p_size还没有凑齐，那么把这个人算上
            if (!LoaderChecker::LoadRank(request_rank) && ScheduleHolder::getVectorSize(reqmsg.filename) < p_size)
            {

                NodeChecker::AddNode(request_name);
                // 需要将他自己的load+1，
                LoaderChecker::AddRank(request_rank);
                // 需要在记录表中记录上，这个变量又有一个人来接收啦
                ScheduleHolder::addValue(reqmsg.filename, request_rank);

                // 自己输出
                CommandMessage msg;
                struct_command send_data;

                msg.setStructValues(send_data, request_rank, command_directwrite, request_rank, subcommand);
                msg.sendStruct(send_data, request_rank, 1, all_comm);

                for (int i = 0; i < waitting_request[charArrayToString(reqmsg.filename)].size(); i++)
                {

                    struct_request waitting_reqmsg = waitting_request[charArrayToString(reqmsg.filename)][i];

                    CommandMessage msg_request;
                    struct_command send_data_request;

                    msg_request.setStructValues(send_data_request, waitting_reqmsg.rank, command_sendandcompute /*交给别人*/, request_rank, subcommand);
                    msg_request.sendStruct(send_data_request, waitting_reqmsg.rank, 1, all_comm);

                    LoaderChecker::AddRank(request_rank);
                    IOMission::addmission(request_rank, waitting_reqmsg.rank);
                }
                waitting_request[charArrayToString(reqmsg.filename)].clear();
            }
            else
            { // 交给别人来输出，自己就去做下面的计算吧

                int io_rank = ScheduleHolder::getRandomValue(reqmsg.filename);

                CommandMessage msg_request;
                struct_command send_data_request; // 当前发起IO请求的人

                msg_request.setStructValues(send_data_request, request_rank, command_sendandcompute /*交给别人*/, io_rank, subcommand);
                msg_request.sendStruct(send_data_request, request_rank, 1, all_comm);

                LoaderChecker::AddRank(io_rank);

                // 正好这个人正在等待分配任务
                if (IOstate::get_state(io_rank) == state_iofinish)
                {
                    IOstate::set_state(io_rank, state_ioing);
                    CommandMessage msg_io;
                    struct_command send_data_io; // 真正执行IO的

                    msg_io.setStructValues(send_data_io, io_rank, command_helppartnerio /*帮助别人*/, request_rank, subcommand);
                    msg_io.sendStruct(send_data_io, io_rank, 1, all_comm);
                }
                else // 需要这样记录一下，因为control和其他进程用的是阻塞式的通信，所以如果IO进程没有准备好，只能记录下来，负责control进程会被阻塞
                {

                    IOMission::addmission(io_rank, request_rank);
                }
            }
        }
        if (reqmsg.state == state_iofinish) // IO完成，等待指示
        {

            // 取出任务
            int partner_rank = IOMission::getmission(request_rank);
            if (partner_rank == -1) // 暂时没有任务
            {

                // 不理会他，但是记录下他现在正在等待这个信息，
                IOstate::set_state(request_rank, state_iofinish);

                int done_num;
                done_num = ScheduleHolder::get_IO_done_num(reqmsg.filename);

                // 有的进程查询到这里时确实还没完成，但是后续也没再给他分配任务了，需要让那些等待的结束等待继续计算
                if (done_num == slave_num)
                {

                    CommandMessage msg_io;
                    struct_command send_data_io; // 真正执行IO的

                    msg_io.setStructValues(send_data_io, request_rank, command_gooncompute /*继续计算*/, partner_rank, subcommand);
                    msg_io.sendStruct(send_data_io, request_rank, 1, all_comm);
                    IOstate::set_state(request_rank, state_other);

                    vector<int> waitingIO = ScheduleHolder::get_wait_IO_processes(reqmsg.filename);
                    for (int i = 0; i < waitingIO.size(); i++)
                    {
                        int otherwaitingrank = waitingIO[i];
                        CommandMessage msg_io;
                        struct_command send_data_io; // 真正执行IO的
                        msg_io.setStructValues(send_data_io, otherwaitingrank, command_gooncompute /*继续计算*/, partner_rank, subcommand);
                        msg_io.sendStruct(send_data_io, otherwaitingrank, 1, all_comm);
                        IOstate::set_state(otherwaitingrank, state_other);
                    }
                }
            }
            else
            {
                CommandMessage msg_io;
                struct_command send_data_io; // 真正执行IO的

                msg_io.setStructValues(send_data_io, request_rank, command_helppartnerio /*帮助别人*/, partner_rank, subcommand);
                msg_io.sendStruct(send_data_io, request_rank, 1, all_comm);
            }
        }
    }
}

struct_request Control::receiveMessages(MPI_Comm all_comm)
{
    // 使用MPI_Recv接收其他进程发送的消息
    // 根据收到的消息更新本地状态表，确定下一步动作，并发送相应的消息回复

    RequestMessage msg;
    struct_request recv_data;

    // 接收结构体
    msg.recvStruct(recv_data, MPI_ANY_SOURCE, 0, all_comm);

    // 获取结构体成员的值
    // msg.getStructValues(recv_data);
    return recv_data;
}
extern "C"
{
    void cfio2_init_fortran(MPI_Fint Fcomm, int IO_process_per_node_, int IO_process_per_var_)
    {

        MPI_Comm Ccomm;
        Ccomm = MPI_Comm_f2c(Fcomm);
        cfio2_init(Ccomm, IO_process_per_node_, IO_process_per_var_);
    }
    void cfio2_init(MPI_Comm all_comm, int IO_process_per_node_, int IO_process_per_var_)
    {
        int rank, size;
        MPI_Comm_rank(all_comm, &rank);
        MPI_Comm_size(all_comm, &size);
        p_size = IO_process_per_var_;
        IO_process_per_node = IO_process_per_node_;

        if (rank == 0)
        {
            Control ctrl;

            int virtual_multinodes = 0;
            ctrl.gather_process_names(virtual_multinodes, all_comm);

            ctrl.startScheduling(all_comm);
        }
        else
        {

            char processorName[MPI_MAX_PROCESSOR_NAME];
            int nameLen;
            MPI_Get_processor_name(processorName, &nameLen);
            std::string machineName(processorName, nameLen);

            MPI_Send(&machineName[0], machineName.size() + 1, MPI_CHAR, 0, 0, all_comm);
        }
    }

    void cfio2_put_vara_fortran(MPI_Fint Fcomm, char *filename_in, char *var_name, int datatype, char *dim_name1, char *dim_name2, char *dim_name3, int global[3], int start[3], int count[3], void *buf, int append)
    {
        MPI_Comm Ccomm;

        Ccomm = MPI_Comm_f2c(Fcomm);

        int new_start[3];
        new_start[0] = start[0] - 1;
        new_start[1] = start[1] - 1;
        new_start[2] = start[2] - 1;

        // cout << "filename_in=<" << filename_in << ">" << endl;
        // cout << "var_name=" << var_name << endl;
        // cout << "datatype=" << datatype << endl;
        // cout << "dim_name1=" << dim_name1 << endl;
        // cout << "dim_name2=" << dim_name2 << endl;
        // cout << "dim_name3=" << dim_name3 << endl;
        // cout << "global=" << global[0] << " " << global[1] << " " << global[2] << endl;
        // cout << "new_start=" << new_start[0] << " " << new_start[1] << " " << new_start[2] << endl;
        // cout << "count=" << count[0] << " " << count[1] << " " << count[2] << endl;
        // cout << "append=" << append << endl;
        // for (int i = 0; i < count[0] * count[1] * count[2]; i++)
        //     cout << ((double *)buf)[i];
        // cout << endl;

        string new_dim_name[3];
        new_dim_name[0] = dim_name1;
        new_dim_name[1] = dim_name2;
        new_dim_name[2] = dim_name3;

        string new_filename = filename_in;

        cfio2_put_vara(Ccomm, new_filename, var_name, datatype, new_dim_name, global, new_start, count, buf, append);
    }
    void cfio2_put_vara(MPI_Comm all_comm, string filename_in, string var_name, int datatype, string dim_name[3], int global[3], int start[3], int count[3], void *buf, int append)
    {
        if (set_filename.find(filename_in) != set_filename.end())
        {
            std::cerr << "Cannot output to the same NC file before cfio2_wait_output operation." << std::endl;
        }
        set_filename.insert(filename_in);

        int myrank;

        MPI_Comm_rank(all_comm, &myrank);

        const char *filename = filename_in.c_str();

        RequestMessage msg;
        struct_request send_data;

        string msgvar = filename;

        msg.setStructValues(send_data, myrank, 1 /*请求IO*/, msgvar.c_str(), 10, global, start, count);
        // 发送结构体

        msg.sendStruct(send_data, 0 /*发给master*/, 0, all_comm);

        int flag = 0;
        while (true)
        {
            CommandMessage cmdmsg;
            struct_command recv_data;
            // 接收结构体
            cmdmsg.recvStruct(recv_data, 0 /*从master接收*/, 1, all_comm);

            if (recv_data.command == 0)
            {
                // 执行直接写文件
                DataReceiver::Recv_myself(global, start, count, buf, all_comm, filename, var_name, datatype, dim_name[0], dim_name[1], dim_name[2], append);
            }
            if (recv_data.command == 1)
            {
                break;
            }
            if (recv_data.command == 2)
            {
                DataReceiver::Partner_send(recv_data.partner_rank, global, start, count, datatype, buf, all_comm);
                break;
            }
            if (recv_data.command == 3)
            {
                DataReceiver::Partner_recv(recv_data.partner_rank, all_comm, datatype);
            }
            // 发送消息告诉master，自己开始等待
            RequestMessage reqmsg;
            struct_request send_data;

            // 设置结构体成员的值
            int global[3] = {-1, -1, -1};
            int start[3] = {-1, -1, -1};
            int count[3] = {-1, -1, -1};

            reqmsg.setStructValues(send_data, myrank, state_iofinish /*IO完成*/, msgvar.c_str(), 10, global, start, count);
            // 发送结构体
            reqmsg.sendStruct(send_data, 0 /*发给master*/, 0, all_comm);
        }
    }

    void cfio2_wait_output_fortran(MPI_Fint Fcomm)
    {
        MPI_Comm Ccomm;
        Ccomm = MPI_Comm_f2c(Fcomm);
        cfio2_wait_output(Ccomm);
    }
    void cfio2_wait_output(MPI_Comm all_comm)
    {
        set_filename.clear();
        int myrank;
        MPI_Comm_rank(all_comm, &myrank);
        int global[3];
        int start[3];
        int count[3];

        RequestMessage msg;
        struct_request send_data;
        msg.setStructValues(send_data, myrank, state_write_out /*结束 finalize*/, "", 10, global, start, count);
        msg.sendStruct(send_data, 0 /*发给master*/, 0, all_comm);

        // 从master接受
        CommandMessage cmdmsg;
        struct_command recv_data;
        // 接收结构体
        cmdmsg.recvStruct(recv_data, 0 /*从master接收*/, 1, all_comm);
        int color = recv_data.subcommand;
        MPI_Comm sub_comm;

        MPI_Comm_split(all_comm, color, myrank, &sub_comm);
        if (color != -1)
        {
            DataReceiver::write_out(myrank, color, all_comm, sub_comm);
        }
    }

    void cfio2_finalize_fortran(MPI_Fint Fcomm)
    {
        MPI_Comm Ccomm;
        Ccomm = MPI_Comm_f2c(Fcomm);
        cfio2_finalize(Ccomm);
    }
    void cfio2_finalize(MPI_Comm all_comm)
    {
        int myrank;
        MPI_Comm_rank(all_comm, &myrank);
        if (myrank == 0)
            return;
        int global[3];
        int start[3];
        int count[3];

        RequestMessage msg;
        struct_request send_data;
        msg.setStructValues(send_data, myrank, state_finalize /*结束 finalize*/, "", 10, global, start, count);
        msg.sendStruct(send_data, 0 /*发给master*/, 0, all_comm);
    }
}
