#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <getopt.h>
#include <inttypes.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <signal.h>
#include <errno.h>

uint8_t cont = 1;

struct cmdline
{
    char *dest;

    uint16_t destport;

    uint64_t timeout;
    uint64_t interval;

    unsigned int verbose : 1;

};

const struct option lopts[] =
{
    {"dest", required_argument, NULL, 'd'},
    {"port", required_argument, NULL, 'p'},
    {"timeout", required_argument, NULL, 't'},
    {"interval", required_argument, NULL, 'i'},
    {"verbose", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
};

void parsecmdline(int argc, char **argv, struct cmdline *cmd)
{
    int c = -1;

    while ((c = getopt_long(argc, argv, "d:p:t:i:v", lopts, NULL)) != -1)
    {
        switch (c)
        {
            case 'd':
                cmd->dest = optarg;

                break;

            case 'p':
                cmd->destport = (uint16_t)atoi(optarg);

                break;

            case 't':
            {
                char *new = strdup(optarg);
                cmd->timeout = (uint64_t)strtoull((const char *)new, (char **)new, 0);

                break;
            }

            case 'i':
            {
                char *new = strdup(optarg);
                cmd->interval = (uint64_t)strtoull((const char *)new, (char **)new, 0);

                break;
            }

            case 'v':
                cmd->verbose = 1;

                break;

            case '?':
                fprintf(stderr, "Missing argument.\n");

                break;
        }
    }
}

void sighdl(int tmp)
{
    cont = 0;
}

int main(int argc, char **argv)
{
    // Parse command line.
    struct cmdline cmd =
    {
        .dest = NULL,
        .timeout = (int)1e6,
        .interval = (int)1e6,
        .destport = 63000,
        .verbose = 0
    };

    parsecmdline(argc, argv, &cmd);

    if (cmd.dest == NULL)
    {
        fprintf(stderr, "Missing destination host/IP. Please use -d or --dest.\n");

        return EXIT_FAILURE;
    }

    // Setup timeout struct.
    struct timeval tv = {0};

    tv.tv_sec = cmd.timeout / (int)1e6;
    tv.tv_usec = cmd.timeout % (int)1e6;

    // Output our arguments.
    fprintf(stdout, "Sending to %s:%" PRIu16 " with %lu.%lu seconds timeout and verbose set to %d\n", cmd.dest, cmd.destport, tv.tv_sec, tv.tv_usec, cmd.verbose);

    // Create socket.
    struct sockaddr_in sin;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        fprintf(stderr, "Error setting up socket :: %s\n", strerror(errno));

        return EXIT_FAILURE;
    }

    // Get host/IP address.
    struct hostent *he = gethostbyname(cmd.dest);

    if (he == NULL || he->h_addr_list[0] == NULL)
    {
        fprintf(stderr, "Failed to retrieve host.\n");

        return EXIT_FAILURE;
    }

    struct in_addr *host = (struct in_addr *)he->h_addr_list[0];

    // Fill out sock addr struct.
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = host->s_addr;
    sin.sin_port = htons(cmd.destport);
    memset(&sin.sin_zero, 0, sizeof(sin.sin_zero));

    // Set socket timeout.
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) != 0)
    {
        fprintf(stderr, "Error setting socket read timeout :: %s\n", strerror(errno));

        return EXIT_FAILURE;
    }

    signal(SIGINT, sighdl);

    char buffer[256];
    uint64_t count = 0;
    uint64_t recv = 0;
    double avglat = 0.0;

    while (cont)
    {
        struct timeval start, stop;
        gettimeofday(&start, NULL);

        if (sendto(sockfd, NULL, 0, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            fprintf(stderr, "Failed to send packet :: %s\n", strerror(errno));

            continue;
        }

        count++;

        socklen_t len;

        if(recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sin, &len) < 0)
        {
            fprintf(stdout, "Timed out.\n");

            continue;
        }

        gettimeofday(&stop, NULL);

        double lat = (double)((stop.tv_usec - start.tv_usec)) / 1000;
        avglat += lat;

        if (cmd.verbose)
        {
            fprintf(stdout, "%.2lfms\t[%" PRIu64 "]\n", lat, count);
        }

        recv++;

        usleep(cmd.interval);
    }
    
    uint64_t lost = count - recv;
    double loss = (lost > 0) ? (double)((double)lost / count) * 100 : 0;
    double avglat2 = (recv > 0) ? (avglat / (double)recv) : 0;

    fprintf(stdout, "Packets Sent => %" PRIu64 "\nPackets Received => %" PRIu64 "\nPackets Lost => %" PRIu64 "\nPacket Loss => %.2lf%%\nAvg Latency => %.2lfms\n", count, recv, lost, loss, avglat2);

    close(sockfd);
    
    return EXIT_SUCCESS;
}