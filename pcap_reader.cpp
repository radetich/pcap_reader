#include "pcap_reader.h"

//LOGIC FOR PCAP_READER
//WRITTEN BY NIK BENDER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>


using namespace boost::filesystem;
using namespace std;

typedef struct __attribute__((packed))
{
    uint16_t length;
    uint16_t type;
    uint8_t tag[8];
    uint32_t time_low;
    uint32_t time_high;
    uint8_t channel;
    uint8_t dlc;
    uint16_t flags;
    uint32_t canid;
    uint8_t data[8];
} 

peakpacket_t;

void print(std::vector<string> &input)
{
	for (int i = 0; i < input.size(); i++) {
		std::cout << input.at(i) << endl;
	}
}


struct raw_frame {
    __u8    data[16] __attribute__((aligned(16)));
};

// get the pcap files in a directory, retun them in a vector
vector <string> get_pcaps(string folder_path){
    path p{folder_path};
    vector <string> files;
    if(is_directory(p)) {
        for(auto& entry : boost::make_iterator_range(directory_iterator(p), {})){
            if(entry.path().extension().string() == ".pcap"){
                files.push_back(entry.path().string());
            }
        }
    }

    return files;
}


//MAIN FUNCTION! CALLED IN DRIVER.CPP
int process(){

    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    strcpy(ifr.ifr_name, "vcan0" );
    ioctl(s, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    vector <string> file_names = get_pcaps("../resources/radar/");

    print(file_names);

    for(auto file_path : file_names){ 
        
        int nbytes;
        struct can_frame frame;
        char errbuff[PCAP_ERRBUF_SIZE];
        pcap_t *pcap = pcap_open_offline(file_path.c_str(), errbuff); 
        u_int packetCount = 0;
        struct pcap_pkthdr *header;
        peakpacket_t *data;
        printf("Sizeof peakpackt: %d \n", sizeof(peakpacket_t));
        while (int returnValue = pcap_next_ex(pcap, &header, (const uint8_t**)&data) >= 0){
            // Show a warning if the length captured is different
            if (header->len != header->caplen)
                printf("Warning! Capture size different than packet size: %ld bytes\n", header->len);

            frame.can_dlc = data->dlc;
            frame.can_id = ntohl(data->canid);
            memcpy(frame.data, data->data, data->dlc);
            

            // write struct to socket
            nbytes = write(s, &frame, sizeof(struct can_frame));
        }

        
    }
    printf("done\n");
    return 0;
    
}


int main() 
{
    cout << "#####################" << endl;
    cout << "PCAP_READER BY NIK BENDER AND COLE RADETICH" << endl;
    cout << "FOR READING RAW PCAP PACKET DATA FROM RADAR" << endl;
    cout << "#####################" << endl;

    //call function from PCAP_READER.CPP to handle real work
    process();
}