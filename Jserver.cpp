#include <thread>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define DEF_BUFFER_SIZE 1024
#define DEF_PORT 23365

using namespace std;

void printMessage(char *pointer_c_saveptr) {
    printf("%s\n\r", pointer_c_saveptr);
}

void sendName(int i_socket_id, string S_nome) {
    send(i_socket_id, S_nome.c_str(), DEF_BUFFER_SIZE, 0);
}

void client_service(int i_socket_id, string S_nome) 
{
    char arr_c_buffer[DEF_BUFFER_SIZE];
    char *pointer_c_request;
    char *pointer_c_saveptr;
    int i_recvRes;

    while (1) {
        i_recvRes = recv(i_socket_id, (void *)arr_c_buffer, sizeof(arr_c_buffer), 0);
        if (i_recvRes <= 0) {
            close(i_socket_id);
            break;
        } else {
            pointer_c_request = strtok_r(arr_c_buffer, "-", &pointer_c_saveptr);

            if (strcmp(pointer_c_request, "MESS") == 0) {
                printMessage(pointer_c_saveptr);
            }

            if (strcmp(pointer_c_request, "NAME") == 0) {
                sendName(i_socket_id, S_nome);
            }
        }
    }
}

int setupServerSocket(struct sockaddr_in &strc_server_add) 
{
    int i_request_socket_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (i_request_socket_id < 0) {
        printf("Errore apertura socket!\r\n");
        return -1;
    }

    memset(&strc_server_add, 0, sizeof(strc_server_add));
    strc_server_add.sin_family = AF_INET;
    strc_server_add.sin_addr.s_addr = 0;     
    strc_server_add.sin_port = htons(DEF_PORT);

    if (bind(i_request_socket_id, (struct sockaddr *)&strc_server_add, sizeof(strc_server_add)) < 0) {
        printf("Errore associazione socket!\r\n");
        close(i_request_socket_id);
        return -1;
    }

    if (listen(i_request_socket_id, 1) < 0) {
        printf("Errore impostazione socket!\r\n");
        close(i_request_socket_id);
        return -1;
    }

    return i_request_socket_id;
}

int acceptConnection(int i_request_socket_id, struct sockaddr_in &strc_client_add) {
    unsigned int uns_i_client_add_size = sizeof(strc_client_add);
    return accept(i_request_socket_id, (struct sockaddr *)&strc_client_add, &uns_i_client_add_size);
}

int main()
{
    string S_nome;
    int i_request_socket_id, i_communication_socket_id;
    struct sockaddr_in strc_server_add, strc_client_add;
    thread *client_thread;

    cout << "dimmi il tuo nome: ";
    cin >> S_nome;
    cout << endl;

    i_request_socket_id = setupServerSocket(strc_server_add);

    if (i_request_socket_id < 0) return -1;

    while (true) 
    {
        i_communication_socket_id = acceptConnection(i_request_socket_id, strc_client_add);

        if (i_communication_socket_id >= 0) {
            client_thread = new thread(client_service, i_communication_socket_id, S_nome);
            client_thread->detach();
            delete client_thread;
        }
    }

    close(i_request_socket_id);
    return 0;
}
