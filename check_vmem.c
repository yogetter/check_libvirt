
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libvirt/libvirt.h>
#include <iostream>
#include <regex>
using namespace std;
 
/* define the exit status for nagios */
#define OK       0
#define WARNING  1
#define CRITICAL 2
#define UNKNOWN  3

bool checkArgv(char *argv[], int argc){
        bool check = true;
        string e = "([0-9]*)";
        regex pattern(e, regex_constants::grep);
        for(int i = 1; i < argc; i++){
                if(!regex_match(argv[i], pattern)){
                        check = false;
                }
        }
        return check;
}
double checkMem(int domid){
    virConnectPtr   c;
    virDomainPtr    d;
    virDomainMemoryStatStruct meminfo[8];
    int check;
    double memUsage;
    c = virConnectOpen(NULL);
    d = virDomainLookupByID(c, domid);
    check =  virDomainMemoryStats(d, meminfo, 7, 0);
    
    memUsage = (double)((double)(meminfo[6].val - meminfo[5].val) / (double)meminfo[6].val) * 100;
    printf("mem_avaliable = %d MB\n", meminfo[5].val/1024);
    printf("mem_total = %d MB\n", meminfo[6].val/1024);
    
    return memUsage; 
}
 
int main(int argc, char **argv) {
    double memUsage;
    int vmID, warning, critical, ret;
    string output;
    if(!checkArgv(argv, argc)){
        cout << "Option not allowed" << endl;
        cout << "Usage:check_vmem <vid> <warning> <critical>\n\n";
        return 1;
    }
    switch (argc){
        case 4:
             vmID  = atoi(argv[1]);
             warning  = atoi(argv[2]);
             critical = atoi(argv[3]);
             break;
        default:
             cout << "Usage:check_vmem <vid> <warning> <critical>\n\n";
             return OK;
    }
    memUsage = checkMem(vmID);

    if (memUsage > critical) {
        output = "CRITICAL";
        ret    = CRITICAL;
    } else if (memUsage > warning){
        output = "WARNING";
        ret    = WARNING;
    } else {
        output = "OK";
        ret    = OK;
    }
    printf("%s Memory:%.2f%|Memory=%.2f\n",output.c_str(),memUsage,memUsage);
    return ret;
}
