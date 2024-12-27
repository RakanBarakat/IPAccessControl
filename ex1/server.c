#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;


bool interactiveMode = false;

typedef struct _serverInterac
{
    bool interactive;
    int port[2];
}serverInterac;

typedef struct _rule
{
    int ip_address1[4];
    int ip_address2[4];
    int port1;
    int port2;
    struct _rule* next;

}rule;

rule* head = NULL;

typedef struct _cmds
{
    char* inputcmd;
    struct _cmds* next;
}comd;

comd* headcmd = NULL;

typedef struct _linkcmd2rule
{
    rule* rule;
    comd* cmd;
    struct _linkcmd2rule* next;
}linkcmd2rule;

linkcmd2rule* headLink = NULL;


bool is_port(char* argv,int* port){

    char* end;
    char* end2;
    char* dash = strchr(argv,'-');
    if(!dash){
    int num = strtol(argv,&end,10);
    if (*end != '\0' || num < 0 || num > 65535)
    {
        return false;       
    }
    port[0] = num;
    port[1] = -1;
    return true;
    }else{
        *dash = '\0';
        char* start =argv;
        if(*argv == '\0'){
            port[0]=-1;
            return false;
        }
        char* last = dash +1;
        int num = strtol(start,&end,10);
        int num2 = strtol(last,&end2,10);
        if (*end != '\0' || num < 0 || num > 65535 || *end2 != '\0' || num2 < 0 || num2 > 65535 || num2 <= num)
        {
            return false;       
        } else{
            port[0] = num;
            port[1] = num2;
            return true;
        }
    }
}

int* validateIPString(char* parsedText){

    static int parsedIP[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    int i = 0;

    char* dash = strchr(parsedText, '-');
    if(dash){
        *dash = '\0';
        char* startIP = parsedText;
        char* endIP = dash + 1;
        int chars;
        int chars2;
        if(sscanf(startIP, "%d.%d.%d.%d%n", &parsedIP[0], &parsedIP[1], &parsedIP[2], &parsedIP[3],&chars) != 4 || startIP[chars] != '\0'){
            parsedIP[0] = -1;
            return parsedIP;
        }
        if(sscanf(endIP, "%d.%d.%d.%d%n", &parsedIP[4], &parsedIP[5], &parsedIP[6], &parsedIP[7],&chars2) != 4 || endIP[chars2] != '\0'){
            parsedIP[0] = -1;
            return parsedIP;
        }else{
            for(int i =0 ;i<4 ; i++){
                if(parsedIP[i]>parsedIP[i+4]){
                    parsedIP[0] = -1;
                    return parsedIP;
                }
            }
        }
    }else{
        int chars;
        if(sscanf(parsedText, "%d.%d.%d.%d%n", &parsedIP[0], &parsedIP[1], &parsedIP[2], &parsedIP[3] ,&chars) != 4 || parsedText[chars] != '\0'){
            parsedIP[0] = -1;
            return parsedIP;
        }
        for(int j = 4; j < 8; j++){
            parsedIP[j] = -1;
        }
    }
    for(i=0 ;  i < 8 && parsedIP[i] != -1; i++) {
        if (parsedIP[i] < 0 || parsedIP[i] > 255) {
            parsedIP[0] = -1;
            return parsedIP;
        }
    }

    return parsedIP;
}

linkcmd2rule* checkIfRuleAdded(rule* head, linkcmd2rule* headlink){
    while(headlink != NULL){
        if(headlink->rule == head){
            return headlink;
        }else{
            headlink = headlink->next;
        }
    }
    return NULL;
}

bool checkifcmdadded(comd* cmd ,linkcmd2rule* linkpointer){
    comd* current_cmd = linkpointer->cmd; // must do this because linkpointer->cmd will not be taken in as a copy but rather affect original

    while(current_cmd != NULL){
        if(strcmp(current_cmd->inputcmd, cmd->inputcmd) == 0){
            return true;
        }
        current_cmd = current_cmd->next;
    }
    return false;
}

void addcmdtoRule(rule* head){ // the comd is already added to the head before checkConnection is called so remember that
    linkcmd2rule* link = checkIfRuleAdded(head,headLink);

    comd* new_cmd = malloc(sizeof(comd));
    new_cmd->inputcmd = strdup(headcmd->inputcmd);
    new_cmd->next = NULL;
    bool added = false;

    while(link){
        if(!checkifcmdadded(new_cmd,link)){
        new_cmd->next = link->cmd;
        link->cmd = new_cmd;
        added = true;
        break;
        }else{
                link = link->next;
        }
    }
    if (!added)
    {
        free(new_cmd->inputcmd);
        free(new_cmd);
    }
    
  }

void addrulelink(rule* head){

    linkcmd2rule* newlink = malloc(sizeof(linkcmd2rule));
    newlink->rule = head;
    newlink->cmd = NULL;

    newlink->next = headLink;
    headLink = newlink;
}

bool addlist(char* input){
    rule* newrule = malloc(sizeof(rule));
    newrule->next = NULL;
    char parsedTextIp[32];
    char portIn[12];
    int chars;
    
    if(sscanf(input, "A %31s %11s%n", parsedTextIp,portIn,&chars)!= 2){
        free(newrule);
        return false;
    }

    for(int i = chars; i < strlen(input); i++){
        if(input[i] != ' ' && input[i] != '\t' && input[i] != '\n'){
            free(newrule);
            return false;
        }
    }

           int* IP = validateIPString(parsedTextIp);
        if (IP[0] != -1 && IP[4] == -1) {
            for (int j = 0; j < 4; j++) {
                newrule->ip_address1[j] = IP[j];
                newrule->ip_address2[j] = -1;
            }
        } else if (IP[0] != -1 && IP[4] != -1) {
            for (int j = 0; j < 4; j++) {
                newrule->ip_address1[j] = IP[j];
                newrule->ip_address2[j] = IP[j + 4];
            }
        } else {
        free(newrule);
        return false;
    }

    int port[2];
    if(is_port(portIn,port)){
        newrule->port1 = port[0];
        newrule->port2 = port[1];
    }else{
        free(newrule);
        return false;
        }    
                newrule->next = head;
                head = newrule;
                addrulelink(head);                    
                return true;
    }

void addlinkedcmds(char* input){
    comd* newcmd = malloc(sizeof(comd));
    newcmd->inputcmd = strdup(input); // have to do this to make sure pointers value isnt overridden
    newcmd->next = headcmd;
    headcmd = newcmd;
}

void listRequests(comd* headcm,char* response){
    if (headcm != NULL)
    {
        listRequests(headcm->next,response);
        if (interactiveMode)
        {
            printf("%s\n", headcm->inputcmd);
        } else if (response != NULL)
        {
            strcat(response, headcm->inputcmd);
            strcat(response, "\n");
        }
    }
    return;
}

uint32_t ip_array_to_uint32(int ip[4]) {
    uint32_t ip_addr = 0;

    uint32_t octet1 = (uint32_t)ip[0];
    uint32_t octet2 = (uint32_t)ip[1];
    uint32_t octet3 = (uint32_t)ip[2];
    uint32_t octet4 = (uint32_t)ip[3];
    uint32_t shifted_octet1 = octet1 << 24;
    uint32_t shifted_octet2 = octet2 << 16;
    uint32_t shifted_octet3 = octet3 << 8;
    uint32_t shifted_octet4 = octet4;

    ip_addr = shifted_octet1 | shifted_octet2 | shifted_octet3 | shifted_octet4;

    return ip_addr;
}


void checkConnection(char* input,rule* head,char* response){
    int i=0;
    char IP[16];
    char port[6];
    bool works=false;
    if(sscanf(input, "C %s %s",IP, port)!=2){
        if (interactiveMode) {
            printf("Illegal IP address or port specified\n");
        } else if (response != NULL) {
            strcat(response, "Illegal IP address or port specified\n");
        }
        return;
    }
    int* parsedIP = validateIPString(IP);
    if(parsedIP[0] == -1 || parsedIP[4] != -1){
            if(interactiveMode){
            printf("Illegal IP address or port specified\n");
        } else if(response != NULL){
            strcat(response, "Illegal IP address or port specified\n");
        }
            return;
        }
    int myPort[2];
    if(!is_port(port,myPort) || myPort[1] != -1){
        if(interactiveMode)
        {
            printf("Illegal IP address or port specified\n");
        }else if(response != NULL)
        {
            strcat(response, "Illegal IP address or port specified\n");
        }
        return;
    }
    while (head != NULL)
    {
        works=false;
        if (head->ip_address2[0] == -1) {
            for (i = 0; i < 4; i++) {
                if (head->ip_address1[i] != parsedIP[i]) {
                    break; 
                }
            }
            if (i == 4) {
                works = true;
            }
        } else {
            uint32_t parsedIP_val = ip_array_to_uint32(parsedIP);
            uint32_t ip_start_val = ip_array_to_uint32(head->ip_address1);
            uint32_t ip_end_val = ip_array_to_uint32(head->ip_address2);

            if (parsedIP_val >= ip_start_val && parsedIP_val <= ip_end_val) {
                works = true;
            }
        }
        if(works){
                if(head->port2 == -1){
                    if (head->port1 != myPort[0]){ 
                        works=false;
                    }
                }else if (myPort[0]< head->port1 
                || myPort[0] > head->port2){
                    works=false;
                }  
        }
        if(works){
            addcmdtoRule(head);
            if (interactiveMode) {
                printf("Connection accepted\n");
            } else {
                strcat(response, "Connection accepted\n");
            }
            return;
            }

        head = head->next;
    }
    if(interactiveMode){
        printf("Connection rejected\n");
    }else{
        strcat(response, "Connection rejected\n");
    }
}
void freecmdsInLink(linkcmd2rule* node){
    comd* temp;
    while(node->cmd != NULL){
        temp = node->cmd;
        node->cmd = node->cmd->next;
        free(temp);
    }

}

void deleterule(char* input,char* response){
    char parsedTextIP[32];
    char PortIn[12];
    int chars;
    if(sscanf(input, "D %31s %11s%n",parsedTextIP,PortIn,&chars) != 2){
        if(interactiveMode){
            printf("Rule invalid\n");
        } else{
            strcat(response, "Rule invalid\n");
        }
        return;
    }
    for(int i = chars; i < strlen(input); i++){
        if(input[i] != ' ' && input[i] != '\t' && input[i] != '\n'){
            if(interactiveMode){
                printf("Rule invalid\n");
            } else{
                strcat(response, "Rule invalid\n");
            }
            return;
        }
    }
    // defining variables
    bool exists = false;
    rule* pointiter = head;
    rule* pointprev = NULL;
    int* IP = validateIPString(parsedTextIP);
    if(IP[0] == -1 ){
            if(interactiveMode){
            printf("Rule invalid\n");
        } else{
            strcat(response, "Rule invalid\n");
        }
            return;
        }
    int port[2];
    if(!is_port(PortIn,port)){
        if(interactiveMode){
            printf("Rule invalid\n");
        } else{
            strcat(response, "Rule invalid\n");
        }
        return;
    }

    //deleting cmdlink
    linkcmd2rule* pointIterLink = headLink;
    linkcmd2rule* pointPrevLink = NULL;
    while(pointIterLink != NULL){
        exists=true;  
        if(pointIterLink->rule->ip_address2[0] != -1){
            for(int i=0 ; i<4 ; i++){
                if(IP[i] != pointIterLink->rule->ip_address1[i]||
                IP[i+4] != pointIterLink->rule->ip_address2[i]){
                    exists=false;
                    break;
                }
            }
        }else{ 
            for(int i = 0; i<4 ; i++){
                if(IP[i] != pointIterLink->rule->ip_address1[i]){
                    exists=false;
                    break;
                }
            }
        }
        if(exists){
            if(port[0] != pointIterLink->rule->port1 
            || port[1] != pointIterLink->rule->port2){
                exists=false;
            }

            if(exists){
                if(!pointPrevLink){
                    headLink = pointIterLink->next;
                    break;
                }else{
                    pointPrevLink->next = pointIterLink->next;
                    break;
                }
            }
        }
    pointPrevLink = pointIterLink;
    pointIterLink = pointIterLink->next;
    }

    //deleting rule
    while(pointiter != NULL){
        exists = true;
        if (pointiter->ip_address2[0] != -1){
            for(int i = 0 ; i<4 ; i++){
                if(IP[i] != pointiter->ip_address1[i] 
                || IP[i+4] != pointiter->ip_address2[i]){
                    exists = false;
                    break;
                }
            }
        }else{
            for(int i = 0 ; i<4 ; i++){
                if(IP[i] != pointiter->ip_address1[i]){
                    exists=false;
                    break;
                }
            }    
        }
            if (exists && (port[0] != pointiter->port1 || port[1] != pointiter->port2)) {
                exists = false;
            }
            if (exists)
            {
                if(!pointprev){
                    head = pointiter->next;
                    if(interactiveMode){
                        printf("Rule deleted\n");
                    }else{
                        strcat(response, "Rule deleted\n");
                    }
                    freecmdsInLink(pointIterLink);
                    free(pointIterLink->rule);
                    free(pointIterLink);
                    return;
                }else{
                    pointprev->next = pointiter->next;
                    if(interactiveMode){
                        printf("Rule deleted\n");
                    }else{
                        strcat(response, "Rule deleted\n");
                    }
                    freecmdsInLink(pointIterLink);
                    free(pointIterLink->rule);
                    free(pointIterLink);
                    return;
                }
            }
        pointprev = pointiter;
        pointiter = pointiter->next;
    }        
    if(interactiveMode){
        printf("Rule not found\n");
    }else{
        strcat(response, "Rule not found\n");
    }
}
void returnlinks(linkcmd2rule* head,char* response){

    while(head != NULL){
        char temp[256];
        if (head->rule->ip_address2[0] == -1)
        {
            if (head->rule->port2 == -1)
            {
                sprintf(temp, "Rule: %d.%d.%d.%d %d\n",
                        head->rule->ip_address1[0],
                        head->rule->ip_address1[1],
                        head->rule->ip_address1[2],
                        head->rule->ip_address1[3],
                        head->rule->port1);
            }else{
                sprintf(temp, "Rule: %d.%d.%d.%d %d-%d\n",
                        head->rule->ip_address1[0],
                        head->rule->ip_address1[1],
                        head->rule->ip_address1[2],
                        head->rule->ip_address1[3],
                        head->rule->port1,
                        head->rule->port2);
            }
            
        }else{if (head->rule->port2 == -1){
            sprintf(temp, "Rule: %d.%d.%d.%d-%d.%d.%d.%d %d\n",
                        head->rule->ip_address1[0],
                        head->rule->ip_address1[1],
                        head->rule->ip_address1[2],
                        head->rule->ip_address1[3],
                        head->rule->ip_address2[0],
                        head->rule->ip_address2[1],
                        head->rule->ip_address2[2],
                        head->rule->ip_address2[3],
                        head->rule->port1);
            }else{
                sprintf(temp, "Rule: %d.%d.%d.%d-%d.%d.%d.%d %d-%d\n",
                        head->rule->ip_address1[0],
                        head->rule->ip_address1[1],
                        head->rule->ip_address1[2],
                        head->rule->ip_address1[3],
                        head->rule->ip_address2[0],
                        head->rule->ip_address2[1],
                        head->rule->ip_address2[2],
                        head->rule->ip_address2[3],
                        head->rule->port1,
                        head->rule->port2);
            }
        }
        if (interactiveMode) {
            printf("%s", temp);
        } else {
            strcat(response, temp);
        }

        comd* cmdTemp = head->cmd;
        char string1[50];
        char string2[20];
        while(cmdTemp != NULL){
            sscanf(cmdTemp->inputcmd,"C %s %s",string1,string2);
            sprintf(temp,"Query: %s %s\n", string1,string2);
            if (interactiveMode) {
                printf("%s", temp);
            } else {
                strcat(response, temp);
            }
            cmdTemp = cmdTemp->next;
        }
    head = head->next;
    }
}
    


bool processargs(int argc, char** argv, serverInterac* cmd){

    cmd->interactive = false;
    cmd->port[0] = -1;

    if(argc != 2){
        return false;
    }

    if(strcmp(argv[1],"-i") == 0 ){

        cmd->interactive = true;
        return true;
        
    }else 
    { 
        if (is_port(argv[1],(cmd->port))){
            cmd->interactive=false;
            return true;
        }else{
            cmd->port[0]=-1;  //error code if port entered is NaN
        }
    }
    return false;

}

void runInteractive(){
    bool legal= false;
    interactiveMode = true;
    char input[100];
    while (true){
        legal=false;
        if (NULL == fgets(input,sizeof(input),stdin)){
            return;}
        input[strcspn(input, "\n")] = '\0';

        if(input[0] == 'A' && input[1] == ' ' ){
            if(addlist(input)){
                addlinkedcmds(input);
                printf("Rule added\n");
            } else{printf("Invalid rule\n");
                }
                legal = true;
        }
        
        if(input[0] == 'R' && input[1] == '\0'){
            addlinkedcmds(input);
            listRequests(headcmd,NULL);
            legal = true;
        }
        if(input[0] == 'C' && input[1] == ' '){
            addlinkedcmds(input); //has to be before checkConnection for addcmdtoRule
            checkConnection(input,head,NULL);
            legal = true;
        }
        if(input[0] == 'D' && input[1] == ' '){
            deleterule(input,NULL);
            addlinkedcmds(input);
            legal = true;
        }
        if (input[0] == 'L' && input[1] == '\0')
        {
            returnlinks(headLink,NULL);
            addlinkedcmds(input);
            legal = true;
        }
        if(!legal){printf("Illegal request\n");}
        fflush(stdout);
    }
}

void* client(void* arg){
    int client_socket = *(int*)arg;
    free(arg);

    char buffer[1024];
    int valread = read(client_socket, buffer, sizeof(buffer) - 1);
    if (valread > 0){
        buffer[valread] = '\0';
        char response[8192] = {0};

        if(buffer[0] == 'A' && buffer[1] == ' ' ){
            pthread_mutex_lock(&data_mutex);
            if(addlist(buffer)){
                addlinkedcmds(buffer);
                strcat(response, "Rule added\n");
            } else{
                strcat(response, "Invalid rule\n");
            }
            pthread_mutex_unlock(&data_mutex);
        }
        else if(buffer[0] == 'R' && buffer[1] == '\0'){
            pthread_mutex_lock(&data_mutex);
            addlinkedcmds(buffer);
            listRequests(headcmd, response);
            pthread_mutex_unlock(&data_mutex);
        }
        else if(buffer[0] == 'C' && buffer[1] == ' '){
            pthread_mutex_lock(&data_mutex);
            addlinkedcmds(buffer);
            checkConnection(buffer, head, response);
            pthread_mutex_unlock(&data_mutex);
        }
        else if(buffer[0] == 'D' && buffer[1] == ' '){
            pthread_mutex_lock(&data_mutex);
            deleterule(buffer, response);
            addlinkedcmds(buffer);
            pthread_mutex_unlock(&data_mutex);
        }
        else if (buffer[0] == 'L' && buffer[1] == '\0'){
            pthread_mutex_lock(&data_mutex);
            returnlinks(headLink, response);
            addlinkedcmds(buffer);
            pthread_mutex_unlock(&data_mutex);
        } else {
            strcat(response, "Illegal request\n");
        }

        if (strlen(response) > 0) {
            send(client_socket, response, strlen(response), 0);
        }
    }

    close(client_socket);
    pthread_exit(NULL);
}

void runServer(int port){
    interactiveMode = false;
   int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0){
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (true){
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0){
            
            continue;
        }

        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;

        if(pthread_create(&thread_id, NULL, client, pclient) != 0){
            close(new_socket);
            free(pclient);
        } else {
            pthread_detach(thread_id);
        }    }
    close(server_fd);

}


int main (int argc, char** argv) {
    serverInterac cmd;
    setvbuf(stdout, NULL, _IONBF, 0);
    if(!processargs(argc,argv, &cmd)){
        printf("Error!\n");   
    }

    
    if(cmd.interactive){
    runInteractive();

    }else{
        if (cmd.port[0] != -1)
        {
        runServer(cmd.port[0]);
                 
    }
    return 0; 
    }
}
