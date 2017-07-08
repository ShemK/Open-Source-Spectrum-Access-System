#include "CentralRemConnector.hpp"
#include <signal.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
//g++ test.cpp CentralRemConnector.cpp -lpqxx -lgnuradio-pmt -std=c++11
int sig_terminate = 0;

void terminate(int signum)
{
    printf("\nCtr+D Pressed - Exiting\n");
    sig_terminate = 1;
}

int main()
{
    // register signal handlers
    signal(SIGINT, terminate);
    signal(SIGQUIT, terminate);
    signal(SIGTERM, terminate);

    CentralRemConnector db_connector("wireless", "wireless", "wireless", "127.0.0.1");
    db_connector.connect();

    struct sockaddr_in udp_server_addr;
    struct sockaddr_in udp_client_addr;
    socklen_t addr_len = sizeof(udp_server_addr);
    memset(&udp_server_addr, 0, addr_len);
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    udp_server_addr.sin_port = htons(6000);
    int udp_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int status = bind(udp_server_sock, (sockaddr *)&udp_server_addr, addr_len);

    int recv_buffer_len = 2000;
    char recv_buffer[recv_buffer_len];
    fd_set read_fds;
    while (sig_terminate == 0)
    {
        FD_ZERO(&read_fds);
        FD_SET(udp_server_sock, &read_fds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        if (select(udp_server_sock + 1, &read_fds, NULL, NULL, &timeout) > 0)
        {
            int recv_len = recvfrom(udp_server_sock, recv_buffer, recv_buffer_len, 0,
                                    (struct sockaddr *)&udp_client_addr, &addr_len);
            db_connector.analyze((const char *)recv_buffer, recv_len);
        }
    }
    close(udp_server_sock);
    /*
    pmt::pmt_t instruction = pmt::make_dict();
    pmt::pmt_t key = pmt::string_to_symbol("UPDATE");
    pmt::pmt_t instruction_value = pmt::make_dict();
    pmt::pmt_t select_list = pmt::list1(key);
    select_list = pmt::list_rm(select_list, key);
    select_list = pmt::list_add(select_list, pmt::string_to_symbol("occ"));
    instruction_value = pmt::dict_add(instruction_value, pmt::string_to_symbol("select_list"), select_list);
    //instruction = pmt::dict_add(instruction,key,instruction_value);
    instruction = pmt::dict_add(instruction, pmt::string_to_symbol("Life"), pmt::from_long(7123));
    instruction = pmt::dict_add(instruction, pmt::string_to_symbol("asj"), pmt::from_double(73));
    instruction = pmt::dict_add(instruction, pmt::string_to_symbol("hjsad"), pmt::string_to_symbol("yo"));
    pmt::pmt_t key_list = pmt::dict_keys(instruction);
    db_connector.insert(instruction);
    pmt_t recv_dict = pmt::make_dict();
    pmt_t table = pmt::string_to_symbol("channelinfo");
    pmt_t parameters = pmt::make_dict();
    parameters = pmt::dict_add(parameters, pmt::string_to_symbol("table"), table);
    parameters = pmt::dict_add(parameters, pmt::string_to_symbol("attributes"), instruction);
    recv_dict = pmt::dict_add(recv_dict, key, parameters);
    std::string data_to_send = pmt::serialize_str(recv_dict);
    db_connector.analyze(data_to_send.c_str(), data_to_send.length());
    */
    // std::cout << pmt::length(pmt::dict_keys(recv_dict)) << std::endl;
    // std::cout << recv_dict << std::endl;
}