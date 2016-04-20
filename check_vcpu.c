/**
 * Program Name: check_vcpu.c
 * Author: getter
 * Description:A simple plugin to get cpu usage
 *             for nagios(nrpe) by libvirt api
 */
#include <stdlib.h>
#include <stdio.h>
#include <libvirt/libvirt.h>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <regex>
using namespace std;
 
/* define the exit status for nagios */
#define OK       0
#define WARNING  1
#define CRITICAL 2
#define UNKNOWN  3
 
/* get cpu time of the given name */
double getCpuTime(int vmID,virConnectPtr conn) {
    virDomainInfo info;
    virDomainPtr domain = NULL;
    int ret;
 
    /* Find the domain of the given name */
    domain = virDomainLookupByID(conn, vmID);
    if (domain == NULL) {
        cout << "Failed to find the vps called " << vmID << endl;
        exit(OK);
    }
 
    /* Get the information of the domain */
    ret = virDomainGetInfo(domain, &info);
    virDomainFree(domain);
 
    if (ret < 0) {
        cout << "Failed to get information for " << vmID << endl;
        exit(OK);
    }
 
    return info.cpuTime;
}

double calCpuUsage(int interval, int vmID, virConnectPtr conn){

    long long startCpuTime;    /* cpu time of the first time */
    long long endCpuTime;      /* cpu time of the second time */
    int  cpuTime;              /* value of startCpuTime - endCpuTime */
    double cpuUsage;
    char CPUcore[10];
    FILE *fp;
    
    fp = popen ("nproc", "r");
    fgets(CPUcore, sizeof(CPUcore), fp);
    pclose(fp);

    /* get cpu time the first time */
    startCpuTime = getCpuTime(vmID, conn);

    /* wait for some seconds  */
    sleep(interval);

    /* get cpu time the second time */
    endCpuTime = getCpuTime(vmID, conn);

    /* colose connection */
    virConnectClose(conn);

    /* calculate the usage of cpu */
    cpuTime = (endCpuTime - startCpuTime) / 1000;
    cpuUsage = cpuTime / (double)(interval * 1000000 * atoi(CPUcore));
    /* display cpuUsage by percentage */
    cpuUsage *= 100;
    //printf("CPUsec: %f\n", (double)(cpuTime) * 1000);
    //cout << "CPUcore: " << atoi(CPUcore) << endl;
    return cpuUsage;
}

bool checkArgv(char *argv[], int argc){
	bool check = true;

	string e = "([0-9]*)";
	regex pattern(e, regex_constants::grep);

	for(int i = 1; i < argc; i++){
		if(!regex_match(argv[i], pattern)){
			//cout << "argv is not a number:" << argv[i] << endl;
			check = false;
		}
	}
	return check;
}
 
int main(int argc,char * argv[])
{
    int  vmID;                 /* vmID */
    int  interval = 1;         /* check interval */
    double warning;            /* warning value */
    double critical;           /* critical value */
    double cpuUsage;
    string output = "";        /* output data for nagios */
    int  ret;                  /* exit status for nagios */
    virConnectPtr conn;        /* connection pointer */

    if(!checkArgv(argv, argc)){
	cout << "Option not allowed" << endl;
        cout << "Usage:check_vcpu <vid> <warning> <critical> [interval]\n\n";
	return 1;
    }
    switch (argc){
        case 5:
             interval = atoi(argv[4]);
        case 4:
             vmID  = atoi(argv[1]);
             warning  = atof(argv[2]);
             critical = atof(argv[3]);
             break;
        default:
             cout << "Usage:check_vcpu <vid> <warning> <critical> [interval]\n\n";
             return OK;
    }
 
    /* connect to local Xen Host */
    conn = virConnectOpenReadOnly(NULL);
    if (conn == NULL) {
        cout << "Failed to connect to local Xen Host\n";
        return OK;
    }
 
    cpuUsage = calCpuUsage(interval, vmID, conn); 

    /* make output data and exit status for nagios*/
    if (cpuUsage > critical) {
        output = "CRITICAL";
        ret    = CRITICAL;
    } else if (cpuUsage > warning){
        output = "WARNING";
        ret    = WARNING;
    } else {
        output = "OK";
        ret    = OK;
    }

    printf("%s CPU:%.2f%|CPU=%.2f\n",output.c_str(),cpuUsage,cpuUsage);
    return ret;
}
