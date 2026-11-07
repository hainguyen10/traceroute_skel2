#include "traceroute.h"

uint16_t checksum(unsigned short *buffer, int size) {
    unsigned long sum = 0;
    while (size > 1) {
        sum += *buffer++;
        size -= 2;
    }
    if (size == 1) {
        sum += *(unsigned char *) buffer;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short) (~sum);
}
void fill_in_IP_Header(char *packet, int ttl, uint32_t destAddr) {
  struct iphdr *ip = (struct iphdr *)packet;
  ip-> version = 4;
  ip->ihl = 5;
  ip->tos = 0;
  ip->tot_len = htons(BUF_SIZE);
  ip->id = 0;
  ip->frag_off = 0;
  ip->ttl = ttl;
  ip->protocol = IPPROTO_ICMP;
  ip->check = 0;
  ip->saddr = 0;
  ip->daddr = destAddr;
}
void fill_in_ICMP_Header(char *packet) {
  struct icmphdr *icmp = (struct icmphdr *)packet;
  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->un.echo.id = htons(getpid());
  icmp->un.echo.sequence = htons(1);
  icmp->checksum = 0;
  icmp->checksum = checksum((unsigned short *)icmp, BUF_SIZE - sizeof(struct iphdr));
}

int main (int argc, char *argv[]) {
  std::string destIP;
  int opt = 0;
  while ((opt = getopt(argc,argv,"t:d:")) != -1) {

    switch (opt) {
    case 't':
      destIP = optarg;
      break;
    case 'd':
      LOG_LEVEL = atoi(optarg);
      break;
    case ':':
    case '?':
    default:
      std::cout << "useage: " << argv[0] << " -t [target ip] -d [Debug Level]" << std::endl;
      exit(-1);
    }
  }
  if(destIP.empty()) {
    std::cerr << "usage: " << argv[0] << " -t [target ip] -d [Debug Level]" << std::endl;
    exit(-1);
  }
  if(LOG_LEVEL > 0){
    std::cout << "[DEBUG] Target IP: " << destIP << " | Debug Level: " << LOG_LEVEL << std::endl;
  }
  char send_buf[BUF_SIZE];
  char recv_buf[BUF_SIZE];
  memset(send_buf, 'A', BUF_SIZE);
  // int sockFD = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  // if(sockFD == -1){
  //   perror("socket (FD)");
  // }
  int sendFd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if(sendFd < 0) {
    perror("socket (sendFd)");
    std::cerr << "Error: Raw sockets require root privileges (sudo).\n";
    return -1;
  }
  int recvFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if(recvFd == -1) {
    perror("socket (recvFd)");
    return -1;
  }
  inet_addr(destIP.c_str());
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr.s_addr = inet_addr(destIP.c_str());
  
  //inet_ntop(AF_INET, &recv_addr.sin_addr, respondent_ip, INET_ADDRSTRLEN);
  int current_ttl = START_TTL;
  bool reply_not_received = true;
  while (current_ttl <= MAX_TTL && reply_not_received) {
    fill_in_IP_Header(send_buf, current_ttl, dest_addr.sin_addr.s_addr);
    fill_in_ICMP_Header(send_buf + sizeof(struct iphdr));
    if(LOG_LEVEL > 0) {
      std :: cout <<"[DEBUG] Sending packet with TTL = " << current_ttl << std:: endl;
    }
    if(sendto(sendFd, send_buf, BUF_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1) {
      perror("sendto");
      break;
    }
    time_t start_time = time(NULL);
    bool not_done_reading = true;
    bool packet_acknowledged = false;
    while((time(NULL) - start_time < 15) && not_done_reading) {
      fd_set readFdSet;
      fd_set writeFdSet;
      fd_set exceptFdsSet;
      struct timeval timeout;
      timeout.tv_sec = 5;
      timeout.tv_usec = 0;
      FD_ZERO(&readFdSet);
      FD_SET(recvFd, &readFdSet);
      if(LOG_LEVEL > 1){
        std:: cout << "[DEBUG] Waiting up to 5 seconds to see if data becomes available. Current TTL: " << current_ttl<< std:: endl;
      }
      int select_returned = select(recvFd + 1, &readFdSet, NULL, NULL, &timeout);
      if(select_returned < 0 ) {
        perror("select");
        break;
      }
      //std:: cout << "[DEBUG] Select_Returned = " << select_returned << std:: endl;
      if(FD_ISSET(recvFd, &readFdSet)){
        //std:: cout << "[DEBUG] Bit # " << recvFd << " is set" << std:: endl;
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);
        ssize_t bytes_received = recvfrom(recvFd, recv_buf, BUF_SIZE, 0, (struct sockaddr *)&recv_addr, &addr_len);
        if(bytes_received < 0){
          perror("recvfrom");
          break;
        }
        struct iphdr *resp_ip = (struct iphdr *)recv_buf;
        int ip_hdr_len = resp_ip->ihl *4;
        struct icmphdr *resp_icmp = (struct icmphdr *)(recv_buf + ip_hdr_len);
        char respondent_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &recv_addr.sin_addr, respondent_ip, INET_ADDRSTRLEN);
        if(resp_icmp->type == ICMP_ECHOREPLY) {
          std::cout << "Echo Reply received from destination: "<< respondent_ip << " with TTL " << current_ttl << std::endl;
          reply_not_received = false;
          not_done_reading = false;
          packet_acknowledged = true;
          break;
        }else if(resp_icmp-> type == ICMP_TIME_EXCEEDED && resp_icmp->code == ICMP_EXC_TTL){
          std::cout << respondent_ip << " (TTL " << current_ttl << ")" << std::endl;
          not_done_reading = false;
          packet_acknowledged = true;
          break;
        }

      }
    }
    if(!packet_acknowledged){
      std::cout << "No response with TTL of " << current_ttl << "\n";
    }
    current_ttl++;
  }
  close(sendFd);
  close(recvFd);
  return 0;
}

