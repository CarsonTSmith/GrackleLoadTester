#include <arpa/inet.h>
#include <future>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

static const char msg_to_send[]      = "00000043{\"path\":\"/echo\",\"message\":\"echo this back\"}";
static constexpr int msg_to_send_len = 51;

static constexpr int msg_to_rec_len  = 70;

// each thread will count the number of requests they complete
thread_local int count = 0;

static int setup_socket(const char *ip, const int port)
{
	int srvrfd;
	struct sockaddr_in serv_addr;

	if ((srvrfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		exit(-1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		exit(-1);
	}

	if (connect(srvrfd, (struct sockaddr*)&serv_addr, 
	    sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		exit(-1);
	}

	return srvrfd;
}

static void write_msg(const int sockfd)
{
    int total = 0, result = 0;

    while (total < msg_to_send_len) {
        result = write(sockfd, msg_to_send, msg_to_send_len);
        if (result > -1)
            total += result;
    }
}

static void read_msg(const int sockfd)
{
    int total = 0, result = 0;
    char buf[msg_to_rec_len] = {0};

    while (total < msg_to_rec_len) {
        result = read(sockfd, buf, msg_to_rec_len);
        if (result > 0)
            total += result;
    }
}

static void do_work(int &count, const char *ip, const int port)
{
    int sockfd = setup_socket(ip, port);
    while (1) {
        write_msg(sockfd);
        read_msg(sockfd);
        count++;
    }
}

static void print_stats(const int runtime, const std::vector<int> &counts)
{
    int   total = 0;
    float reqps = 0;

    for (const auto &count: counts)
        total += count;

    reqps = (float)total / (float)runtime;
    printf("Total requests: %d\nTime(s): %d\nRequests per second: %f", total, runtime, reqps);
}

// client
int main(int argc, char *argv[])
{
    int port, num_threads, runtime;
    char *ip;

    ip = argv[1];
    port = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    runtime = atoi(argv[4]);

    std::vector<int> counts;
    for (int i = 0; i < num_threads; ++i) {
        counts.push_back(0);
    }

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(std::thread(do_work, std::ref(counts[i]), ip, port));
    }

    sleep(runtime);
    print_stats(runtime, counts);
    exit(0);
	return 0;

    // Have each thread count the number of response.
    // At the end of the runtime we sum all the threads count
    // and print the stats.
}
