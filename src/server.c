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
    uint16_t port;
    unsigned int verbose : 1;
};

const struct option lopts[] =
{
    {"port", required_argument, NULL, 'p'},
    {"verbose", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
};

void parsecmdline(int argc, char **argv, struct cmdline *cmd)
{
    int c = -1;

    while ((c = getopt_long(argc, argv, "p:v", lopts, NULL)) != -1)
    {
        switch (c)
        {
            case 'p':
                cmd->port = (uint16_t)atoi(optarg);

                break;

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
        .port = 63000,
        .verbose = 0
    };

    parsecmdline(argc, argv, &cmd);

    // Output our arguments.
    fprintf(stdout, "Listening on port %" PRIu16 " with verbose set to %d\n", cmd.port, cmd.verbose);

    // Create socket.
    struct sockaddr_in sin;
    struct sockaddr_in cin;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        fprintf(stderr, "Error setting up socket :: %s\n", strerror(errno));

        return EXIT_FAILURE;
    }

    // Fill sock address.
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(cmd.port);
    memset(&sin.sin_zero, 0, sizeof(sin.sin_zero));

    // Bind the socket.
    if (bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) != 0)
    {
        fprintf(stderr, "Error binding socket :: %s.\n", strerror(errno));

        return EXIT_FAILURE;
    }

    // Set socket timeout.
    struct timeval tv = {0};
    tv.tv_sec = 1;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) != 0)
    {
        fprintf(stderr, "Error setting socket read timeout :: %s\n", strerror(errno));

        return EXIT_FAILURE;
    }

    signal(SIGINT, sighdl);

    char buffer[256];
    uint64_t count = 0;

    while (cont)
    {
        socklen_t len = sizeof(cin);

        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cin, &len) < 0)
        {
            continue;
        }

        if (sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (struct sockaddr *)&cin, sizeof(cin)) < 0)
        {
            fprintf(stderr, "Failed to send packet :: %s\n", strerror(errno));

            continue;
        }

        count++;

        char *ip = inet_ntoa(cin.sin_addr);

        if (cmd.verbose)
        {
            fprintf(stdout, "Received packet from %s (%" PRIu64 ")\n", (ip != NULL) ? ip : "N/A", count);
        }
    }
    
    fprintf(stdout, "Received a total of %" PRIu64 " packets.\n", count);

    close(sockfd);
    
    return EXIT_SUCCESS;
}